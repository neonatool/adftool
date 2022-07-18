#include <adftool_private.h>

#define _(String) dgettext (PACKAGE, (String))
#define N_(String) (String)

static int
fix_child (const struct node *node, size_t i, struct node *aux,
	   struct adftool_bplus *bplus)
{
  /* 0 can’t be a child of node. Helps detect if the result of a split
     has too many keys. */
  assert (node_value (node, i) != 0);
  int error = node_fetch (bplus, node_value (node, i), aux);
  if (error)
    {
      return 1;
    }
  node_set_parent (aux, node->id);
  node_store (bplus, aux);
  return 0;
}

static int
fix_children (const struct node *node, struct node *aux,
	      struct adftool_bplus *bplus)
{
  size_t n_fixed = 0;
  if (node_is_leaf (node))
    {
      return 0;
    }
  for (n_fixed = 0;
       (n_fixed + 1 < node->order
	&& node_key (node, n_fixed) != ((uint32_t) (-1))); n_fixed++)
    {
      if (fix_child (node, n_fixed, aux, bplus) != 0)
	{
	  return 1;
	}
    }
  if (fix_child (node, n_fixed, aux, bplus) != 0)
    {
      return 1;
    }
  return 0;
}

static int
bubble_compare (struct node *node, uint32_t * key_to_add,
		uint32_t * value_to_add, size_t *n_keys,
		struct adftool_bplus *bplus)
{
  int error = 0;
  for (*n_keys = 0;
       (*n_keys + 1 < node->order
	&& node_key (node, *n_keys) != ((uint32_t) (-1))); (*n_keys)++)
    {
      int comparison_result;
      error =
	compare_known (bplus, *key_to_add, node_key (node, *n_keys),
		       &comparison_result);
      if (error)
	{
	  goto cleanup;
	}
      if (comparison_result <= 0)
	{
	  size_t i_value = *n_keys + 1;
	  if (node_is_leaf (node))
	    {
	      i_value = *n_keys;
	    }
	  uint32_t new_key_to_add = node_key (node, *n_keys);
	  uint32_t new_value_to_add = node_value (node, i_value);
	  node_set_key (node, *n_keys, *key_to_add);
	  node_set_value (node, i_value, *value_to_add);
	  *key_to_add = new_key_to_add;
	  *value_to_add = new_value_to_add;
	}
    }
cleanup:
  return error;
}

static int
bubble_after (struct node *node, uint32_t * key_to_add,
	      uint32_t * value_to_add, size_t *n_keys, uint32_t reference)
{
  int reference_found = (node_value (node, 0) == reference);
  assert (!node_is_leaf (node));
  for (*n_keys = 0;
       (*n_keys + 1 < node->order
	&& node_key (node, *n_keys) != ((uint32_t) (-1))); (*n_keys)++)
    {
      if (reference_found)
	{
	  size_t i_value = *n_keys + 1;
	  uint32_t new_key_to_add = node_key (node, *n_keys);
	  uint32_t new_value_to_add = node_value (node, i_value);
	  node_set_key (node, *n_keys, *key_to_add);
	  node_set_value (node, i_value, *value_to_add);
	  *key_to_add = new_key_to_add;
	  *value_to_add = new_value_to_add;
	}
      if (node_value (node, *n_keys + 1) == reference)
	{
	  reference_found = 1;
	}
    }
  return 0;
}

static int
recursive_insert (struct node *node, struct node *aux, uint32_t key_to_add,
		  uint32_t value_to_add, uint32_t * after_sibling,
		  struct adftool_bplus *bplus)
{
  /* In this function, aux is used to hold a new node with half the
     keys. Once it is saved, it is used to fix the parent value of all
     children that have been moved to the new node. Finally, if the
     tree needs growing, and the value to add is a child, aux is also
     used to fix the parent of the node to add. */
  /* The first step is to bubble insert (key_to_add, value_to_add) so
     that insert is just append. There are 2 options: the first time,
     to insert to a leaf, put it just before the keys compare
     equal. Otherwise, put it right AFTER the child whose ID is
     *after_sibling. */
  int error = 0;
  size_t i;
  if (after_sibling == NULL)
    {
      error = bubble_compare (node, &key_to_add, &value_to_add, &i, bplus);
    }
  else
    {
      error =
	bubble_after (node, &key_to_add, &value_to_add, &i, *after_sibling);
    }
  if (error)
    {
      goto cleanup;
    }
  if (i + 1 == node->order)
    {
      /* No room! Split the node. First, make sure that the node has a
         parent. */
      int node_is_root = (node->id == 0);
      int node_has_no_parent = (node_parent (node) == ((uint32_t) (-1)));
      assert (node_is_root == node_has_no_parent);
      if (node_is_root)
	{
	  /* Before growing the tree, we must save what we did when we
	     bubbled the key/value to insert in the root. Otherwise,
	     the growth operation won’t fix the correct children
	     correctly. */
	  node_store (bplus, node);
	  int growth_error = adftool_bplus_grow (bplus);
	  if (growth_error)
	    {
	      error = 1;
	      goto cleanup;
	    }
	  /* Now the node that we want to split is not 0 anymore,
	     since 0 is the new root. However, the new 0 should have
	     exactly 1 child, the one that we want to split. */
	  int fetch_new_root_error = node_fetch (bplus, 0, node);
	  if (fetch_new_root_error)
	    {
	      error = 1;
	      goto cleanup;
	    }
	  assert (node_key (node, 0) == ((uint32_t) (-1)));
	  assert (!node_is_leaf (node));
	  int fetch_node_to_split_error =
	    node_fetch (bplus, node_value (node, 0), node);
	  if (fetch_node_to_split_error)
	    {
	      error = 1;
	      goto cleanup;
	    }
	  /* If the thing we want to insert is a child, we must reset
	     its parent because it still thinks it will be inserted as
	     a child of 0. */
	  if (!node_is_leaf (node))
	    {
	      /* value_to_add is a node ID, who still thinks its
	         parent is 0, while in fact it is node->id. */
	      error = node_fetch (bplus, value_to_add, aux);
	      if (error)
		{
		  goto cleanup;
		}
	      assert (node_parent (aux) == 0);
	      node_set_parent (aux, node->id);
	      node_store (bplus, aux);
	    }
	}
      uint32_t old_node_id = node->id;
      uint32_t new_node_id;
      adftool_bplus_allocate (bplus, &new_node_id);
      size_t n_given = node->order / 2;
      size_t n_kept = node->order - n_given;
      aux->id = new_node_id;
      node_set_parent (aux, node_parent (node));
      if (node_is_leaf (node))
	{
	  node_set_leaf (aux);
	  node_set_next_leaf (aux, node_next_leaf (node));
	  node_set_next_leaf (node, new_node_id);
	}
      else
	{
	  node_set_non_leaf (aux);
	}
      for (size_t j = n_kept; j + 1 < node->order; j++)
	{
	  /* Give all key/values starting at n_kept to the new
	     node. */
	  node_set_key (aux, j - n_kept, node_key (node, j));
	  node_set_value (aux, j - n_kept, node_value (node, j));
	  node_set_key (node, j, (uint32_t) (-1));
	  node_set_value (node, j, 0);
	}
      /* Remaining to add: the key to insert, and (if not a leaf) the
         last value of the node to split, and finally the value to
         insert. */
      node_set_key (aux, node->order - 1 - n_kept, key_to_add);
      assert (node->order - n_kept < node->order - 1);
      node_set_key (aux, node->order - n_kept, (uint32_t) (-1));
      if (!node_is_leaf (node))
	{
	  node_set_value (aux, n_given - 1,
			  node_value (node, node->order - 1));
	  node_set_value (aux, n_given, value_to_add);
	}
      else
	{
	  node_set_value (aux, node->order - 1 - n_kept, value_to_add);
	}
      /* The last key in node is useless if in a non-leaf, use it in
         the parent to distinguish the node from the new node. */
      key_to_add = node_key (node, n_kept - 1);
      value_to_add = new_node_id;
      if (!node_is_leaf (node))
	{
	  node_set_key (node, n_kept - 1, (uint32_t) (-1));
	}
      /* Save the nodes. */
      node_store (bplus, node);
      node_store (bplus, aux);
      /* Now use the node pointer to fetch each child of new_node, so
         that we can fix their parent. */
      error = fix_children (aux, node, bplus);
      if (error)
	{
	  goto cleanup;
	}
      /* Finally we insert to the parent of new_node (which was also
         the parent of node before we used node as a variable). */
      error = node_fetch (bplus, node_parent (aux), node);
      /* Recursive tail call, all arguments are meaningful. Since we
         insert an inner node, key-based comparisons are meaningless,
         so we rather insert the new node right after the node that we
         vampirised. */
      return recursive_insert (node, aux, key_to_add, value_to_add,
			       &old_node_id, bplus);
    }
  else
    {
      /* Add it in place and call it a day. */
      size_t i_value = i + 1;
      if (node_is_leaf (node))
	{
	  i_value = i;
	}
      node_set_key (node, i, key_to_add);
      node_set_value (node, i_value, value_to_add);
      if (i + 2 < node->order)
	{
	  node_set_key (node, i + 1, (uint32_t) (-1));
	}
      node_store (bplus, node);
    }
cleanup:
  return error;
}

static int
recursive_insert_down (struct node *node, uint32_t key, uint32_t value,
		       struct adftool_bplus *bplus)
{
  if (node_is_leaf (node))
    {
      struct node aux;
      int error = node_init (node->order, 0, &aux);
      if (error)
	{
	  return 1;
	}
      error = recursive_insert (node, &aux, key, value, NULL, bplus);
      node_clean (&aux);
      return error;
    }
  size_t i;
  for (i = 0; i + 1 < node->order && node_key (node, i) != ((uint32_t) (-1));
       i++)
    {
      int comparison_result;
      int comparison_error =
	compare_known (bplus, key, node_key (node, i), &comparison_result);
      if (comparison_error)
	{
	  return 1;
	}
      if (comparison_result <= 0)
	{
	  /* We must use child i. */
	  break;
	}
    }
  /* If we went through the whole loop, then i == order - 1 or i ==
     number of keys. Otherwise, it must go down child i. So in any
     case, it must go down child i. */
  int fetch_error = node_fetch (bplus, node_value (node, i), node);
  if (fetch_error)
    {
      return 1;
    }
  return recursive_insert_down (node, key, value, bplus);
}

int
adftool_bplus_insert (uint32_t key, uint32_t value,
		      struct adftool_bplus *bplus)
{
  int error = 0;
  struct node node;
  error = node_init (16, 0, &node);
  if (error)
    {
      goto cleanup;
    }
  error = node_fetch (bplus, 0, &node);
  if (error)
    {
      goto cleanup_node;
    }
  error = recursive_insert_down (&node, key, value, bplus);
cleanup_node:
  node_clean (&node);
cleanup:
  return error;
}
