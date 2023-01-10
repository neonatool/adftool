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

static struct adftool_statement *
build_statement (const char **quad)
{
  struct adftool_statement *statement = adftool_statement_alloc ();
  if (statement == NULL)
    {
      abort ();
    }
  struct adftool_term *terms[4];
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      if (quad[i] == NULL)
	{
	  terms[i] = NULL;
	}
      else
	{
	  terms[i] = adftool_term_alloc ();
	  if (terms[i] == NULL)
	    {
	      abort ();
	    }
	  adftool_term_set_named (terms[i], quad[i]);
	}
    }
  adftool_statement_set (statement, &(terms[0]), &(terms[1]), &(terms[2]),
			 &(terms[3]), NULL);
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      /* It even works when NULL */
      adftool_term_free (terms[i]);
    }
  return statement;
}

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  remove ("multi_statement_example.adf");
  struct adftool_file *file =
    adftool_file_open ("multi_statement_example.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  /* For simplicity, only URIs here. */
  static const char *statements[][4] = {
    {"a", "b", "c", NULL},
    {"d", "e", "f", NULL},
    {"g", "h", "i", NULL},
    {"j", "k", "l", NULL},
    {"a", "b", "c", "a"},
    {"d", "e", "f", "a"},
    {"g", "h", "i", "a"},
    {"j", "k", "l", "a"},
    {"a", "b", "c", "x"},
    {"d", "e", "f", "x"},
    {"g", "h", "i", "x"},
    {"j", "k", "l", "x"}
  };
  static const size_t n_statements =
    sizeof (statements) / sizeof (statements[0]);
  for (size_t i = 0; i < n_statements; i++)
    {
      struct adftool_statement *statement = build_statement (statements[i]);
      if (adftool_insert (file, statement) != 0)
	{
	  abort ();
	}
      adftool_statement_free (statement);
    }
  /* Queries: */
  /* a ? ? ? -> (a, b, c), (a, b, c, a), (a, b, c, x) */
  /* ? e ? ? -> (d, e, f), (d, e, f, a), (d, e, f, x) */
  /* ? ? i ? -> (g, h, i), (g, h, i, a), (g, h, i, x) */
  /* ? ? ? a -> (a, b, c, a), (d, e, f, a), … */
  static const char *requests[][4] = {
    {"a", NULL, NULL, NULL},
    {NULL, "e", NULL, NULL},
    {NULL, NULL, "i", NULL},
    {NULL, NULL, NULL, "a"}
  };
  static const size_t n_requests = sizeof (requests) / sizeof (requests[0]);
  struct adftool_statement **results[4];
  size_t n_results[4];
  assert (sizeof (results) / sizeof (results[0]) == n_requests);
  assert (sizeof (n_results) / sizeof (n_results[0]) == n_requests);
  for (size_t i = 0; i < n_requests; i++)
    {
      struct adftool_statement *pattern = build_statement (requests[i]);
      if (adftool_lookup (file, pattern, 0, 0, &(n_results[i]), NULL) != 0)
	{
	  abort ();
	}
      results[i] =
	malloc (n_results[i] * sizeof (struct adftool_statement *));
      if (results[i] == NULL)
	{
	  abort ();
	}
      for (size_t j = 0; j < n_results[i]; j++)
	{
	  results[i][j] = adftool_statement_alloc ();
	  if (results[i][j] == NULL)
	    {
	      abort ();
	    }
	}
      size_t check;
      if (adftool_lookup (file, pattern, 0, n_results[i], &check, results[i])
	  != 0)
	{
	  abort ();
	}
      if (n_results[i] != check)
	{
	  abort ();
	}
      adftool_statement_free (pattern);
    }
  static const char *expected_results_1[][4] = {
    {"a", "b", "c", NULL},
    {"a", "b", "c", "a"},
    {"a", "b", "c", "x"}
  };
  static const char *expected_results_2[][4] = {
    {"d", "e", "f", NULL},
    {"d", "e", "f", "a"},
    {"d", "e", "f", "x"}
  };
  static const char *expected_results_3[][4] = {
    {"g", "h", "i", NULL},
    {"g", "h", "i", "a"},
    {"g", "h", "i", "x"}
  };
  static const char *expected_results_4[][4] = {
    {"a", "b", "c", "a"},
    {"d", "e", "f", "a"},
    {"g", "h", "i", "a"},
    {"j", "k", "l", "a"}
  };
  for (size_t i = 0; i < n_requests; i++)
    {
      size_t n_expected_results;
      struct adftool_statement **expected_statements;
      switch (i)
	{
	case 0:
	  n_expected_results =
	    sizeof (expected_results_1) / sizeof (expected_results_1[0]);
	  break;
	case 1:
	  n_expected_results =
	    sizeof (expected_results_2) / sizeof (expected_results_2[0]);
	  break;
	case 2:
	  n_expected_results =
	    sizeof (expected_results_3) / sizeof (expected_results_2[0]);
	  break;
	case 3:
	  n_expected_results =
	    sizeof (expected_results_4) / sizeof (expected_results_2[0]);
	  break;
	default:
	  abort ();
	}
      expected_statements =
	malloc (n_expected_results * sizeof (struct adftool_statement *));
      if (expected_statements == NULL)
	{
	  abort ();
	}
      for (size_t j = 0; j < n_expected_results; j++)
	{
	  switch (i)
	    {
	    case 0:
	      expected_statements[j] =
		build_statement (expected_results_1[j]);
	      break;
	    case 1:
	      expected_statements[j] =
		build_statement (expected_results_2[j]);
	      break;
	    case 2:
	      expected_statements[j] =
		build_statement (expected_results_3[j]);
	      break;
	    case 3:
	      expected_statements[j] =
		build_statement (expected_results_4[j]);
	      break;
	    default:
	      abort ();
	    }
	}
      if (n_expected_results != n_results[i])
	{
	  abort ();
	}
      for (size_t j = 0; j < n_expected_results; j++)
	{
	  /* The statements are equal, for whatever comparison
	     order. */
	  static const char *compare_orders[] = {
	    "GSPO", "GPOS", "GOSP",
	    "SPOG", "POSG", "OSPG"
	  };
	  static const size_t n_indices =
	    sizeof (compare_orders) / sizeof (compare_orders[0]);
	  for (size_t k = 0; k < n_indices; k++)
	    {
	      if (adftool_statement_compare
		  (expected_statements[j], results[i][j],
		   compare_orders[k]) != 0)
		{
		  abort ();
		}
	    }
	}
      for (size_t j = 0; j < n_expected_results; j++)
	{
	  adftool_statement_free (expected_statements[j]);
	}
      free (expected_statements);
    }
  for (size_t i = 0; i < n_requests; i++)
    {
      for (size_t j = 0; j < n_results[i]; j++)
	{
	  adftool_statement_free (results[i][j]);
	}
      free (results[i]);
    }
  adftool_file_close (file);
  return 0;
}
