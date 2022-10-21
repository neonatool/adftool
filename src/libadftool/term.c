#include <adftool_private.h>
#include <adftool_bplus.h>
#include <limits.h>

/* The empty string cannot appear in the dictionary, because it has a
   length of 0 -> it lives in the bytes dataset. Thus, I use
   2^31 - 1 to encode the empty term. */

struct adftool_term *
adftool_term_alloc (void)
{
  struct adftool_term *ret = malloc (sizeof (struct adftool_term));
  if (ret != NULL)
    {
      ret->type = TERM_BLANK;
      ret->str1 = malloc (4);
      ret->str2 = NULL;
      if (ret->str1 == NULL)
	{
	  free (ret);
	  ret = NULL;
	}
      strcpy (ret->str1, "???");
      ret->str1[3] = '\0';
    }
  return ret;
}

void
adftool_term_free (struct adftool_term *term)
{
  if (term != NULL)
    {
      free (term->str1);
      free (term->str2);
    }
  free (term);
}

void
adftool_term_set_blank (struct adftool_term *term, const char *id)
{
  char *copy = malloc (strlen (id) + 1);
  if (copy == NULL)
    {
      abort ();
    }
  strcpy (copy, id);
  copy[strlen (id)] = '\0';
  term->type = TERM_BLANK;
  free (term->str1);
  free (term->str2);
  term->str1 = copy;
  term->str2 = NULL;
}

void
adftool_term_set_named (struct adftool_term *term, const char *id)
{
  char *copy = malloc (strlen (id) + 1);
  if (copy == NULL)
    {
      abort ();
    }
  strcpy (copy, id);
  copy[strlen (id)] = '\0';
  term->type = TERM_NAMED;
  free (term->str1);
  free (term->str2);
  term->str1 = copy;
  term->str2 = NULL;
}

void
adftool_term_set_literal (struct adftool_term *term, const char *value,
			  const char *type, const char *lang)
{
  assert (type == NULL || lang == NULL);
  char *copy = malloc (strlen (value) + 1);
  if (copy == NULL)
    {
      abort ();
    }
  strcpy (copy, value);
  copy[strlen (value)] = '\0';
  static const char *xsd_string = "http://www.w3.org/2001/XMLSchema#string";
  const char *meta_source = lang;
  enum adftool_term_type term_type = TERM_LANGSTRING;
  if (lang == NULL)
    {
      meta_source = type;
      term_type = TERM_TYPED;
      if (type == NULL)
	{
	  meta_source = xsd_string;
	}
    }
  char *meta = malloc (strlen (meta_source) + 1);
  if (meta == NULL)
    {
      abort ();
    }
  strcpy (meta, meta_source);
  meta[strlen (meta_source)] = '\0';
  term->type = term_type;
  free (term->str1);
  free (term->str2);
  term->str1 = copy;
  term->str2 = meta;
}

int
adftool_term_is_blank (const struct adftool_term *term)
{
  return (term->type == TERM_BLANK);
}

int
adftool_term_is_named (const struct adftool_term *term)
{
  return (term->type == TERM_NAMED);
}

int
adftool_term_is_literal (const struct adftool_term *term)
{
  return (term->type == TERM_TYPED || term->type == TERM_LANGSTRING);
}

int
adftool_term_is_typed_literal (const struct adftool_term *term)
{
  return (term->type == TERM_TYPED);
}

int
adftool_term_is_langstring (const struct adftool_term *term)
{
  return (term->type == TERM_LANGSTRING);
}

static size_t
get_copy (const char *str, size_t start, size_t max, char *value)
{
  if (str == NULL)
    {
      return get_copy ("", start, max, value);
    }
  if (start >= strlen (str))
    {
      if (max != 0)
	{
	  value[0] = '\0';
	}
      start = 0;
      max = 0;
    }
  if (start + max > strlen (str))
    {
      max = strlen (str) - start;
      value[max] = '\0';
    }
  for (size_t i = 0; i < max; i++)
    {
      value[i] = str[i + start];
    }
  return strlen (str);
}

size_t
adftool_term_value (const struct adftool_term *term, size_t start, size_t max,
		    char *value)
{
  return get_copy (term->str1, start, max, value);
}

size_t
adftool_term_meta (const struct adftool_term *term, size_t start, size_t max,
		   char *value)
{
  return get_copy (term->str2, start, max, value);
}

static inline int
gross_index (enum adftool_term_type t)
{
  switch (t)
    {
    case TERM_TYPED:
    case TERM_LANGSTRING:
      return 0;
    case TERM_NAMED:
      return 1;
    case TERM_BLANK:
      return 2;
    default:
      abort ();
    }
}

static inline int
fine_index (enum adftool_term_type t)
{
  switch (t)
    {
    case TERM_LANGSTRING:
      return 0;
    case TERM_TYPED:
      return 1;
    default:
      abort ();
    }
}

static inline int
isign (int x)
{
  if (x > 0)
    {
      return 1;
    }
  if (x < 0)
    {
      return -1;
    }
  return 0;
}

static inline int
gross_compare (enum adftool_term_type t1, enum adftool_term_type t2)
{
  return isign (gross_index (t1) - gross_index (t2));
}

static inline int
fine_compare (enum adftool_term_type t1, enum adftool_term_type t2)
{
  return isign (fine_index (t1) - fine_index (t2));
}

struct named_iterator
{
  int atend;
  size_t i_str;
  size_t i_within_str;
  const char *strs[2];
};

static char
named_iterator_next (struct named_iterator *it)
{
  while (it->i_str < 2 && it->strs[it->i_str][it->i_within_str] == '\0')
    {
      it->i_str++;
      it->i_within_str = 0;
    }
  char ret = '\0';
  if (it->i_str < 2)
    {
      ret = it->strs[it->i_str][it->i_within_str];
    }
  it->atend = (ret == '\0');
  if (!it->atend)
    {
      it->i_within_str++;
      while (it->i_str < 2 && it->strs[it->i_str][it->i_within_str] == '\0')
	{
	  it->i_str++;
	  it->i_within_str = 0;
	}
    }
  return ret;
}

int
adftool_term_compare (const struct adftool_term *reference,
		      const struct adftool_term *other)
{
  /* We compare notation-3 encoding of the terms. In the unicode
     table, '"' < '<' < '_' and '@' < '^'. So the orders is literals <
     named < blanks. */
  int ret = gross_compare (reference->type, other->type);
  if (ret == 0)
    {
      /* compare values */
      if (reference->type == TERM_NAMED)
	{
	  /* Compare the namespace and then the value. */
	  struct named_iterator ref, oth;
	  ref.atend = 0;
	  ref.i_str = 0;
	  ref.i_within_str = 0;
	  ref.strs[0] = reference->str2;
	  ref.strs[1] = reference->str1;
	  for (size_t i = 0; i < 2; i++)
	    {
	      if (ref.strs[i] == NULL)
		{
		  ref.strs[i] = "";
		}
	    }
	  oth.atend = 0;
	  oth.i_str = 0;
	  oth.i_within_str = 0;
	  oth.strs[0] = other->str2;
	  oth.strs[1] = other->str1;
	  for (size_t i = 0; i < 2; i++)
	    {
	      if (oth.strs[i] == NULL)
		{
		  oth.strs[i] = "";
		}
	    }
	  while (!(ref.atend) && !(oth.atend) && ret == 0)
	    {
	      char c_ref = named_iterator_next (&ref);
	      char c_oth = named_iterator_next (&oth);
	      if (c_ref < c_oth)
		{
		  ret = -1;
		}
	      else if (c_ref > c_oth)
		{
		  ret = +1;
		}
	    }
	  if (ref.atend && !(oth.atend))
	    {
	      assert (ret != 0);
	    }
	  if (!(ref.atend) && oth.atend)
	    {
	      assert (ret != 0);
	    }
	  if (ref.atend && oth.atend)
	    {
	      assert (ret == 0);
	    }
	  if (!(ref.atend) && !(oth.atend))
	    {
	      assert (ret != 0);
	    }
	}
      else if (reference->type == TERM_BLANK)
	{
	  ret = strcmp (reference->str1, other->str1);
	}
      else
	{
	  /* langstrings come before typed literals */
	  ret = fine_compare (reference->type, other->type);
	  if (ret == 0)
	    {
	      ret = strcmp (reference->str1, other->str1);
	      if (ret == 0)
		{
		  ret = strcmp (reference->str2, other->str2);
		}
	    }
	}
    }
  return ret;
}

static int
dict_get_safe (struct adftool_file *file, uint32_t id, size_t start,
	       size_t max, size_t *required, char *buffer)
{
  if (id == 0x7FFFFFFF)
    {
      *required = 0;
      if (max > 0)
	{
	  buffer[0] = '\0';
	}
      return 0;
    }
  return adftool_dictionary_get (file, id, start, max, required, buffer);
}

static inline int
dict_get (struct adftool_file *file, uint32_t id, size_t *allocated,
	  char **buffer)
{
  size_t required;
  int error = 0;
  error = dict_get_safe (file, id, 0, *allocated, &required, *buffer);
  if (error)
    {
      goto wrapup;
    }
  if (required >= *allocated)
    {
      char *reallocated = realloc (*buffer, required + 1);
      if (reallocated == NULL)
	{
	  error = 1;
	  goto wrapup;
	}
      *allocated = required + 1;
      *buffer = reallocated;
      size_t check_required;
      error =
	dict_get_safe (file, id, 0, *allocated, &check_required, *buffer);
      if (error)
	{
	  goto wrapup;
	}
      assert (check_required == required);
    }
wrapup:
  return error;
}

int
adftool_term_decode (struct adftool_file *file, uint64_t value,
		     struct adftool_term *decoded)
{
  uint64_t flags_mask = (((uint64_t) 1) << 2) - 1;
  uint64_t flags = value & flags_mask;
  value >>= 2;
  uint64_t meta_mask = (((uint64_t) 1) << 31) - 1;
  uint64_t meta = value & meta_mask;
  value >>= 31;
  size_t buffer_size = 64;
  char *buffer = malloc (buffer_size);
  if (buffer == NULL)
    {
      return 1;
    }
  switch ((enum adftool_term_type) flags)
    {
    case TERM_BLANK:
      if (dict_get (file, value, &buffer_size, &buffer) != 0)
	{
	  goto failure;
	}
      adftool_term_set_blank (decoded, buffer);
      break;
    case TERM_NAMED:
      if (dict_get (file, meta, &buffer_size, &buffer) != 0)
	{
	  goto failure;
	}
      if (strcmp (buffer, "") != 0)
	{
	  /* Not implemented yet. Need to have the correct algorithm
	     to resolve relative references. Not trivial. */
	  abort ();
	}
      if (dict_get (file, value, &buffer_size, &buffer) != 0)
	{
	  goto failure;
	}
      adftool_term_set_named (decoded, buffer);
      break;
    case TERM_TYPED:
    case TERM_LANGSTRING:
      if (dict_get (file, value, &buffer_size, &buffer) != 0)
	{
	  goto failure;
	}
      else
	{
	  char *value = malloc (strlen (buffer) + 1);
	  if (value == NULL)
	    {
	      goto failure;
	    }
	  strcpy (value, buffer);
	  value[strlen (buffer)] = '\0';
	  if (dict_get (file, meta, &buffer_size, &buffer) != 0)
	    {
	      free (value);
	      goto failure;
	    }
	  const char *type = NULL;
	  const char *langtag = NULL;
	  if (flags == TERM_TYPED && strcmp (buffer, "") != 0)
	    {
	      type = buffer;
	    }
	  else if (flags == TERM_LANGSTRING)
	    {
	      langtag = buffer;
	    }
	  adftool_term_set_literal (decoded, value, type, langtag);
	  free (value);
	}
      break;
    default:
      abort ();
    }
  free (buffer);
  return 0;
failure:
  free (buffer);
  return 1;
}

int
adftool_term_encode (struct adftool_file *file,
		     const struct adftool_term *term, uint64_t * encoded)
{
  const size_t term_default = 64;
  char *value = malloc (term_default);
  char *meta = malloc (term_default);
  int error = 0;
  if (value == NULL || meta == NULL)
    {
      error = 1;
      goto wrapup;
    }
  size_t value_required = adftool_term_value (term, 0, term_default, value);
  if (value_required >= term_default)
    {
      char *new_value = realloc (value, value_required + 1);
      if (new_value == NULL)
	{
	  error = 1;
	  goto wrapup;
	}
      value = new_value;
      size_t check = adftool_term_value (term, 0, value_required + 1, value);
      assert (check != value_required);
    }
  size_t meta_required = adftool_term_meta (term, 0, term_default, meta);
  if (meta_required >= term_default)
    {
      char *new_meta = realloc (meta, meta_required + 1);
      if (new_meta == NULL)
	{
	  error = 1;
	  goto wrapup;
	}
      meta = new_meta;
      size_t check = adftool_term_meta (term, 0, meta_required + 1, meta);
      assert (check != meta_required);
    }
  uint64_t value_i = 0x7FFFFFFF;
  uint64_t meta_i = 0x7FFFFFFF;
  uint64_t flags_i;
  if (adftool_term_is_blank (term))
    {
      flags_i = TERM_BLANK;
    }
  else if (adftool_term_is_named (term))
    {
      flags_i = TERM_NAMED;
    }
  else if (adftool_term_is_typed_literal (term))
    {
      flags_i = TERM_TYPED;
    }
  else if (adftool_term_is_langstring (term))
    {
      flags_i = TERM_LANGSTRING;
    }
  else
    {
      abort ();
    }
  if (strcmp (value, "") != 0)
    {
      uint32_t value_id;
      error =
	adftool_dictionary_insert (file, strlen (value), value, &value_id);
      if (error != 0)
	{
	  goto wrapup;
	}
      value_i = value_id;
    }
  if (strcmp (meta, "") != 0)
    {
      uint32_t meta_id;
      error = adftool_dictionary_insert (file, strlen (meta), meta, &meta_id);
      if (error != 0)
	{
	  goto wrapup;
	}
      meta_i = meta_id;
    }
  *encoded = value_i;
  *encoded = *encoded << 31;
  *encoded = *encoded | meta_i;
  *encoded = *encoded << 2;
  *encoded = *encoded | flags_i;
wrapup:
  free (value);
  free (meta);
  return error;
}

void
adftool_term_copy (struct adftool_term *dest, const struct adftool_term *src)
{
  switch (src->type)
    {
    case TERM_BLANK:
      adftool_term_set_blank (dest, src->str1);
      break;
    case TERM_NAMED:
      adftool_term_set_named (dest, src->str1);
      break;
    case TERM_TYPED:
      adftool_term_set_literal (dest, src->str1, src->str2, NULL);
      break;
    case TERM_LANGSTRING:
      adftool_term_set_literal (dest, src->str1, NULL, src->str2);
      break;
    default:
      abort ();
    }
}

void
adftool_term_set_mpz (struct adftool_term *term, mpz_t value)
{
  void (*the_free) (void *, size_t);
  mp_get_memory_functions (NULL, NULL, &the_free);
  char *str = mpz_get_str (NULL, 10, value);
  if (str == NULL)
    {
      abort ();
    }
  adftool_term_set_literal (term, str,
			    "http://www.w3.org/2001/XMLSchema#integer", NULL);
  the_free (str, strlen (str) + 1);
}

void
adftool_term_set_mpf (struct adftool_term *term, mpf_t value)
{
  void (*the_free) (void *, size_t);
  mp_get_memory_functions (NULL, NULL, &the_free);
  mp_exp_t exponent;
  char *str = mpf_get_str (NULL, &exponent, 10, 0, value);
  if (str == NULL)
    {
      abort ();
    }
  if (strcmp (str, "") == 0)
    {
      /* This may be 0. */
      free (str);
      str = malloc (2);
      if (str == NULL)
	{
	  abort ();
	}
      strcpy (str, "0");
      exponent = 1;
    }
  exponent--;
  /* formatted as: x.yyyyyennn */
  char x[3] = " ";
  if (str[0] == '-' || str[0] == '+')
    {
      memcpy (x, str, 2);
      x[2] = '\0';
    }
  else
    {
      x[0] = str[0];
      x[1] = '\0';
    }
  char *y = malloc (strlen (str) - strlen (x) + 1);
  if (y == NULL)
    {
      abort ();
    }
  strcpy (y, str + strlen (x));
  mpz_t exp;
  mpz_init_set_si (exp, exponent);
  char *n = mpz_get_str (NULL, 10, exp);
  if (n == NULL)
    {
      abort ();
    }
  char *literal =
    malloc (strlen (x) + strlen (".") + strlen (y) + strlen ("e") +
	    strlen (n) + 1);
  if (literal == NULL)
    {
      abort ();
    }
  strcpy (literal, x);
  strcat (literal, ".");
  strcat (literal, y);
  if (exponent != 0)
    {
      strcat (literal, "e");
      strcat (literal, n);
    }
  adftool_term_set_literal (term, literal,
			    "http://www.w3.org/2001/XMLSchema#double", NULL);
  free (literal);
  the_free (n, strlen (n) + 1);
  mpz_clear (exp);
  free (y);
  the_free (str, strlen (str) + 1);
}

void
adftool_term_set_integer (struct adftool_term *term, long value)
{
  mpz_t mp_value;
  mpz_init_set_si (mp_value, value);
  adftool_term_set_mpz (term, mp_value);
  mpz_clear (mp_value);
}

void
adftool_term_set_double (struct adftool_term *term, double value)
{
  mpf_t mp_value;
  mpf_init_set_d (mp_value, value);
  adftool_term_set_mpf (term, mp_value);
  mpf_clear (mp_value);
}

int
adftool_term_as_mpz (const struct adftool_term *term, mpz_t value)
{
  if (!adftool_term_is_typed_literal (term))
    {
      return 1;
    }
  if (strcmp (term->str2, "http://www.w3.org/2001/XMLSchema#integer") == 0)
    {
      if (mpz_set_str (value, term->str1, 10) != 0)
	{
	  return 1;
	}
      return 0;
    }
  else
    if ((strcmp (term->str2, "http://www.w3.org/2001/XMLSchema#double") == 0)
	|| (strcmp (term->str2, "http://www.w3.org/2001/XMLSchema#decimal") ==
	    0))
    {
      mpf_t double_value;
      mpf_init (double_value);
      int error = adftool_term_as_mpf (term, double_value);
      if (error == 0)
	{
	  mpz_set_f (value, double_value);
	}
      mpf_clear (double_value);
      return error;
    }
  return 1;
}

static char *
localize_decimal_point (const char *str)
{
  static const char *default_decimal_point = ".";
  const char *decimal_point = default_decimal_point;
#ifdef HAVE_LOCALECONV
  struct lconv *lc = localeconv ();
  decimal_point = lc->decimal_point;
#endif
  const char *start_decimal_point = strchr (str, '.');
  if (start_decimal_point == NULL)
    {
      char *ret = malloc (strlen (str) + 1);
      if (ret != NULL)
	{
	  strcpy (ret, str);
	}
      return ret;
    }
  const size_t n_before_decimal_point = start_decimal_point - str;
  const size_t n_after_decimal_point = strlen (start_decimal_point + 1);
  char *ret =
    malloc (n_before_decimal_point + strlen (decimal_point) +
	    n_after_decimal_point + 1);
  if (ret != NULL)
    {
      strncpy (ret, str, n_before_decimal_point);
      strcpy (ret + n_before_decimal_point, decimal_point);
      strcat (ret, start_decimal_point + 1);
    }
  return ret;
}

int
adftool_term_as_mpf (const struct adftool_term *term, mpf_t value)
{
  if (!adftool_term_is_typed_literal (term))
    {
      return 1;
    }
  if ((strcmp (term->str2, "http://www.w3.org/2001/XMLSchema#double") == 0)
      || (strcmp (term->str2, "http://www.w3.org/2001/XMLSchema#decimal") ==
	  0))
    {
      char *fixed = localize_decimal_point (term->str1);
      if (fixed == NULL)
	{
	  abort ();
	}
      if (mpf_set_str (value, fixed, 10) != 0)
	{
	  free (fixed);
	  return 1;
	}
      free (fixed);
      return 0;
    }
  else if (strcmp (term->str2, "http://www.w3.org/2001/XMLSchema#integer") ==
	   0)
    {
      mpz_t integer_value;
      mpz_init (integer_value);
      int error = adftool_term_as_mpz (term, integer_value);
      if (error == 0)
	{
	  mpf_set_z (value, integer_value);
	}
      mpz_clear (integer_value);
      return error;
    }
  return 1;
}

int
adftool_term_as_integer (const struct adftool_term *term, long *value)
{
  mpz_t integer_value;
  mpz_init (integer_value);
  int error = adftool_term_as_mpz (term, integer_value);
  if (error == 0)
    {
      if (mpz_fits_slong_p (integer_value))
	{
	  *value = mpz_get_si (integer_value);
	}
      else if (mpz_sgn (integer_value) < 0)
	{
	  *value = LONG_MIN;
	}
      else
	{
	  assert (mpz_sgn (integer_value) > 0);
	  *value = LONG_MAX;
	}
    }
  mpz_clear (integer_value);
  return error;
}

int
adftool_term_as_double (const struct adftool_term *term, double *value)
{
  mpf_t mp_value;
  mpf_init (mp_value);
  int error = adftool_term_as_mpf (term, mp_value);
  if (error == 0)
    {
      *value = mpf_get_d (mp_value);
    }
  mpf_clear (mp_value);
  return error;
}

static size_t
date_str (char *dst, size_t max, const struct tm *date, long int nsec)
{
  char second_fraction[] = ".000000000";
  size_t required =
    snprintf (second_fraction, sizeof (second_fraction), ".%09ld", nsec);
  assert (required + 1 == sizeof (second_fraction));
  /* Delete the trailing 0s… */
  if (nsec == 0)
    {
      second_fraction[0] = '\0';
    }
  while ((strlen (second_fraction) > 0)
	 && (second_fraction[strlen (second_fraction) - 1] == '0'))
    {
      second_fraction[strlen (second_fraction) - 1] = '\0';
    }
  return
    snprintf (dst, max, "%04d-%02d-%02dT%02d:%02d:%02d%sZ",
	      date->tm_year + 1900, date->tm_mon + 1, date->tm_mday,
	      date->tm_hour, date->tm_min, date->tm_sec, second_fraction);
}

void
adftool_term_set_date (struct adftool_term *term, const struct timespec *nsec)
{
  struct tm date;
  gmtime_r (&(nsec->tv_sec), &date);
  static const char *type = "http://www.w3.org/2001/XMLSchema#dateTime";
  char easy[64];
  size_t required = date_str (easy, sizeof (easy), &date, nsec->tv_nsec);
  if (required >= sizeof (easy))
    {
      char *full = malloc (required + 1);
      if (full == NULL)
	{
	  abort ();
	}
      date_str (full, required + 1, &date, nsec->tv_nsec);
      adftool_term_set_literal (term, full, type, NULL);
      free (full);
    }
  else
    {
      adftool_term_set_literal (term, easy, type, NULL);
    }
}

int
adftool_term_as_date (const struct adftool_term *term, struct timespec *nsec)
{
  struct tm date;
  long int nano = 0;
  memset (&date, 0, sizeof (date));
  if (!adftool_term_is_typed_literal (term))
    {
      return 1;
    }
  if (strcmp (term->str2, "http://www.w3.org/2001/XMLSchema#dateTime") != 0)
    {
      return 1;
    }
  char *cur = term->str1;
  char *end;
  date.tm_year = strtol (cur, &end, 10) - 1900;
  if (end == cur || *end != '-')
    {
      return 1;
    }
  cur = end + 1;
  date.tm_mon = strtol (cur, &end, 10) - 1;
  if (end == cur || *end != '-')
    {
      return 1;
    }
  cur = end + 1;
  date.tm_mday = strtol (cur, &end, 10);
  if (end == cur || *end != 'T')
    {
      return 1;
    }
  cur = end + 1;
  date.tm_hour = strtol (cur, &end, 10);
  if (end == cur || *end != ':')
    {
      return 1;
    }
  cur = end + 1;
  date.tm_min = strtol (cur, &end, 10);
  if (end == cur || *end != ':')
    {
      return 1;
    }
  cur = end + 1;
  date.tm_sec = strtol (cur, &end, 10);
  if (end == cur)
    {
      return 1;
    }
  cur = end;
  if (*cur == '.')
    {
      cur++;
      char digits[] = "000000000";
      for (size_t i = 0; i < sizeof (digits); i++)
	{
	  if (*cur != '\0' && *cur != 'Z' && *cur != '+' && *cur != '-')
	    {
	      if (*cur >= '0' && *cur <= '9')
		{
		  digits[i] = *cur;
		}
	      else
		{
		  return 1;
		}
	      cur++;
	    }
	}
      nano = strtol (digits, NULL, 10);
    }
  if (*cur == 'Z')
    {
      end = cur + 1;
    }
  else if (*cur == '\0')
    {
      end = cur;
    }
  else
    {
      /* Parse the timezone */
      date.tm_hour -= strtol (cur, &end, 10);
      if (end == cur || *end != ':')
	{
	  return 1;
	}
      cur = end + 1;
      date.tm_min -= strtol (cur, &end, 10);
      if (end == cur)
	{
	  return 1;
	}
    }
  if (*end != '\0')
    {
      return 1;
    }
  time_t seconds = timegm (&date);
  if (seconds == ((time_t) (-1)))
    {
      return 1;
    }
  nsec->tv_sec = seconds;
  nsec->tv_nsec = nano;
  return 0;
}
