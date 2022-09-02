#ifndef H_ADFTOOL_INCLUDED
#define H_ADFTOOL_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <gmp.h>
#include <time.h>

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
    int adftool_file_open_data (struct adftool_file *file, size_t nbytes,
				const void *bytes);

  extern LIBADFTOOL_API
    size_t adftool_file_get_data (struct adftool_file *file, size_t start,
				  size_t max, void *bytes);

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
    void adftool_term_copy (struct adftool_term *dest,
			    const struct adftool_term *src);

  extern LIBADFTOOL_API
    void adftool_term_set_blank (struct adftool_term *term, const char *id);
  extern LIBADFTOOL_API
    void adftool_term_set_named (struct adftool_term *term, const char *id);
  extern LIBADFTOOL_API
    void adftool_term_set_literal (struct adftool_term *term,
				   const char *value, const char *type,
				   const char *lang);
  extern LIBADFTOOL_API
    void adftool_term_set_integer (struct adftool_term *term, mpz_t value);
  extern LIBADFTOOL_API
    void adftool_term_set_double (struct adftool_term *term, mpf_t value);
  extern LIBADFTOOL_API
    void adftool_term_set_date (struct adftool_term *term,
				const struct timespec *date);

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
    int adftool_term_as_integer (const struct adftool_term *term,
				 mpz_t value);
  extern LIBADFTOOL_API
    int adftool_term_as_double (const struct adftool_term *term, mpf_t value);

  extern LIBADFTOOL_API
    int adftool_term_as_date (const struct adftool_term *term,
			      struct timespec *date);

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

  /* FIXME: it only recognizes N-Triples, no namespace support, and
     only double quotes for literal values. consumed is set to the
     number of bytes that can be part of the literal. Return 0 (and
     set term) if the term has been parsed, 1 if it cannot be parsed
     or if the value may differ with more data available. If it cannot
     be parsed, *consumed will be set to 0. If a term has been parsed,
     *consumed is set to the number of bytes used to parse the
     term. */
  extern LIBADFTOOL_API
    int adftool_term_parse_n3 (const char *text, size_t available,
			       size_t *consumed, struct adftool_term *term);

  extern LIBADFTOOL_API
    size_t adftool_term_to_n3 (const struct adftool_term *term, size_t start,
			       size_t max, char *str);

  struct adftool_statement;
  extern LIBADFTOOL_API
    struct adftool_statement *adftool_statement_alloc (void);
  extern LIBADFTOOL_API
    void adftool_statement_free (struct adftool_statement *statement);

#define ADFTOOL_STATEMENT_NOT_DELETED ((uint64_t) (-1))

  extern LIBADFTOOL_API
    void adftool_statement_set (struct adftool_statement *statement,
				struct adftool_term **subject,
				struct adftool_term **predicate,
				struct adftool_term **object,
				struct adftool_term **graph,
				const uint64_t * deletion_date);

  extern LIBADFTOOL_API
    void adftool_statement_get (const struct adftool_statement *statement,
				struct adftool_term **subject,
				struct adftool_term **predicate,
				struct adftool_term **object,
				struct adftool_term **graph,
				uint64_t * deletion_date);

  extern LIBADFTOOL_API
    void adftool_statement_copy (struct adftool_statement *dest,
				 const struct adftool_statement *source);

  extern LIBADFTOOL_API
    int adftool_statement_compare (const struct adftool_statement *reference,
				   const struct adftool_statement *other,
				   const char *order);

  extern LIBADFTOOL_API
    int adftool_quads_get (const struct adftool_file *file, uint32_t id,
			   struct adftool_statement *statement);

  extern LIBADFTOOL_API
    int adftool_quads_delete (struct adftool_file *file, uint32_t id,
			      uint64_t deletion_date);

  extern LIBADFTOOL_API
    int adftool_quads_insert (struct adftool_file *file,
			      const struct adftool_statement *statement,
			      uint32_t * id);

  struct adftool_results;

  extern LIBADFTOOL_API struct adftool_results *adftool_results_alloc (void);

  extern LIBADFTOOL_API
    void adftool_results_free (struct adftool_results *results);

  extern LIBADFTOOL_API
    size_t adftool_results_count (const struct adftool_results *results);

  extern LIBADFTOOL_API
    const struct adftool_statement *adftool_results_get (const struct
							 adftool_results
							 *results, size_t i);

  extern LIBADFTOOL_API
    int adftool_results_resize (struct adftool_results *results, size_t size);

  extern LIBADFTOOL_API
    void adftool_results_set (struct adftool_results *results, size_t i,
			      const struct adftool_statement *statement);

  extern LIBADFTOOL_API
    int adftool_lookup (const struct adftool_file *file,
			const struct adftool_statement *pattern,
			struct adftool_results *results);

  extern LIBADFTOOL_API
    int adftool_delete (struct adftool_file *file,
			const struct adftool_statement *pattern,
			uint64_t deletion_date);

  extern LIBADFTOOL_API
    int adftool_insert (struct adftool_file *file,
			const struct adftool_statement *statement);

  extern LIBADFTOOL_API
    int adftool_find_channel_identifier (const struct adftool_file *file,
					 size_t channel_index,
					 struct adftool_term *identifier);

  extern LIBADFTOOL_API
    int adftool_set_channel_identifier (struct adftool_file *file,
					size_t channel_index,
					const struct adftool_term
					*identifier);

  extern LIBADFTOOL_API
    int adftool_get_channel_column (const struct adftool_file *file,
				    const struct adftool_term *identifier,
				    size_t *column);

  extern LIBADFTOOL_API
    int adftool_add_channel_type (struct adftool_file *file,
				  const struct adftool_term *channel,
				  const struct adftool_term *type);

  extern LIBADFTOOL_API
    size_t adftool_get_channel_types (const struct adftool_file *file,
				      const struct adftool_term *channel,
				      size_t start, size_t max,
				      struct adftool_term **types);

  extern LIBADFTOOL_API
    size_t adftool_find_channels_by_type (const struct adftool_file *file,
					  const struct adftool_term *type,
					  size_t start, size_t max,
					  struct adftool_term **identifiers);

  extern LIBADFTOOL_API
    int adftool_get_channel_decoder (const struct adftool_file *file,
				     const struct adftool_term *identifier,
				     double *scale, double *offset);

  extern LIBADFTOOL_API
    int adftool_set_channel_decoder (struct adftool_file *file,
				     const struct adftool_term *identifier,
				     double scale, double offset);

  extern LIBADFTOOL_API
    int adftool_eeg_set_data (struct adftool_file *file, size_t n_points,
			      size_t n_channels, const double *data);

  extern LIBADFTOOL_API
    int adftool_eeg_get_data (const struct adftool_file *file,
			      size_t time_start, size_t time_length,
			      size_t *time_max, size_t channel_start,
			      size_t channel_length, size_t *channel_max,
			      double *data);

  struct adftool_fir;

  extern LIBADFTOOL_API
    struct adftool_fir *adftool_fir_alloc (double sfreq,
					   double transition_bandwidth);

  extern LIBADFTOOL_API void adftool_fir_free (struct adftool_fir *filter);

  extern LIBADFTOOL_API
    void adftool_fir_design_bandpass (struct adftool_fir *filter,
				      double freq_low, double freq_high);

  extern LIBADFTOOL_API
    void adftool_fir_apply (const struct adftool_fir *filter,
			    size_t signal_length, const double *signal,
			    double *filtered);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* not H_ADFTOOL_INCLUDED */
