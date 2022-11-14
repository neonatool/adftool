data <- read.csv ('src/filter_test_data.csv')
coef <- read.csv ('src/filter_test_coef.csv')

filter_order <- 6607 # MNE told me so
sfreq <- 600.6 # MNE told me so
cutoff_low <- 0.3 # I chose it
cutoff_high <- 30 # I chose it
transition_bandwidth <- 0.36365 # It gives the correct filter order

low <- 0.15 # MNE chooses a low transition bandwidth of 0.3 Hz, so the
            # lowest cutoff is actually 0.15
high <- 33.75 # MNE chooses a high transition bandwidth of 7.50 Hz

# Code generation:
init_signal = paste (sapply (data$unfiltered, function (y) {
  sprintf ("%.20f", y)
}), collapse = ",\n      ")
init_filtered = paste (sapply (data$filtered, function (y) {
  sprintf ("%.20f", y)
}), collapse = ",\n      ")
init_coef = paste (sapply (coef$coef, function (y) {
  sprintf ("%.20f", y)
}), collapse = ",\n      ")

cat (sprintf ("#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

void
_adftool_filter_test_data (double *sfreq,
                           double *transition_bandwidth,
			   double *low,
			   double *high,
                           size_t *length,
			   double **signal,
			   double **expected_filtered,
			   double **expected_coef)
{
  *sfreq = %f;
  *transition_bandwidth = %f;
  *low = %f;
  *high = %f;
  *length = %d;
  static const double my_signal[%d] =
    {
      %s
    };
  static const double my_expected_filtered[%d] =
    {
      %s
    };
  static const double my_expected_coef[%d] =
    {
      %s
    };
  *signal = malloc (%d * sizeof (double));
  *expected_filtered = malloc (%d * sizeof (double));
  *expected_coef = malloc (%d * sizeof (double));
  if (*signal == NULL || *expected_filtered == NULL || expected_coef == NULL)
    {
      abort ();
    }
  memcpy (*signal, my_signal, sizeof (my_signal));
  memcpy (*expected_filtered, my_expected_filtered, sizeof (my_expected_filtered));
  memcpy (*expected_coef, my_expected_coef, sizeof (my_expected_coef));
}
", sfreq, transition_bandwidth, low, high, length (data$unfiltered),
   length (data$unfiltered), init_signal,
   length (data$unfiltered), init_filtered,
   filter_order, init_coef,
   length (data$unfiltered), length (data$unfiltered), filter_order))
