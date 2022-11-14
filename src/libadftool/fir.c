#include <adftool_private.h>
#include <adftool_bplus.h>
#include <math.h>

struct adftool_fir
{
  double sfreq;
  size_t half_m;
  double coef_0;
  double *coefficients;		/* just the strictly positive half_m of them. */
};

struct adftool_fir *
adftool_fir_alloc (double sfreq, double transition_bandwidth)
{
  struct adftool_fir *ret = malloc (sizeof (struct adftool_fir));
  if (ret != NULL)
    {
      const double bw = transition_bandwidth / sfreq;
      size_t m = 4 / bw;
      if (m < 8)
	{
	  /* bw > 0.5, every frequency is in the transition bandwidth,
	     so any response is correct: no filtering. This is very
	     suspicious, so we’ll still put 8 here. */
	  m = 8;
	}
      if (m % 2 == 1)
	{
	  m++;
	}
      ret->sfreq = sfreq;
      ret->half_m = m / 2;
      ret->coef_0 = 0;
      ret->coefficients = calloc (m / 2, sizeof (double));
      if (ret->coefficients == NULL)
	{
	  free (ret);
	  ret = NULL;
	}
    }
  return ret;
}

size_t
adftool_fir_order (const struct adftool_fir *filter)
{
  return 2 * filter->half_m + 1;
}

void
adftool_fir_free (struct adftool_fir *filter)
{
  if (filter)
    {
      free (filter->coefficients);
    }
  free (filter);
}

#define BLACKMAN \
  (0.42 - 0.5 * cos (2 * M_PI * i_window) \
   + 0.08 * cos (4 * M_PI * i_window))

#define HAMMING \
  (0.54 + 0.46 * cos (2 * M_PI * i_window))

#define WINDOW HAMMING

static void
add_lowpass (struct adftool_fir *filter, double freq)
{
  const double fc = freq / filter->sfreq;
  /* Compute K so that the sum of all coefficients is 1. */
  double coef_sum = 2 * M_PI * fc;
  const double window_size = 2 * filter->half_m;
  for (size_t i = 0; i < filter->half_m; i++)
    {
      const double i_f = i + 1;
      const double sinc = sin (2 * M_PI * fc * i_f) / i_f;
      const double i_window = i_f / window_size;
      const double window = WINDOW;
      coef_sum += 2 * sinc * window;
    }
  const double k = 1 / coef_sum;
  filter->coef_0 += 2 * M_PI * fc * k;
  for (size_t i = 0; i < filter->half_m; i++)
    {
      const double i_f = i + 1;
      const double sinc = sin (2 * M_PI * fc * i_f) / i_f;
      const double i_window = i_f / window_size;
      const double window = WINDOW;
      filter->coefficients[i] += k * sinc * window;
    }
}

static void
spectral_reverse (struct adftool_fir *filter)
{
  for (size_t i = 0; i < filter->half_m; i += 2)
    {
      filter->coefficients[i] = -(filter->coefficients[i]);
    }
}

static void
spectral_invert (struct adftool_fir *filter)
{
  filter->coef_0 = -(filter->coef_0);
  for (size_t i = 0; i < filter->half_m; i += 1)
    {
      filter->coefficients[i] = -(filter->coefficients[i]);
    }
  filter->coef_0 += 1;
}

void
adftool_fir_design_bandpass (struct adftool_fir *filter, double freq_low,
			     double freq_high)
{
  filter->coef_0 = 0;
  for (size_t i = 0; i < filter->half_m; i++)
    {
      filter->coefficients[i] = 0;
    }
  add_lowpass (filter, (filter->sfreq / 2) - freq_high);
  spectral_reverse (filter);
  add_lowpass (filter, freq_low);
  spectral_invert (filter);
}

void
adftool_fir_apply (const struct adftool_fir *filter, size_t signal_length,
		   const double *signal, double *filtered)
{
  for (size_t i = 0; i < signal_length; i++)
    {
      filtered[i] = filter->coef_0 * signal[i];
    }
  /* Forward pass: */
  for (size_t i = 0; i < signal_length; i++)
    {
      for (size_t j = 0; j < filter->half_m; j++)
	{
	  if (i + j < signal_length)
	    {
	      filtered[i] += filter->coefficients[j] * signal[i + j];
	    }
	}
    }
  /* Backward: */
  for (size_t i = 0; i < signal_length; i++)
    {
      for (size_t j = 0; j < filter->half_m; j++)
	{
	  if (i + j < signal_length)
	    {
	      filtered[i + j] += filter->coefficients[j] * signal[i];
	    }
	}
    }
}
