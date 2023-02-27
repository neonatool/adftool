#ifndef H_ADFTOOL_INCLUDED
# define H_ADFTOOL_INCLUDED

# include <stdint.h>
# include <stddef.h>
# include <gmp.h>
# include <time.h>

# if BUILDING_LIBADFTOOL && HAVE_EMSCRIPTEN_H
#  include "emscripten.h"
#  define LIBADFTOOL_EMSCRIPTEN_KEEPALIVE EMSCRIPTEN_KEEPALIVE
# else
#  define LIBADFTOOL_EMSCRIPTEN_KEEPALIVE
# endif

# if defined _WIN32 && !defined __CYGWIN__
#  define LIBADFTOOL_DLL_MADNESS 1
# else
#  define LIBADFTOOL_DLL_MADNESS 0
# endif

# if BUILDING_LIBADFTOOL && HAVE_VISIBILITY
#  define LIBADFTOOL_DLL_EXPORTED __attribute__((__visibility__("default")))
# elif BUILDING_LIBADFTOOL && LIBADFTOOL_DLL_MADNESS
#  define LIBADFTOOL_DLL_EXPORTED __declspec(dllexport)
# elif LIBADFTOOL_DLL_MADNESS
#  define LIBADFTOOL_DLL_EXPORTED __declspec(dllimport)
# else
#  define LIBADFTOOL_DLL_EXPORTED
# endif

# define LIBADFTOOL_API \
  LIBADFTOOL_EMSCRIPTEN_KEEPALIVE \
  LIBADFTOOL_DLL_EXPORTED

# ifdef __cplusplus
extern "C"
{
# endif				/* __cplusplus */

  struct adftool_file;

  extern LIBADFTOOL_API
    struct adftool_file *adftool_file_open (const char *filename, int write);
  extern LIBADFTOOL_API void adftool_file_close (struct adftool_file *file);

  extern LIBADFTOOL_API
    struct adftool_file *adftool_file_open_data (size_t nbytes,
						 const void *bytes);

  extern LIBADFTOOL_API
    size_t adftool_file_get_data (struct adftool_file *file, size_t start,
				  size_t max, void *bytes);

  extern LIBADFTOOL_API
    int adftool_dictionary_get (struct adftool_file *file, uint32_t id,
				size_t start, size_t max, size_t *length,
				char *dest);
  extern LIBADFTOOL_API
    int adftool_dictionary_lookup (struct adftool_file *file,
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
    void adftool_term_set_mpz (struct adftool_term *term, mpz_t value);
  extern LIBADFTOOL_API
    void adftool_term_set_integer (struct adftool_term *term, long value);
  extern LIBADFTOOL_API
    void adftool_term_set_mpf (struct adftool_term *term, mpf_t value);
  extern LIBADFTOOL_API
    void adftool_term_set_double (struct adftool_term *term, double value);
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
    int adftool_term_as_mpz (const struct adftool_term *term, mpz_t value);
  extern LIBADFTOOL_API
    int adftool_term_as_integer (const struct adftool_term *term,
				 long *value);
  extern LIBADFTOOL_API
    int adftool_term_as_mpf (const struct adftool_term *term, mpf_t value);

  extern LIBADFTOOL_API
    int adftool_term_as_double (const struct adftool_term *term,
				double *value);

  extern LIBADFTOOL_API
    int adftool_term_as_date (const struct adftool_term *term,
			      struct timespec *date);

  extern LIBADFTOOL_API
    int adftool_term_compare (const struct adftool_term *reference,
			      const struct adftool_term *other);

  extern LIBADFTOOL_API
    int adftool_term_decode (struct adftool_file *file, uint64_t value,
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

# define ADFTOOL_STATEMENT_NOT_DELETED ((uint64_t) (-1))

  extern LIBADFTOOL_API
    void adftool_statement_set (struct adftool_statement *statement,
				struct adftool_term **subject,
				struct adftool_term **predicate,
				struct adftool_term **object,
				struct adftool_term **graph,
				const uint64_t * deletion_date);

  /* WARNING: subject, predicate, object and graph are output
     parameters. They should be declared const, because you’re not
     supposed to touch them, bust const + double pointers are not
     great. */
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

  extern LIBADFTOOL_API
    int adftool_lookup (struct adftool_file *file,
			const struct adftool_statement *pattern,
			size_t start, size_t max, size_t *n_results,
			struct adftool_statement **results);

  extern LIBADFTOOL_API
    size_t adftool_lookup_objects (struct adftool_file *file,
				   const struct adftool_term *subject,
				   const char *predicate,
				   size_t start, size_t max,
				   struct adftool_term **objects);

  extern LIBADFTOOL_API
    size_t adftool_lookup_integer (struct adftool_file *file,
				   const struct adftool_term *subject,
				   const char *predicate, size_t start,
				   size_t max, long *objects);

  extern LIBADFTOOL_API
    size_t adftool_lookup_double (struct adftool_file *file,
				  const struct adftool_term *subject,
				  const char *predicate, size_t start,
				  size_t max, double *objects);

  extern LIBADFTOOL_API
    size_t adftool_lookup_date (struct adftool_file *file,
				const struct adftool_term *subject,
				const char *predicate, size_t start,
				size_t max, struct timespec **objects);

  extern LIBADFTOOL_API
    size_t adftool_lookup_string (struct adftool_file *file,
				  const struct adftool_term *subject,
				  const char *predicate,
				  size_t *storage_required,
				  size_t storage_size, char *storage,
				  size_t start, size_t max,
				  size_t *langtag_length, char **langtags,
				  size_t *object_length, char **objects);

  extern LIBADFTOOL_API
    size_t adftool_lookup_subjects (struct adftool_file *file,
				    const struct adftool_term *object,
				    const char *predicate,
				    size_t start, size_t max,
				    struct adftool_term **subjects);

  extern LIBADFTOOL_API
    int adftool_delete (struct adftool_file *file,
			const struct adftool_statement *pattern,
			uint64_t deletion_date);

  extern LIBADFTOOL_API
    int adftool_insert (struct adftool_file *file,
			const struct adftool_statement *statement);

  extern LIBADFTOOL_API
    int adftool_find_channel_identifier (struct adftool_file *file,
					 size_t channel_index,
					 struct adftool_term *identifier);

  extern LIBADFTOOL_API
    int adftool_get_channel_column (struct adftool_file *file,
				    const struct adftool_term *identifier,
				    size_t *column);

  extern LIBADFTOOL_API
    int adftool_add_channel_type (struct adftool_file *file,
				  const struct adftool_term *channel,
				  const struct adftool_term *type);

  extern LIBADFTOOL_API
    size_t adftool_get_channel_types (struct adftool_file *file,
				      const struct adftool_term *channel,
				      size_t start, size_t max,
				      struct adftool_term **types);

  extern LIBADFTOOL_API
    size_t adftool_find_channels_by_type (struct adftool_file *file,
					  const struct adftool_term *type,
					  size_t start, size_t max,
					  struct adftool_term **identifiers);

  extern LIBADFTOOL_API
    int adftool_get_channel_decoder (struct adftool_file *file,
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
    int adftool_eeg_get_data (struct adftool_file *file,
			      size_t time_start, size_t time_length,
			      size_t *time_max, size_t channel_start,
			      size_t channel_length, size_t *channel_max,
			      double *data);

  extern LIBADFTOOL_API
    int adftool_eeg_get_time (struct adftool_file *file,
			      size_t observation, struct timespec *time,
			      double *sampling_frequency);

  extern LIBADFTOOL_API
    int adftool_eeg_set_time (struct adftool_file *file,
			      const struct timespec *time,
			      double sampling_frequency);

  struct adftool_fir;

  extern LIBADFTOOL_API
    void adftool_fir_auto_bandwidth (double sfreq, double freq_low,
				     double freq_high, double *trans_low,
				     double *trans_high);

  extern LIBADFTOOL_API
    size_t adftool_fir_auto_order (double sfreq,
				   double tightest_transition_bandwidth);

  extern LIBADFTOOL_API struct adftool_fir *adftool_fir_alloc (size_t order);

  extern LIBADFTOOL_API
    size_t adftool_fir_order (const struct adftool_fir *filter);

  extern LIBADFTOOL_API
    void adftool_fir_coefficients (const struct adftool_fir *filter,
				   double *coefficients);

  extern LIBADFTOOL_API void adftool_fir_free (struct adftool_fir *filter);

  extern LIBADFTOOL_API
    void adftool_fir_design_bandpass (struct adftool_fir *filter,
				      double sfreq,
				      double freq_low, double freq_high,
				      double trans_low, double trans_high);

  extern LIBADFTOOL_API
    void adftool_fir_apply (const struct adftool_fir *filter,
			    size_t signal_length, const double *signal,
			    double *filtered);

  /* These API functions are needed for emscripten, because it’s not
     easy to compute the address of something in JS. */

  extern LIBADFTOOL_API struct timespec *adftool_timespec_alloc (void);

  extern LIBADFTOOL_API void adftool_timespec_free (struct timespec *time);

  extern LIBADFTOOL_API
    void adftool_timespec_set_js (struct timespec *time, double milliseconds);

  extern LIBADFTOOL_API
    double adftool_timespec_get_js (const struct timespec *time);

  struct adftool_array_long;

  extern LIBADFTOOL_API
    struct adftool_array_long *adftool_array_long_alloc (size_t n_elements);

  extern LIBADFTOOL_API
    void adftool_array_long_free (struct adftool_array_long *array);

  extern LIBADFTOOL_API
    long *adftool_array_long_address (struct adftool_array_long *array,
				      size_t i);

  extern LIBADFTOOL_API
    void adftool_array_long_set (struct adftool_array_long *array,
				 size_t i, long value);

  extern LIBADFTOOL_API
    long adftool_array_long_get (const struct adftool_array_long *array,
				 size_t i);

  struct adftool_array_double;

  extern LIBADFTOOL_API
    struct adftool_array_double *adftool_array_double_alloc (size_t
							     n_elements);

  extern LIBADFTOOL_API
    void adftool_array_double_free (struct adftool_array_double *array);

  extern LIBADFTOOL_API
    double *adftool_array_double_address (struct adftool_array_double *array,
					  size_t i);

  extern LIBADFTOOL_API
    void adftool_array_double_set (struct adftool_array_double *array,
				   size_t i, double value);

  extern LIBADFTOOL_API
    double adftool_array_double_get (const struct adftool_array_double *array,
				     size_t i);

  struct adftool_array_size_t;

  extern LIBADFTOOL_API
    struct adftool_array_size_t *adftool_array_size_t_alloc (size_t
							     n_elements);

  extern LIBADFTOOL_API
    void adftool_array_size_t_free (struct adftool_array_size_t *array);

  extern LIBADFTOOL_API
    size_t *adftool_array_size_t_address (struct adftool_array_size_t *array,
					  size_t i);

  extern LIBADFTOOL_API
    void adftool_array_size_t_set (struct adftool_array_size_t *array,
				   size_t i, size_t value);

  extern LIBADFTOOL_API
    size_t adftool_array_size_t_get (const struct adftool_array_size_t *array,
				     size_t i);

  struct adftool_array_uint64_t;

  extern LIBADFTOOL_API
    struct adftool_array_uint64_t *adftool_array_uint64_t_alloc (size_t
								 n_elements);

  extern LIBADFTOOL_API
    void adftool_array_uint64_t_free (struct adftool_array_uint64_t *array);

  extern LIBADFTOOL_API
    uint64_t *
    adftool_array_uint64_t_address (struct adftool_array_uint64_t *array,
				    size_t i);

  extern LIBADFTOOL_API
    void adftool_array_uint64_t_set (struct adftool_array_uint64_t *array,
				     size_t i, uint64_t value);

  extern LIBADFTOOL_API
    void adftool_array_uint64_t_set_js (struct adftool_array_uint64_t *array,
					size_t i, double high, double low);

  extern LIBADFTOOL_API
    uint64_t adftool_array_uint64_t_get (const struct adftool_array_uint64_t
					 *array, size_t i);

  extern LIBADFTOOL_API
    double adftool_array_uint64_t_get_js_high (const struct
					       adftool_array_uint64_t *array,
					       size_t i);

  extern LIBADFTOOL_API
    double adftool_array_uint64_t_get_js_low (const struct
					      adftool_array_uint64_t *array,
					      size_t i);

  struct adftool_array_pointer;

  extern LIBADFTOOL_API
    struct adftool_array_pointer *adftool_array_pointer_alloc (size_t
							       n_elements);

  extern LIBADFTOOL_API
    void adftool_array_pointer_free (struct adftool_array_pointer *array);

  extern LIBADFTOOL_API
    void **adftool_array_pointer_address (struct adftool_array_pointer *array,
					  size_t i);

  extern LIBADFTOOL_API
    void adftool_array_pointer_set (struct adftool_array_pointer *array,
				    size_t i, void *value);

  extern LIBADFTOOL_API
    void *adftool_array_pointer_get (const struct adftool_array_pointer
				     *array, size_t i);

# ifdef __cplusplus
}

#  include <string>
#  include <vector>
#  include <chrono>
#  include <optional>
#  include <tuple>
#  include <cassert>

/* *INDENT-OFF* */
namespace adftool
{
  class term
  {
  private:
    struct adftool_term *ptr;
  public:
    term ()
    {
      this->ptr = adftool_term_alloc ();
      if (this->ptr == nullptr)
	{
	  std::bad_alloc error;
	  throw error;
	}
    }
    term (term &&v) noexcept: ptr (v.ptr)
    {
      v.ptr = nullptr;
    }
    term (const term &v)
    {
      this->ptr = adftool_term_alloc ();
      if (this->ptr == nullptr)
	{
	  std::bad_alloc error;
	  throw error;
	}
      this->copy (v);
    }
    ~term (void) noexcept
    {
      adftool_term_free (this->ptr);
    }
    term &operator= (term &v)
    {
      this->copy (v);
      return *this;
    }
    term &operator= (term &&v) noexcept
    {
      adftool_term_free (this->ptr);
      this->ptr = v.ptr;
      v.ptr = nullptr;
      return *this;
    }
    void copy (const struct adftool_term *other) noexcept
    {
      adftool_term_copy (this->ptr, other);
    }
    void copy (const term & other) noexcept
    {
      this->copy (other.ptr);
    }
    void set_blank (const std::string & id) noexcept
    {
      adftool_term_set_blank (this->ptr, id.c_str ());
    }
    void set_named (const std::string & id) noexcept
    {
      adftool_term_set_named (this->ptr, id.c_str ());
    }
    void set_string (const std::string & value) noexcept
    {
      adftool_term_set_literal (this->ptr, value.c_str (), nullptr, nullptr);
    }
    void set_typed_literal (const std::string & value,
			    const std::string & type) noexcept
    {
      adftool_term_set_literal (this->ptr, value.c_str (), type.c_str (),
				nullptr);
    }
    void set_langstring (const std::string & value,
			 const std::string & langtag) noexcept
    {
      adftool_term_set_literal (this->ptr, value.c_str (), nullptr,
				langtag.c_str ());
    }
    void set_integer (long value) noexcept
    {
      adftool_term_set_integer (this->ptr, value);
    }
    void set_double (double value) noexcept
    {
      adftool_term_set_double (this->ptr, value);
    }
    void set_date (std::chrono::time_point<std::chrono::high_resolution_clock> date) noexcept
    {
      using clock = std::chrono::high_resolution_clock;
      using seconds = std::chrono::seconds;
      using nanoseconds = std::chrono::nanoseconds;
      const clock::duration since_epoch = date.time_since_epoch ();
      const seconds date_seconds =
	std::chrono::duration_cast<seconds> (since_epoch);
      const clock::duration since_epoch_floor =
	std::chrono::duration_cast<clock::duration> (date_seconds);
      const clock::duration remaining = since_epoch - since_epoch_floor;
      const nanoseconds date_nanoseconds =
	std::chrono::duration_cast<nanoseconds> (remaining);
      struct timespec c_date;
      c_date.tv_sec = date_seconds.count ();
      c_date.tv_nsec = date_nanoseconds.count ();
      adftool_term_set_date (this->ptr, &c_date);
    }
    bool is_blank (void) const noexcept
    {
      return adftool_term_is_blank (this->ptr);
    }
    bool is_named (void) const noexcept
    {
      return adftool_term_is_named (this->ptr);
    }
    bool is_literal (void) const noexcept
    {
      return adftool_term_is_literal (this->ptr);
    }
    bool is_typed_literal (void) const noexcept
    {
      return adftool_term_is_typed_literal (this->ptr);
    }
    bool is_langstring (void) const noexcept
    {
      return adftool_term_is_langstring (this->ptr);
    }
    size_t value (size_t discard, std::string::iterator begin,
		  std::string::iterator end) const noexcept
    {
      return adftool_term_value (this->ptr, discard, end - begin, &(*begin));
    }
    std::string value_alloc (void) const
    {
      std::string nothing;
      size_t required = this->value (0, nothing.begin (), nothing.end ());
      std::string ret;
      ret.resize (required);
      this->value (0, ret.begin (), ret.end ());
      return ret;
    }
    size_t meta (size_t discard, std::string::iterator begin,
		 std::string::iterator end) const noexcept
    {
      return adftool_term_meta (this->ptr, discard, end - begin, &(*begin));
    }
    std::string meta_alloc (void) const
    {
      std::string nothing;
      size_t required = this->meta (0, nothing.begin (), nothing.end ());
      std::string ret;
      ret.resize (required);
      this->meta (0, ret.begin (), ret.end ());
      return ret;
    }
    bool as_mpz (mpz_t value) const
    {
      return (adftool_term_as_mpz (this->ptr, value) == 0);
    }
    std::optional<long> as_integer (void) const
    {
      long value;
      if (adftool_term_as_integer (this->ptr, &value) == 0)
	{
	  std::optional<long> ret = value;
	  return ret;
	}
      return std::nullopt;
    }
    bool as_mpf (mpf_t value) const
    {
      return (adftool_term_as_mpf (this->ptr, value) == 0);
    }
    std::optional<double> as_double (void) const
    {
      double value;
      if (adftool_term_as_double (this->ptr, &value) == 0)
	{
	  std::optional<double> ret = value;
	  return ret;
	}
      return std::nullopt;
    }
    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> as_date (void) const
    {
      struct timespec value;
      if (adftool_term_as_date (this->ptr, &value) == 0)
	{
	  using clock = std::chrono::high_resolution_clock;
	  using seconds = std::chrono::seconds;
	  using nanoseconds = std::chrono::nanoseconds;
	  const auto seconds_since_epoch = seconds (value.tv_sec);
	  const auto remaining = nanoseconds (value.tv_nsec);
	  const auto since_epoch =
	    std::chrono::duration_cast<clock::duration> (seconds_since_epoch)
	    + std::chrono::duration_cast<clock::duration> (remaining);
	  const std::chrono::time_point<clock> cxx_value =
	    std::chrono::time_point<clock> (since_epoch);
	  const std::optional<std::chrono::time_point<clock>> ret = cxx_value;
	  return ret;
	}
      return std::nullopt;
    }
    int compare (const term & other) const
    {
      return adftool_term_compare (this->ptr, other.ptr);
    }
    std::pair<bool, std::string::const_iterator>
      parse_n3 (const std::string::const_iterator & begin,
		const std::string::const_iterator & end)
    {
      size_t consumed;
      if (adftool_term_parse_n3 (&(*begin), end - begin, &consumed, this->ptr)
	  == 0)
	{
	  return std::pair<bool, std::string::const_iterator> (true, begin + consumed);
	}
      return std::pair<bool, std::string::const_iterator> (false, begin);
    }
    size_t to_n3 (size_t discard, const std::string::iterator begin,
		  const std::string::iterator end) const noexcept
    {
      return adftool_term_to_n3 (this->ptr, discard, end - begin, &(*begin));
    }
    std::string to_n3_alloc (void) const
    {
      std::string nothing;
      size_t required = this->to_n3 (0, nothing.begin (), nothing.end ());
      std::string ret;
      ret.resize (required);
      this->to_n3 (0, ret.begin (), ret.end ());
      return ret;
    }
    const struct adftool_term *c_ptr (void) const
    {
      return this->ptr;
    }
    struct adftool_term *c_ptr (void)
    {
      return this->ptr;
    }
  };

  using optional_term = std::optional<term>;

  class statement
  {
  private:
    struct adftool_statement *ptr;
  public:
    statement ()
    {
      this->ptr = adftool_statement_alloc ();
      if (this->ptr == nullptr)
	{
	  std::bad_alloc error;
	  throw error;
	}
    }
    statement (statement &&v) noexcept: ptr (v.ptr)
    {
      v.ptr = nullptr;
    }
    statement (const statement &v)
    {
      this->ptr = adftool_statement_alloc ();
      if (this->ptr == nullptr)
	{
	  std::bad_alloc error;
	  throw error;
	}
      this->copy (v);
    }
    ~statement (void) noexcept
    {
      adftool_statement_free (this->ptr);
    }
    statement &operator= (statement &v)
    {
      this->copy (v);
      return *this;
    }
    statement &operator= (statement &&v) noexcept
    {
      adftool_statement_free (this->ptr);
      this->ptr = v.ptr;
      v.ptr = nullptr;
      return *this;
    }
    void set (const std::tuple<std::optional<optional_term>, std::optional<optional_term>, std::optional<optional_term>, std::optional<optional_term>, std::optional<std::optional<uint64_t>>> &data)
    {
      const struct adftool_term *c_terms[4];
      struct adftool_term **c_term_pointers[4];
      for (size_t i = 0; i < 4; i++)
	{
	  c_terms[i] = nullptr;
	  c_term_pointers[i] = nullptr;
	}
#define adftool_set_ptr(i) \
      if (std::get<i> (data).has_value () && std::get<i> (data).value ().has_value ()) \
	{ \
	  c_terms[i] = std::get<i> (data).value ().value ().c_ptr ();	\
	} \
      if (std::get<i> (data).has_value ())	\
	{ \
	  c_term_pointers[i] = (struct adftool_term **) & (c_terms[i]); \
	}
      adftool_set_ptr (0);
      adftool_set_ptr (1);
      adftool_set_ptr (2);
      adftool_set_ptr (3);
#undef adftool_set_ptr
      uint64_t c_date = ((uint64_t) (-1));
      uint64_t *c_date_ptr = nullptr;
      if (std::get<4> (data).has_value () && std::get<4> (data).value ().has_value ())
	{
	  c_date = std::get<4> (data).value ().value ();
	}
      if (std::get<4> (data).has_value ())
	{
	  c_date_ptr = &c_date;
	}
      adftool_statement_set (this->ptr, c_term_pointers[0], c_term_pointers[1], c_term_pointers[2], c_term_pointers[3], c_date_ptr);
    }
    std::tuple<term, term, term, term, std::optional<uint64_t>> get (void) const
    {
      std::tuple<term, term, term, term, std::optional<uint64_t>> ret;
      uint64_t date;
      struct adftool_term *ptrs[4];
      adftool_statement_get (this->ptr,
			     & (ptrs[0]), & (ptrs[1]), & (ptrs[2]), & (ptrs[3]),
			     &date);
      if (date != ((uint64_t) (-1)))
	{
	  std::get<4> (ret) = date;
	}
      std::get<0> (ret).copy (ptrs[0]);
      std::get<1> (ret).copy (ptrs[1]);
      std::get<2> (ret).copy (ptrs[2]);
      std::get<3> (ret).copy (ptrs[3]);
      return ret;
    }
    void copy (const struct adftool_statement *other) noexcept
    {
      adftool_statement_copy (this->ptr, other);
    }
    void copy (const statement & other) noexcept
    {
      this->copy (other.ptr);
    }
    int compare (const statement & other, std::string order) const
    {
      return adftool_statement_compare (this->ptr, other.ptr, order.c_str ());
    }
    const struct adftool_statement *c_ptr (void) const
    {
      return this->ptr;
    }
    struct adftool_statement *c_ptr (void)
    {
      return this->ptr;
    }
  };

  class fir
  {
  private:
    struct adftool_fir *ptr;
  public:
    fir (size_t order)
    {
      this->ptr = adftool_fir_alloc (order);
      if (this->ptr == nullptr)
	{
	  std::bad_alloc error;
	  throw error;
	}
    }
    fir (double sfreq, double freq_low, double freq_high)
    {
      std::pair<double, double> transition =
	fir::auto_bandwidth (sfreq, freq_low, freq_high);
      size_t order =
	fir::auto_order (sfreq, transition);
      this->ptr = adftool_fir_alloc (order);
      if (this->ptr == nullptr)
	{
	  std::bad_alloc error;
	  throw error;
	}
      this->design_bandpass (sfreq, freq_low, freq_high, transition.first, transition.second);
    }
    fir (double sfreq, double freq_low, double freq_high, double trans_low, double trans_high)
    {
      size_t order =
	fir::auto_order (sfreq, std::pair<double, double> (trans_low, trans_high));
      this->ptr = adftool_fir_alloc (order);
      if (this->ptr == nullptr)
	{
	  std::bad_alloc error;
	  throw error;
	}
      this->design_bandpass (sfreq, freq_low, freq_high, trans_low, trans_high);
    }
    fir (fir && v) noexcept: ptr (v.ptr)
    {
      v.ptr = nullptr;
    }
    ~fir (void) noexcept
    {
      adftool_fir_free (this->ptr);
    }
    fir & operator= (fir && v) noexcept
    {
      adftool_fir_free (this->ptr);
      this->ptr = v.ptr;
      v.ptr = nullptr;
      return *this;
    }
    static std::pair<double, double> auto_bandwidth (double sfreq, double freq_low, double freq_high)
    {
      double trans_low, trans_high;
      adftool_fir_auto_bandwidth (sfreq, freq_low, freq_high, &trans_low, &trans_high);
      return std::pair<double, double> (trans_low, trans_high);
    }
    static size_t auto_order (double sfreq, double tightest_transition_bandwidth) noexcept
    {
      return adftool_fir_auto_order (sfreq, tightest_transition_bandwidth);
    }
    static size_t auto_order (double sfreq, std::pair<double, double> transition) noexcept
    {
      double tightest = transition.first;
      if (transition.second < tightest)
	{
	  tightest = transition.second;
	}
      return adftool_fir_auto_order (sfreq, tightest);
    }
    static size_t auto_order (double sfreq, double freq_low, double freq_high)
    {
      std::pair<double, double> transition = fir::auto_bandwidth (sfreq, freq_low, freq_high);
      return fir::auto_order (sfreq, transition);
    }
    size_t order (void) const noexcept
    {
      return adftool_fir_order (this->ptr);
    }
    void coefficients (std::vector<double> &output) const noexcept
    {
      assert (output.size () >= this->order ());
      adftool_fir_coefficients (this->ptr, output.data ());
    }
    std::vector<double> coefficients (void) const
    {
      std::vector<double> coef;
      coef.resize (this->order ());
      this->coefficients (coef);
      return coef;
    }
    void design_bandpass (double sfreq, double freq_low, double freq_high, double trans_low, double trans_high) noexcept
    {
      adftool_fir_design_bandpass (this->ptr, sfreq, freq_low, freq_high, trans_low, trans_high);
    }
    void design_bandpass (double sfreq, double freq_low, double freq_high) noexcept
    {
      double trans_low, trans_high;
      adftool_fir_auto_bandwidth (sfreq, freq_low, freq_high, &trans_low, &trans_high);
      this->design_bandpass (sfreq, freq_low, freq_high, trans_low, trans_high);
    }
    void apply (const std::vector<double> &input, std::vector<double> &output) const noexcept
    {
      assert (input.size () == output.size ());
      adftool_fir_apply (this->ptr, input.size (), input.data (), output.data ());
    }
    std::vector<double> apply (const std::vector<double> &input) const
    {
      std::vector<double> output;
      output.resize (input.size ());
      this->apply (input, output);
      return output;
    }
  };
}
/* *INDENT-ON* */
# endif				/* __cplusplus */

#endif				/* not H_ADFTOOL_INCLUDED */
