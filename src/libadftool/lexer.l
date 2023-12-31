/* *INDENT-OFF* */
%top{
/* *INDENT-ON* */
#include <config.h>
#include <attribute.h>
#include <adftool.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "term.h"

#include <assert.h>

/* *INDENT-OFF* */
}

%{
/* *INDENT-ON* */
static char *unescape_string (const char *text, char **end);
static char *unescape_uri (const char *text, char **end);
static char *unescape_langtag (const char *text);
int yywrap (yyscan_t scanner);

#define YY_NO_INPUT
#define YY_NO_UNPUT

 /* The lexer returns 0 if it can’t find a new buffer to read, so we
    have to use the value 0 to mean error. So, we use 1 to indicate
    that the thing has been parsed, and 0 otherwise.  */
#define YY_DECL \
 static int yylex (yyscan_t *yyscanner,		\
		   struct adftool_term *term,	\
		   size_t *term_length)
#define YY_USER_ACTION { *term_length += yyleng; }
/* *INDENT-OFF* */
%}

%option reentrant
%option outfile="lex.yy.c"

URI "<"[^>\n]*">"
PN_CHARS_BASE [A-Z]|[a-z]|[\x80-\xff]
PN_CHARS_U {PN_CHARS_BASE}|"_"
PN_CHARS {PN_CHARS_U}|"-"|[0-9]
STRING "\""("\\\""|"\\\\"|"\\n"|"\\r"|"\\t"|[^\r\n\"\\])*"\""
EXPONENT [eE][+-]?[0-9]+

%%

{URI} {
/* *INDENT-ON* */
char *decoded_uri = unescape_uri (yytext, NULL);
if (decoded_uri == NULL)
  {
    abort ();
  }
adftool_term_set_named (term, decoded_uri);
free (decoded_uri);
return 1;
/* *INDENT-OFF* */
}

"_:"({PN_CHARS_U}|[0-9])(({PN_CHARS}|".")*{PN_CHARS})? {
/* *INDENT-ON* */
adftool_term_set_blank (term, yytext + strlen ("_:"));
return 1;
/* *INDENT-OFF* */
}

{STRING} {
/* *INDENT-ON* */
char *unescaped = unescape_string (yytext, NULL);
if (unescaped == NULL)
  {
    abort ();
  }
adftool_term_set_literal (term, unescaped, NULL, NULL);
free (unescaped);
return 1;
/* *INDENT-OFF* */
}

{STRING}[ \t\r\n]*"^^"[ \t\r\n]*{URI} {
/* *INDENT-ON* */
char *end;
char *unescaped = unescape_string (yytext, &end);
if (unescaped == NULL)
  {
    abort ();
  }
while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'
       || *end == '^')
  {
    end++;
  }

char *type = unescape_uri (end, NULL);
if (type == NULL)
  {
    abort ();
  }
adftool_term_set_literal (term, unescaped, type, NULL);
free (type);
free (unescaped);
return 1;
/* *INDENT-OFF* */
}

{STRING}[ \t\r\n]*"@"[ \t\r\n]*[A-Za-z0-9-]+ {
/* *INDENT-ON* */
char *end;
char *unescaped = unescape_string (yytext, &end);
if (unescaped == NULL)
  {
    abort ();
  }
while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
  {
    end++;
  }

char *langtag = unescape_langtag (end);
if (langtag == NULL)
  {
    abort ();
  }
adftool_term_set_literal (term, unescaped, NULL, langtag);
free (langtag);
free (unescaped);
return 1;
/* *INDENT-OFF* */
}

"true" {
/* *INDENT-ON* */
adftool_term_set_literal
  (term, "true", "http://www.w3.org/2001/XMLSchema#boolean", NULL);
return 1;
/* *INDENT-OFF* */
}

"false" {
/* *INDENT-ON* */
adftool_term_set_literal
  (term, "false", "http://www.w3.org/2001/XMLSchema#boolean", NULL);
return 1;
/* *INDENT-OFF* */
}

[+-]?[0-9]+ {
/* *INDENT-ON* */
adftool_term_set_literal
  (term, yytext, "http://www.w3.org/2001/XMLSchema#integer", NULL);
return 1;
/* *INDENT-OFF* */
}

[+-]?[0-9]*"."[0-9]+ {
/* *INDENT-ON* */
adftool_term_set_literal
  (term, yytext, "http://www.w3.org/2001/XMLSchema#decimal", NULL);
return 1;
/* *INDENT-OFF* */
}

[+-]?([0-9]+"."[0-9]*{EXPONENT}|"."[0-9]+{EXPONENT}|[0-9]+{EXPONENT}) {
/* *INDENT-ON* */
adftool_term_set_literal
  (term, yytext, "http://www.w3.org/2001/XMLSchema#double", NULL);
return 1;
/* *INDENT-OFF* */
}

. {
/* *INDENT-ON* */
*term_length -= strlen (yytext);
return 0;
/* *INDENT-OFF* */
}

[ \r\n\t]+

%%

/* *INDENT-ON* */
int
adftool_term_parse_n3 (const char *text, size_t available, size_t *consumed,
		       struct adftool_term *term)
{
  size_t term_length = 0;
  yyscan_t scanner;
  if (yylex_init (&scanner) != 0)
    {
      return 2;
    }
  assert (available <= INT_MAX);
  YY_BUFFER_STATE buffer = yy_scan_bytes (text, available, scanner);
  yy_switch_to_buffer (buffer, scanner);
  int parsed = yylex (scanner, term, &term_length);
  yypop_buffer_state (scanner);
  yylex_destroy (scanner);
  if (parsed)
    {
      *consumed = term_length;
      return 0;
    }
  else
    {
      /* This means a failure, because no more data will come. */
      *consumed = 0;
      return 1;
    }
}

static char *
unescape_string (const char *text, char **end)
{
  char *ret = malloc (strlen (text) - strlen ("\"\"") + 1);
  if (ret == NULL)
    {
      return NULL;
    }
  size_t n_ret = 0;
  assert (text[0] == '"');
  size_t i = strlen ("\"");
  while (text[i] != '"')
    {
      if (text[i] == '\\')
	{
	  switch (text[i + 1])
	    {
	    case '"':
	    case '\\':
	      ret[n_ret] = text[i + 1];
	      break;
	    case 'n':
	      ret[n_ret] = '\n';
	      break;
	    case 't':
	      ret[n_ret] = '\t';
	      break;
	    case 'r':
	      ret[n_ret] = '\r';
	      break;
	    default:
	      assert (0);
	    }
	  i += 2;
	}
      else
	{
	  ret[n_ret] = text[i];
	  i++;
	}
      n_ret++;
    }
  ret[n_ret] = '\0';
  assert (text[i] == '"');
  if (end != NULL)
    {
      *end = ((char *) text) + i + strlen ("\"");
    }
  return ret;
}

static char *
unescape_uri (const char *text, char **end)
{
  char *ret = malloc (strlen (text) - strlen ("<>") + 1);
  if (ret == NULL)
    {
      return NULL;
    }
  size_t n_ret = 0;
  assert (text[0] == '<');
  size_t i = strlen ("<");
  while (text[i] != '>')
    {
      ret[n_ret] = text[i];
      n_ret++;
      i++;
    }
  ret[n_ret] = '\0';
  assert (text[i] == '>');
  if (end != NULL)
    {
      *end = ((char *) text) + i + strlen (">");
    }
  return ret;
}

static char *
unescape_langtag (const char *text)
{
  char *ret = malloc (strlen (text) - strlen ("@") + 1);
  if (ret == NULL)
    {
      return NULL;
    }
  strcpy (ret, text + strlen ("@"));
  ret[strlen (text) - strlen ("@")] = '\0';
  return ret;
}

int
yywrap (yyscan_t scanner)
{
  (void) scanner;
  return 1;
}

static void
push_char (char c, size_t start, size_t max, size_t *length, char *dst)
{
  if (*length >= start && *length - start < max)
    {
      dst[*length - start] = c;
    }
  *length += 1;
}

static void
push_string (const char *str, size_t start, size_t max, size_t *length,
	     char *dst)
{
  for (size_t i = 0; i < strlen (str); i++)
    {
      push_char (str[i], start, max, length, dst);
    }
  push_char ('\0', start, max, length, dst);
  *length -= 1;
}

static void
push_string_1 (char c, size_t start, size_t max, size_t *length, char *dst)
{
  char str[2] = " ";
  str[0] = c;
  push_string (str, start, max, length, dst);
}

static void
push_iri (const char *unescaped, size_t start, size_t max, size_t *length,
	  char *dst)
{
  /* FIXME: I only escape the > character, because it could be used
     for data injection. */
  push_string ("<", start, max, length, dst);
  for (size_t i = 0; i < strlen (unescaped); i++)
    {
      if (unescaped[i] == '>')
	{
	  push_string ("%3E", start, max, length, dst);
	}
      else
	{
	  push_string_1 (unescaped[i], start, max, length, dst);
	}
    }
  push_string (">", start, max, length, dst);
}

static void
push_literal (const char *unescaped, size_t start, size_t max, size_t *length,
	      char *dst)
{
  push_string ("\"", start, max, length, dst);
  for (size_t i = 0; i < strlen (unescaped); i++)
    {
      switch (unescaped[i])
	{
	case '\r':
	  push_string ("\\r", start, max, length, dst);
	  break;
	case '\n':
	  push_string ("\\n", start, max, length, dst);
	  break;
	case '\"':
	case '\\':
	  push_string ("\\", start, max, length, dst);
	  /* We still need to push either " or \ */
	  /* fallthrough */
	default:
	  push_string_1 (unescaped[i], start, max, length, dst);
	}
    }
  push_string ("\"", start, max, length, dst);
}

size_t
adftool_term_to_n3 (const struct adftool_term *term, size_t start, size_t max,
		    char *str)
{
  size_t length = 0;
  if (adftool_term_is_blank (term))
    {
      push_string ("_:", start, max, &length, str);
      push_string (term->str1, start, max, &length, str);
    }
  else if (adftool_term_is_named (term))
    {
      if (term->str2 != NULL && STRNEQ (term->str2, ""))
	{
	  fprintf (stderr, _("\
I don’t know how to represent namespaced terms yet, sorry.\n"));
	  abort ();
	}
      push_iri (term->str1, start, max, &length, str);
    }
  else if (adftool_term_is_literal (term))
    {
      push_literal (term->str1, start, max, &length, str);
      if (adftool_term_is_typed_literal (term))
	{
	  push_string ("^^", start, max, &length, str);
	  push_iri (term->str2, start, max, &length, str);
	}
      else if (adftool_term_is_langstring (term))
	{
	  push_string ("@", start, max, &length, str);
	  for (size_t i = 0; i < strlen (term->str2); i++)
	    {
	      if ((term->str2[i] >= 'a' && term->str2[i] <= 'z')
		  || (term->str2[i] >= 'A' && term->str2[i] <= 'Z')
		  || (term->str2[i] >= '0' && term->str2[i] <= '9'))
		{
		  push_string_1 (term->str2[i], start, max, &length, str);
		}
	      else
		{
		  push_string ("-", start, max, &length, str);
		}
	    }
	}
      else
	{
	  assert (0);
	}
    }
  else
    {
      assert (0);
    }
  return length;
}
