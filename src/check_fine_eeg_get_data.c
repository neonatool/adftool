#include <config.h>

#include <attribute.h>
#include <adftool.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>

#define _(String) gettext(String)
#define N_(String) (String)

static const double example_data[4][2] = {
  {0.6062011, 0.6326078},
  {-2.7099204, -0.2362881},
  {-0.6014201, -0.1521410},
  {0.4972079, -1.0824288}
};

#define test_fail() \
  { \
    fprintf (stderr, "%s:%d: test failed.\n", __FILE__, __LINE__); \
    abort (); \
  }

#define float_eq(f, expected) \
  ((((f) - (expected)) <= 1e-4) && (((expected) - (f)) <= 1e-4))

#define float_neq(f, expected) \
  (! (float_eq (f, expected)))

static void
check_first_channel (struct adftool_file *file)
{
  double answer[4];
  size_t n_points, n_channels;
  int error =
    adftool_eeg_get_data (file, 0, 4, &n_points, 0, 1, &n_channels, answer);
  if (error)
    {
      test_fail ();
    }
  if (n_points != 4 || n_channels != 2)
    {
      test_fail ();
    }
  if (float_neq (answer[0], 0.6062011) || float_neq (answer[3], 0.4972079))
    {
      test_fail ();
    }
}

static void
check_second_channel (struct adftool_file *file)
{
  double answer[4];
  size_t n_points, n_channels;
  int error =
    adftool_eeg_get_data (file, 0, 4, &n_points, 1, 1, &n_channels, answer);
  if (error)
    {
      test_fail ();
    }
  if (n_points != 4 || n_channels != 2)
    {
      test_fail ();
    }
  if (float_neq (answer[0], 0.6326078) || float_neq (answer[3], -1.0824288))
    {
      test_fail ();
    }
}

static void
check_overflow_large (struct adftool_file *file)
{
  double answer[4];
  size_t n_points, n_channels;
  int error =
    adftool_eeg_get_data (file, 2, 5, &n_points, 1, 1, &n_channels, answer);
  if (error)
    {
      test_fail ();
    }
  if (n_points != 4 || n_channels != 2)
    {
      test_fail ();
    }
  if (float_neq (answer[0], -0.1521410) || float_neq (answer[1], -1.0824288))
    {
      test_fail ();
    }
}

static void
check_overflow_offset (struct adftool_file *file)
{
  double answer[4];
  size_t n_points, n_channels;
  int error =
    adftool_eeg_get_data (file, 2, 4, &n_points, 1, 1, &n_channels, answer);
  if (error)
    {
      test_fail ();
    }
  if (n_points != 4 || n_channels != 2)
    {
      test_fail ();
    }
  if (float_neq (answer[0], -0.1521410) || float_neq (answer[1], -1.0824288))
    {
      test_fail ();
    }
}

static void
check_overflow_small (struct adftool_file *file)
{
  double answer[4];
  size_t n_points, n_channels;
  int error =
    adftool_eeg_get_data (file, 2, 3, &n_points, 1, 1, &n_channels, answer);
  if (error)
    {
      test_fail ();
    }
  if (n_points != 4 || n_channels != 2)
    {
      test_fail ();
    }
  if (float_neq (answer[0], -0.1521410) || float_neq (answer[1], -1.0824288))
    {
      test_fail ();
    }
}

static void
check_offset_just (struct adftool_file *file)
{
  double answer[4];
  size_t n_points, n_channels;
  int error =
    adftool_eeg_get_data (file, 2, 2, &n_points, 1, 1, &n_channels, answer);
  if (error)
    {
      test_fail ();
    }
  if (n_points != 4 || n_channels != 2)
    {
      test_fail ();
    }
  if (float_neq (answer[0], -0.1521410) || float_neq (answer[1], -1.0824288))
    {
      test_fail ();
    }
}

static void
check_offset (struct adftool_file *file)
{
  double answer[4];
  size_t n_points, n_channels;
  int error =
    adftool_eeg_get_data (file, 2, 1, &n_points, 1, 1, &n_channels, answer);
  if (error)
    {
      test_fail ();
    }
  if (n_points != 4 || n_channels != 2)
    {
      test_fail ();
    }
  if (float_neq (answer[0], -0.1521410))
    {
      test_fail ();
    }
}

static void
check_empty (struct adftool_file *file)
{
  double answer[4];
  size_t n_points, n_channels;
  int error =
    adftool_eeg_get_data (file, 2, 0, &n_points, 1, 1, &n_channels, answer);
  if (error)
    {
      test_fail ();
    }
  if (n_points != 4 || n_channels != 2)
    {
      test_fail ();
    }
}

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  static const size_t n_points =
    sizeof (example_data) / sizeof (example_data[0]);
  static const size_t n_channels =
    sizeof (example_data[0]) / sizeof (example_data[0][0]);
  double *row_wise = malloc (n_points * n_channels * sizeof (*row_wise));
  if (row_wise == NULL)
    {
      test_fail ();
    }
  for (size_t i = 0; i < n_points; i++)
    {
      for (size_t j = 0; j < n_channels; j++)
	{
	  const size_t index = i * n_channels + j;
	  row_wise[index] = example_data[i][j];
	}
    }
  struct adftool_file *file = adftool_file_open_data (0, NULL);
  if (file == NULL)
    {
      test_fail ();
    }
  if (adftool_eeg_set_data (file, n_points, n_channels, row_wise) != 0)
    {
      test_fail ();
    }
  check_first_channel (file);
  check_second_channel (file);
  check_overflow_large (file);
  check_overflow_offset (file);
  check_overflow_small (file);
  check_offset_just (file);
  check_offset (file);
  check_empty (file);
  adftool_file_close (file);
  free (row_wise);
  return 0;
}
