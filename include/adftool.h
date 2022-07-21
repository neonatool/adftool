#ifndef H_ADFTOOL_INCLUDED
#define H_ADFTOOL_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <hdf5.h>

#if BUILDING_LIBADFTOOL && HAVE_EMSCRIPTEN_H
#include "emscripten.h"
#define LIBADFTOOL_EMSCRIPTEN_KEEPALIVE EMSCRIPTEN_KEEPALIVE
#else
#define LIBADFTOOL_EMSCRIPTEN_KEEPALIVE
#endif

#if defined _WIN32 && !defined __CYGWIN__
#define LIBADFTOOL_DLL_MADNESS 1
#else
#define LIBADFTOOL_DLL_MADNESS 0
#endif

#if BUILDING_LIBADFTOOL && HAVE_VISIBILITY
#define LIBADFTOOL_DLL_EXPORTED __attribute__((__visibility__("default")))
#elif BUILDING_LIBADFTOOL && LIBADFTOOL_DLL_MADNESS
#define LIBADFTOOL_DLL_EXPORTED __declspec(dllexport)
#elif LIBADFTOOL_DLL_MADNESS
#define LIBADFTOOL_DLL_EXPORTED __declspec(dllimport)
#else
#define LIBADFTOOL_DLL_EXPORTED
#endif

#define LIBADFTOOL_API \
  LIBADFTOOL_EMSCRIPTEN_KEEPALIVE \
  LIBADFTOOL_DLL_EXPORTED

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

  struct adftool_bplus_key;

  struct adftool_bplus;

  extern LIBADFTOOL_API struct adftool_bplus *adftool_bplus_alloc (void);

  extern LIBADFTOOL_API void adftool_bplus_free (struct adftool_bplus *bplus);

  extern LIBADFTOOL_API
    void adftool_bplus_set_fetch (struct adftool_bplus *bplus,
				  int (*fetch) (uint32_t, size_t *, size_t,
						size_t, uint32_t *, void *),
				  void *context);

  extern LIBADFTOOL_API
    void adftool_bplus_set_compare (struct adftool_bplus *bplus,
				    int (*compare) (const struct
						    adftool_bplus_key *,
						    const struct
						    adftool_bplus_key *,
						    int *, void *),
				    void *context);

  /* The allocation and storage should not signal errors, because
     there’s no way to recover from a partially failing update. If you
     cannot guarantee error-free updates in the backend, store the
     updates in a cache and do them all at once. If you do that, make
     sure to also tweak the "fetch" callback to first look in the
     cache. */

  extern LIBADFTOOL_API
    void adftool_bplus_set_allocate (struct adftool_bplus *bplus,
				     void (*allocate) (uint32_t *, void *),
				     void *context);

  extern LIBADFTOOL_API
    void adftool_bplus_set_store (struct adftool_bplus *bplus,
				  void (*store) (uint32_t, size_t, size_t,
						 const uint32_t *, void *),
				  void *context);

  extern LIBADFTOOL_API
    int adftool_bplus_from_hdf5 (struct adftool_bplus *bplus, hid_t dataset,
				 hid_t next_id_attribute);

  /* The lookup function. The parameters "fetch" and "compare" must be
     set. */

  extern LIBADFTOOL_API
    int adftool_bplus_lookup (const struct adftool_bplus_key *needle,
			      struct adftool_bplus *bplus, size_t start,
			      size_t max, size_t *n_results,
			      uint32_t * results);

  /* Add a parent to the root. The parameters "fetch", "allocate" and
     "store" must be set. */

  extern LIBADFTOOL_API int adftool_bplus_grow (struct adftool_bplus *bplus);

  /* The insert function requires "fetch", "compare", "allocate" and
     "store" parameters.
   */
  extern LIBADFTOOL_API
    int adftool_bplus_insert (uint32_t key, uint32_t value,
			      struct adftool_bplus *bplus);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* not H_ADFTOOL_INCLUDED */
