#include <adftool_private.h>

int
adftool_bplus_grow (struct adftool_bplus_parameters *parameters)
{
  struct node node, child;
  int error = 0;
  int init_error = node_init (16, 0, &node);
  int child_init_error = node_init (16, 0, &child);
  if (init_error || child_init_error)
    {
      if (init_error == 0)
	{
	  node_clean (&node);
	}
      if (child_init_error == 0)
	{
	  node_clean (&child);
	}
      return 1;
    }
  int fetch_root_error = node_fetch (parameters, 0, &node);
  if (fetch_root_error)
    {
      error = 1;
      goto cleanup;
    }
  uint32_t new_id;
  adftool_bplus_parameters_allocate (parameters, &new_id);
  node.id = new_id;
  node_set_parent (&node, 0);
  node_store (parameters, &node);
  /* Fix the old root children. */
  if (!node_is_leaf (&node))
    {
      for (size_t i = 0;
	   ((i < node.order)
	    && (i == 0 || ((node_key (&node, i - 1)) != ((uint32_t) (-1)))));
	   i++)
	{
	  int fetch_child_error =
	    node_fetch (parameters, node_value (&node, i), &child);
	  if (fetch_child_error)
	    {
	      goto cleanup;
	    }
	  node_set_parent (&child, new_id);
	  node_store (parameters, &child);
	}
    }
  /* Now create the root. */
  node.id = 0;
  node_set_non_leaf (&node);
  node_set_value (&node, 0, new_id);
  for (size_t i = 0; i + 1 < node.order; i++)
    {
      node_set_key (&node, i, (uint32_t) (-1));
      node_set_value (&node, i + 1, 0);
    }
  node_set_parent (&node, (uint32_t) (-1));
  node_set_non_leaf (&node);
  node_store (parameters, &node);
cleanup:
  node_clean (&child);
  node_clean (&node);
  return error;
}
