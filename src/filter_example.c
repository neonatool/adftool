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
#include <math.h>

#define _(String) gettext(String)
#define N_(String) (String)

extern void _adftool_filter_test_data (double *, double *, double *,
				       size_t *, double **, double **,
				       size_t *, double **);

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  double sfreq;
  double low;
  double high;
  size_t signal_length;
  double *signal;
  double *expected_filtered;
  size_t filter_order;
  double *expected_coef;
  _adftool_filter_test_data (&sfreq, &low, &high,
			     &signal_length, &signal, &expected_filtered,
			     &filter_order, &expected_coef);
  double *filtered = malloc (signal_length * sizeof (double));
  if (filtered == NULL)
    {
      abort ();
    }
  double trans_low, trans_high;
  adftool_fir_auto_bandwidth (sfreq, low, high, &trans_low, &trans_high);
  double tightest = trans_low;
  if (trans_high < tightest)
    {
      tightest = trans_high;
    }
  const size_t order = adftool_fir_auto_order (sfreq, tightest);
  struct adftool_fir *filter = adftool_fir_alloc (order);
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
  adftool_fir_design_bandpass (filter, sfreq, low, high, trans_low,
			       trans_high);
  double *actual_coef = malloc (adftool_fir_order (filter) * sizeof (double));
  if (actual_coef == NULL)
    {
      abort ();
    }
  adftool_fir_coefficients (filter, actual_coef);
  double coef_max_diff = 0;
  printf ("Coefficient\tExpected\tActual\n");
  for (size_t i = 0; i < filter_order; i++)
    {
      const double actual = actual_coef[i];
      const double expected = expected_coef[i];
      double diff = actual - expected;
      if (diff < 0)
	{
	  diff = -diff;
	}
      printf ("%lu\t%f\t%f\n", i, expected, actual);
      if (diff > coef_max_diff)
	{
	  coef_max_diff = diff;
	}
    }
  if (coef_max_diff > 1.2e-4)
    {
      fprintf (stderr, "Filter coefficients do not match (%f).\n",
	       coef_max_diff);
      abort ();
    }
  free (actual_coef);
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
  if (cossim <= 0.997)
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
  free (expected_coef);
  return 0;
}
