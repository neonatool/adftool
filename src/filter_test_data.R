data <- read.csv ('src/filter_test_data.csv')
coef <- read.csv ('src/filter_test_coef.csv')

sfreq <- 600.6 # MNE told me so
low <- 0.3 # I chose it
high <- 30 # I chose it

filter_order <- 6607

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

cat (sprintf ("#include <config.h>

#include <stdlib.h>
#include <string.h>

void
_adftool_filter_test_data (double *sfreq,
			   double *low,
			   double *high,
                           size_t *length,
			   double **signal,
			   double **expected_filtered,
                           size_t *filter_length,
			   double **expected_coef)
{
  *sfreq = %f;
  *low = %f;
  *high = %f;
  *length = %d;
  *filter_length = %d;
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
",
sfreq,
low,
high,
length (data$unfiltered),
filter_order,

length (data$unfiltered), init_signal,
length (data$unfiltered), init_filtered,
filter_order, init_coef,

length (data$unfiltered),
length (data$unfiltered),
filter_order))
