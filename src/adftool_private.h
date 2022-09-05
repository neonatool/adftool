#ifndef H_ADFTOOL_PRIVATE_INCLUDED
#define H_ADFTOOL_PRIVATE_INCLUDED

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <adftool.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <locale.h>

#include "gettext.h"

#ifdef BUILDING_LIBADFTOOL
#define _(String) dgettext (PACKAGE, (String))
#define N_(String) (String)
#else
#define _(String) gettext (String)
#define N_(String) (String)
#endif

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

#include <hdf5.h>
#include "adftool_bplus.h"

struct literal
{
  /* This is the thing pointed to by "unknown" b+ keys. */
  size_t length;
  char *data;
};

struct adftool_dictionary
{
  hid_t group;
  hid_t bplus_dataset;
  hid_t bplus_nextid;
  struct bplus bplus;
  hid_t strings_dataset;
  hid_t strings_nextid;
  hid_t bytes_dataset;
  hid_t bytes_nextid;
};

struct adftool_quads
{
  hid_t dataset;
  hid_t nextid;
};

struct adftool_index
{
  hid_t dataset;
  hid_t nextid;
  struct bplus bplus;
};

struct adftool_data_description
{
  hid_t group;
  struct adftool_quads quads;
  struct adftool_index indices[6];
};

struct adftool_file
{
  hid_t hdf5_file;
  struct adftool_dictionary dictionary;
  struct adftool_data_description data_description;
  hid_t eeg_dataset;
  FILE *file_handle;
};

enum adftool_term_type
{
  TERM_BLANK = 0,
  TERM_NAMED = 1,
  TERM_TYPED = 2,
  TERM_LANGSTRING = 3
};

struct adftool_term
{
  enum adftool_term_type type;
  char *str1;
  char *str2;
};

struct adftool_statement
{
  struct adftool_term *subject;
  struct adftool_term *predicate;
  struct adftool_term *object;
  struct adftool_term *graph;
  uint64_t deletion_date;
};

#endif /* not H_ADFTOOL_PRIVATE_INCLUDED */
