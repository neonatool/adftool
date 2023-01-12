#ifndef H_BPLUS_KEY_INCLUDED
# define H_BPLUS_KEY_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>

  /* If your compiler doesn’t know how to pack structs or unions (like
     the (system foreign) guile module), use a memory region of the
     required size and aligned correctly, call key_set_* and
     you’re good to go. */

static inline size_t key_type_size (size_t *alignment);

  /* These function return 0 on success, a non-zero value in case of
     an error. Error in these two functions can only happen if the key
     is not of the requested type. */

static inline int key_get_known (const struct bplus_key *key,
				 uint32_t * index);
static inline int key_get_unknown (const struct bplus_key *key,
				   void **key_value);

static inline void key_set_known (struct bplus_key *key, uint32_t key_index);
static inline void key_set_unknown (struct bplus_key *key, void *ptr);

static inline int key_identical (const struct bplus_key *a,
				 const struct bplus_key *b);

static inline size_t
key_type_size (size_t *alignment)
{
  if (alignment)
    {
# ifdef ALIGNOF_STRUCT_BPLUS_KEY
      *alignment = ALIGNOF_STRUCT_BPLUS_KEY;
# else/* not ALIGNOF_STRUCT_BPLUS_KEY */
      *alignment = 16;
# endif/* not ALIGNOF_STRUCT_BPLUS_KEY */
    }
  return sizeof (struct bplus_key);
}

static inline int
key_get_known (const struct bplus_key *key, uint32_t * index)
{
  if (key->type == BPLUS_KEY_KNOWN)
    {
      *index = key->arg.known;
      return 0;
    }
  return 1;
}

static inline int
key_get_unknown (const struct bplus_key *key, void **value)
{
  if (key->type == BPLUS_KEY_UNKNOWN)
    {
      memcpy (value, key->arg.unknown, sizeof (void *));
      return 0;
    }
  return 1;
}

static inline void
key_set_known (struct bplus_key *key, uint32_t index)
{
  key->type = BPLUS_KEY_KNOWN;
  key->arg.known = index;
}

static inline void
key_set_unknown (struct bplus_key *key, void *data)
{
  key->type = BPLUS_KEY_UNKNOWN;
  key->arg.unknown = data;
}

static inline int
key_identical (const struct bplus_key *a, const struct bplus_key *b)
{
  /* We can’t check if two keys are identical with memcmp, because if
     any of a or b is BPLUS_KEY_KNOWN, then reading the whole key with
     memcmp leads to a read of 4 uninitialized bytes. */
  if (a->type == BPLUS_KEY_KNOWN && b->type == BPLUS_KEY_KNOWN)
    {
      return a->arg.known == b->arg.known;
    }
  if (a->type == BPLUS_KEY_UNKNOWN && b->type == BPLUS_KEY_UNKNOWN)
    {
      return a->arg.unknown == b->arg.unknown;
    }
  return 0;
}

#endif /* not H_BPLUS_KEY_INCLUDED */
