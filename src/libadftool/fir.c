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
adftool_fir_alloc_n (double sfreq, size_t order)
{
  struct adftool_fir *ret = malloc (sizeof (struct adftool_fir));
  if (ret != NULL)
    {
      assert (order % 2 == 1);
      ret->sfreq = sfreq;
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

struct adftool_fir *
adftool_fir_alloc (double sfreq, double transition_bandwidth)
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
  if (m % 2 == 0)
    {
      m++;
    }
  return adftool_fir_alloc_n (sfreq, m);
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
adftool_fir_design_bandpass (struct adftool_fir *filter, double freq_low,
			     double freq_high)
{
  const double fcl = freq_low / filter->sfreq;
  const double fch = freq_high / filter->sfreq;
  const double window_size = 2 * filter->half_m;
  filter->coef_0 = 2 * (fch - fcl);
  for (size_t i = 0; i < filter->half_m; i++)
    {
      const double i_f = i + 1;
      const double sincdiff =
	(((sin (2 * M_PI * fch * i_f) / i_f)
	  - (sin (2 * M_PI * fcl * i_f) / i_f)) / M_PI);
      const double i_window = i_f / window_size;
      const double window = WINDOW;
      filter->coefficients[i] = sincdiff * window;
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
