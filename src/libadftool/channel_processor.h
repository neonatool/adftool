#ifndef H_ADFTOOL_CHANNEL_PROCESSOR_INCLUDED
# define H_ADFTOOL_CHANNEL_PROCESSOR_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

# include "term.h"

# include <stdlib.h>
# include <assert.h>
# include <string.h>
# include <locale.h>
# include <stdbool.h>
# include <pthread.h>
# include <math.h>
# include "safe-alloc.h"

# include "gettext.h"

# ifndef _
#  ifdef BUILDING_LIBADFTOOL
#   define _(String) dgettext (PACKAGE, (String))
#   define N_(String) (String)
#  else
#   define _(String) gettext (String)
#   define N_(String) (String)
#  endif
# endif/* not _ */

static inline void ensure_init (void);

void _adftool_ensure_init (void);

static inline void
ensure_init (void)
{
  static volatile int is_initialized = 0;
  if (!is_initialized)
    {
      _adftool_ensure_init ();
      is_initialized = 1;
    }
}

# define DEALLOC_CHANNEL_PROCESSOR \
  ATTRIBUTE_DEALLOC (channel_processor_free, 1)

struct adftool_channel_processor;

MAYBE_UNUSED static void
channel_processor_free (struct adftool_channel_processor *reader);

MAYBE_UNUSED DEALLOC_CHANNEL_PROCESSOR
  static struct adftool_channel_processor
  *channel_processor_alloc (struct adftool_file *file,
			    pthread_mutex_t * file_synchronizer,
			    const struct adftool_term *channel_type,
			    double filter_low, double filter_high);

MAYBE_UNUSED
  static bool channel_processor_can_serve (const struct
					   adftool_channel_processor
					   *processor,
					   const struct adftool_term
					   *channel_type, double filter_low,
					   double filter_high);

MAYBE_UNUSED
  static int channel_processor_get (struct adftool_channel_processor
				    *processor, size_t start_index,
				    size_t length, size_t *nearest_start,
				    size_t *nearest_length, double *data);

MAYBE_UNUSED
  static int
channel_processor_populate_cache (struct adftool_channel_processor
				  *processor, bool *work_done);

struct adftool_channel_processor_page
{
  size_t index;
  double scale;
  int16_t data[5120];
};

struct adftool_channel_processor
{
  struct adftool_file *file;
  struct adftool_term *channel_type;
  size_t channel_index;
  struct adftool_fir *filter;
  double filter_low;
  double filter_high;
  pthread_mutex_t *file_synchronizer;
  pthread_mutex_t cache_synchronizer;
  struct adftool_channel_processor_page *cache[256];
  size_t start_index;
  size_t window_length;
  size_t time_max;
};

static inline bool
channel_processor_page_is_relevant (const struct adftool_channel_processor
				    *processor,
				    const struct
				    adftool_channel_processor_page *page)
{
  const size_t page_start =
    page->index * (sizeof (page->data) / sizeof (page->data[0]));
  const size_t page_length = sizeof (page->data) / sizeof (page->data[0]);
  const size_t page_stop = page_start + page_length;
  const size_t request_start = processor->start_index;
  const size_t request_length = processor->window_length;
  const size_t request_stop = request_start + request_length;
  const bool page_is_too_early = page_stop <= request_start;
  const bool page_is_too_late = page_start >= request_stop;
  const bool page_is_irrelevant = page_is_too_early || page_is_too_late;
  return !page_is_irrelevant;
}

static inline void
channel_processor_swap_pages (struct adftool_channel_processor *processor,
			      size_t i, size_t j)
{
  struct adftool_channel_processor_page *hold = processor->cache[i];
  processor->cache[i] = processor->cache[j];
  processor->cache[j] = hold;
}

static inline size_t
channel_processor_reorder (struct adftool_channel_processor *processor)
{
  /* Put the relevant cache pages on top of the cache. Return the
     number of relevant cache pages. If it is 256, then there is
     nothing to optimize. */
  size_t n_relevant = 0;
  size_t n_checked = 0;
  while (n_checked < sizeof (processor->cache) / sizeof (processor->cache[0]))
    {
      /* Loop invariant: for each i < n_relevant, page i is relevant. */
      /* Loop invariant: for each n_relevant <= i < n_checked, page i
         is irrelevant. */
      assert (n_relevant <= n_checked);
      if (processor->cache[n_checked] != NULL
	  && channel_processor_page_is_relevant (processor,
						 processor->cache[n_checked]))
	{
	  channel_processor_swap_pages (processor, n_checked, n_relevant);
	  n_relevant++;
	}
      n_checked++;
    }
  return n_relevant;
}

static inline int
channel_processor_get_data_page (struct adftool_channel_processor *processor,
				 size_t page_index,
				 size_t *restrict margin_before,
				 size_t *restrict margin_after,
				 size_t *restrict length,
				 size_t *restrict time_max, double **data)
{
  int error = 0;
  const size_t page_size =
    sizeof (processor->cache[0]->data) /
    sizeof (processor->cache[0]->data[0]);
  const size_t start_index = page_index * page_size;
  if (pthread_mutex_lock (processor->file_synchronizer) != 0)
    {
      error = -2;
      goto cleanup;
    }
  const size_t normal_margin = adftool_fir_order (processor->filter);
  *margin_after = normal_margin;
  if (normal_margin >= start_index)
    {
      /* Subtracting normal_margin to the start index would be a negative number. */
      *margin_before = start_index;
    }
  else
    {
      *margin_before = normal_margin;
    }
  assert (*margin_before <= normal_margin);
  assert (start_index >= *margin_before);
  /* i.e. start_index - *margin_before >= 0 */
  *length = page_size;
  const size_t total_length = *margin_before + *length + *margin_after;
  if (ALLOC_N (*data, total_length) < 0)
    {
      error = -2;
      goto unlock;
    }
  size_t channel_max;
  error =
    adftool_eeg_get_data (processor->file, start_index - *margin_before,
			  *margin_before + *length + *margin_after, time_max,
			  processor->channel_index, 1, &channel_max, *data);
unlock:
  if (pthread_mutex_unlock (processor->file_synchronizer) != 0)
    {
      abort ();
    }
cleanup:
  return error;
}

static inline int
channel_processor_push_page (struct adftool_channel_processor *processor,
			     size_t page_index, bool *work_done)
{
  int error = 0;
  struct adftool_channel_processor_page *page = NULL;
  for (size_t i = 0;
       i < sizeof (processor->cache) / sizeof (processor->cache[0]); i++)
    {
      if (processor->cache[i] != NULL
	  && processor->cache[i]->index == page_index)
	{
	  page = processor->cache[i];
	  processor->cache[i] = NULL;
	  break;
	}
    }
  if (page == NULL)
    {
      if (ALLOC (page) < 0)
	{
	  error = -2;
	  goto cleanup;
	}
      size_t margin_before, margin_after;
      size_t page_size;
      double *data_to_filter;
      error =
	channel_processor_get_data_page (processor, page_index,
					 &margin_before, &margin_after,
					 &page_size, &(processor->time_max),
					 &data_to_filter);
      if (error < 0)
	{
	  goto cleanup_page;
	}
      double *filtered;
      if (ALLOC_N (filtered, margin_before + page_size + margin_after) < 0)
	{
	  goto cleanup_data_to_filter;
	}
      adftool_fir_apply (processor->filter,
			 margin_before + page_size + margin_after,
			 data_to_filter, filtered);
      double amplitude_max = 0;
      for (size_t i = 0; i < page_size; i++)
	{
	  double ampl = filtered[i + margin_before];
	  if (ampl < 0)
	    {
	      ampl = -ampl;
	    }
	  if (ampl > amplitude_max)
	    {
	      amplitude_max = ampl;
	    }
	}
      page->index = page_index;
      page->scale = amplitude_max / 32767.0;
      for (size_t i = 0; i < page_size; i++)
	{
	  double v = 0;
	  if (amplitude_max != 0)
	    {
	      v = round (filtered[i + margin_before] / page->scale);
	    }
	  assert (v >= -32767);
	  assert (v <= 32767);
	  page->data[i] = v;
	}
      FREE (filtered);
    cleanup_data_to_filter:
      FREE (data_to_filter);
      if (error)
	{
	cleanup_page:
	  FREE (page);
	  goto cleanup;
	}
      *work_done = true;
    }
  for (size_t i = 0;
       i < sizeof (processor->cache) / sizeof (processor->cache[0])
       && page != NULL; i++)
    {
      struct adftool_channel_processor_page *current = processor->cache[i];
      processor->cache[i] = page;
      page = current;
    }
  FREE (page);
cleanup:
  return error;
}

static int
channel_processor_populate_cache (struct adftool_channel_processor *processor,
				  bool *work_done)
{
  int error = 0;
  const size_t page_size =
    sizeof (processor->cache[0]->data) /
    sizeof (processor->cache[0]->data[0]);
  *work_done = false;
  if (pthread_mutex_lock (&(processor->cache_synchronizer)) != 0)
    {
      error = -2;
      goto cleanup;
    }
  size_t n_relevant = channel_processor_reorder (processor);
  size_t n_total = sizeof (processor->cache) / sizeof (processor->cache[0]);
  size_t n_irrelevant = n_total - n_relevant;
  const size_t first_page_to_load = processor->start_index / page_size;
  const size_t index_stop = processor->start_index + processor->window_length;
  for (size_t i = first_page_to_load;
       i * page_size < index_stop && (i - first_page_to_load) < n_irrelevant;
       i++)
    {
      error = channel_processor_push_page (processor, i, work_done);
      if (error != 0)
	{
	  goto unlock;
	}
    }
unlock:
  if (pthread_mutex_unlock (&(processor->cache_synchronizer)) != 0)
    {
      abort ();
    }
cleanup:
  return error;
}

static void
channel_processor_free (struct adftool_channel_processor *processor)
{
  if (processor != NULL)
    {
      adftool_term_free (processor->channel_type);
      adftool_fir_free (processor->filter);
      for (size_t i = 0;
	   i < sizeof (processor->cache) / sizeof (processor->cache[0]); i++)
	{
	  FREE (processor->cache[i]);
	}
      if (pthread_mutex_destroy (&(processor->cache_synchronizer)) != 0)
	{
	  abort ();
	}
    }
  FREE (processor);
}

static struct adftool_channel_processor *
channel_processor_alloc (struct adftool_file *file,
			 pthread_mutex_t * file_synchronizer,
			 const struct adftool_term *channel_type,
			 double filter_low, double filter_high)
{
  struct adftool_channel_processor *ret;
  struct adftool_term *channel = term_alloc ();
  if (channel == NULL)
    {
      return NULL;
    }
  if (ALLOC (ret) == 0)
    {
      ret->file = file;
      ret->channel_type = term_alloc ();
      if (ret->channel_type == NULL)
	{
	  goto cleanup;
	}
      term_copy (ret->channel_type, channel_type);
      size_t n_candidates =
	adftool_find_channels_by_type (file, channel_type, 0, 1, &channel);
      if (n_candidates != 1)
	{
	  goto cleanup_channel_type;
	}
      int error = adftool_get_channel_column (file, channel,
					      &(ret->channel_index));
      if (error != 0)
	{
	  goto cleanup_channel_type;
	}
      struct timespec start_time;
      double sfreq;
      error = adftool_eeg_get_time (file, 0, &start_time, &sfreq);
      if (error != 0)
	{
	  goto cleanup_channel_type;
	}
      double trans_low, trans_high, smallest_trans;
      adftool_fir_auto_bandwidth (sfreq, filter_low, filter_high, &trans_low,
				  &trans_high);
      smallest_trans = trans_low;
      if (trans_high < trans_low)
	{
	  smallest_trans = trans_high;
	}
      size_t order = adftool_fir_auto_order (sfreq, smallest_trans);
      ret->filter = adftool_fir_alloc (order);
      if (ret->filter == NULL)
	{
	  goto cleanup_channel_type;
	}
      adftool_fir_design_bandpass (ret->filter, sfreq, filter_low,
				   filter_high, trans_low, trans_high);
      ret->filter_low = filter_low;
      ret->filter_high = filter_high;
      ret->file_synchronizer = file_synchronizer;
      for (size_t i = 0; i < sizeof (ret->cache) / sizeof (ret->cache[0]);
	   i++)
	{
	  ret->cache[i] = NULL;
	}
      ret->start_index = 0;
      ret->window_length = 5120;
      ret->time_max = 0;
      error = pthread_mutex_init (&(ret->cache_synchronizer), NULL);
      if (error != 0)
	{
	  goto cleanup_filter;
	}
    }
  term_free (channel);
  return ret;
cleanup_filter:
  adftool_fir_free (ret->filter);
cleanup_channel_type:
  term_free (ret->channel_type);
cleanup:
  FREE (ret);
  term_free (channel);
  return NULL;
}

static int
channel_processor_get (struct adftool_channel_processor *processor,
		       size_t start_index,
		       size_t length,
		       size_t *nearest_start,
		       size_t *nearest_length, double *data)
{
  int error = 0;
  for (size_t i = 0; i < length; i++)
    {
      data[i] = NAN;
    }
  error = pthread_mutex_lock (&(processor->cache_synchronizer));
  if (error != 0)
    {
      error = -2;
      goto cleanup;
    }
  if (processor->time_max == 0)
    {
      /* Assume we do not know the maximum time index yet. */
    }
  else
    {
      size_t stop_index = start_index + length;
      if (stop_index > processor->time_max)
	{
	  size_t back = stop_index - processor->time_max;
	  if (start_index >= back)
	    {
	      start_index -= back;
	    }
	  else
	    {
	      /* First, move the window as far backward as it can
	         go. */
	      back -= start_index;
	      start_index = 0;
	      /* Then, shrink the window. */
	      length = processor->time_max;
	    }
	  stop_index = start_index + length;
	}
      assert (stop_index <= processor->time_max);
    }
  *nearest_start = start_index;
  *nearest_length = length;
  processor->start_index = start_index;
  processor->window_length = length;
  const size_t n_pages =
    sizeof (processor->cache) / sizeof (processor->cache[0]);
  for (size_t i = 0; i < n_pages; i++)
    {
      const struct adftool_channel_processor_page *page = processor->cache[i];
      if (page != NULL
	  && channel_processor_page_is_relevant (processor, page))
	{
	  const size_t page_size =
	    sizeof (page->data) / sizeof (page->data[0]);
	  const size_t page_start = page->index * page_size;
	  for (size_t i = 0; i < page_size; i++)
	    {
	      const size_t index_abs = i + page_start;
	      if (index_abs >= start_index
		  && index_abs - start_index < length)
		{
		  data[index_abs - start_index] = page->data[i] * page->scale;
		}
	    }
	}
    }
  error = pthread_mutex_unlock (&(processor->cache_synchronizer));
  if (error != 0)
    {
      abort ();
    }
cleanup:
  return error;
}

static bool
channel_processor_can_serve (const struct adftool_channel_processor
			     *processor,
			     const struct adftool_term *channel_type,
			     double filter_low, double filter_high)
{
  return (filter_low == processor->filter_low
	  && filter_high == processor->filter_high
	  && term_compare (processor->channel_type, channel_type) == 0);
}

#endif /* not H_ADFTOOL_CHANNEL_PROCESSOR_INCLUDED */
