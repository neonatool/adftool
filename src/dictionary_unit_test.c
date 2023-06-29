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

#include "libadftool/dictionary_index.h"

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#define die_here()							\
  {									\
    fprintf (stderr,							\
	     _ ("%s:%d: the test failed.\n"),				\
	     __FILE__, __LINE__);					\
    abort ();								\
  }

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  remove ("dictionary_example.adf");
  hid_t file =
    H5Fcreate ("dictionary_example.adf", H5F_ACC_TRUNC, H5P_DEFAULT,
	       H5P_DEFAULT);
  if (file == H5I_INVALID_HID)
    {
      die_here ();
    }
  struct adftool_dictionary_index *dico =
    adftool_dictionary_index_alloc (file, 256, 8191, 512);
  if (dico == NULL)
    {
      die_here ();
    }
  uint32_t id = 0;
  int found = 0;
  int error =
    adftool_dictionary_index_find (dico, strlen ("hello"), "hello", true,
				   &found, &id);
  if (error || !found || id != 0)
    {
      die_here ();
    }
  error =
    adftool_dictionary_index_find (dico, strlen ("world"), "world", true,
				   &found, &id);
  if (error || !found || id != 1)
    {
      die_here ();
    }
  /* If we insert the same thing again, we should not allocate a new
     row. */
  error =
    adftool_dictionary_index_find (dico, strlen ("hello"), "hello", true,
				   &found, &id);
  if (error || !found || id != 0)
    {
      die_here ();
    }
  /* Now do the same thing with long strings */
  error =
    adftool_dictionary_index_find (dico, strlen ("hello, hello, hello"),
				   "hello, hello, hello", true, &found, &id);
  if (error || !found || id != 2)
    {
      die_here ();
    }
  error =
    adftool_dictionary_index_find (dico, strlen ("world, world, world"),
				   "world, world, world", true, &found, &id);
  if (error || !found || id != 3)
    {
      die_here ();
    }
  /* If we insert the same thing again, we should not allocate a new
     row. */
  error =
    adftool_dictionary_index_find (dico, strlen ("hello, hello, hello"),
				   "hello, hello, hello", true, &found, &id);
  if (error || !found || id != 2)
    {
      die_here ();
    }
  /* Check that we inserted the correct things. */
  error =
    adftool_dictionary_index_find (dico, strlen ("hello"), "hello", false,
				   &found, &id);
  if (error || !found || id != 0)
    {
      die_here ();
    }
  error =
    adftool_dictionary_index_find (dico, strlen ("world"), "world", false,
				   &found, &id);
  if (error || !found || id != 1)
    {
      die_here ();
    }
  /* This should fail. */
  error =
    adftool_dictionary_index_find (dico, strlen ("hey"), "hey", false, &found,
				   &id);
  if (error || found)
    {
      die_here ();
    }
  error =
    adftool_dictionary_index_find (dico, strlen ("hello, hello, hello"),
				   "hello, hello, hello", false, &found, &id);
  if (error || !found || id != 2)
    {
      die_here ();
    }
  error =
    adftool_dictionary_index_find (dico, strlen ("world, world, world"),
				   "world, world, world", false, &found, &id);
  if (error || !found || id != 3)
    {
      die_here ();
    }
  /* This should fail. */
  error =
    adftool_dictionary_index_find (dico, strlen ("hey, hey, hey, hey"),
				   "hey, hey, hey, hey", false, &found, &id);
  if (error || found)
    {
      die_here ();
    }
  adftool_dictionary_index_free (dico);
  struct adftool_dictionary_strings *dico_data =
    adftool_dictionary_strings_alloc ();
  if (dico_data == NULL)
    {
      die_here ();
    }
  if (adftool_dictionary_strings_setup (dico_data, file) != 0)
    {
      die_here ();
    }
  char *buffer;
  size_t buffer_length;
  error =
    adftool_dictionary_strings_get_a (dico_data, 0, &buffer_length, &buffer);
  if (error || buffer_length != strlen ("hello") || STRNEQ (buffer, "hello"))
    {
      die_here ();
    }
  free (buffer);
  error =
    adftool_dictionary_strings_get_a (dico_data, 1, &buffer_length, &buffer);
  if (error || buffer_length != strlen ("world") || STRNEQ (buffer, "world"))
    {
      die_here ();
    }
  free (buffer);
  error =
    adftool_dictionary_strings_get_a (dico_data, 2, &buffer_length, &buffer);
  if (error || buffer_length != strlen ("hello, hello, hello")
      || STRNEQ (buffer, "hello, hello, hello"))
    {
      die_here ();
    }
  free (buffer);
  error =
    adftool_dictionary_strings_get_a (dico_data, 3, &buffer_length, &buffer);
  if (error || buffer_length != strlen ("world, world, world")
      || STRNEQ (buffer, "world, world, world"))
    {
      die_here ();
    }
  free (buffer);
  buffer = NULL;
  error =
    adftool_dictionary_strings_get_a (dico_data, 4, &buffer_length, &buffer);
  if (error == 0 || buffer != NULL)
    {
      die_here ();
    }
  adftool_dictionary_strings_free (dico_data);
  H5Fclose (file);
  return 0;
}
