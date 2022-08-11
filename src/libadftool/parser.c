#include <adftool_private.h>
#include <adftool_bplus.h>

static size_t
eat_up_whitespace (const char *text, size_t available)
{
  size_t start = 0;
  while (start < available
	 && (text[start] == ' ' || text[start] == '\t' || text[start] == '\r'
	     || text[start] == '\n'))
    {
      start++;
    }
  return start;
}

static inline int
string_prefix (const char *prefix, const char *text, size_t text_length)
{
  return (text_length >= strlen (prefix)
	  && strncmp (text, prefix, strlen (prefix)) == 0);
}

static inline int
parse_uri (const char *text, size_t available, int atend, size_t *consumed,
	   char **value)
{
  size_t start = eat_up_whitespace (text, available);
  if (start == available)
    {
      /* Empty string */
      if (atend)
	{
	  *consumed = 0;
	}
      else
	{
	  *consumed = start;
	}
      return 1;
    }
  if (string_prefix ("<", text + start, available - start))
    {
      start += strlen ("<");
      size_t length = 0;
      while (start + length < available
	     &&
	     !(string_prefix
	       (">", text + start + length, available - start - length)))
	{
	  length++;
	}
      if (start + length < available)
	{
	  assert (string_prefix
		  (">", text + start + length, available - start - length));
	  *consumed = start + length + strlen (">");
	  *value = malloc (length + 1);
	  if (*value == NULL)
	    {
	      return 2;
	    }
	  memcpy (*value, text + start, length);
	  (*value)[length] = '\0';
	  return 0;
	}
      else if (!atend)
	{
	  *consumed = available;
	  return 1;
	}
      else
	{
	  *consumed = 0;
	  return 1;
	}
    }
  else
    {
      *consumed = 0;
      return 1;
    }
}

static inline int
is_blank_node_label_char (char c)
{
  return ((c >= 'A' && c <= 'Z')
	  || (c >= 'a' && c <= 'z')
	  || (c >= '0' && c <= '9') || c == '_' || c == ':' || c == '-');
}

static inline int
parse_blank_node (const char *text, size_t available, int atend,
		  size_t *consumed, struct adftool_term *term)
{
  size_t start = eat_up_whitespace (text, available);
  if (start == available)
    {
      /* Empty string */
      if (atend)
	{
	  *consumed = 0;
	}
      else
	{
	  *consumed = start;
	}
      return 1;
    }
  if (string_prefix ("_:", text + start, available - start))
    {
      start += strlen ("_:");
      size_t length = 0;
      while (start + length < available
	     && (text[start + length]
		 && is_blank_node_label_char (text[start + length])))
	{
	  length++;
	}
      if (length == 0)
	{
	  if (start + length < available || atend)
	    {
	      /* Empty blank node identifiers are not allowed. */
	      *consumed = 0;
	    }
	  else
	    {
	      *consumed = available;
	    }
	  return 1;
	}
      if (start + length < available || atend)
	{
	  assert (!is_blank_node_label_char (text[start + length]));
	  *consumed = start + length;
	  char *value = malloc (length + 1);
	  if (value == NULL)
	    {
	      return 2;
	    }
	  memcpy (value, text + start, length);
	  value[length] = '\0';
	  if (adftool_term_set_blank (term, value) != 0)
	    {
	      free (value);
	      return 2;
	    }
	  free (value);
	  return 0;
	}
      else if (!atend)
	{
	  *consumed = available;
	  return 1;
	}
      else
	{
	  *consumed = 0;
	  return 1;
	}
    }
  else if (string_prefix ("_", text + start, available - start))
    {
      if (atend)
	{
	  *consumed = 0;
	}
      else
	{
	  *consumed = start + strlen ("_");
	}
      return 1;
    }
  else
    {
      *consumed = 0;
      return 1;
    }
}

static inline int
parse_naked_literal (const char *text, size_t available, int atend,
		     size_t *consumed, char **value)
{
  size_t start = eat_up_whitespace (text, available);
  if (start == available)
    {
      /* Empty string */
      if (atend)
	{
	  *consumed = 0;
	}
      else
	{
	  *consumed = start;
	}
      return 1;
    }
  if (string_prefix ("\"", text + start, available - start))
    {
      start += strlen ("\"");
      size_t length = 0;
      while (start + length < available
	     &&
	     !(string_prefix
	       ("\"", text + start + length, available - start - length)))
	{
	  if (string_prefix
	      ("\\", text + start + length, available - start - length))
	    {
	      length += 2;
	    }
	  else
	    {
	      length++;
	    }
	}
      if (start + length < available)
	{
	  assert (string_prefix
		  ("\"", text + start + length, available - start - length));
	  *consumed = start + length + strlen ("\"");
	  *value = malloc (length + 1);
	  if (*value == NULL)
	    {
	      return 2;
	    }
	  size_t n = 0;
	  for (size_t i = 0; i < length; i++)
	    {
	      if (text[start + i] == '\\')
		{
		  i++;
		}
	      assert (i < length);
	      (*value)[n++] = text[start + i];
	    }
	  assert (n <= length);
	  (*value)[n] = '\0';
	  return 0;
	}
      else if (!atend)
	{
	  *consumed = available;
	  return 1;
	}
      else
	{
	  *consumed = 0;
	  return 1;
	}
    }
  else
    {
      *consumed = 0;
      return 1;
    }
}

static int
is_langtag_char (char c)
{
  return ((c >= 'A' && c <= 'Z')
	  || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-');
}

static inline int
parse_langtag (const char *text, size_t available, int atend,
	       size_t *consumed, char **value)
{
  size_t start = eat_up_whitespace (text, available);
  if (start == available)
    {
      /* Empty string */
      if (atend)
	{
	  *consumed = 0;
	}
      else
	{
	  *consumed = start;
	}
      return 1;
    }
  if (string_prefix ("@", text + start, available - start))
    {
      start += strlen ("@");
      size_t length = 0;
      while (start + length < available
	     && (text[start + length]
		 && is_langtag_char (text[start + length])))
	{
	  length++;
	}
      if (atend || start + length < available)
	{
	  assert (!is_blank_node_label_char (text[start + length]));
	  *consumed = start + length;
	  *value = malloc (length + 1);
	  if (*value == NULL)
	    {
	      return 2;
	    }
	  memcpy (*value, text + start, length);
	  (*value)[length] = '\0';
	  return 0;
	}
      else if (!atend)
	{
	  *consumed = available;
	  return 1;
	}
      else
	{
	  *consumed = 0;
	  return 1;
	}
    }
  else
    {
      *consumed = 0;
      return 1;
    }
}

static inline int
parse_type_annotation (const char *text, size_t available, int atend,
		       size_t *consumed, char **value)
{
  size_t start = eat_up_whitespace (text, available);
  if (start == available)
    {
      /* Empty string */
      if (atend)
	{
	  *consumed = 0;
	}
      else
	{
	  *consumed = start;
	}
      return 1;
    }
  if (string_prefix ("^^", text + start, available - start))
    {
      start += strlen ("^^");
      size_t uri_consumed = 0;
      int result =
	parse_uri (text + start, available - start, atend, &uri_consumed,
		   value);
      *consumed = uri_consumed + start;
      return result;
    }
  else if (string_prefix ("^", text + start, available - start)
	   && start + strlen ("^") == available)
    {
      *consumed = start + strlen ("^");
      return 1;
    }
  else
    {
      *consumed = 0;
      return 1;
    }
}

static inline int
parse_literal (const char *text, size_t available, int atend,
	       size_t *consumed, struct adftool_term *term)
{
  size_t naked_consumed = 0;
  char *value = NULL;
  char *meta = NULL;
  int result =
    parse_naked_literal (text, available, atend, &naked_consumed, &value);
  if (result == 0)
    {
      assert (naked_consumed <= available);
      size_t type_consumed = 0;
      int type_annotation_result =
	parse_type_annotation (text + naked_consumed,
			       available - naked_consumed, atend,
			       &type_consumed, &meta);
      if (type_annotation_result == 2)
	{
	  free (value);
	  return 2;
	}
      else if (type_annotation_result == 0)
	{
	  if (adftool_term_set_literal (term, value, meta, NULL) != 0)
	    {
	      free (value);
	      free (meta);
	      return 2;
	    }
	  free (value);
	  free (meta);
	  *consumed = naked_consumed + type_consumed;
	  return 0;
	}
      else
	{
	  /* Try a langtag */
	  size_t lang_consumed = 0;
	  int lang_result =
	    parse_langtag (text + naked_consumed, available - naked_consumed,
			   atend, &lang_consumed, &meta);
	  if (lang_result == 2)
	    {
	      free (value);
	      return 2;
	    }
	  else if (lang_result == 0)
	    {
	      if (adftool_term_set_literal (term, value, NULL, meta) != 0)
		{
		  free (value);
		  free (meta);
		  return 2;
		}
	      free (value);
	      free (meta);
	      *consumed = naked_consumed + lang_consumed;
	      return 0;
	    }
	  else
	    {
	      /* Cannot parse a type nor a langtag. */
	      if (atend
		  || (lang_consumed < available - naked_consumed
		      && type_consumed < available - naked_consumed))
		{
		  /* We won’t be able to ever parse a langtag or a
		     type, so this is a xsd:string. */
		  if (adftool_term_set_literal (term, value, NULL, NULL) != 0)
		    {
		      free (value);
		      return 2;
		    }
		  *consumed = naked_consumed;
		  free (value);
		  return 0;
		}
	      /* We can still parse a type or a langtag. */
	      *consumed = available;
	      free (value);
	      return 1;
	    }
	}
    }
  else if (result == 1)
    {
      *consumed = naked_consumed;
    }
  return result;
}

static inline int
parse_named (const char *text, size_t available, int atend, size_t *consumed,
	     struct adftool_term *term)
{
  char *value;
  int result = parse_uri (text, available, atend, consumed, &value);
  if (result == 0)
    {
      if (adftool_term_set_named (term, value) != 0)
	{
	  free (value);
	  return 2;
	}
      free (value);
    }
  return result;
}

int
adftool_term_parse_n3 (const char *text, size_t available, int atend,
		       size_t *consumed, struct adftool_term *term)
{
  int result = 0;
  *consumed = 0;
  size_t my_consumed;
  result = parse_blank_node (text, available, atend, &my_consumed, term);
  if (result != 2 && my_consumed > *consumed)
    {
      *consumed = my_consumed;
    }
  if (result == 1)
    {
      result = parse_literal (text, available, atend, &my_consumed, term);
    }
  if (result != 2 && my_consumed > *consumed)
    {
      *consumed = my_consumed;
    }
  if (result == 1)
    {
      result = parse_named (text, available, atend, &my_consumed, term);
    }
  if (result != 2 && my_consumed > *consumed)
    {
      *consumed = my_consumed;
    }
  if (result == 1)
    {
      /* I need to set consumed to either 0 or available. */
      if (*consumed < available)
	{
	  *consumed = 0;
	}
    }
  return result;
}
