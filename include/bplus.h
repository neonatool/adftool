#ifndef H_BPLUS_INCLUDED
# define H_BPLUS_INCLUDED

# include <stdint.h>
# include <stddef.h>
# include <hdf5.h>

# if defined _WIN32 && !defined __CYGWIN__
#  define LIBBPLUS_DLL_MADNESS 1
# else
#  define LIBBPLUS_DLL_MADNESS 0
# endif

# if BUILDING_LIBBPLUS && HAVE_VISIBILITY
#  define LIBBPLUS_DLL_EXPORTED __attribute__((__visibility__("default")))
# elif BUILDING_LIBBPLUS && LIBBPLUS_DLL_MADNESS
#  define LIBBPLUS_DLL_EXPORTED __declspec(dllexport)
# elif LIBBPLUS_DLL_MADNESS
#  define LIBBPLUS_DLL_EXPORTED __declspec(dllimport)
# else
#  define LIBBPLUS_DLL_EXPORTED
# endif

# define LIBBPLUS_API \
  LIBBPLUS_DLL_EXPORTED

# ifdef __cplusplus
extern "C"
{
# endif				/* __cplusplus */

  /* This is an implementation of B+ that is used for indexing RDF
     terms in adftool. */

  /* The code of this interface is part of this API. Changes in the
     enumeration values, structure and union fields of types defined
     in this interface are considered a change of the API as defined
     in libtool terms. */

  enum
  {
    BPLUS_KEY_KNOWN = 0,	/* the key is present in the tree */
    BPLUS_KEY_UNKNOWN = 1	/* the key cannot be dereferenced, it is
				   known from outside */
  };

  union bplus_key_arg
  {
    uint32_t known;
    void *unknown;
  };

  struct bplus_key
  {
    unsigned int type;		/* BPLUS_KEY_KNOWN or BPLUS_KEY_UNKNOWN */
    union bplus_key_arg arg;
  };

  /* If your compiler doesn’t know how to pack structs or unions (like
     the (system foreign) guile module), use a memory region of the
     required size and aligned correctly, call bplus_key_set_* and
     you’re good to go. */

  static inline size_t bplus_key_type_size (size_t *alignment);

  /* These function return 0 on success, a non-zero value in case of
     an error. Error in these two functions can only happen if the key
     is not of the requested type. */

  static inline int bplus_key_get_known (const struct bplus_key *key,
					 uint32_t * index);
  static inline int bplus_key_get_unknown (const struct bplus_key *key,
					   void **key_value);

  static inline void bplus_key_set_known (struct bplus_key *key,
					  uint32_t key_index);
  static inline void bplus_key_set_unknown (struct bplus_key *key, void *ptr);

  static inline int bplus_key_identical (const struct bplus_key *a,
					 const struct bplus_key *b);

  struct bplus_node
  {
    size_t n_entries;
    uint32_t *restrict keys;	/* order - 1 elements are allocated. */
    uint32_t *restrict values;	/* order elements are allocated. For
				   leaf node, the last value (index:
				   order - 1) is used as the next
				   leaf. */
    int is_leaf;
    uint32_t parent_node;	/* ((uint32_t) (-1)), 2^32 - 1 if this is
				   the root */
  };

  static inline size_t bplus_node_type_size (size_t *alignment);

  /* keys must have order - 1 allocated, values must have order
     allocated. The pointer must remain valid, because they will be
     used as the "keys" and "values" of the node. */
  static inline void bplus_node_setup (struct bplus_node *node,
				       size_t order,
				       uint32_t * restrict keys,
				       uint32_t * restrict values);

  /* The nodes must have the same order, obviously. */
  static inline void bplus_node_copy (size_t order,
				      struct bplus_node *dest,
				      const struct bplus_node *src);

  static inline int bplus_node_is_leaf (const struct bplus_node *node);
  static inline void bplus_node_set_leaf (struct bplus_node *node);
  static inline void bplus_node_set_non_leaf (struct bplus_node *node);

  /* Return a non-zero value if this is the root. Otherwise, set
   *parent to the parent. */
  static inline int bplus_node_get_parent (const struct bplus_node
					   *node, uint32_t * parent);
  static inline void bplus_node_set_parent (struct bplus_node *node,
					    uint32_t parent);
  static inline void bplus_node_unset_parent (struct bplus_node *node);

  static inline size_t bplus_node_count_entries (const struct bplus_node
						 *node);

  /* For inner nodes, next_value is set to the first child going
     strictly after the key for entry, and for leaves, it is set to a
     meaningless value. You can pass NULL in either cases. Bounds are
     not checked. */
  static inline void bplus_node_get_entry (const struct bplus_node
					   *node, size_t entry,
					   uint32_t * key,
					   uint32_t * value,
					   uint32_t * next_value);

  static inline void bplus_node_set_entry (struct bplus_node *node,
					   size_t entry, uint32_t key,
					   uint32_t value);

  static inline void bplus_node_truncate (struct bplus_node *node,
					  size_t n_entries);

  struct bplus_tree;

  static inline struct bplus_tree *bplus_tree_alloc (size_t order);

  static inline void bplus_tree_free (struct bplus_tree *tree);

  static inline size_t bplus_tree_order (struct bplus_tree *tree);

  static inline void bplus_tree_cache (struct bplus_tree *tree,
				       uint32_t id,
				       const struct bplus_node *node);

  static inline const struct bplus_node *bplus_tree_get_cache (const
							       struct
							       bplus_tree
							       *tree,
							       uint32_t id);

  /* This is the "pull" API. */

  /* The callbacks are always called with the first argument bound to
     a user-defined pointer context. They should return 0 if
     everything went according to plan, or an error code. The
     callbacks that update the storage are not supposed to fail,
     because the library is not able to recover to a partial update
     (HDF5 can’t down its own level). */

  /* This callback is passed a user-defined context, the row ID to
     fetch, the start and length elements to fetch, and the user is
     expected to set the penultimate argument to the number of
     elements read and fill the last element array starting at 0, up
     to length, with the loaded data.  */
  typedef int (*bplus_fetch_cb) (void *, size_t, size_t, size_t, size_t *,
				 uint32_t *);

  /* This callback is called with a context and 2 keys, and the user
     is expected to set the last argument to a negative value if the
     first key comes before the second, 0 if they compare equal, or a
     positive value otherwise. */
  typedef int (*bplus_compare_cb) (void *, const struct bplus_key *,
				   const struct bplus_key *, int *);

  /* This callback is called with just a context, and the user is
     expected to set the other argument to a row ID that can be used
     immediately and that is different from all other IDs of nodes
     already used. */
  typedef void (*bplus_allocate_cb) (void *, uint32_t *);

  /* This callback is called with a context, a node ID, start and
     length, and an array of data. The user is expected to update the
     storage for that row section with the supplied data. */
  typedef void (*bplus_update_cb) (void *, size_t, size_t, size_t,
				   const uint32_t *);

  /* The iterator is called with at least 1 element iteratively until
     the range has been visited in full. Then, it is called with 0
     elements. The arguments are the context, the number of elements,
     and an array of elements. The user should return a non-zero value
     to stop reading. */
  typedef int (*bplus_iterator_cb) (void *, size_t, const struct bplus_key *,
				    const uint32_t *);

  /* An insertion process is composed of 2 steps: first, search for
     the key that we want to insert. Then, insert it or not, depending
     on the user decision. The first argument is bound to a context,
     the second is whether the key was found. If the decision is to
     not insert a new instance of the key, the user should return a
     non-zero value. Otherwise, the user should insert the search key
     (third arg) to the key lookup table, set the fourth argument to
     the new key ID, and set the last argument to 0 to insert the key
     before all other matches or 1 to insert the key after all other
     matches. */
  typedef int (*bplus_decision_cb) (void *, int, const struct bplus_key *,
				    uint32_t *, int *);

  static inline int bplus_fetch (struct bplus_tree *tree,
				 bplus_fetch_cb fetch,
				 void *fetch_context, uint32_t id,
				 struct bplus_node *node);

  static inline int bplus_find (struct bplus_tree *tree,
				bplus_fetch_cb fetch,
				void *fetch_context,
				bplus_compare_cb compare,
				void *compare_context,
				bplus_iterator_cb iterator,
				void *iterator_context,
				const struct bplus_key *key);

  static inline int bplus_insert (struct bplus_tree *tree,
				  bplus_fetch_cb fetch,
				  void *fetch_context,
				  bplus_compare_cb compare,
				  void *compare_context,
				  bplus_allocate_cb allocate,
				  void *allocate_context,
				  bplus_update_cb update,
				  void *update_context,
				  bplus_decision_cb decide,
				  void *decide_context,
				  const struct bplus_key *key);

  /* This is the "push" API. Control flow is released as soon as code
     from the user would be triggered, instead of calling a user
     callback. */

  /* The fetcher process requests to load a specific node. */
  struct bplus_fetcher;

  static inline
    struct bplus_fetcher *bplus_fetcher_alloc (struct bplus_tree *tree);

  static inline void bplus_fetcher_free (struct bplus_fetcher *fetcher);

  static inline
    void bplus_fetcher_setup (struct bplus_fetcher *fetcher,
			      uint32_t node_id);

  /* Return NULL if fetching a row is required. In this case, you have
     to fetch the requested row (discarding the first start elements,
     and remembering the length next ones), and call
     bplus_fetcher_data. */
  static inline
    const struct bplus_node *bplus_fetcher_status (struct bplus_fetcher
						   *fetcher,
						   size_t *row_to_fetch,
						   size_t *start,
						   size_t *length);

  static inline
    void bplus_fetcher_data (struct bplus_fetcher *fetcher,
			     size_t row, size_t start, size_t length,
			     const uint32_t * data);

  /* A range is a list of leaves, with a start and stop key
     position. You can iterate over the range. */
  struct bplus_range;

  static inline
    struct bplus_range *bplus_range_alloc (struct bplus_tree *tree);

  static inline void bplus_range_free (struct bplus_range *range);

  /* May fail if the head or tail aren’t leaves. In this case, return
     non-zero. Otherwise, return 0. */
  static inline
    int bplus_range_setup (struct bplus_range *range, uint32_t head_id,
			   const struct bplus_node *head, size_t first_key,
			   uint32_t tail_id, const struct bplus_node *tail,
			   size_t stop_key);

  /* Return the number of records that are available in the head, and
     fill keys and values up to that (discard the start first). Do not
     touch the output arrays at or after the max index. Call
     bplus_range_next to examine the next leaf, don’t stop there! If
     there are records to process in the next leaf, set *has_next to 1
     (otherwise, set it to 0). If tree rows must be loaded before you
     can consider the next leaf, then set *n_load_requests to the
     number of requests to make, and fill *load_request_row,
     *load_request_start and *load_request_length with the requests to
     load (up to max). The first start_load_requests are discarded. */
  static inline
    size_t bplus_range_get (const struct bplus_range *range, size_t start,
			    size_t max, struct bplus_key *keys,
			    uint32_t * values, int *has_next,
			    size_t *n_load_requests,
			    size_t start_load_requests,
			    size_t max_load_requests,
			    size_t *load_request_row,
			    size_t *load_request_start,
			    size_t *load_request_length);

  /* Prepare the next leaf. After this function call, the head is not
     modified yet. */
  static inline
    void bplus_range_data (struct bplus_range *range, size_t row,
			   size_t start, size_t length,
			   const uint32_t * data);

  /* Update the head to point to the next leaf. If the data for the
     next leaf is still unknown, or if there are no next leaves at
     all, return non-zero. Otherwise, switch and return
     0. bplus_range_get tells you whether you should do this or not,
     see the output parameter has_next. */
  static inline int bplus_range_next (struct bplus_range *range);

  /* The finder process is a recursive explorer. It starts at the
     root, and advances both a "first" and a "last" leg until it
     touches leaves. The value produced by the finder is a range. This
     range contains all the values in the tree that compare equal to
     the search key. */

  struct bplus_finder;

  static inline
    struct bplus_finder *bplus_finder_alloc (struct bplus_tree *tree);

  static inline void bplus_finder_free (struct bplus_finder *finder);

  static inline
    void bplus_finder_setup (struct bplus_finder *finder,
			     const struct bplus_key *search_key,
			     uint32_t root_id, const struct bplus_node *root);

  static inline
    void bplus_finder_status (const struct bplus_finder *finder,
			      int *done,
			      struct bplus_range *result,
			      size_t *n_fetch_requests,
			      size_t start_fetch_request,
			      size_t max_fetch_requests,
			      size_t *fetch_rows,
			      size_t *fetch_starts,
			      size_t *fetch_lengths,
			      size_t *n_compare_requests,
			      size_t start_compare_request,
			      size_t max_compare_requests,
			      struct bplus_key *as, struct bplus_key *bs);

  static inline
    void bplus_finder_data (struct bplus_finder *finder, size_t row,
			    size_t start, size_t length,
			    const uint32_t * data);

  static inline
    void bplus_finder_compared (struct bplus_finder *finder,
				const struct bplus_key *a,
				const struct bplus_key *b, int result);

  struct bplus_insertion;

  static inline
    struct bplus_insertion *bplus_insertion_alloc (struct bplus_tree *tree);

  static inline void bplus_insertion_free (struct bplus_insertion *insertion);

  /* Start the insertion problem: insert record -> record in the
     tree. If back, insert it as the last position of the
     range. Otherwise, insert it in the first position of the
     range. If the range is empty, both positions refer to the same
     thing. Once the insertion is started, reading through the where
     range is forbidden. */
  static inline
    void bplus_insertion_setup (struct bplus_insertion *insertion,
				uint32_t record,
				const struct bplus_range *where, int back);

  static inline
    void bplus_insertion_status (const struct bplus_insertion *insertion,
				 int *done,
				 /* The fetches you have to do */
				 size_t *n_fetches_to_do,
				 size_t start_fetch_to_do,
				 size_t max_fetches_to_do,
				 size_t *fetch_rows,
				 size_t *fetch_starts, size_t *fetch_lengths,
				 /* You also have to allocate nodes */
				 size_t *n_allocations_to_do,
				 /* You also have to update the tree storage */
				 size_t *n_stores_to_do,
				 size_t start_store_to_do,
				 size_t max_stores_to_do,
				 size_t *store_rows,
				 size_t *store_starts,
				 size_t *store_lengths,
				 const uint32_t ** stores);

  static inline
    void bplus_insertion_data (struct bplus_insertion *insertion, size_t row,
			       size_t start, size_t length,
			       const uint32_t * data);

  static inline
    int bplus_insertion_allocated (struct bplus_insertion *insertion,
				   uint32_t new_id);

  static inline
    void bplus_insertion_updated (struct bplus_insertion *insertion,
				  size_t row);

  static inline void bplus_prime (size_t order, uint32_t * row_0);

  struct bplus_hdf5_table;

  static inline struct bplus_hdf5_table *bplus_hdf5_table_alloc (void);

  static inline void bplus_hdf5_table_free (struct bplus_hdf5_table *table);

  static inline
    int bplus_hdf5_table_set (struct bplus_hdf5_table *table, hid_t data);

  static inline
    size_t bplus_hdf5_table_order (struct bplus_hdf5_table *table);

  static inline
    hid_t bplus_hdf5_table_type (const struct bplus_hdf5_table *table);

  /* I provide here a set of convenience callbacks for working with
     HDF5 tables. The first argument is of type struct
     bplus_hdf5_table, but gcc emits a warning if we do that. */

  static inline
    int bplus_hdf5_fetch (void *table, size_t row,
			  size_t start, size_t length, size_t *actual_length,
			  uint32_t * data);

  static inline void bplus_hdf5_allocate (void *table, uint32_t * new_id);

  static inline
    void bplus_hdf5_update (void *table, size_t row,
			    size_t start, size_t length,
			    const uint32_t * data);

# include "../src/libbplus/bplus_hdf5.h"
# include "../src/libbplus/bplus_analyzer.h"
# include "../src/libbplus/bplus_divider.h"
# include "../src/libbplus/bplus_explorer.h"
# include "../src/libbplus/bplus_fetch.h"
# include "../src/libbplus/bplus_fetcher.h"
# include "../src/libbplus/bplus_find.h"
# include "../src/libbplus/bplus_finder.h"
# include "../src/libbplus/bplus_growth.h"
# include "../src/libbplus/bplus_insert.h"
# include "../src/libbplus/bplus_insertion.h"
# include "../src/libbplus/bplus_key.h"
# include "../src/libbplus/bplus_node.h"
# include "../src/libbplus/bplus_parent_fetcher.h"
# include "../src/libbplus/bplus_prime.h"
# include "../src/libbplus/bplus_range.h"
# include "../src/libbplus/bplus_reparentor.h"
# include "../src/libbplus/bplus_tree.h"

  static inline size_t bplus_key_type_size (size_t *alignment)
  {
    return key_type_size (alignment);
  }

  static inline int
    bplus_key_get_known (const struct bplus_key *key, uint32_t * index)
  {
    return key_get_known (key, index);
  }

  static inline int
    bplus_key_get_unknown (const struct bplus_key *key, void **key_value)
  {
    return key_get_unknown (key, key_value);
  }

  static inline void
    bplus_key_set_known (struct bplus_key *key, uint32_t key_index)
  {
    key_set_known (key, key_index);
  }

  static inline void bplus_key_set_unknown (struct bplus_key *key, void *ptr)
  {
    key_set_unknown (key, ptr);
  }

  static inline int
    bplus_key_identical (const struct bplus_key *a, const struct bplus_key *b)
  {
    return key_identical (a, b);
  }

  static inline size_t bplus_node_type_size (size_t *alignment)
  {
    return node_type_size (alignment);
  }

  static inline void
    bplus_node_setup (struct bplus_node *node, size_t order,
		      uint32_t * restrict keys, uint32_t * restrict values)
  {
    node_setup (node, order, keys, values);
  }

  static inline void
    bplus_node_copy (size_t order, struct bplus_node *dest,
		     const struct bplus_node *src)
  {
    node_copy (order, dest, src);
  }

  static inline int bplus_node_is_leaf (const struct bplus_node *node)
  {
    return node_is_leaf (node);
  }

  static inline void bplus_node_set_leaf (struct bplus_node *node)
  {
    node_set_leaf (node);
  }

  static inline void bplus_node_set_non_leaf (struct bplus_node *node)
  {
    node_set_non_leaf (node);
  }

  static inline int
    bplus_node_get_parent (const struct bplus_node *node, uint32_t * parent)
  {
    return node_get_parent (node, parent);
  }

  static inline void
    bplus_node_set_parent (struct bplus_node *node, uint32_t parent)
  {
    node_set_parent (node, parent);
  }

  static inline void bplus_node_unset_parent (struct bplus_node *node)
  {
    node_unset_parent (node);
  }

  static inline size_t
    bplus_node_count_entries (const struct bplus_node *node)
  {
    return node_count_entries (node);
  }

  static inline void
    bplus_node_get_entry (const struct bplus_node *node, size_t entry,
			  uint32_t * key, uint32_t * value,
			  uint32_t * next_value)
  {
    node_get_entry (node, entry, key, value, next_value);
  }

  static inline void
    bplus_node_set_entry (struct bplus_node *node, size_t entry, uint32_t key,
			  uint32_t value)
  {
    node_set_entry (node, entry, key, value);
  }

  static inline void
    bplus_node_truncate (struct bplus_node *node, size_t n_entries)
  {
    node_truncate (node, n_entries);
  }

  static inline struct bplus_tree *bplus_tree_alloc (size_t order)
  {
    return tree_alloc (order);
  }

  static inline void bplus_tree_free (struct bplus_tree *tree)
  {
    tree_free (tree);
  }

  static inline size_t bplus_tree_order (struct bplus_tree *tree)
  {
    return tree_order (tree);
  }

  static inline void
    bplus_tree_cache (struct bplus_tree *tree, uint32_t id,
		      const struct bplus_node *node)
  {
    tree_cache (tree, id, node);
  }

  static inline const struct bplus_node *bplus_tree_get_cache (const struct
							       bplus_tree
							       *tree,
							       uint32_t id)
  {
    return tree_get_cache (tree, id);
  }

  static inline int
    bplus_fetch (struct bplus_tree *tree, bplus_fetch_cb fetch_impl,
		 void *fetch_context, uint32_t id, struct bplus_node *node)
  {
    return fetch (tree, fetch_impl, fetch_context, id, node);
  }

  static inline int
    bplus_find (struct bplus_tree *tree, bplus_fetch_cb fetch,
		void *fetch_context, bplus_compare_cb compare,
		void *compare_context, bplus_iterator_cb iterator,
		void *iterator_context, const struct bplus_key *key)
  {
    return find (tree, fetch, fetch_context, compare, compare_context,
		 iterator, iterator_context, key);
  }

  static inline int
    bplus_insert (struct bplus_tree *tree, bplus_fetch_cb fetch,
		  void *fetch_context, bplus_compare_cb compare,
		  void *compare_context, bplus_allocate_cb allocate,
		  void *allocate_context, bplus_update_cb update,
		  void *update_context, bplus_decision_cb decide,
		  void *decide_context, const struct bplus_key *key)
  {
    return insert (tree, fetch, fetch_context, compare, compare_context,
		   allocate, allocate_context, update, update_context, decide,
		   decide_context, key);
  }

  static inline struct bplus_fetcher *bplus_fetcher_alloc (struct bplus_tree
							   *tree)
  {
    return fetcher_alloc (tree);
  }

  static inline void bplus_fetcher_free (struct bplus_fetcher *fetcher)
  {
    fetcher_free (fetcher);
  }

  static inline void
    bplus_fetcher_setup (struct bplus_fetcher *fetcher, uint32_t node_id)
  {
    fetcher_setup (fetcher, node_id);
  }

  static inline const struct bplus_node *bplus_fetcher_status (struct
							       bplus_fetcher
							       *fetcher,
							       size_t
							       *row_to_fetch,
							       size_t *start,
							       size_t *length)
  {
    return fetcher_status (fetcher, row_to_fetch, start, length);
  }

  static inline void
    bplus_fetcher_data (struct bplus_fetcher *fetcher, size_t row,
			size_t start, size_t length, const uint32_t * data)
  {
    fetcher_data (fetcher, row, start, length, data);
  }

  static inline struct bplus_range *bplus_range_alloc (struct bplus_tree
						       *tree)
  {
    return range_alloc (tree);
  }

  static inline void bplus_range_free (struct bplus_range *range)
  {
    range_free (range);
  }

  static inline int
    bplus_range_setup (struct bplus_range *range, uint32_t head_id,
		       const struct bplus_node *head, size_t first_key,
		       uint32_t tail_id, const struct bplus_node *tail,
		       size_t stop_key)
  {
    return range_setup (range, head_id, head, first_key, tail_id, tail,
			stop_key);
  }

  static inline size_t
    bplus_range_get (const struct bplus_range *range, size_t start,
		     size_t max, struct bplus_key *keys, uint32_t * values,
		     int *has_next, size_t *n_load_requests,
		     size_t start_load_request, size_t max_load_requests,
		     size_t *load_request_row, size_t *load_request_start,
		     size_t *load_request_length)
  {
    return range_get (range, start, max, keys, values, has_next,
		      n_load_requests, start_load_request, max_load_requests,
		      load_request_row, load_request_start,
		      load_request_length);
  }

  static inline void
    bplus_range_data (struct bplus_range *range, size_t row, size_t start,
		      size_t length, const uint32_t * data)
  {
    range_data (range, row, start, length, data);
  }

  static inline int bplus_range_next (struct bplus_range *range)
  {
    return range_next (range);
  }

  static inline struct bplus_finder *bplus_finder_alloc (struct bplus_tree
							 *tree)
  {
    return finder_alloc (tree);
  }

  static inline void bplus_finder_free (struct bplus_finder *finder)
  {
    finder_free (finder);
  }

  static inline void
    bplus_finder_setup (struct bplus_finder *finder,
			const struct bplus_key *search_key, uint32_t root_id,
			const struct bplus_node *root)
  {
    finder_setup (finder, search_key, root_id, root);
  }

  static inline void
    bplus_finder_status (const struct bplus_finder *finder, int *done,
			 struct bplus_range *result, size_t *n_fetch_requests,
			 size_t start_fetch_request,
			 size_t max_fetch_requests, size_t *fetch_rows,
			 size_t *fetch_starts, size_t *fetch_lengths,
			 size_t *n_compare_requests,
			 size_t start_compare_request,
			 size_t max_compare_requests, struct bplus_key *as,
			 struct bplus_key *bs)
  {
    finder_status (finder, done, result, n_fetch_requests,
		   start_fetch_request, max_fetch_requests, fetch_rows,
		   fetch_starts, fetch_lengths, n_compare_requests,
		   start_compare_request, max_compare_requests, as, bs);
  }

  static inline void
    bplus_finder_data (struct bplus_finder *finder, size_t row, size_t start,
		       size_t length, const uint32_t * data)
  {
    finder_data (finder, row, start, length, data);
  }

  static inline void
    bplus_finder_compared (struct bplus_finder *finder,
			   const struct bplus_key *a,
			   const struct bplus_key *b, int result)
  {
    finder_compared (finder, a, b, result);
  }

  static inline struct bplus_insertion *bplus_insertion_alloc (struct
							       bplus_tree
							       *tree)
  {
    return insertion_alloc (tree);
  }

  static inline void bplus_insertion_free (struct bplus_insertion *insertion)
  {
    insertion_free (insertion);
  }

  static inline void
    bplus_insertion_setup (struct bplus_insertion *insertion, uint32_t record,
			   const struct bplus_range *where, int back)
  {
    insertion_setup (insertion, record, where, back);
  }

  static inline void
    bplus_insertion_status (const struct bplus_insertion *insertion,
			    int *done,
			    /* The fetches you have to do */
			    size_t *n_fetches_to_do,
			    size_t start_fetch_to_do,
			    size_t max_fetches_to_do,
			    size_t *fetch_rows,
			    size_t *fetch_starts, size_t *fetch_lengths,
			    /* You also have to allocate nodes */
			    size_t *n_allocations_to_do,
			    /* You also have to update the tree storage */
			    size_t *n_stores_to_do,
			    size_t start_store_to_do,
			    size_t max_stores_to_do,
			    size_t *store_rows,
			    size_t *store_starts,
			    size_t *store_lengths, const uint32_t ** stores)
  {
    insertion_status (insertion, done, n_fetches_to_do, start_fetch_to_do,
		      max_fetches_to_do, fetch_rows, fetch_starts,
		      fetch_lengths, n_allocations_to_do, n_stores_to_do,
		      start_store_to_do, max_stores_to_do, store_rows,
		      store_starts, store_lengths, stores);
  }

  static inline void
    bplus_insertion_data (struct bplus_insertion *insertion, size_t row,
			  size_t start, size_t length, const uint32_t * data)
  {
    insertion_data (insertion, row, start, length, data);
  }

  static inline int
    bplus_insertion_allocated (struct bplus_insertion *insertion,
			       uint32_t new_id)
  {
    return insertion_allocated (insertion, new_id);
  }

  static inline void
    bplus_insertion_updated (struct bplus_insertion *insertion, size_t row)
  {
    insertion_updated (insertion, row);
  }

  static inline void bplus_prime (size_t order, uint32_t * row_0)
  {
    prime (order, row_0);
  }

  static inline struct bplus_hdf5_table *bplus_hdf5_table_alloc (void)
  {
    return hdf5_table_alloc ();
  }

  static inline void bplus_hdf5_table_free (struct bplus_hdf5_table *table)
  {
    hdf5_table_free (table);
  }

  static inline int
    bplus_hdf5_table_set (struct bplus_hdf5_table *table, hid_t data)
  {
    return hdf5_table_set (table, data);
  }

  static inline size_t bplus_hdf5_table_order (struct bplus_hdf5_table *table)
  {
    return hdf5_table_order (table);
  }

  static inline hid_t
    bplus_hdf5_table_type (const struct bplus_hdf5_table *table)
  {
    return hdf5_table_type (table);
  }

  static inline int
    bplus_hdf5_fetch (void *table, size_t row, size_t start, size_t length,
		      size_t *actual_length, uint32_t * data)
  {
    return hdf5_fetch (table, row, start, length, actual_length, data);
  }

  static inline void bplus_hdf5_allocate (void *table, uint32_t * new_id)
  {
    hdf5_allocate (table, new_id);
  }

  static inline void
    bplus_hdf5_update (void *table, size_t row, size_t start, size_t length,
		       const uint32_t * data)
  {
    hdf5_update (table, row, start, length, data);
  }

# ifdef __cplusplus
}
# endif/* __cplusplus */

#endif /* H_BPLUS_INCLUDED */
