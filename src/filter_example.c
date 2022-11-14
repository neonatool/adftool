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
#include <math.h>

#define _(String) gettext(String)
#define N_(String) (String)

extern void _adftool_filter_test_data (double *, double *, double *, double *,
				       size_t *, double **, double **);

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  const size_t filter_order = 6607;
  double sfreq;
  double transition_bandwidth;
  double low;
  double high;
  size_t signal_length;
  double *signal;
  double *expected_filtered;
  _adftool_filter_test_data (&sfreq, &transition_bandwidth, &low, &high,
			     &signal_length, &signal, &expected_filtered);
  double *filtered = malloc (signal_length * sizeof (double));
  if (filtered == NULL)
    {
      abort ();
    }
  struct adftool_fir *filter =
    adftool_fir_alloc (sfreq, transition_bandwidth);
  if (filter == NULL)
    {
      abort ();
    }
  if (adftool_fir_order (filter) != filter_order)
    {
      fprintf (stderr, _("Error: the filter order is %lu != %lu.\n"),
	       adftool_fir_order (filter), filter_order);
      abort ();
    }
  adftool_fir_design_bandpass (filter, low, high);
  adftool_fir_apply (filter, signal_length, signal, filtered);
  FILE *log = fopen ("filter-log", "wt");
  if (log == NULL)
    {
      abort ();
    }
  fprintf (log, "Time\tSignal\tFiltered (MNE)\tFiltered (adftool)\n");
  double dot = 0;
  double actual2 = 0;
  double expected2 = 0;
  for (size_t i = 0; i < signal_length; i++)
    {
      const double actual = filtered[i];
      const double expected = expected_filtered[i];
      dot += actual * expected;
      actual2 += actual * actual;
      expected2 += expected * expected;
      fprintf (log, "%lu\t%f\t%f\t%f\n", i, signal[i], expected, actual);
    }
  const double cossim = dot / (sqrt (actual2) * sqrt (expected2));
  fclose (log);
  if (cossim <= 0.98)
    {
      fprintf (stderr, _("The filter test failed (%f).\n"), cossim);
      abort ();
    }
  else
    {
      fprintf (stderr, _("The filter result is not great but at least \
it is closer to the expected result than to the raw signal."));
    }
  adftool_fir_free (filter);
  free (filtered);
  free (signal);
  free (expected_filtered);
  return 0;
}
