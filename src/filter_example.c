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
  adftool_fir_design_bandpass (filter, low, high);
  adftool_fir_apply (filter, signal_length, signal, filtered);
  for (size_t i = 0; i < signal_length; i++)
    {
      const double actual = filtered[i];
      const double expected = expected_filtered[i];
      double diff = actual - expected;
      if (diff < 0)
	{
	  diff = -diff;
	}
      if (diff >= 1e-6)
	{
	  fprintf (stderr, _("The filter test failed at point %lu.\n"), i);
	  abort ();
	}
    }
  adftool_fir_free (filter);
  free (filtered);
  free (signal);
  free (expected_filtered);
  return 0;
}
