#include <config.h>

#include <attribute.h>
#include <adftool.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "term.h"
#include "file.h"

struct adftool_term *
adftool_term_alloc (void)
{
  return term_alloc ();
}

void
adftool_term_free (struct adftool_term *term)
{
  term_free (term);
}

void
adftool_term_set_blank (struct adftool_term *term, const char *id)
{
  term_set_blank (term, id);
}

void
adftool_term_set_named (struct adftool_term *term, const char *id)
{
  term_set_named (term, id);
}

void
adftool_term_set_literal (struct adftool_term *term, const char *value,
			  const char *type, const char *lang)
{
  term_set_literal (term, value, type, lang);
}

int
adftool_term_is_blank (const struct adftool_term *term)
{
  return term_is_blank (term);
}

int
adftool_term_is_named (const struct adftool_term *term)
{
  return term_is_named (term);
}

int
adftool_term_is_literal (const struct adftool_term *term)
{
  return term_is_literal (term);
}

int
adftool_term_is_typed_literal (const struct adftool_term *term)
{
  return term_is_typed_literal (term);
}

int
adftool_term_is_langstring (const struct adftool_term *term)
{
  return term_is_langstring (term);
}

size_t
adftool_term_value (const struct adftool_term *term, size_t start, size_t max,
		    char *value)
{
  return term_value (term, start, max, value);
}

size_t
adftool_term_meta (const struct adftool_term *term, size_t start, size_t max,
		   char *value)
{
  return term_meta (term, start, max, value);
}

int
adftool_term_compare (const struct adftool_term *reference,
		      const struct adftool_term *other)
{
  return term_compare (reference, other);
}

int
adftool_term_decode (struct adftool_file *file, uint64_t value,
		     struct adftool_term *decoded)
{
  return term_decode (file->dictionary, value, decoded);
}

int
adftool_term_encode (struct adftool_file *file,
		     const struct adftool_term *term, uint64_t * encoded)
{
  return term_encode (file->dictionary, term, encoded);
}

void
adftool_term_copy (struct adftool_term *dest, const struct adftool_term *src)
{
  term_copy (dest, src);
}

void
adftool_term_set_mpz (struct adftool_term *term, mpz_t value)
{
  term_set_mpz (term, value);
}

void
adftool_term_set_mpf (struct adftool_term *term, mpf_t value)
{
  term_set_mpf (term, value);
}

void
adftool_term_set_integer (struct adftool_term *term, long value)
{
  term_set_integer (term, value);
}

void
adftool_term_set_double (struct adftool_term *term, double value)
{
  term_set_double (term, value);
}

int
adftool_term_as_mpz (const struct adftool_term *term, mpz_t value)
{
  return term_as_mpz (term, value);
}

int
adftool_term_as_mpf (const struct adftool_term *term, mpf_t value)
{
  return term_as_mpf (term, value);
}

int
adftool_term_as_integer (const struct adftool_term *term, long *value)
{
  return term_as_integer (term, value);
}

int
adftool_term_as_double (const struct adftool_term *term, double *value)
{
  return term_as_double (term, value);
}

void
adftool_term_set_date (struct adftool_term *term, const struct timespec *nsec)
{
  term_set_date (term, nsec);
}

int
adftool_term_as_date (const struct adftool_term *term, struct timespec *nsec)
{
  return term_as_date (term, nsec);
}
