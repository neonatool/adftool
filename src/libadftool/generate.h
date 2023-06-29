#ifndef H_ADFTOOL_GENERATE_INCLUDED
# define H_ADFTOOL_GENERATE_INCLUDED

# include <adftool.h>

# include <stdlib.h>
# include <assert.h>
# include <string.h>
# include <locale.h>
# include <stdbool.h>
# include <limits.h>
# include <math.h>

# include "gettext.h"

# ifdef BUILDING_LIBADFTOOL
#  define _(String) dgettext (PACKAGE, (String))
#  define N_(String) (String)
# else
#  define _(String) gettext (String)
#  define N_(String) (String)
# endif

struct adftool_file;

MAYBE_UNUSED static struct adftool_file *file_open_generated (void);

# include "file.h"

# define ADFTOOL_FILE_GENERATE_N_POINTS 5120

static double *
adftool_file_generate_background (const double *times, double phase)
{
  int error = 0;
  double *ret = malloc (ADFTOOL_FILE_GENERATE_N_POINTS * sizeof (double));
  if (ret == NULL)
    {
      error = 1;
      goto cleanup;
    }
  const double amplitude = 15 * 1.0e-6;
  const double frequency = 0.9;
  for (size_t i = 0; i < ADFTOOL_FILE_GENERATE_N_POINTS; i++)
    {
      ret[i] = amplitude * sin (2 * M_PI * frequency * times[i] + phase);
    }
cleanup:
  if (error)
    {
      free (ret);
      ret = NULL;
    }
  return ret;
}

static void
adftool_file_generate_pointe (const double *restrict times,
			      double *restrict data, double pointe)
{
  const double time_start = pointe - 0.05;
  const double time_peak = pointe;
  const double time_stop = pointe + 0.05;
  const double amplitude = -20 * 1e-6;
  for (size_t i = 0; i < ADFTOOL_FILE_GENERATE_N_POINTS; i++)
    {
      const double t = times[i];
      if (t >= time_start && t <= time_peak)
	{
	  const double advancement =
	    (t - time_start) / (time_peak - time_start);
	  data[i] += amplitude * advancement;
	}
      else if (t >= time_peak && t <= time_stop)
	{
	  const double advancement =
	    (t - time_peak) / (time_stop - time_peak);
	  data[i] += amplitude * (1 - advancement);
	}
    }
}

static void
adftool_file_generate_encoche (const double *restrict times,
			       double *restrict data, double encoche)
{
  const double time_start = encoche - 0.2;
  const double time_first_peak = encoche - 0.1;
  const double time_second_peak = encoche + 0.1;
  const double time_stop = encoche + 0.2;
  const double amplitude = 20 * 1e-6;
  for (size_t i = 0; i < ADFTOOL_FILE_GENERATE_N_POINTS; i++)
    {
      const double t = times[i];
      if (t >= time_start && t <= time_first_peak)
	{
	  const double advancement =
	    (t - time_start) / (time_first_peak - time_start);
	  data[i] += amplitude * advancement;
	}
      else if (t >= time_first_peak && t <= time_second_peak)
	{
	  const double advancement =
	    (t - time_first_peak) / (time_second_peak - time_first_peak);
	  data[i] += amplitude * (1 - 2 * advancement);
	}
      else if (t >= time_second_peak && t <= time_stop)
	{
	  const double advancement =
	    (t - time_second_peak) / (time_stop - time_second_peak);
	  data[i] += amplitude * (advancement - 1);
	}
    }
}

static double *
adftool_file_generate_channel (const double *restrict times, double phase,
			       size_t n_pointes,
			       const double *restrict pointes,
			       size_t n_encoches,
			       const double *restrict encoches)
{
  int error = 0;
  double *ret = adftool_file_generate_background (times, phase);
  if (ret == NULL)
    {
      error = 1;
      goto cleanup;
    }
  double start_artifact = 10;
  double stop_artifact = 15;
  for (size_t i = 0; i < ADFTOOL_FILE_GENERATE_N_POINTS; i++)
    {
      if (times[i] >= start_artifact && times[i] <= stop_artifact)
	{
	  ret[i] *= 5;
	}
    }
  for (size_t i = 0; i < n_pointes; i++)
    {
      adftool_file_generate_pointe (times, ret, pointes[i]);
    }
  for (size_t i = 0; i < n_encoches; i++)
    {
      adftool_file_generate_encoche (times, ret, encoches[i]);
    }
cleanup:
  if (error)
    {
      free (ret);
      ret = NULL;
    }
  return ret;
}

# define ADFTOOL_FILE_DO_GENERATE                                       \
  return adftool_file_generate_channel                                  \
  (times,                                                               \
   phase,                                                               \
   sizeof (pointes) / sizeof (pointes[0]),                              \
   pointes,                                                             \
   sizeof (encoches) / sizeof (encoches[0]),                            \
   encoches);

static double *
adftool_file_generate_fp2 (const double *times)
{
  static const double phase = 0;
  static const double pointes[] = {
  };
  static const double encoches[] = {
    18
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_c4 (const double *times)
{
  static const double phase = M_PI / 2;
  static const double pointes[] = {
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_o2 (const double *times)
{
  static const double phase = 0;
  static const double pointes[] = {
    2, 3, 16
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_t4 (const double *times)
{
  static const double phase = M_PI / 2;
  static const double pointes[] = {
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_fp1 (const double *times)
{
  static const double phase = 0;
  static const double pointes[] = {
  };
  static const double encoches[] = {
    4, 5
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_c3 (const double *times)
{
  static const double phase = M_PI / 2;
  static const double pointes[] = {
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_o1 (const double *times)
{
  static const double phase = 0;
  static const double pointes[] = {
    7
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_t3 (const double *times)
{
  static const double phase = M_PI / 2;
  static const double pointes[] = {
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_cz (const double *times)
{
  static const double phase = 0;
  static const double pointes[] = {
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_fz (const double *times)
{
  static const double phase = M_PI / 2;
  static const double pointes[] = {
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

static double *
adftool_file_generate_pz (const double *times)
{
  static const double phase = M_PI / 2;
  static const double pointes[] = {
  };
  static const double encoches[] = {
  };
  ADFTOOL_FILE_DO_GENERATE;
}

typedef double *(*adftool_file_channel_generator) (const double *);

static double *
adftool_file_generate_matrix (const double *times, size_t *n_rows,
			      size_t *n_columns)
{
  static const adftool_file_channel_generator generators[] = {
    adftool_file_generate_fp2,
    adftool_file_generate_c4,
    adftool_file_generate_o2,
    adftool_file_generate_t4,
    adftool_file_generate_fp1,
    adftool_file_generate_c3,
    adftool_file_generate_o1,
    adftool_file_generate_t3,
    adftool_file_generate_cz,
    adftool_file_generate_fz,
    adftool_file_generate_pz
  };
  const size_t n_cols = sizeof (generators) / sizeof (generators[0]);
  *n_rows = ADFTOOL_FILE_GENERATE_N_POINTS;
  *n_columns = n_cols;
  double *columns[11] = { NULL };
  double *matrix = NULL;
  assert (sizeof (columns) / sizeof (columns[0]) == n_cols);
  int error = 0;
  for (size_t i = 0; i < n_cols; i++)
    {
      columns[i] = generators[i] (times);
      if (columns[i] == NULL)
	{
	  error = 1;
	}
    }
  if (error)
    {
      goto cleanup;
    }
  matrix = malloc (*n_columns * *n_rows * sizeof (double));
  if (matrix == NULL)
    {
      error = 1;
      goto cleanup;
    }
  for (size_t i = 0; i < *n_rows; i++)
    {
      for (size_t j = 0; j < n_cols; j++)
	{
	  matrix[i * n_cols + j] = columns[j][i];
	}
    }
cleanup:
  for (size_t i = 0; i < n_cols; i++)
    {
      free (columns[i]);
    }
  if (error)
    {
      free (matrix);
      matrix = NULL;
    }
  return matrix;
}

static struct adftool_file *
file_open_generated (void)
{
  struct adftool_file *f = file_open_data (0, NULL);
  int error = 0;
  if (f == NULL)
    {
      error = 1;
      goto cleanup;
    }
  struct timespec start_date = {
    .tv_sec = 1678961223,
    .tv_nsec = 0
  };
  double sampling_frequency = 256;
  if (adftool_eeg_set_time (f, &start_date, sampling_frequency) != 0)
    {
      error = 1;
      goto cleanup;
    }
  double *time_array =
    malloc (ADFTOOL_FILE_GENERATE_N_POINTS * sizeof (double));
  if (time_array == NULL)
    {
      error = 1;
      goto cleanup;
    }
  for (size_t i = 0; i < ADFTOOL_FILE_GENERATE_N_POINTS; i++)
    {
      double point = i;
      time_array[i] = point / sampling_frequency;
    }
  size_t n_rows, n_cols;
  double *time_matrix =
    adftool_file_generate_matrix (time_array, &n_rows, &n_cols);
  if (time_matrix == NULL)
    {
      error = 1;
      goto cleanup_times;
    }
  error = adftool_eeg_set_data (f, n_rows, n_cols, time_matrix);
  if (error != 0)
    {
      goto cleanup_matrix;
    }
  static const char *short_names[] = {
    "Fp2", "C4", "O2", "T4", "Fp1", "C3", "O1", "T3", "Cz", "Fz", "Pz"
  };
  assert (sizeof (short_names) / sizeof (short_names[0]) == n_cols);
  for (size_t i = 0; i < n_cols; i++)
    {
      struct adftool_term *term = term_alloc ();
      if (term == NULL)
	{
	  error = 1;
	  goto cleanup_term;
	}
      error = adftool_find_channel_identifier (f, i, term);
      if (error)
	{
	  goto cleanup_term;
	}
      static const char *lyto = "https://localhost/lytonepal#";
      char *id = malloc (strlen (lyto) + strlen (short_names[i]) + 1);
      if (id == NULL)
	{
	  error = 1;
	  goto cleanup_term;
	}
      strcpy (id, lyto);
      strcat (id, short_names[i]);
      struct adftool_term *type = term_alloc ();
      if (type == NULL)
	{
	  error = 1;
	  goto cleanup_id;
	}
      term_set_named (type, id);
      error = adftool_add_channel_type (f, term, type);
      if (error)
	{
	  goto cleanup_type;
	}
    cleanup_type:
      term_free (type);
    cleanup_id:
      free (id);
    cleanup_term:
      term_free (term);
      if (error)
	{
	  goto cleanup_matrix;
	}
    }
cleanup_matrix:
  free (time_matrix);
cleanup_times:
  free (time_array);
cleanup:
  if (error)
    {
      adftool_file_free (f);
      f = NULL;
    }
  return f;
}

#endif /* not H_ADFTOOL_GENERATE_INCLUDED */
