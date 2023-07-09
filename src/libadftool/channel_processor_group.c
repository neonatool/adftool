#include <config.h>
#include <attribute.h>
#include <adftool.h>
#include <time.h>
#include <stdlib.h>
#include "channel_processor_group.h"

void
adftool_channel_processor_group_free (struct adftool_channel_processor_group
				      *group)
{
  channel_processor_group_free (group);
}

struct adftool_channel_processor_group *
adftool_channel_processor_group_alloc (struct adftool_file *file,
				       pthread_mutex_t * file_synchronizer,
				       size_t max_active_channels)
{
  return channel_processor_group_alloc (file, file_synchronizer,
					max_active_channels);
}

int
adftool_channel_processor_group_get (struct adftool_channel_processor_group
				     *group,
				     const struct adftool_term *channel_type,
				     double filter_low, double filter_high,
				     size_t start_index, size_t length,
				     size_t *nearest_start,
				     size_t *nearest_length, double *data)
{
  return channel_processor_group_get (group, channel_type, filter_low,
				      filter_high, start_index, length,
				      nearest_start, nearest_length, data);
}

int
adftool_channel_processor_group_populate_cache (struct
						adftool_channel_processor_group
						*group, int *work_done)
{
  bool aux = false;
  int error = channel_processor_group_populate_cache (group, &aux);
  *work_done = aux;
  return error;
}
