#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <attribute.h>
#include <adftool.h>
#include <time.h>
#include <pthread.h>
#include "readline.h"
#include "safe-alloc.h"
#include <math.h>

#define _(String) gettext(String)
#define N_(String) (String)
#define NP_(Context, String) (String)

#if defined ENABLE_NLS && ENABLE_NLS
# define adftool_pgettext pgettext_expr
# define P_(Context, String) pgettext(Context, String)
#else
# define adftool_pgettext(Context, String) (String)
# define P_(Context, String) (String)
#endif

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

static inline int parse_command (void);

static int continue_thread = 0;
static pthread_mutex_t file_synchronizer;
static struct adftool_file *file;
static struct adftool_channel_processor_group *group = NULL;

static void *worker_thread (void *ctx);

#define NUM_THREADS 7

static pthread_t threads[NUM_THREADS];

int
main (int argc, char *argv[])
{
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  if (argc < 2)
    {
      fprintf (stderr, "Usage: adftool-mt [FILE]\n");
      return 1;
    }
  if (pthread_mutex_init (&file_synchronizer, NULL) != 0)
    {
      fprintf (stderr, "Cannot allocate a mutex\n");
      return 1;
    }
  file = adftool_file_open (argv[1], 0);
  if (file == NULL)
    {
      fprintf (stderr, "Cannot open %s.\n", argv[1]);
      return 1;
    }
  group =
    adftool_channel_processor_group_alloc (file, &file_synchronizer, 50);
  if (group == NULL)
    {
      fprintf (stderr, "Cannot allocate a group.\n");
      return 1;
    }
  continue_thread = 1;
  for (size_t i = 0; i < NUM_THREADS; i++)
    {
      if (pthread_create (&(threads[i]), NULL, worker_thread, NULL) != 0)
	{
	  fprintf (stderr, "Cannot create a thread.\n");
	  return 1;
	}
    }
  while (parse_command () != 0)
    {
      /* continue */
    }
  if (pthread_mutex_lock (&file_synchronizer) != 0)
    {
      abort ();
    }
  continue_thread = 0;
  if (pthread_mutex_unlock (&file_synchronizer) != 0)
    {
      abort ();
    }
  for (size_t i = 0; i < NUM_THREADS; i++)
    {
      void *retval = NULL;
      if (pthread_join (threads[i], &retval) != 0)
	{
	  fprintf (stderr, "Cannot cancel a thread.\n");
	  return 1;
	}
    }
  adftool_channel_processor_group_free (group);
  adftool_file_close (file);
  pthread_mutex_destroy (&file_synchronizer);
  return 0;
}

static int read_header (struct adftool_term **channel_type,
			double *filter_low,
			double *filter_high,
			size_t *start_index, size_t *length, int *done);

static inline int
parse_command (void)
{
  int cont = 0;
  struct adftool_term *channel_type = NULL;
  double filter_low = 0.53, filter_high = 35.0;
  size_t start_index = 0, length = 5120;
  int done = 0;
  do
    {
      cont =
	read_header (&channel_type, &filter_low, &filter_high, &start_index,
		     &length, &done);
    }
  while (!done);
  if (channel_type != NULL)
    {
      size_t nearest_start, nearest_length;
      double *data = calloc (length, sizeof (double));
      if (data == NULL)
	{
	  abort ();
	}
      int error =
	adftool_channel_processor_group_get (group, channel_type, filter_low,
					     filter_high, start_index, length,
					     &nearest_start, &nearest_length,
					     data);
      if (error < 0)
	{
	  printf ("HTTP/1.1 400 Bad Request\r\n" "\r\n");
	}
      else
	{
	  printf ("HTTP/1.1 200 OK\r\n"
		  "Adftool-Start: %lu\r\n"
		  "Adftool-Length: %lu\r\n"
		  "Content-Type: text/plain\r\n"
		  "\r\n", nearest_start, nearest_length);
	  for (size_t i = 0; i < length; i++)
	    {
	      printf ("%.12g\r\n", data[i]);
	    }
	  printf ("\r\n");
	}
      free (data);
    }
  adftool_term_free (channel_type);
  return cont;
}

static int
read_header (struct adftool_term **channel_type,
	     double *filter_low,
	     double *filter_high,
	     size_t *start_index, size_t *length, int *done)
{
  char *line = readline ("HTTP header: ");
  *done = 0;
  if (line == NULL)
    {
      /* stdin closed */
      *done = 1;
      return 0;
    }
  if (STREQ (line, ""))
    {
      /* empty line, so end of request, but more requests may follow. */
      FREE (line);
      *done = 1;
      return 1;
    }
  const char *colon = strstr (line, ":");
  if (colon == NULL)
    {
      /* Invalid header line, continue still. */
      FREE (line);
      *done = 0;
      return 1;
    }
  size_t header_name_start = 0, header_name_stop = (colon - line);
  size_t value_start = header_name_stop + strlen (":");
  while (header_name_start < header_name_stop
	 && (line[header_name_start] == ' '
	     || line[header_name_start] == '\t'))
    {
      header_name_start++;
    }
  while (header_name_start < header_name_stop
	 && (line[header_name_stop - 1] == ' '
	     || line[header_name_stop - 1] == '\t'))
    {
      header_name_stop--;
    }
  size_t header_name_length = header_name_stop - header_name_start;
  /* ASCII-downcase the header name. */
  for (size_t i = header_name_start; i < header_name_stop; i++)
    {
      if (line[i] >= 'A' && line[i] <= 'Z')
	{
	  line[i] += 'a' - 'A';
	}
    }
  const char *header_value = line + value_start;
#define HEADER_IS(header) \
  (header_name_length == strlen (header) \
    && strncmp (line + header_name_start, \
                header, \
		strlen (header)) == 0)
  if (HEADER_IS ("adftool-channel-type"))
    {
      size_t consumed = 0;
      if (*channel_type == NULL)
	{
	  *channel_type = adftool_term_alloc ();
	  if (*channel_type == NULL)
	    {
	      abort ();
	    }
	}
      while (*header_value == ' ' || *header_value == '\t')
	{
	  header_value++;
	}
      if (adftool_term_parse_n3
	  (header_value, strlen (header_value), &consumed,
	   *channel_type) != 0)
	{
	  printf ("HTTP/1.1 400 Bad Request\r\n\r\n");
	  goto cleanup;
	}
      if (consumed < strlen (header_value))
	{
	  /* The channel type has garbage, such as: */
	  /* Adftool-Channel-Type: <https://a>, garbage\r\n */
	  printf ("HTTP/1.1 400 Bad Request\r\n\r\n");
	  goto cleanup;
	}
    }
  else if (HEADER_IS ("adftool-highpass") || HEADER_IS ("adftool-lowpass"))
    {
      char *enddouble = NULL;
      char *user_locale = setlocale (LC_NUMERIC, "C");
      if (user_locale == NULL)
	{
	  abort ();
	}
      const double value = strtod (header_value, &enddouble);
      if (setlocale (LC_NUMERIC, user_locale) == NULL)
	{
	  abort ();
	}
      double value_hz = value;
      if (enddouble == header_value)
	{
	  /* This is not a number. */
	  printf ("HTTP/1.1 400 Bad Request\r\n\r\n");
	  goto cleanup;
	}
      while (*enddouble == ' ' || *enddouble == '\t')
	{
	  enddouble++;
	}
      if (STREQ (enddouble, "s"))
	{
	  value_hz = (1 / (2 * M_PI * value));
	}
      else if (STREQ (enddouble, "Hz") || STREQ (enddouble, "hz")
	       || STREQ (enddouble, ""))
	{
	  value_hz = value;
	}
      else
	{
	  /* Bad frequency unit. */
	  printf ("HTTP/1.1 400 Bad Request\r\n\r\n");
	  goto cleanup;
	}
      if (HEADER_IS ("adftool-highpass"))
	{
	  *filter_low = value_hz;
	}
      else
	{
	  *filter_high = value_hz;
	}
    }
  else if (HEADER_IS ("adftool-start-index") || HEADER_IS ("adftool-length"))
    {
      char *endvalue = NULL;
      size_t value = strtoul (header_value, &endvalue, 10);
      if (endvalue == header_value)
	{
	  /* Not a number */
	  printf ("HTTP/1.1 400 Bad Request\r\n\r\n");
	  goto cleanup;
	}
      else if (STREQ (endvalue, ""))
	{
	  if (HEADER_IS ("adftool-start-index"))
	    {
	      *start_index = value;
	    }
	  else
	    {
	      *length = value;
	    }
	}
      else
	{
	  /* Trailing garbage */
	  printf ("HTTP/1.1 400 Bad Request\r\n\r\n");
	  goto cleanup;
	}
    }
cleanup:
  FREE (line);
  return 1;
}

static void *
worker_thread (void *ctx)
{
  (void) ctx;
  int work_done = 0;
  int my_continue = 1;
  do
    {
      if (adftool_channel_processor_group_populate_cache (group, &work_done) <
	  0)
	{
	  fprintf (stderr, "Error: thread failed.\n");
	  return NULL;
	}
      if (!work_done)
	{
	  /* Sleep a bit. */
	  static const struct timespec sleep_request = {
	    .tv_sec = 0,
	    .tv_nsec = 150 * 1000 * 1000
	  };
	  struct timespec sleep_remaining = { 0 };
	  if (nanosleep (&sleep_request, &sleep_remaining) != 0)
	    {
	      fprintf (stderr, "Error: thread can’t sleep.\n");
	      return NULL;
	    }
	}
      if (pthread_mutex_lock (&file_synchronizer) != 0)
	{
	  abort ();
	}
      my_continue = continue_thread;
      if (pthread_mutex_unlock (&file_synchronizer) != 0)
	{
	  abort ();
	}
    }
  while (my_continue);
  return NULL;
}
