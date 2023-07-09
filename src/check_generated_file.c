#include <config.h>

#include <attribute.h>
#include <adftool.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>

#define _(String) gettext(String)
#define N_(String) (String)

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#define fail_test()                                                     \
  {                                                                     \
    fprintf (stderr, "%s:%d: failed test.\n", __FILE__, __LINE__);      \
    abort ();								\
  }

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  struct adftool_file *file = adftool_file_open_generated ();
  if (file == NULL)
    {
      fail_test ();
    }
  size_t n_times, n_channels, check_n_times, check_n_channels;
  int error =
    adftool_eeg_get_data (file, 0, 0, &n_times, 0, 0, &n_channels, NULL);
  if (error)
    {
      fail_test ();
    }
  double *data = malloc (n_times * n_channels * sizeof (double));
  if (data == NULL)
    {
      fail_test ();
    }
  error =
    adftool_eeg_get_data (file, 0, n_times, &check_n_times, 0, n_channels,
			  &check_n_channels, data);
  if (error || check_n_times != n_times || check_n_channels != n_channels)
    {
      fail_test ();
    }
  for (size_t i = 0; i < n_channels; i++)
    {
      struct adftool_term *identifier = adftool_term_alloc ();
      if (identifier == NULL)
	{
	  fail_test ();
	}
      error = adftool_find_channel_identifier (file, i, identifier);
      if (error)
	{
	  fail_test ();
	}
      struct adftool_term *type = adftool_term_alloc ();
      if (type == NULL)
	{
	  fail_test ();
	}
      size_t n_types =
	adftool_get_channel_types (file, identifier, 0, 1, &type);
      if (n_types != 1)
	{
	  fail_test ();
	}
      size_t n3_length = adftool_term_to_n3 (type, 0, 0, NULL);
      char *n3 = malloc (n3_length + 1);
      if (n3 == NULL)
	{
	  fail_test ();
	}
      size_t n3_length_check = adftool_term_to_n3 (type, 0, n3_length, n3);
      if (n3_length_check != n3_length)
	{
	  fail_test ();
	}
      n3[n3_length] = '\0';
      static const char *expected_prefix = "<" LYTONEPAL_ONTOLOGY_PREFIX;
      if (strncmp (n3, expected_prefix, strlen (expected_prefix)) != 0)
	{
	  fprintf (stderr, "%s:%d: error: the N3 representation \
of the type of channel %lu is %s.\n", __FILE__, __LINE__, i, n3);
	  fail_test ();
	}
      if (i != 0)
	{
	  printf ("\t");
	}
      printf ("%s", n3);
      free (n3);
      adftool_term_free (type);
      adftool_term_free (identifier);
    }
  printf ("\n");
  for (size_t i = 0; i < n_times; i++)
    {
      for (size_t j = 0; j < n_channels; j++)
	{
	  if (j != 0)
	    {
	      printf ("\t");
	    }
	  const int acceptable_range =
	    (data[i * n_channels + j] >= -0.00015
	     && data[i * n_channels + j] <= 0.00015);
	  if (!acceptable_range)
	    {
	      fail_test ();
	    }
	  printf ("%g", data[i * n_channels + j]);
	}
      printf ("\n");
    }
  free (data);
  adftool_file_close (file);
  remove ("typed_literal_lookup.adf");
  return 0;
}
