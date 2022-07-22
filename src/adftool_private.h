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
};

struct adftool_file
{
  hid_t hdf5_file;
  struct adftool_dictionary dictionary;
};

#endif /* not H_ADFTOOL_PRIVATE_INCLUDED */
