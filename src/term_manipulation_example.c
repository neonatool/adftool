#include <config.h>

#include <attribute.h>
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
  /* Named nodes: <hello>, <world>, <0>, <%3E> */
  /* Typed literals: "hello"^^<xsd:string>, "world"^^<my-type>, "hello"^^<0>, "hello"^^<%3E> */
  /* Lang strings: "hello"@en, "world"@zz, "\""@zz, "A"@zz */
  struct adftool_term *all_terms[14];
  static const int types[][5] = {
    /* Blank? Named? Literal? Typed? Langstring? */
    {1, 0, 0, 0, 0},		/* _:hello */
    {1, 0, 0, 0, 0},		/* _:world */
    {0, 1, 0, 0, 0},		/* <hello> */
    {0, 1, 0, 0, 0},		/* <world> */
    {0, 1, 0, 0, 0},		/* <0> */
    {0, 1, 0, 0, 0},		/* <%3E> */
    {0, 0, 1, 1, 0},		/* "hello"^^<xsd:string> */
    {0, 0, 1, 1, 0},		/* "world"^^<my-type> */
    {0, 0, 1, 1, 0},		/* "hello"^^<0> */
    {0, 0, 1, 1, 0},		/* "hello"^^<%3E> */
    {0, 0, 1, 0, 1},		/* "hello"@en */
    {0, 0, 1, 0, 1},		/* "world"@zz */
    {0, 0, 1, 0, 1},		/* "\""@zz */
    {0, 0, 1, 0, 1},		/* "A"@zz */
  };
  static const char *expected_value[] = {
    "hello", "world",		/* blanks */
    "hello", "world", "0", ">",	/* URIs */
    "hello", "world", "hello", "hello",	/* typed */
    "hello", "world", "\"", "A"	/* langstrings */
  };
  static const char *expected_meta[] = {
    "", "",			/* blanks */
    "", "", "", "",		/* URIs */
    "http://www.w3.org/2001/XMLSchema#string", "my-type", "0", ">",
    "en", "zz", "zz", "zz"
  };
  static const char *encoded[] = {
    "_:hello",
    "_:world",
    "<hello>",
    "<world>",
    "<0>",
    "<%3E>",
    "\"hello\"^^<http://www.w3.org/2001/XMLSchema#string>",
    "\"world\"^^<my-type>",
    "\"hello\"^^<0>",
    "\"hello\"^^<%3E>",
    "\"hello\"@en",
    "\"world\"@zz",
    "\"\\\"\"@zz",
    "\"A\"@zz"
  };
  static const size_t n_terms = sizeof (all_terms) / sizeof (all_terms[0]);
  assert (sizeof (types) / sizeof (types[0]) == n_terms);
  assert (sizeof (expected_value) / sizeof (expected_value[0]) == n_terms);
  assert (sizeof (encoded) / sizeof (encoded[0]) == n_terms);
  for (size_t i = 0; i < n_terms; i++)
    {
      all_terms[i] = adftool_term_alloc ();
    }
  adftool_term_set_blank (all_terms[0], "hello");
  adftool_term_set_blank (all_terms[1], "world");
  adftool_term_set_named (all_terms[2], "hello");
  adftool_term_set_named (all_terms[3], "world");
  adftool_term_set_named (all_terms[4], "0");
  adftool_term_set_named (all_terms[5], ">");
  adftool_term_set_literal (all_terms[6], "hello", NULL, NULL);
  adftool_term_set_literal (all_terms[7], "world", "my-type", NULL);
  adftool_term_set_literal (all_terms[8], "hello", "0", NULL);
  adftool_term_set_literal (all_terms[9], "hello", ">", NULL);
  adftool_term_set_literal (all_terms[10], "hello", NULL, "en");
  adftool_term_set_literal (all_terms[11], "world", NULL, "zz");
  adftool_term_set_literal (all_terms[12], "\"", NULL, "zz");
  adftool_term_set_literal (all_terms[13], "A", NULL, "zz");
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
      char n3_form[256];
      size_t n3_length =
	adftool_term_to_n3 (all_terms[i], 0, sizeof (n3_form), n3_form);
      if (n3_length >= sizeof (n3_form))
	{
	  fprintf (stderr,
		   _("Term %lu cannot be encoded to N3 "
		     "in less than 256 characters (%lu required).\n"), i,
		   n3_length);
	  goto failure;
	}
      if (STRNEQ (n3_form, encoded[i]))
	{
	  fprintf (stderr,
		   _("Term %lu N3 encoding is not correct: "
		     "%s vs expected %s.\n"), i, n3_form, encoded[i]);
	  goto failure;
	}
      char buffer[40];
      static const size_t max = sizeof (buffer) / sizeof (buffer[0]);
      size_t n_used = adftool_term_value (all_terms[i], 0, max, buffer);
      if (n_used != strlen (expected_value[i])
	  || STRNEQ (buffer, expected_value[i]))
	{
	  fprintf (stderr,
		   _("Value for number %lu (%s) is %s, should be %s.\n"), i,
		   n3_form, buffer, expected_value[i]);
	  goto failure;
	}
      n_used = adftool_term_meta (all_terms[i], 0, max, buffer);
      if (n_used != strlen (expected_meta[i])
	  || STRNEQ (buffer, expected_meta[i]))
	{
	  fprintf (stderr,
		   _("Meta for number %lu (%s) is %s, should be %s.\n"), i,
		   n3_form, buffer, expected_meta[i]);
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
	  int expected_cmp = strcmp (encoded[i], encoded[j]);
	  if (expected_cmp < 0)
	    {
	      expected_cmp = -1;
	    }
	  if (expected_cmp > 0)
	    {
	      expected_cmp = 1;
	    }
	  if (cmp != expected_cmp)
	    {
	      fprintf (stderr, _("Comparison between %lu (%s) and %lu (%s) \
should be %d, it is %d.\n"), i, encoded[i], j, encoded[j], expected_cmp, cmp);
	      goto failure;
	    }
	}
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
