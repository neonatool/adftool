#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <adftool.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <assert.h>

#define _(String) gettext(String)
#define N_(String) (String)

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  struct timespec *timespec = adftool_timespec_alloc ();
  if (timespec == NULL)
    {
      abort ();
    }
  adftool_timespec_set_js (timespec, 1665050076788);
  const double value = adftool_timespec_get_js (timespec);
  adftool_timespec_free (timespec);
  assert (value == 1665050076788);
  return 0;
}
