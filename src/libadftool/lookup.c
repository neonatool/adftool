#include <adftool_private.h>

static int
is_leaf (int order, const uint32_t * row)
{
  uint32_t flags = row[2 * order];
  uint32_t bit = 1;
  uint32_t flag_is_leaf = bit << 31;
  return (flags & flag_is_leaf) != 0;
}

static int
do_fetch (const struct adftool_bplus_parameters *parameters,
	  uint32_t node_id, int *order, size_t *node_data_allocated,
	  uint32_t ** node_data)
{
  size_t node_data_required;
  int fetch_error =
    adftool_bplus_parameters_fetch (parameters, node_id, &node_data_required,
				    0, *node_data_allocated, *node_data);
  if (!fetch_error && node_data_required != *node_data_allocated)
    {
      assert (node_data_required % 2 == 1);
      *order = (node_data_required - 1) / 2;
      *node_data_allocated = node_data_required;
      uint32_t *new_node_data =
	realloc (*node_data, node_data_required * sizeof (uint32_t));
      if (new_node_data == NULL)
	{
	  abort ();
	}
      *node_data = new_node_data;
      fetch_error =
	adftool_bplus_parameters_fetch (parameters, node_id,
					&node_data_required, 0,
					*node_data_allocated, *node_data);
      if (!fetch_error)
	{
	  assert (node_data_required == *node_data_allocated);
	}
    }
  return fetch_error;
}

int
adftool_bplus_lookup (const struct adftool_bplus_key *needle,
		      const struct adftool_bplus_parameters
		      *parameters, size_t start, size_t max,
		      size_t *n_results, uint32_t * results)
{
  int order = 0;
  uint32_t current_node = 0;
  size_t current_key = 0;
  int compared_to_current = 0;
  size_t node_data_allocated = 0;
  uint32_t *node_data = malloc (0);
  if (node_data == NULL)
    {
      abort ();
    }
#define DO_FETCH \
  do_fetch (parameters, current_node, \
	    &order, &node_data_allocated, &node_data)
  int error = 0;
  int first_found = 0;
  /* The first step is to find the first match. */
  do
    {
      error = DO_FETCH;
      if (!error)
	{
	  assert (order > 0);
	  for (current_key = 0;
	       current_key < (size_t) (order - 1)
	       && node_data[current_key] != (uint32_t) (-1); current_key++)
	    {
	      struct adftool_bplus_key cursor;
	      cursor.type = ADFTOOL_BPLUS_KEY_KNOWN;
	      cursor.arg.known = node_data[current_key];
	      error =
		parameters->compare (needle, &cursor, &compared_to_current,
				     parameters->compare_context);
	      if (error)
		{
		  break;
		}
	      if (compared_to_current <= 0)
		{
		  /* Will descend down to current_key. */
		  break;
		}
	    }
	}
      if (!error)
	{
	  if (is_leaf (order, node_data))
	    {
	      first_found = 1;
	    }
	  else
	    {
	      current_node = node_data[current_key + order - 1];
	      assert (current_node != 0);
	      current_key = 0;
	      error = DO_FETCH;
	    }
	}
    }
  while (error == 0 && !first_found);
  /* Now that we found the first match, fill in the results until the
     end. */
  *n_results = 0;
  if (!error)
    {
      /* We are past the last key iff compared_to_current > 0 */
      assert ((current_key >= (size_t) (order - 1)
	       || node_data[current_key] == (uint32_t) (-1)) ==
	      (compared_to_current > 0));
    }
  while (!error && compared_to_current == 0)
    {
      /* So we point to a valid key in a leaf node. */
      size_t index = (*n_results)++;
      if (index >= start && index < start + max)
	{
	  results[index - start] = node_data[current_key + order - 1];
	}
      current_key++;
      if (current_key >= (size_t) (order - 1)
	  || node_data[current_key] == (uint32_t) (-1))
	{
	  current_key = 0;
	  current_node = node_data[2 * order - 2];
	  error = DO_FETCH;
	}
      if (!error)
	{
	  struct adftool_bplus_key cursor;
	  cursor.type = ADFTOOL_BPLUS_KEY_KNOWN;
	  cursor.arg.known = node_data[current_key];
	  error =
	    parameters->compare (needle, &cursor, &compared_to_current,
				 parameters->compare_context);
	}
    }
  free (node_data);
  return error;
}
