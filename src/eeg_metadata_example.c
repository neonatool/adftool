#include <config.h>

#include <attribute.h>
#include <adftool.h>
#include <hdf5.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <assert.h>
#include <time.h>

#define _(String) gettext(String)
#define N_(String) (String)

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  remove ("eeg_metadata_example.adf");
  struct adftool_file *file =
    adftool_file_open ("eeg_metadata_example.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  struct timespec t;
  if (timespec_get (&t, TIME_UTC) != TIME_UTC)
    {
      fprintf (stderr, _("Could not get the current time.\n"));
      abort ();
    }
  const struct timespec start = {.tv_sec = t.tv_sec,.tv_nsec = t.tv_nsec };
  const double sampling_frequency = 256;
  if (adftool_eeg_set_time (file, &start, sampling_frequency) != 0)
    {
      fprintf (stderr, _("Could not set the time and sampling frequency.\n"));
      abort ();
    }
  adftool_file_close (file);
  file = adftool_file_open ("eeg_metadata_example.adf", 0);
  if (file == NULL)
    {
      abort ();
    }
  struct timespec first_observation;
  if (adftool_eeg_get_time (file, 0, &first_observation, NULL) != 0)
    {
      fprintf (stderr,
	       _("Could not compute the date of the first observation.\n"));
      abort ();
    }
  assert (first_observation.tv_sec == start.tv_sec);
  assert (first_observation.tv_nsec == start.tv_nsec);
  struct timespec second_observation;
  double sfreq;
  if (adftool_eeg_get_time (file, 1, &second_observation, &sfreq) != 0)
    {
      fprintf (stderr, _("Could not compute the date \
of the second observation.\n"));
      abort ();
    }
  assert (sfreq == sampling_frequency);
  struct timespec time_between_observations;
  time_between_observations.tv_sec =
    second_observation.tv_sec - first_observation.tv_sec;
  time_between_observations.tv_nsec =
    second_observation.tv_nsec - first_observation.tv_nsec;
  if (time_between_observations.tv_nsec < 0)
    {
      time_between_observations.tv_sec--;
      time_between_observations.tv_nsec += 1 * 1000 * 1000 * 1000;
    }
  assert (time_between_observations.tv_sec == 0);
  /* 1 billion / 256 = 3906250 */
  assert (time_between_observations.tv_nsec == 1000000000 / 256);
  struct timespec observation_1s_later;
  if (adftool_eeg_get_time (file, 256, &observation_1s_later, NULL) != 0)
    {
      fprintf (stderr, _("Could not compute the date \
of the third observation.\n"));
      abort ();
    }
  assert (observation_1s_later.tv_sec == start.tv_sec + 1);
  assert (observation_1s_later.tv_nsec == start.tv_nsec);
  adftool_file_close (file);
  return 0;
}
