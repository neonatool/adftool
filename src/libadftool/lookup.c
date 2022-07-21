#include <adftool_private.h>
#include <adftool_bplus_node.h>

int
adftool_bplus_lookup (const struct adftool_bplus_key *needle,
		      struct adftool_bplus *bplus, size_t start, size_t max,
		      size_t *n_results, uint32_t * results)
{
  struct node node;
  int init_error = node_init (16, 0, &node);
  if (init_error)
    {
      return 1;
    }
  uint32_t current_node = 0;
  size_t current_key = 0;
  int compared_to_current = 0;
  int error = 0;
  int first_found = 0;
  /* The first step is to find the first match. */
  do
    {
      error = node_fetch (bplus, current_node, &node);
      if (!error)
	{
	  assert (node.order > 0);
	  for (current_key = 0;
	       current_key < (size_t) (node.order - 1)
	       && node_key (&node, current_key) != (uint32_t) (-1);
	       current_key++)
	    {
	      struct adftool_bplus_key cursor;
	      cursor.type = ADFTOOL_BPLUS_KEY_KNOWN;
	      cursor.arg.known = node_key (&node, current_key);
	      error =
		bplus->compare (needle, &cursor, &compared_to_current,
				bplus->compare_context);
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
	  if (node_is_leaf (&node))
	    {
	      first_found = 1;
	    }
	  else
	    {
	      current_node = node_value (&node, current_key);
	      assert (current_node != 0);
	      current_key = 0;
	      error = node_fetch (bplus, current_node, &node);
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
      int no_more_keys =
	(current_key >= (size_t) (node.order - 1)
	 || node_key (&node, current_key) == (uint32_t) (-1));
      int after_current_key = compared_to_current > 0;
      assert (no_more_keys == after_current_key);
    }
  while (!error && compared_to_current == 0)
    {
      /* So we point to a valid key in a leaf node. */
      size_t index = (*n_results)++;
      if (index >= start && index < start + max)
	{
	  results[index - start] = node_value (&node, current_key);
	}
      current_key++;
      if (current_key >= (size_t) (node.order - 1)
	  || node_key (&node, current_key) == (uint32_t) (-1))
	{
	  current_key = 0;
	  current_node = node_next_leaf (&node);
	  if (current_node == 0)
	    {
	      /* The last leaf has been scanned. */
	      break;
	    }
	  error = node_fetch (bplus, current_node, &node);
	}
      if (!error)
	{
	  struct adftool_bplus_key cursor;
	  cursor.type = ADFTOOL_BPLUS_KEY_KNOWN;
	  cursor.arg.known = node_key (&node, current_key);
	  error =
	    bplus->compare (needle, &cursor, &compared_to_current,
			    bplus->compare_context);
	}
    }
  node_clean (&node);
  return error;
}
