#include <config.h>

#include <attribute.h>
#include <adftool.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <assert.h>

#define _(String) gettext(String)
#define N_(String) (String)

#include "libadftool/channel_processor.h"
#include "libadftool/term.h"

#define fail_test() \
  (fprintf (stderr, "%s:%d: test failed.\n", __FILE__, __LINE__), abort ())

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
  pthread_mutex_t sync;
  if (pthread_mutex_init (&sync, NULL) != 0)
    {
      fail_test ();
    }
  struct adftool_term *channel_type = term_alloc ();
  if (channel_type == NULL)
    {
      fail_test ();
    }
  term_set_named (channel_type, "https://localhost/lytonepal#Fp2");
  struct adftool_channel_processor *processor =
    channel_processor_alloc (file, &sync, channel_type, 0.3, 35);
  if (processor == NULL)
    {
      fail_test ();
    }
  size_t start_index;
  size_t length;
  double *data;
  static const size_t n_data = 10240;
  if (ALLOC_N (data, n_data) < 0)
    {
      fail_test ();
    }
  for (size_t i = 0; i < n_data; i++)
    {
      data[i] = 42;
    }
  int error =
    channel_processor_get (processor, 2560, n_data, &start_index, &length,
			   data);
  assert (error == 0);
  /* The cache is empty; thus we don’t know the limits yet. */
  assert (start_index == 2560);
  assert (length == n_data);
  for (size_t i = 0; i < n_data; i++)
    {
      assert (isnan (data[i]));
    }
  bool work_done = false;
  error = channel_processor_populate_cache (processor, &work_done);
  assert (error == 0);
  assert (work_done);
  error =
    channel_processor_get (processor, 2560, n_data, &start_index, &length,
			   data);
  assert (error == 0);
  /* The cache is now filled; thus we do know the limits. */
  assert (start_index == 0);
  assert (length == 5120);
  for (size_t i = 0; i < length; i++)
    {
      assert (!isnan (data[i]));
      assert (data[i] != 42);
    }
  for (size_t i = length; i < n_data; i++)
    {
      assert (isnan (data[i]));
    }
  FREE (data);
  channel_processor_free (processor);
  term_free (channel_type);
  adftool_file_close (file);
  return 0;
}
