#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

struct test
{
  const char *text;
  int can_be_accepted;
  size_t consumed;
  int accepted_as_blank;
  int accepted_as_named;
  int accepted_as_literal;
  const char *expected_literal_value;
  const char *expected_type;
  const char *expected_langtag;
};

static void
check_test_aux (const char *file, int line,
		const char *text, int can_be_accepted,
		size_t expected_consumed, int accepted_as_blank,
		int accepted_as_named, int accepted_as_literal,
		const char *expected_literal_value, const char *expected_type,
		const char *expected_langtag)
{
  struct adftool_term *term = adftool_term_alloc ();
  if (term == NULL)
    {
      abort ();
    }
  size_t consumed;
  int parsing_error =
    adftool_term_parse_n3 (text, strlen (text), &consumed, term);
  if (can_be_accepted && parsing_error)
    {
      fprintf (stderr, "%s:%d: `%s` should be accepted, but it is not.\n",
	       file, line, text);
      abort ();
    }
  if (!can_be_accepted && parsing_error == 0)
    {
      fprintf (stderr, "%s:%d: `%s` should not be accepted, but it is.\n",
	       file, line, text);
      abort ();
    }
  if (consumed != expected_consumed)
    {
      fprintf (stderr, "%s:%d: `%s` should consume %lu bytes, \
but it consumes %lu.\n", file, line, text, expected_consumed, consumed);
      abort ();
    }
  if (can_be_accepted)
    {
      if (accepted_as_blank && !adftool_term_is_blank (term))
	{
	  fprintf (stderr, "%s:%d: `%s` should be a blank node, \
but it is not.\n", file, line, text);
	  abort ();
	}
      if (!accepted_as_blank && adftool_term_is_blank (term))
	{
	  fprintf (stderr, "%s:%d: `%s` should not be a blank node, \
but it is.\n", file, line, text);
	  abort ();
	}
      if (accepted_as_named && !adftool_term_is_named (term))
	{
	  fprintf (stderr, "%s:%d: `%s` should be a named node, \
but it is not.\n", file, line, text);
	  abort ();
	}
      if (!accepted_as_named && adftool_term_is_named (term))
	{
	  fprintf (stderr, "%s:%d: `%s` should not be a named node, \
but it is.\n", file, line, text);
	  abort ();
	}
      if ((accepted_as_literal
	   && expected_type != NULL) && !adftool_term_is_typed_literal (term))
	{
	  fprintf (stderr, "%s:%d: `%s` should be a typed literal, \
but it is not.\n", file, line, text);
	  abort ();
	}
      if (!(accepted_as_literal
	    && expected_type != NULL) && adftool_term_is_typed_literal (term))
	{
	  fprintf (stderr, "%s:%d: `%s` should not be a typed literal, \
but it is.\n", file, line, text);
	  abort ();
	}
      if ((accepted_as_literal
	   && expected_langtag != NULL) && !adftool_term_is_langstring (term))
	{
	  fprintf (stderr, "%s:%d: `%s` should be a langstring, \
but it is not.\n", file, line, text);
	  abort ();
	}
      if (!(accepted_as_literal
	    && expected_langtag != NULL) && adftool_term_is_langstring (term))
	{
	  fprintf (stderr, "%s:%d: `%s` should not be a langstring, \
but it is.\n", file, line, text);
	  abort ();
	}
      size_t str_length, str_length_check;
      char *buffer;
      str_length = adftool_term_value (term, 0, 0, NULL);
      buffer = malloc (str_length + 1);
      if (buffer == NULL)
	{
	  abort ();
	}
      str_length_check = adftool_term_value (term, 0, str_length + 1, buffer);
      assert (str_length == str_length_check);
      if (strcmp (expected_literal_value, buffer) != 0)
	{
	  fprintf (stderr, "%s:%d: `%s` should have `%s` as literal value, \
but it is `%s`.\n", file, line, text, expected_literal_value, buffer);
	  abort ();
	}
      free (buffer);
      if (expected_type != NULL || expected_langtag != NULL)
	{
	  assert (expected_type == NULL || expected_langtag == NULL);
	  str_length = adftool_term_meta (term, 0, 0, NULL);
	  buffer = malloc (str_length + 1);
	  if (buffer == NULL)
	    {
	      abort ();
	    }
	  str_length_check =
	    adftool_term_meta (term, 0, str_length + 1, buffer);
	  assert (str_length == str_length_check);
	  const char *source = expected_type;
	  if (source == NULL)
	    {
	      source = expected_langtag;
	    }
	  if (strcmp (source, buffer) != 0)
	    {
	      fprintf (stderr, "%s:%d: `%s` should have `%s` as \
literal value metadata, but it is `%s`.\n", file, line, text, source, buffer);
	      abort ();
	    }
	  free (buffer);
	}
    }
  adftool_term_free (term);
}

static void
check_test (const char *file, int line, const struct test *test)
{
  check_test_aux (file, line, test->text, test->can_be_accepted,
		  test->consumed, test->accepted_as_blank,
		  test->accepted_as_named, test->accepted_as_literal,
		  test->expected_literal_value, test->expected_type,
		  test->expected_langtag);
}

#define CHECK_TEST(a, b, c, d, e, f, g, h, i)           \
  {                                                     \
    static const struct test __my_test =                \
      {                                                 \
        a, b, c, d, e, f, g, h, i                       \
      };                                                \
    check_test (__FILE__, __LINE__, &__my_test);        \
  }

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  /* Parsing normally in any case. */
  CHECK_TEST ("<a>", 1, 3, 0, 1, 0, "a", NULL, NULL);
  CHECK_TEST ("_:b ", 1, 3, 1, 0, 0, "b", NULL, NULL);
  CHECK_TEST ("\"c\" . ", 1, 3, 0, 0, 1, "c",
	      "http://www.w3.org/2001/XMLSchema#string", NULL);
  CHECK_TEST ("\"c'\"^ . ", 1, 4, 0, 0, 1, "c'",
	      "http://www.w3.org/2001/XMLSchema#string", NULL);
  CHECK_TEST ("\"c\\\"\"^^ . ", 1, 5, 0, 0, 1, "c\"",
	      "http://www.w3.org/2001/XMLSchema#string", NULL);
  CHECK_TEST ("\"d\"^^<my-string>", 1, 16, 0, 0, 1, "d", "my-string", NULL);
  CHECK_TEST ("\"e\"@lang ", 1, 8, 0, 0, 1, "e", NULL, "lang");
  CHECK_TEST ("true", 1, 4, 0, 0, 1, "true",
	      "http://www.w3.org/2001/XMLSchema#boolean", NULL);
  CHECK_TEST ("false", 1, 5, 0, 0, 1, "false",
	      "http://www.w3.org/2001/XMLSchema#boolean", NULL);
  CHECK_TEST ("1234 ", 1, 4, 0, 0, 1, "1234",
	      "http://www.w3.org/2001/XMLSchema#integer", NULL);
  CHECK_TEST ("1234.5 ", 1, 6, 0, 0, 1, "1234.5",
	      "http://www.w3.org/2001/XMLSchema#decimal", NULL);
  CHECK_TEST ("1234.5e6 ", 1, 8, 0, 0, 1, "1234.5e6",
	      "http://www.w3.org/2001/XMLSchema#double", NULL);

  /* If more data is available, the parsing might change. */
  CHECK_TEST ("_:f", 1, 3, 1, 0, 0, "f", NULL, NULL);
  CHECK_TEST ("\"g\"", 1, 3, 0, 0, 1, "g",
	      "http://www.w3.org/2001/XMLSchema#string", NULL);
  /* Notice that if there won’t be more data, the `my-string' part is ignored. */
  CHECK_TEST ("\"h\"^^<my-string", 1, 3, 0, 0, 1, "h",
	      "http://www.w3.org/2001/XMLSchema#string", NULL);
  CHECK_TEST ("\"h'\"^^", 1, 4, 0, 0, 1, "h'",
	      "http://www.w3.org/2001/XMLSchema#string", NULL);
  CHECK_TEST ("\"h\\\"\"^^", 1, 5, 0, 0, 1, "h\"",
	      "http://www.w3.org/2001/XMLSchema#string", NULL);
  CHECK_TEST ("\"i\"@lang", 1, 8, 0, 0, 1, "i", NULL, "lang");
  CHECK_TEST ("1234", 1, 4, 0, 0, 1, "1234",
	      "http://www.w3.org/2001/XMLSchema#integer", NULL);
  CHECK_TEST ("1234.5", 1, 6, 0, 0, 1, "1234.5",
	      "http://www.w3.org/2001/XMLSchema#decimal", NULL);
  CHECK_TEST ("1234.5e6", 1, 8, 0, 0, 1, "1234.5e6",
	      "http://www.w3.org/2001/XMLSchema#double", NULL);

  /* The parsing is incomplete. */
  CHECK_TEST (" ", 0, 0, 0, 0, 0, NULL, NULL, NULL);
  CHECK_TEST ("<j", 0, 0, 0, 0, 0, NULL, NULL, NULL);
  CHECK_TEST ("_", 0, 0, 0, 0, 0, NULL, NULL, NULL);
  CHECK_TEST ("_:", 0, 0, 0, 0, 0, NULL, NULL, NULL);
  CHECK_TEST ("\"", 0, 0, 0, 0, 0, NULL, NULL, NULL);
  CHECK_TEST ("\"k", 0, 0, 0, 0, 0, NULL, NULL, NULL);
  CHECK_TEST ("tru", 0, 0, 0, 0, 0, NULL, NULL, NULL);
  return 0;
}
