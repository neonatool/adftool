#ifndef H_ADFTOOL_INCLUDED
#define H_ADFTOOL_INCLUDED

#include <stdint.h>
#include <stddef.h>

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

  struct adftool_file;

  extern LIBADFTOOL_API struct adftool_file *adftool_file_alloc (void);
  extern LIBADFTOOL_API void adftool_file_free (struct adftool_file *file);

  extern LIBADFTOOL_API
    int adftool_file_open (struct adftool_file *file, const char *filename,
			   int write);
  extern LIBADFTOOL_API void adftool_file_close (struct adftool_file *file);

  extern LIBADFTOOL_API
    int adftool_dictionary_get (const struct adftool_file *file, uint32_t id,
				size_t start, size_t max, size_t *length,
				char *dest);
  extern LIBADFTOOL_API
    int adftool_dictionary_lookup (const struct adftool_file *file,
				   size_t length, const char *key, int *found,
				   uint32_t * id);
  /* insert will do nothing if the key is already present. */
  extern LIBADFTOOL_API
    int adftool_dictionary_insert (struct adftool_file *file, size_t length,
				   const char *key, uint32_t * id);

  struct adftool_term;
  extern LIBADFTOOL_API struct adftool_term *adftool_term_alloc (void);
  extern LIBADFTOOL_API void adftool_term_free (struct adftool_term *term);

  extern LIBADFTOOL_API
    int adftool_term_set_blank (struct adftool_term *term, const char *id);
  extern LIBADFTOOL_API
    int adftool_term_set_named (struct adftool_term *term, const char *id);
  extern LIBADFTOOL_API
    int adftool_term_set_literal (struct adftool_term *term,
				  const char *value, const char *type,
				  const char *lang);

  extern LIBADFTOOL_API
    int adftool_term_is_blank (const struct adftool_term *term);
  extern LIBADFTOOL_API
    int adftool_term_is_named (const struct adftool_term *term);
  extern LIBADFTOOL_API
    int adftool_term_is_literal (const struct adftool_term *term);

  extern LIBADFTOOL_API
    int adftool_term_is_typed_literal (const struct adftool_term *term);
  extern LIBADFTOOL_API
    int adftool_term_is_langstring (const struct adftool_term *term);

  extern LIBADFTOOL_API
    size_t adftool_term_value (const struct adftool_term *term, size_t start,
			       size_t max, char *value);
  /* meta gets the type or langtag of a literal */
  extern LIBADFTOOL_API
    size_t adftool_term_meta (const struct adftool_term *term, size_t start,
			      size_t max, char *meta);

  extern LIBADFTOOL_API
    int adftool_term_compare (const struct adftool_term *reference,
			      const struct adftool_term *other);

  extern LIBADFTOOL_API
    int adftool_term_decode (const struct adftool_file *file, uint64_t value,
			     struct adftool_term *decoded);

  extern LIBADFTOOL_API
    int adftool_term_encode (struct adftool_file *file,
			     const struct adftool_term *term,
			     uint64_t * encoded);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* not H_ADFTOOL_INCLUDED */
