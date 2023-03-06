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
  remove ("channel_metadata_example.adf");
  struct adftool_file *file =
    adftool_file_open ("channel_metadata_example.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  static const double data_example[2] = { 1, 2 };
  if (adftool_eeg_set_data (file, 2, 1, data_example) != 0)
    {
      abort ();
    }
  struct adftool_term *identifier = adftool_term_alloc ();
  if (identifier == NULL)
    {
      abort ();
    }
  if (adftool_find_channel_identifier (file, 0, identifier) != 0)
    {
      abort ();
    }
  if (adftool_set_channel_decoder (file, identifier, 42, 18) != 0)
    {
      abort ();
    }
  struct adftool_term *type_channel = adftool_term_alloc ();
  if (type_channel == NULL)
    {
      abort ();
    }
  adftool_term_set_named (type_channel, "https://example.com/channel-type");
  if (adftool_add_channel_type (file, identifier, type_channel) != 0)
    {
      abort ();
    }
  adftool_file_close (file);
  file = adftool_file_open ("channel_metadata_example.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  struct adftool_term *expected_identifier = adftool_term_alloc ();
  if (expected_identifier == NULL)
    {
      abort ();
    }
  adftool_term_set_named (expected_identifier, "#channel-0");
  if (adftool_find_channel_identifier (file, 0, identifier) != 0)
    {
      abort ();
    }
  if (adftool_term_compare (identifier, expected_identifier) != 0)
    {
      abort ();
    }
  size_t column_index;
  if (adftool_get_channel_column (file, identifier, &column_index) != 0)
    {
      abort ();
    }
  assert (column_index == 0);
  double scale, offset;
  if (adftool_get_channel_decoder (file, identifier, &scale, &offset) != 0)
    {
      abort ();
    }
  assert (scale == 42);
  assert (offset == 18);
  struct adftool_term *one_result = adftool_term_alloc ();
  if (one_result == NULL)
    {
      abort ();
    }
  if (adftool_get_channel_types (file, identifier, 0, 1, &one_result) != 1)
    {
      abort ();
    }
  if (adftool_term_compare (one_result, type_channel) != 0)
    {
      abort ();
    }
  if (adftool_find_channels_by_type (file, type_channel, 0, 1, &one_result) !=
      1)
    {
      abort ();
    }
  if (adftool_term_compare (one_result, identifier) != 0)
    {
      abort ();
    }
  adftool_term_free (one_result);
  adftool_term_free (type_channel);
  adftool_term_free (expected_identifier);
  adftool_term_free (identifier);
  adftool_file_close (file);
  return 0;
}
