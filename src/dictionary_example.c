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
  struct adftool_file *file = adftool_file_alloc ();
  if (file == NULL)
    {
      abort ();
    }
  remove ("dictionary_example.adf");
  int error = adftool_file_open (file, "dictionary_example.adf", 1);
  if (error)
    {
      abort ();
    }
  uint32_t id = 0;
  error = adftool_dictionary_insert (file, strlen ("hello"), "hello", &id);
  if (error || id != 0)
    {
      abort ();
    }
  error = adftool_dictionary_insert (file, strlen ("world"), "world", &id);
  if (error || id != 1)
    {
      abort ();
    }
  /* If we insert the same thing again, we should not allocate a new
     row. */
  error = adftool_dictionary_insert (file, strlen ("hello"), "hello", &id);
  if (error || id != 0)
    {
      abort ();
    }
  /* Now do the same thing with long strings */
  error =
    adftool_dictionary_insert (file, strlen ("hello, hello, hello"),
			       "hello, hello, hello", &id);
  if (error || id != 2)
    {
      abort ();
    }
  error =
    adftool_dictionary_insert (file, strlen ("world, world, world"),
			       "world, world, world", &id);
  if (error || id != 3)
    {
      abort ();
    }
  /* If we insert the same thing again, we should not allocate a new
     row. */
  error =
    adftool_dictionary_insert (file, strlen ("hello, hello, hello"),
			       "hello, hello, hello", &id);
  if (error || id != 2)
    {
      abort ();
    }
  /* Check that we inserted the correct things. */
  int found = 0;
  error =
    adftool_dictionary_lookup (file, strlen ("hello"), "hello", &found, &id);
  if (error || !found || id != 0)
    {
      abort ();
    }
  error =
    adftool_dictionary_lookup (file, strlen ("world"), "world", &found, &id);
  if (error || !found || id != 1)
    {
      abort ();
    }
  /* This should fail. */
  error =
    adftool_dictionary_lookup (file, strlen ("hey"), "hey", &found, &id);
  if (error || found)
    {
      abort ();
    }
  error =
    adftool_dictionary_lookup (file, strlen ("hello, hello, hello"),
			       "hello, hello, hello", &found, &id);
  if (error || !found || id != 2)
    {
      abort ();
    }
  error =
    adftool_dictionary_lookup (file, strlen ("world, world, world"),
			       "world, world, world", &found, &id);
  if (error || !found || id != 3)
    {
      abort ();
    }
  /* This should fail. */
  error =
    adftool_dictionary_lookup (file, strlen ("hey, hey, hey, hey"),
			       "hey, hey, hey, hey", &found, &id);
  if (error || found)
    {
      abort ();
    }
  char buffer[256];
  size_t buffer_length;
  error =
    adftool_dictionary_get (file, 0, 0, sizeof (buffer), &buffer_length,
			    buffer);
  if (error || buffer_length != strlen ("hello")
      || strcmp (buffer, "hello") != 0)
    {
      abort ();
    }
  error =
    adftool_dictionary_get (file, 1, 0, sizeof (buffer), &buffer_length,
			    buffer);
  if (error || buffer_length != strlen ("world")
      || strcmp (buffer, "world") != 0)
    {
      abort ();
    }
  error =
    adftool_dictionary_get (file, 2, 0, sizeof (buffer), &buffer_length,
			    buffer);
  if (error || buffer_length != strlen ("hello, hello, hello")
      || strcmp (buffer, "hello, hello, hello") != 0)
    {
      abort ();
    }
  error =
    adftool_dictionary_get (file, 3, 0, sizeof (buffer), &buffer_length,
			    buffer);
  if (error || buffer_length != strlen ("world, world, world")
      || strcmp (buffer, "world, world, world") != 0)
    {
      abort ();
    }
  adftool_file_close (file);
  adftool_file_free (file);
  return 0;
}
