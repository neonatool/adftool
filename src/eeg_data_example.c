#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <adftool.h>
#include <hdf5.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <assert.h>

#ifdef HAVE_MPFR_H
#include <mpfr.h>
#endif /* HAVE_MPFR_H */

#define _(String) gettext(String)
#define N_(String) (String)

static const double eeg_data[] = {
  -1.1, 1.7, -0.4,
  -1.8, -0.7, -3.1,
  -1.7, -2.8, 3.1,
  -1.5, -3.4, 3.5,
  -1.3, -1.1, 3.5
};

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  struct adftool_file *file = adftool_file_alloc ();
  if (file == NULL)
    {
      abort ();
    }
  if (adftool_file_open (file, "eeg_data_example.adf", 1) != 0)
    {
      abort ();
    }
  if (adftool_eeg_set_data (file, 5, 3, eeg_data) != 0)
    {
      abort ();
    }
  adftool_file_close (file);
  /* Now read the file. */
  if (adftool_file_open (file, "eeg_data_example.adf", 0) != 0)
    {
      abort ();
    }
  size_t time_dimension;
  size_t channel_dimension;
  if (adftool_eeg_get_data
      (file, 0, 0, &time_dimension, 0, 0, &channel_dimension, NULL) != 0)
    {
      abort ();
    }
  assert (time_dimension == 5);
  assert (channel_dimension == 3);
  double *file_eeg_data =
    malloc (time_dimension * channel_dimension * sizeof (double));
  if (file_eeg_data == NULL)
    {
      abort ();
    }
  size_t check_time_dimension;
  size_t check_channel_dimension;
  if (adftool_eeg_get_data
      (file, 0, time_dimension, &check_time_dimension, 0, channel_dimension,
       &check_channel_dimension, file_eeg_data) != 0)
    {
      abort ();
    }
  assert (check_time_dimension == time_dimension);
  assert (check_channel_dimension == channel_dimension);
  for (size_t i = 0; i < time_dimension * channel_dimension; i++)
    {
      double expected = eeg_data[i];
      double actual = file_eeg_data[i];
      double difference = expected - actual;
      if (difference < 0)
	{
	  difference = -difference;
	}
      assert (difference < 1e-4);
    }
  free (file_eeg_data);
  adftool_file_close (file);
  adftool_file_free (file);
  #ifdef HAVE_MPFR_FREE_CACHE
  mpfr_free_cache ();
  #endif
  return 0;
}
