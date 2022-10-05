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
  const size_t sizeof_timespec = adftool_sizeof_timespec ();
  assert (sizeof_timespec == sizeof (struct timespec));
  struct timespec *array = malloc (2 * sizeof_timespec);
  if (array == NULL)
    {
      abort ();
    }
  adftool_array_set_timespec (array, 0, 1664969280, 421000000);
  adftool_array_set_timespec (array, 1, 1664969281, 422000000);
  assert (adftool_array_get_tv_sec (array, 0) == 1664969280);
  assert (adftool_array_get_tv_nsec (array, 0) == 421000000);
  assert (adftool_array_get_tv_sec (array, 1) == 1664969281);
  assert (adftool_array_get_tv_nsec (array, 1) == 422000000);
  free (array);
  return 0;
}
