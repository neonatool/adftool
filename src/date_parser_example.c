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

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  static const char timezoned_date[] = "2002-10-10T12:00:00-05:00";
  static const char untimezoned[] = "2002-10-10T17:00:00Z";
  static const char with_milliseconds[] = "2002-10-10T12:00:00.123-05:00";
  static const char *xsd_datetime =
    "http://www.w3.org/2001/XMLSchema#dateTime";
  char value[1024];
  char meta[1024];
  struct adftool_term *term = adftool_term_alloc ();
  if (term == NULL)
    {
      abort ();
    }
  adftool_term_set_literal (term, timezoned_date, xsd_datetime, NULL);
  struct timespec date;
  if (adftool_term_as_date (term, &date) != 0)
    {
      assert (0);
    }
  assert (date.tv_sec == 1034269200);
  assert (date.tv_nsec == 0);
  adftool_term_set_date (term, &date);
  if (adftool_term_value (term, 0, sizeof (value), value) !=
      strlen (untimezoned))
    {
      assert (0);
    }
  assert (STREQ (value, untimezoned));
  if (adftool_term_meta (term, 0, sizeof (meta), meta) !=
      strlen (xsd_datetime))
    {
      assert (0);
    }
  assert (STREQ (meta, xsd_datetime));
  /* Test that milliseconds are not parsed as nanoseconds */
  adftool_term_set_literal (term, with_milliseconds, xsd_datetime, NULL);
  if (adftool_term_as_date (term, &date) != 0)
    {
      assert (0);
    }
  assert (date.tv_sec == 1034269200);
  assert (date.tv_nsec == 123000000);
  adftool_term_free (term);
  return 0;
}
