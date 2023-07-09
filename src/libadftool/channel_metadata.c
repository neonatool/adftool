#include <config.h>
#include <attribute.h>
#include <adftool.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include <unistd.h>

#include "term.h"
#include "file.h"
#include "channel_decoder.h"

int
adftool_find_channel_identifier (struct adftool_file *file,
				 size_t channel_index,
				 struct adftool_term *identifier)
{
  mpz_t i;
  mpz_init_set_ui (i, channel_index);
  struct adftool_term *object = term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  term_set_mpz (object, i);
  mpz_clear (i);
  static const char *p = LYTONEPAL_ONTOLOGY_PREFIX "column-number";
  size_t n_results =
    adftool_lookup_subjects (file, object, p, 0, 1, &identifier);
  term_free (object);
  if (n_results == 0)
    {
      return 1;
    }
  return 0;
}

int
adftool_get_channel_decoder (struct adftool_file *file,
			     const struct adftool_term *identifier,
			     double *scale, double *offset)
{
  return channel_decoder_get (file, identifier, scale, offset);
}

int
adftool_set_channel_decoder (struct adftool_file *file,
			     const struct adftool_term *identifier,
			     double scale, double offset)
{
  return channel_decoder_set (file, identifier, scale, offset);
}

int
adftool_get_channel_column (struct adftool_file *file,
			    const struct adftool_term *identifier,
			    size_t *column)
{
  struct adftool_term *object = term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  static const char *p = LYTONEPAL_ONTOLOGY_PREFIX "column-number";
  size_t n_results =
    adftool_lookup_objects (file, identifier, p, 0, 1, &object);
  mpz_t i;
  mpz_init (i);
  if (n_results > 0)
    {
      if (term_as_mpz (object, i) != 0)
	{
	  n_results = 0;
	}
    }
  if (n_results > 0)
    {
      *column = mpz_get_ui (i);
    }
  mpz_clear (i);
  term_free (object);
  return n_results == 0;
}

int
adftool_add_channel_type (struct adftool_file *file,
			  const struct adftool_term *channel,
			  const struct adftool_term *type)
{
  static const char *const rdf_type =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
  static const struct adftool_term predicate = {
    .type = TERM_NAMED,
    .str1 = (char *) rdf_type,
    .str2 = NULL
  };
  const struct adftool_statement statement = {
    .subject = (struct adftool_term *) channel,
    .predicate = (struct adftool_term *) &predicate,
    .object = (struct adftool_term *) type,
    .graph = NULL,
    .deletion_date = ((uint64_t) (-1))
  };
  int error = adftool_file_insert (file, &statement);
  return error;
}

size_t
adftool_get_channel_types (struct adftool_file *file,
			   const struct adftool_term *channel, size_t start,
			   size_t max, struct adftool_term **types)
{
  static const char *const rdf_type =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
  return adftool_lookup_objects (file, channel, rdf_type, start, max, types);
}

size_t
adftool_find_channels_by_type (struct adftool_file *file,
			       const struct adftool_term *type, size_t start,
			       size_t max, struct adftool_term **channels)
{
  static const char *const rdf_type =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
  return adftool_lookup_subjects (file, type, rdf_type, start, max, channels);
}
