#ifndef H_ADFTOOL_BPLUS_KEY_INCLUDED
#define H_ADFTOOL_BPLUS_KEY_INCLUDED

#include <adftool_private.h>

/* Keys are compared against each other in a B+ tree. However, since
   we are defining an index, some keys are already known and so we
   just use their index in the key table, or they are not known. For
   instance, the key used in a query can be random and thus we know
   its literal value, but it does not exist in the table. */

enum key_type
{
  KEY_KNOWN,
  KEY_UNKNOWN
};

union key_arg
{
  uint32_t known;
  void *unknown;
};

struct adftool_bplus_key
{
  enum key_type type;
  union key_arg arg;
};

typedef struct adftool_bplus_key key;

/* Return 0 if no error, something else if the key is not of the
   correct type. */

static inline int key_get_known (const key * key, uint32_t * index);
static inline int key_get_unknown (const key * key, void **key_value);

static inline void key_set_known (key * key, uint32_t key_index);
static inline void key_set_unknown (key * key, void *key_value);

static inline int
key_get_known (const key * key, uint32_t * index)
{
  if (key->type == KEY_KNOWN)
    {
      *index = key->arg.known;
      return 0;
    }
  return 1;
}

static inline int
key_get_unknown (const key * key, void **key_value)
{
  if (key->type == KEY_UNKNOWN)
    {
      *key_value = key->arg.unknown;
      return 0;
    }
  return 1;
}

static inline void
key_set_known (key * key, uint32_t key_index)
{
  key->type = KEY_KNOWN;
  key->arg.known = key_index;
}

static inline void
key_set_unknown (key * key, void *key_value)
{
  key->type = KEY_UNKNOWN;
  key->arg.unknown = key_value;
}

#endif /* not H_ADFTOOL_BPLUS_KEY_INCLUDED */
