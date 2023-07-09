#ifndef H_ADFTOOL_CHANNEL_PROCESSOR_GROUP_INCLUDED
# define H_ADFTOOL_CHANNEL_PROCESSOR_GROUP_INCLUDED

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

# define DEALLOC_CHANNEL_PROCESSOR_GROUP \
  ATTRIBUTE_DEALLOC (channel_processor_group_free, 1)

struct adftool_channel_processor_group;

MAYBE_UNUSED static void
channel_processor_group_free (struct adftool_channel_processor_group *group);

MAYBE_UNUSED DEALLOC_CHANNEL_PROCESSOR_GROUP
  static struct adftool_channel_processor_group
  *channel_processor_group_alloc (struct adftool_file *file,
				  pthread_mutex_t * file_synchronizer,
				  size_t max_active_channels);

MAYBE_UNUSED
  static int channel_processor_group_get (struct
					  adftool_channel_processor_group
					  *group,
					  const struct adftool_term
					  *channel_type, double filter_low,
					  double filter_high,
					  size_t start_index, size_t length,
					  size_t *nearest_start,
					  size_t *nearest_length,
					  double *data);

MAYBE_UNUSED
  static int
channel_processor_group_populate_cache (struct adftool_channel_processor_group
					*group, bool *work_done);

# include "channel_processor.h"

struct adftool_channel_processor_group
{
  struct adftool_file *file;
  pthread_mutex_t *file_synchronizer;
  pthread_mutex_t channel_list_synchronizer;
  size_t n_active_channels;
  size_t max_active_channels;
  struct adftool_channel_processor **active_channels;
  size_t next_to_populate;
};

static struct adftool_channel_processor_group *
channel_processor_group_alloc (struct adftool_file *file,
			       pthread_mutex_t * file_synchronizer,
			       size_t max_active_channels)
{
  int error = 0;
  struct adftool_channel_processor_group *ret = NULL;
  ensure_init ();
  if (ALLOC (ret) < 0)
    {
      error = -2;
      goto cleanup;
    }
  ret->file = file;
  ret->file_synchronizer = file_synchronizer;
  error = pthread_mutex_init (&(ret->channel_list_synchronizer), NULL);
  if (error != 0)
    {
      error = -2;
      goto cleanup;
    }
  ret->n_active_channels = 0;
  ret->max_active_channels = max_active_channels;
  if (ALLOC_N (ret->active_channels, max_active_channels) < 0)
    {
      error = -2;
      goto cleanup;
    }
  ret->next_to_populate = 0;
cleanup:
  if (error != 0)
    {
      channel_processor_group_free (ret);
      ret = NULL;
    }
  return ret;
}

static void
channel_processor_group_free (struct adftool_channel_processor_group *group)
{
  for (size_t i = 0; i < group->n_active_channels; i++)
    {
      channel_processor_free (group->active_channels[i]);
      group->active_channels[i] = NULL;
    }
  FREE (group->active_channels);
  if (pthread_mutex_destroy (&(group->channel_list_synchronizer)) != 0)
    {
      abort ();
    }
  FREE (group);
}

static int
channel_processor_group_find_task (struct adftool_channel_processor_group
				   *group,
				   const struct adftool_term *channel_type,
				   double filter_low, double filter_high,
				   struct adftool_channel_processor **task)
{
  /* Find the task in the queue and bring it to front, or allocate one
     and push it at the front if no task has been allocated for that
     triple (channel_type, filter_low, filter_high). */
  int error = 0;
  if (pthread_mutex_lock (&(group->channel_list_synchronizer)) != 0)
    {
      error = -1;
      goto cleanup;
    }
  size_t i = 0;
  for (i = 0;
       (i < group->n_active_channels)
       && (!channel_processor_can_serve (group->active_channels[i],
					 channel_type, filter_low,
					 filter_high)); i++)
    ;
  if (i == group->n_active_channels)
    {
      struct adftool_channel_processor *new_task =
	channel_processor_alloc (group->file, group->file_synchronizer,
				 channel_type, filter_low, filter_high);
      if (new_task == NULL)
	{
	  error = -2;
	  goto unlock;
	}
      if (i >= group->max_active_channels)
	{
	  /* Drop the last one. */
	  assert (group->max_active_channels != 0);
	  channel_processor_free (group->active_channels
				  [group->max_active_channels - 1]);
	  group->n_active_channels -= 1;
	  i = group->n_active_channels;
	}
      assert (i < group->max_active_channels);
      group->n_active_channels += 1;
      group->active_channels[i] = new_task;
    }
  *task = group->active_channels[i];
  for (size_t j = i; j-- > 0;)
    {
      group->active_channels[j + 1] = group->active_channels[j];
    }
  group->active_channels[0] = *task;
unlock:
  if (pthread_mutex_unlock (&(group->channel_list_synchronizer)) != 0)
    {
      error = -1;
      goto cleanup;
    }
cleanup:
  return error;
}

static int
channel_processor_group_get (struct adftool_channel_processor_group *group,
			     const struct adftool_term *channel_type,
			     double filter_low,
			     double filter_high,
			     size_t start_index,
			     size_t length,
			     size_t *nearest_start,
			     size_t *nearest_length, double *data)
{
  struct adftool_channel_processor *task = NULL;
  int error =
    channel_processor_group_find_task (group, channel_type, filter_low,
				       filter_high, &task);
  if (error != 0)
    {
      return error;
    }
  return channel_processor_get (task, start_index, length, nearest_start,
				nearest_length, data);
}

static int
channel_processor_group_populate_cache (struct adftool_channel_processor_group
					*group, bool *work_done)
{
  /* Try and populate each element in the queue, until one has done
     work. */
  int error = 0;
  size_t n_tried = 0, n_max = 42;
  *work_done = false;
  do
    {
      if (pthread_mutex_lock (&(group->channel_list_synchronizer)) != 0)
	{
	  error = -2;
	  goto cleanup;
	}
      n_max = group->n_active_channels;
      struct adftool_channel_processor *next = NULL;
      if (group->n_active_channels != 0)
	{
	  if (group->next_to_populate >= group->n_active_channels)
	    {
	      group->next_to_populate = 0;
	    }
	  next = group->active_channels[group->next_to_populate];
	  group->next_to_populate += 1;
	}
      if (pthread_mutex_unlock (&(group->channel_list_synchronizer)) != 0)
	{
	  error = -2;
	  goto cleanup;
	}
      if (n_max != 0)
	{
	  assert (next != NULL);
	  error = channel_processor_populate_cache (next, work_done);
	  if (error != 0)
	    {
	      goto cleanup;
	    }
	  n_tried++;
	}
      else
	{
	  /* Exit do…while immediately. */
	}
    }
  while (n_tried < n_max && !(*work_done));
cleanup:
  return error;
}

#endif /* not H_ADFTOOL_CHANNEL_PROCESSOR_GROUP_INCLUDED */
