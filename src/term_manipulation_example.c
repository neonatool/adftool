#include <config.h>

#include <adftool.h>
#include <hdf5.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <assert.h>

#define _(String) gettext(String)
#define N_(String) (String)

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#define CHECK_PACK(value, meta, flags) \
  ((((((uint64_t) (value)) << 31) | ((uint64_t) (meta))) << 2) | ((uint64_t) (flags)))

#define EMPTY_TERM 0x7FFFFFFF

static void check_double_0 (void);

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  remove ("term_example.adf");
  struct adftool_file *file = adftool_file_open ("term_example.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  /* In this example, we are encoding the following terms: */
  /* Blank nodes: _:hello, _:world */
  /* Named nodes: <hello>, <world> */
  /* Typed literals: "hello"^^<xsd:string>, "world"^^<my-type> */
  /* Lang strings: "hello"@en, "world"@zz */
  struct adftool_term *all_terms[8];
  static const int types[][5] = {
    /* Blank? Named? Literal? Typed? Langstring? */
    {1, 0, 0, 0, 0},		/* _:hello */
    {1, 0, 0, 0, 0},		/* _:world */
    {0, 1, 0, 0, 0},		/* <hello> */
    {0, 1, 0, 0, 0},		/* <world> */
    {0, 0, 1, 1, 0},		/* "hello"^^<xsd:string> */
    {0, 0, 1, 1, 0},		/* "world"^^<my-type> */
    {0, 0, 1, 0, 1},		/* "hello"@en */
    {0, 0, 1, 0, 1}		/* "world"@zz */
  };
  static const char *expected_value[] = {
    "hello", "world",		/* blanks */
    "hello", "world",		/* URIs */
    "hello", "world",		/* typed */
    "hello", "world",		/* langstrings */
  };
  static const char *expected_meta[] = {
    "", "",			/* blanks */
    "", "",			/* URIs */
    "http://www.w3.org/2001/XMLSchema#string", "my-type",
    "en", "zz",
  };
  static const int compare_table[8][8] = {
    {0, -1, +1, +1, +1, +1, +1, +1},
    {+1, 0, +1, +1, +1, +1, +1, +1},
    {-1, -1, 0, -1, +1, +1, +1, +1},
    {-1, -1, +1, 0, +1, +1, +1, +1},
    {-1, -1, -1, -1, 0, -1, +1, -1},
    {-1, -1, -1, -1, +1, 0, +1, +1},
    {-1, -1, -1, -1, -1, -1, 0, -1},
    {-1, -1, -1, -1, +1, -1, +1, 0}
  };
  static const size_t n_terms = sizeof (all_terms) / sizeof (all_terms[0]);
  assert (sizeof (types) / sizeof (types[0]) == n_terms);
  assert (sizeof (expected_value) / sizeof (expected_value[0]) == n_terms);
  assert (sizeof (compare_table) / sizeof (compare_table[0]) == n_terms);
  assert (sizeof (compare_table[0]) / sizeof (compare_table[0][0]) ==
	  n_terms);
  for (size_t i = 0; i < n_terms; i++)
    {
      for (size_t j = 0; j < n_terms; j++)
	{
	  /* compare_table is antisymmetric */
	  assert (compare_table[i][j] + compare_table[j][i] == 0);
	}
    }
  for (size_t i = 0; i < n_terms; i++)
    {
      all_terms[i] = adftool_term_alloc ();
    }
  adftool_term_set_blank (all_terms[0], "hello");
  adftool_term_set_blank (all_terms[1], "world");
  adftool_term_set_named (all_terms[2], "hello");
  adftool_term_set_named (all_terms[3], "world");
  adftool_term_set_literal (all_terms[4], "hello", NULL, NULL);
  adftool_term_set_literal (all_terms[5], "world", "my-type", NULL);
  adftool_term_set_literal (all_terms[6], "hello", NULL, "en");
  adftool_term_set_literal (all_terms[7], "world", NULL, "zz");
  for (size_t i = 0; i < n_terms; i++)
    {
      int my_types[5];
      my_types[0] = adftool_term_is_blank (all_terms[i]);
      my_types[1] = adftool_term_is_named (all_terms[i]);
      my_types[2] = adftool_term_is_literal (all_terms[i]);
      my_types[3] = adftool_term_is_typed_literal (all_terms[i]);
      my_types[4] = adftool_term_is_langstring (all_terms[i]);
      assert (sizeof (my_types) / sizeof (my_types[0]) ==
	      sizeof (types[0]) / sizeof (types[0][0]));
      for (size_t j = 0; j < sizeof (my_types) / sizeof (my_types[0]); j++)
	{
	  if (types[i][j] != my_types[j])
	    {
	      fprintf (stderr,
		       _("Type %lu for number %lu is %d, should be %d.\n"), j,
		       i, my_types[j], types[i][j]);
	      goto failure;
	    }
	}
      char buffer[40];
      static const size_t max = sizeof (buffer) / sizeof (buffer[0]);
      size_t n_used = adftool_term_value (all_terms[i], 0, max, buffer);
      if (n_used != strlen (expected_value[i])
	  || STRNEQ (buffer, expected_value[i]))
	{
	  fprintf (stderr, _("Value for number %lu is %s, should be %s.\n"),
		   i, buffer, expected_value[i]);
	  goto failure;
	}
      n_used = adftool_term_meta (all_terms[i], 0, max, buffer);
      if (n_used != strlen (expected_meta[i])
	  || STRNEQ (buffer, expected_meta[i]))
	{
	  fprintf (stderr, _("Meta for number %lu is %s, should be %s.\n"),
		   i, buffer, expected_meta[i]);
	  goto failure;
	}
      for (size_t j = 0; j < n_terms; j++)
	{
	  int cmp = adftool_term_compare (all_terms[i], all_terms[j]);
	  if (cmp < 0)
	    {
	      cmp = -1;
	    }
	  if (cmp > 0)
	    {
	      cmp = +1;
	    }
	  if (cmp != compare_table[i][j])
	    {
	      fprintf (stderr, _("Comparison between %lu and %lu \
should be %d, it is %d.\n"), i, j, compare_table[i][j], cmp);
	      goto failure;
	    }
	}
    }
  static const char *dict_terms[] = {
    "hello", "world", "my-type", "en", "zz",
    "http://www.w3.org/2001/XMLSchema#string"
  };
  uint32_t dict_term_ids[6];
  assert (sizeof (dict_term_ids) / sizeof (dict_term_ids[0]) ==
	  sizeof (dict_terms) / sizeof (dict_terms[0]));
  for (size_t i = 0; i < sizeof (dict_terms) / sizeof (dict_terms[0]); i++)
    {
      int error = adftool_dictionary_insert (file, strlen (dict_terms[i]),
					     dict_terms[i],
					     &(dict_term_ids[i]));
      if (error)
	{
	  fprintf (stderr, _("Could not add %s to the dictionary.\n"),
		   dict_terms[i]);
	  goto failure;
	}
      assert (dict_term_ids[i] == i);
    }
  static const uint64_t encoded_forms[] = {
    CHECK_PACK (0, EMPTY_TERM, 0),	/* _:hello */
    CHECK_PACK (1, EMPTY_TERM, 0),	/* _:world */
    CHECK_PACK (0, EMPTY_TERM, 1),	/* <hello> */
    CHECK_PACK (1, EMPTY_TERM, 1),	/* <world> */
    CHECK_PACK (0, 5, 2),	/* "hello" */
    CHECK_PACK (1, 2, 2),	/* "world"^^<my-type> */
    CHECK_PACK (0, 3, 3),	/* "hello"@en */
    CHECK_PACK (1, 4, 3)	/* "world"@zz */
  };
  assert (sizeof (encoded_forms) / sizeof (encoded_forms[0]) == n_terms);
  for (size_t i = 0; i < n_terms; i++)
    {
      uint64_t encoded;
      struct adftool_term *term = adftool_term_alloc ();
      if (term == NULL)
	{
	  fprintf (stderr, _("Not enough memory to allocate a term.\n"));
	  goto failure;
	}
      if (adftool_term_encode (file, all_terms[i], &encoded) != 0)
	{
	  fprintf (stderr, _("Failed to encode term %lu.\n"), i);
	  goto failure;
	}
      if (encoded != encoded_forms[i])
	{
	  fprintf (stderr, _("Encoding term %lu gives %016lx, not %016lx.\n"),
		   i, encoded, encoded_forms[i]);
	  goto failure;
	}
      if (adftool_term_decode (file, encoded_forms[i], term) != 0)
	{
	  fprintf (stderr, _("Failed to decode term %lu (%016lx).\n"), i,
		   encoded_forms[i]);
	  goto failure;
	}
      int my_types[5];
      my_types[0] = adftool_term_is_blank (term);
      my_types[1] = adftool_term_is_named (term);
      my_types[2] = adftool_term_is_literal (term);
      my_types[3] = adftool_term_is_typed_literal (term);
      my_types[4] = adftool_term_is_langstring (term);
      assert (sizeof (my_types) / sizeof (my_types[0]) ==
	      sizeof (types[0]) / sizeof (types[0][0]));
      for (size_t j = 0; j < sizeof (my_types) / sizeof (my_types[0]); j++)
	{
	  if (types[i][j] != my_types[j])
	    {
	      fprintf (stderr, _("%s:%d: \
Type %lu for number %lu is %d, should be %d.\n"), __FILE__, __LINE__, j, i, my_types[j], types[i][j]);
	      goto failure;
	    }
	}
      char buffer[40];
      static const size_t max = sizeof (buffer) / sizeof (buffer[0]);
      size_t n_used = adftool_term_value (term, 0, max, buffer);
      if (n_used != strlen (expected_value[i])
	  || STRNEQ (buffer, expected_value[i]))
	{
	  fprintf (stderr, _("%s:%d: \
Value for number %lu is %s, should be %s.\n"), __FILE__, __LINE__, i, buffer, expected_value[i]);
	  goto failure;
	}
      n_used = adftool_term_meta (term, 0, max, buffer);
      if (n_used != strlen (expected_meta[i])
	  || STRNEQ (buffer, expected_meta[i]))
	{
	  fprintf (stderr, _("%s:%d: \
Meta for number %lu is %s, should be %s.\n"), __FILE__, __LINE__, i, buffer, expected_meta[i]);
	  goto failure;
	}
      adftool_term_free (term);
    }
  for (size_t i = 0; i < n_terms; i++)
    {
      adftool_term_free (all_terms[i]);
    }
  adftool_file_close (file);
  check_double_0 ();
  return 0;
failure:
  fprintf (stderr, _("The test failed, keeping term_example.adf around.\n"));
  adftool_file_close (file);
  return 1;
}

static void
check_double_0 (void)
{
  struct adftool_term *term = adftool_term_alloc ();
  if (term == NULL)
    {
      abort ();
    }
  adftool_term_set_double (term, 0);
  double zero = 42;
  if (adftool_term_as_double (term, &zero) != 0)
    {
      abort ();
    }
  if (zero != 0)
    {
      abort ();
    }
  adftool_term_free (term);
}
