#ifndef H_ADFTOOL_CHANNEL_DECODER_INCLUDED
# define H_ADFTOOL_CHANNEL_DECODER_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

# include "term.h"
# include "statement.h"
# include "file.h"

# include <stdlib.h>
# include <assert.h>
# include <string.h>
# include <locale.h>
# include <stdbool.h>

# include "gettext.h"

# ifdef BUILDING_LIBADFTOOL
#  define _(String) dgettext (PACKAGE, (String))
#  define N_(String) (String)
# else
#  define _(String) gettext (String)
#  define N_(String) (String)
# endif

static inline int channel_decoder_get (struct adftool_file *file,
				       const struct adftool_term *identifier,
				       double *scale, double *offset);

static inline int channel_decoder_set (struct adftool_file *file,
				       const struct adftool_term *identifier,
				       double scale, double offset);

static int
channel_decoder_find (struct adftool_file *file,
		      const struct adftool_term *identifier,
		      const char *scale_or_offset, double *value)
{
  char predicate_str[256];
  sprintf (predicate_str,
	   LYTONEPAL_ONTOLOGY_PREFIX "has-channel-decoder-%s",
	   scale_or_offset);
  struct adftool_term *object = term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  size_t n_results =
    adftool_lookup_objects (file, identifier, predicate_str, 0, 1, &object);
  if (n_results > 0)
    {
      int error = term_as_double (object, value);
      if (error)
	{
	  n_results = 0;
	}
    }
  term_free (object);
  return (n_results == 0);
}

static int
channel_decoder_replace (struct adftool_file *file,
			 const struct adftool_term *identifier,
			 const char *scale_or_offset,
			 const struct adftool_term *replacement)
{
  int error = 0;
  char predicate_str[256];
  sprintf (predicate_str,
	   LYTONEPAL_ONTOLOGY_PREFIX "has-channel-decoder-%s",
	   scale_or_offset);
  struct adftool_term p = {
    .type = TERM_NAMED,
    .str1 = predicate_str,
    .str2 = NULL
  };
  struct adftool_statement pattern = {
    .subject = (struct adftool_term *) identifier,
    .predicate = &p,
    .object = NULL,
    .graph = NULL,
    .deletion_date = ((uint64_t) (-1))
  };
  if (adftool_file_delete (file, &pattern, time (NULL) * 1000) != 0)
    {
      error = 1;
      goto cleanup;
    }
  pattern.object = (struct adftool_term *) replacement;
  if (adftool_insert (file, &pattern) != 0)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  return error;
}

static inline int
channel_decoder_get (struct adftool_file *file,
		     const struct adftool_term *identifier,
		     double *scale, double *offset)
{
  int scale_error =
    channel_decoder_find ((struct adftool_file *) file, identifier, "scale",
			  scale);
  int offset_error =
    channel_decoder_find ((struct adftool_file *) file, identifier, "offset",
			  offset);
  if (scale_error || offset_error)
    {
      return 1;
    }
  return 0;
}

static inline int
channel_decoder_set (struct adftool_file *file,
		     const struct adftool_term *identifier,
		     double scale, double offset)
{
  int error = 0;
  struct adftool_term *literal_scale = term_alloc ();
  struct adftool_term *literal_offset = term_alloc ();
  if (literal_scale == NULL || literal_offset == NULL)
    {
      if (literal_scale)
	{
	  term_free (literal_scale);
	}
      if (literal_offset)
	{
	  term_free (literal_offset);
	}
      return 1;
    }
  term_set_double (literal_scale, scale);
  term_set_double (literal_offset, offset);
  int scale_error =
    channel_decoder_replace (file, identifier, "scale", literal_scale);
  int offset_error =
    channel_decoder_replace (file, identifier, "offset", literal_offset);
  if (scale_error || offset_error)
    {
      error = 1;
      goto wrapup;
    }
wrapup:
  term_free (literal_scale);
  term_free (literal_offset);
  return error;
}

#endif /* not H_ADFTOOL_CHANNEL_DECODER_INCLUDED */
