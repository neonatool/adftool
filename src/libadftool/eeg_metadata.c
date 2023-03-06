#include <config.h>
#include <attribute.h>
#include <adftool.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "term.h"
#include "statement.h"
#include "file.h"

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
get_start_date (struct adftool_file *file, struct timespec *time)
{
  struct adftool_term *object = term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  size_t n_results =
    adftool_lookup_objects (file, &default_eeg, lyto_start_date, 0, 1,
			    &object);
  if (n_results > 0)
    {
      if (term_as_date (object, time) != 0)
	{
	  n_results = 0;
	}
    }
  term_free (object);
  return (n_results == 0);
}

static int
get_sampling_frequency (struct adftool_file *file, double *sampling_frequency)
{
  struct adftool_term *object = term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  size_t n_results =
    adftool_lookup_objects (file, &default_eeg, lyto_sampling_frequency, 0, 1,
			    &object);
  if (n_results > 0)
    {
      if (term_as_double (object, sampling_frequency) != 0)
	{
	  n_results = 0;
	}
      else
	{
	  /* sampling_frequency is set because term_as_double
	     succeeded. */
	  assert (*sampling_frequency >= 0);
	}
    }
  term_free (object);
  return (n_results == 0);
}

int
adftool_eeg_get_time (struct adftool_file *file,
		      size_t observation,
		      struct timespec *time, double *sampling_frequency)
{
  double sfreq = 0;
  bool sfreq_initialized = false;
  if ((time && observation != 0) || sampling_frequency)
    {
      if (get_sampling_frequency (file, &sfreq) != 0)
	{
	  return 1;
	}
      sfreq_initialized = true;
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
      assert (sfreq_initialized);
      *sampling_frequency = sfreq;
    }
  return 0;
}

int
adftool_eeg_set_time (struct adftool_file *file, const struct timespec *time,
		      double sampling_frequency)
{
  int error = 0;
  struct adftool_term *o_time = term_alloc ();
  struct adftool_term *o_sfreq = term_alloc ();
  if (o_time == NULL || o_sfreq == NULL)
    {
      abort ();
    }
  term_set_date (o_time, time);
  term_set_double (o_sfreq, sampling_frequency);
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
  error = (adftool_file_insert (file, &set_time)
	   || adftool_file_insert (file, &set_sfreq));
  term_free (o_time);
  term_free (o_sfreq);
  return error;
}
