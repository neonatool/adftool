#include <config.h>

#include <attribute.h>
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

#include "libadftool/version.c"
#include "libadftool/channel_processor_group.h"
#include "libadftool/term.h"

#define fail_test() \
  (fprintf (stderr, "%s:%d: test failed.\n", __FILE__, __LINE__), abort ())

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  struct adftool_file *file = adftool_file_open_generated ();
  if (file == NULL)
    {
      fail_test ();
    }
  pthread_mutex_t sync;
  if (pthread_mutex_init (&sync, NULL) != 0)
    {
      fail_test ();
    }
  struct adftool_term *fp1 = term_alloc ();
  struct adftool_term *fp2 = term_alloc ();
  if (fp1 == NULL || fp2 == NULL)
    {
      fail_test ();
    }
  term_set_named (fp1, "https://localhost/lytonepal#Fp1");
  term_set_named (fp2, "https://localhost/lytonepal#Fp2");
  struct adftool_channel_processor_group *group =
    channel_processor_group_alloc (file, &sync, 2);
  if (group == NULL)
    {
      fail_test ();
    }
  size_t start_index;
  size_t length;
  double *data1, *data2;
  static const size_t n_data = 10240;
  if (ALLOC_N (data1, n_data) < 0 || ALLOC_N (data2, n_data) < 0)
    {
      fail_test ();
    }
  for (size_t i = 0; i < n_data; i++)
    {
      data1[i] = 42;
      data2[i] = 42;
    }
  int error1 =
    channel_processor_group_get (group, fp1, 0.53, 35, 2560, n_data,
				 &start_index, &length,
				 data1);
  assert (error1 == 0);
  /* The cache is empty; thus we don’t know the limits yet. */
  assert (start_index == 2560);
  assert (length == n_data);
  for (size_t i = 0; i < n_data; i++)
    {
      assert (isnan (data1[i]));
    }
  int error2 =
    channel_processor_group_get (group, fp2, 0.53, 35, 2560, n_data,
				 &start_index, &length,
				 data2);
  assert (error2 == 0);
  /* The cache is empty; thus we don’t know the limits yet. */
  assert (start_index == 2560);
  assert (length == n_data);
  for (size_t i = 0; i < n_data; i++)
    {
      assert (isnan (data2[i]));
    }
  bool work_done = false;
  do
    {
      int error = channel_processor_group_populate_cache (group, &work_done);
      assert (error == 0);
    }
  while (work_done);
  error1 =
    channel_processor_group_get (group, fp1, 0.53, 35, 0, n_data,
				 &start_index, &length, data1);
  assert (error1 == 0);
  /* The cache is now filled; thus we do know the limits. */
  assert (start_index == 0);
  assert (length == 5120);
  for (size_t i = 0; i < length; i++)
    {
      assert (!isnan (data1[i]));
      assert (data1[i] != 42);
    }
  for (size_t i = length; i < n_data; i++)
    {
      assert (isnan (data1[i]));
    }
  error2 =
    channel_processor_group_get (group, fp2, 0.53, 35, 0, n_data,
				 &start_index, &length, data2);
  assert (error2 == 0);
  /* The cache is now filled; thus we do know the limits. */
  assert (start_index == 0);
  assert (length == 5120);
  for (size_t i = 0; i < length; i++)
    {
      assert (!isnan (data2[i]));
      assert (data2[i] != 42);
    }
  for (size_t i = length; i < n_data; i++)
    {
      assert (isnan (data2[i]));
    }
  FREE (data1);
  FREE (data2);
  channel_processor_group_free (group);
  term_free (fp1);
  term_free (fp2);
  adftool_file_close (file);
  return 0;
}
