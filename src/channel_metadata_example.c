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
  remove ("channel_metadata_example.adf");
  int error = adftool_file_open (file, "channel_metadata_example.adf", 1);
  if (error)
    {
      abort ();
    }
  struct adftool_term *identifier = adftool_term_alloc ();
  if (identifier == NULL)
    {
      abort ();
    }
  adftool_term_set_named (identifier, "initial-name");
  if (adftool_set_channel_identifier (file, 42, identifier) != 0)
    {
      abort ();
    }
  /* Woops, it was in fact <the-name>. */
  adftool_term_set_named (identifier, "the-name");
  if (adftool_set_channel_identifier (file, 42, identifier) != 0)
    {
      abort ();
    }
  if (adftool_set_channel_decoder (file, identifier, 42, 18) != 0)
    {
      abort ();
    }
  adftool_file_close (file);
  error = adftool_file_open (file, "channel_metadata_example.adf", 1);
  if (error)
    {
      abort ();
    }
  struct adftool_term *expected_identifier = adftool_term_alloc ();
  if (expected_identifier == NULL)
    {
      abort ();
    }
  adftool_term_set_named (expected_identifier, "the-name");
  if (adftool_find_channel_identifier (file, 42, identifier) != 0)
    {
      abort ();
    }
  if (adftool_term_compare (identifier, expected_identifier) != 0)
    {
      abort ();
    }
  double scale, offset;
  if (adftool_get_channel_decoder (file, identifier, &scale, &offset) != 0)
    {
      abort ();
    }
  assert (scale == 42);
  assert (offset == 18);
  adftool_term_free (expected_identifier);
  adftool_term_free (identifier);
  adftool_file_close (file);
  adftool_file_free (file);
  return 0;
}
