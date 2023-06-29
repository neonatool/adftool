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
#include <unistd.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "libadftool/file.h"
#include "libadftool/dictionary_index.h"
#include "libadftool/term.h"

#define CHECK_PACK(value, meta, flags) \
  ((((((uint64_t) (value)) << 31) | ((uint64_t) (meta))) << 2) | ((uint64_t) (flags)))

#define EMPTY_TERM 0x7FFFFFFF

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  remove ("encoding_unit_test.adf");
  struct adftool_file *file = adftool_file_open ("encoding_unit_test.adf", 1);
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
  static const size_t n_terms = sizeof (all_terms) / sizeof (all_terms[0]);
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
  static const char *dict_terms[] = {
    "hello", "world", "0", ">", "my-type", "en", "zz",
    "http://www.w3.org/2001/XMLSchema#string", "\"", "A"
  };
  uint32_t dict_term_ids[10];
  assert (sizeof (dict_term_ids) / sizeof (dict_term_ids[0]) ==
	  sizeof (dict_terms) / sizeof (dict_terms[0]));
  struct adftool_dictionary_index *dico = file->dictionary;
  for (size_t i = 0; i < sizeof (dict_terms) / sizeof (dict_terms[0]); i++)
    {
      int found;
      int error = adftool_dictionary_index_find (dico, strlen (dict_terms[i]),
						 dict_terms[i], true, &found,
						 &(dict_term_ids[i]));
      if (error || !found)
	{
	  fprintf (stderr, _("The dictionary does not know %s.\n"),
		   dict_terms[i]);
	  goto failure;
	}
      if (dict_term_ids[i] != i)
	{
	  fprintf (stderr,
		   _("The dictionary put %s in position %u, not %lu.\n"),
		   dict_terms[i], dict_term_ids[i], i);
	  goto failure;
	}
    }
  static const uint64_t encoded_forms[] = {
    CHECK_PACK (0, EMPTY_TERM, 0),	/* _:hello */
    CHECK_PACK (1, EMPTY_TERM, 0),	/* _:world */
    CHECK_PACK (0, EMPTY_TERM, 1),	/* <hello> */
    CHECK_PACK (1, EMPTY_TERM, 1),	/* <world> */
    CHECK_PACK (2, EMPTY_TERM, 1),	/* <0> */
    CHECK_PACK (3, EMPTY_TERM, 1),	/* <%3E> */
    CHECK_PACK (0, 7, 2),	/* "hello" */
    CHECK_PACK (1, 4, 2),	/* "world"^^<my-type> */
    CHECK_PACK (0, 2, 2),	/* "hello"^^<0> */
    CHECK_PACK (0, 3, 2),	/* "hello"^^<%3E> */
    CHECK_PACK (0, 5, 3),	/* "hello"@en */
    CHECK_PACK (1, 6, 3),	/* "world"@zz */
    CHECK_PACK (8, 6, 3),	/* "\""@zz */
    CHECK_PACK (9, 6, 3)	/* "A"@zz */
  };
  assert (sizeof (encoded_forms) / sizeof (encoded_forms[0]) == n_terms);
  for (size_t i = 0; i < n_terms; i++)
    {
      uint64_t encoded;
      struct adftool_term *term = adftool_term_alloc ();
      if (term == NULL)
	{
	  fprintf (stderr, _("Not enough memory to allocate a term.\n"));
	  goto term_failure;
	}
      if (term_encode (dico, all_terms[i], &encoded) != 0)
	{
	  fprintf (stderr, _("Failed to encode term %lu.\n"), i);
	  goto term_failure;
	}
      if (encoded != encoded_forms[i])
	{
	  fprintf (stderr, _("Encoding term %lu gives %016lx, not %016lx.\n"),
		   i, encoded, encoded_forms[i]);
	  goto term_failure;
	}
      if (term_decode (dico, encoded_forms[i], term) != 0)
	{
	  fprintf (stderr, _("Failed to decode term %lu (%016lx).\n"), i,
		   encoded_forms[i]);
	  goto term_failure;
	}
      if (adftool_term_compare (all_terms[i], term) != 0)
	{
	  fprintf (stderr, _("Term %lu is not decoded properly (%016lx).\n"),
		   i, encoded_forms[i]);
	  goto term_failure;
	}
      adftool_term_free (term);
      continue;
    term_failure:
      adftool_term_free (term);
      goto failure;
    }
  for (size_t i = 0; i < n_terms; i++)
    {
      adftool_term_free (all_terms[i]);
    }
  adftool_file_close (file);
  return 0;
failure:
  fprintf (stderr,
	   _("The test failed, keeping encoding_unit_test.adf around.\n"));
  adftool_file_close (file);
  return 1;
}
