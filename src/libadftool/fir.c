#include <config.h>
#include <attribute.h>
#include <adftool.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

struct adftool_fir
{
  size_t half_m;
  double coef_0;
  double *coefficients;		/* just the strictly positive half_m of them. */
};

void
adftool_fir_auto_bandwidth (double sfreq, double freq_low, double freq_high,
			    double *trans_low, double *trans_high)
{
  /* MNE: Why compare transition bandwidths to 2 Hz?????? */
  *trans_low = freq_low / 4;
  if (*trans_low < 2)
    {
      *trans_low = 2;
    }
  if (*trans_low > freq_low)
    {
      *trans_low = freq_low;
    }
  *trans_high = freq_high / 4;
  if (*trans_high < 2)
    {
      *trans_high = 2;
    }
  if (*trans_high > sfreq / 2 - freq_high)
    {
      *trans_high = sfreq / 2 - freq_high;
    }
}

size_t
adftool_fir_auto_order (double sfreq, double bw)
{
  size_t relative = ceil (3.3 * sfreq / bw);
  if (relative % 2 == 0)
    {
      relative++;
    }
  return relative;
}

struct adftool_fir *
adftool_fir_alloc (size_t order)
{
  struct adftool_fir *ret = malloc (sizeof (struct adftool_fir));
  if (ret != NULL)
    {
      assert (order % 2 == 1);
      ret->half_m = order / 2;
      ret->coef_0 = 0;
      ret->coefficients = calloc (order / 2, sizeof (double));
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
adftool_fir_coefficients (const struct adftool_fir *filter,
			  double *coefficients)
{
  coefficients[filter->half_m] = filter->coef_0;
  for (size_t i = 0; i < filter->half_m; i++)
    {
      const size_t low_index = filter->half_m - i - 1;
      const size_t high_index = filter->half_m + i + 1;
      const double coef = filter->coefficients[i];
      coefficients[low_index] = coef;
      coefficients[high_index] = coef;
    }
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

void
adftool_fir_design_bandpass (struct adftool_fir *filter, double sfreq,
			     double freq_low, double freq_high,
			     double trans_low, double trans_high)
{
  for (size_t i = 0; i < filter->half_m; i++)
    {
      filter->coefficients[i] = 0;
    }
  const double fcl = (freq_low - trans_low / 2) / sfreq;
  const double fch = (freq_high + trans_high / 2) / sfreq;
  filter->coef_0 = 2 * (fch - fcl);
  /* Add the low-pass component: up to fch */
  if (fch * 2 < 1)
    {
      const size_t stop = adftool_fir_auto_order (sfreq, trans_high) / 2;
      assert (stop <= filter->half_m);
      const double lowpass_window_size = 2 * stop;
      for (size_t i = 0; i < stop; i++)
	{
	  const double i_f = i + 1;
	  const double i_window = i_f / lowpass_window_size;
	  if (i_window <= 0.5)
	    {
	      const double sinc = (sin (2 * M_PI * fch * i_f) / i_f) / M_PI;
	      const double window = WINDOW;
	      filter->coefficients[i] += sinc * window;
	    }
	}
    }
  /* Add the high-pass component: start at fcl */
  if (fcl > 0)
    {
      const size_t stop = adftool_fir_auto_order (sfreq, trans_low) / 2;
      assert (stop <= filter->half_m);
      const double highpass_window_size = 2 * stop;
      for (size_t i = 0; i < stop; i++)
	{
	  const double i_f = i + 1;
	  const double i_window = i_f / highpass_window_size;
	  if (i_window <= 0.5)
	    {
	      const double sinc = -(sin (2 * M_PI * fcl * i_f) / i_f) / M_PI;
	      const double window = WINDOW;
	      filter->coefficients[i] += sinc * window;
	    }
	}
    }
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
	  if (i + j + 1 < signal_length)
	    {
	      filtered[i] += filter->coefficients[j] * signal[i + j + 1];
	    }
	}
    }
  /* Backward: */
  for (size_t i = 0; i < signal_length; i++)
    {
      for (size_t j = 0; j < filter->half_m; j++)
	{
	  if (i + j + 1 < signal_length)
	    {
	      filtered[i + j + 1] += filter->coefficients[j] * signal[i];
	    }
	}
    }
}
