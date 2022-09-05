#include <adftool_private.h>
#include <time.h>
#include <math.h>

static const struct adftool_term default_eeg = {
  .type = TERM_NAMED,
  .str1 = "",
  .str2 = NULL
};

static const char *const lyto_start_date =
  "https://localhost/lytonepal#start-date";
static const char *const lyto_sampling_frequency =
  "https://localhost/lytonepal#sampling-frequency";

static const struct adftool_term p_start_date = {
  .type = TERM_NAMED,
  .str1 = (char *) lyto_start_date,
  .str2 = NULL
};

static const struct adftool_term p_sampling_frequency = {
  .type = TERM_NAMED,
  .str1 = (char *) lyto_sampling_frequency,
  .str2 = NULL
};

static int
get_start_date (const struct adftool_file *file, struct timespec *time)
{
  struct adftool_term *object = adftool_term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  size_t n_results =
    adftool_lookup_objects (file, &default_eeg, lyto_start_date, 0, 1,
			    &object);
  if (n_results > 0)
    {
      if (adftool_term_as_date (object, time) != 0)
	{
	  n_results = 0;
	}
    }
  adftool_term_free (object);
  return (n_results == 0);
}

static int
get_sampling_frequency (const struct adftool_file *file,
			double *sampling_frequency)
{
  struct adftool_term *object = adftool_term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  mpf_t sfreq;
  mpf_init (sfreq);
  size_t n_results =
    adftool_lookup_objects (file, &default_eeg, lyto_sampling_frequency, 0, 1,
			    &object);
  if (n_results > 0)
    {
      if (adftool_term_as_double (object, sfreq) != 0)
	{
	  n_results = 0;
	}
    }
  if (n_results > 0)
    {
      *sampling_frequency = mpf_get_d (sfreq);
    }
  mpf_clear (sfreq);
  adftool_term_free (object);
  return (n_results == 0);
}

int
adftool_eeg_get_time (const struct adftool_file *file,
		      size_t observation,
		      struct timespec *time, double *sampling_frequency)
{
  double sfreq;
  if ((time && observation != 0) || sampling_frequency)
    {
      if (get_sampling_frequency (file, &sfreq) != 0)
	{
	  return 1;
	}
    }
  /* sfreq is set if needed. */
  if (time)
    {
      double delay = 0;
      if (observation != 0)
	{
	  delay = observation / sfreq;
	}
      if (get_start_date (file, time) != 0)
	{
	  return 1;
	}
      double integer_part;
      const double frac = modf (delay, &integer_part);
      const long added_sec = integer_part;
      const long added_nsec = roundf (frac * 1000 * 1000 * 1000);
      time->tv_sec += added_sec;
      time->tv_nsec += added_nsec;
      if (time->tv_nsec >= 1000 * 1000 * 1000)
	{
	  time->tv_sec++;
	  time->tv_nsec -= 1000 * 1000 * 1000;
	}
    }
  if (sampling_frequency)
    {
      *sampling_frequency = sfreq;
    }
  return 0;
}

int
adftool_eeg_set_time (struct adftool_file *file, const struct timespec *time,
		      double sampling_frequency)
{
  int error = 0;
  struct adftool_term *o_time = adftool_term_alloc ();
  struct adftool_term *o_sfreq = adftool_term_alloc ();
  if (o_time == NULL || o_sfreq == NULL)
    {
      abort ();
    }
  adftool_term_set_date (o_time, time);
  mpf_t sfreq;
  mpf_init_set_d (sfreq, sampling_frequency);
  adftool_term_set_double (o_sfreq, sfreq);
  mpf_clear (sfreq);
  const struct adftool_statement set_time = {
    .subject = (struct adftool_term *) &default_eeg,
    .predicate = (struct adftool_term *) &p_start_date,
    .object = o_time,
    .graph = NULL,
    .deletion_date = ((uint64_t) (-1))
  };
  const struct adftool_statement set_sfreq = {
    .subject = (struct adftool_term *) &default_eeg,
    .predicate = (struct adftool_term *) &p_sampling_frequency,
    .object = o_sfreq,
    .graph = NULL,
    .deletion_date = ((uint64_t) (-1))
  };
  error = (adftool_insert (file, &set_time)
	   || adftool_insert (file, &set_sfreq));
  adftool_term_free (o_time);
  adftool_term_free (o_sfreq);
  return error;
}
