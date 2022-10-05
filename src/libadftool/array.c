#include <adftool_private.h>
#include <time.h>

#define ARRAY_TYPE(type, type_in_fname)                         \
  struct adftool_array_##type_in_fname                          \
  {                                                             \
    type *container;                                            \
  };                                                            \
  struct adftool_array_##type_in_fname *                        \
  adftool_array_##type_in_fname##_alloc (size_t n_elements)     \
  {                                                             \
    struct adftool_array_##type_in_fname *ret =                 \
      malloc (sizeof (struct adftool_array_##type_in_fname));   \
    if (ret != NULL)                                            \
      {                                                         \
        ret->container = malloc (n_elements * sizeof (type));   \
        if (ret->container == NULL)                             \
          {                                                     \
            free (ret);                                         \
            ret = NULL;                                         \
          }                                                     \
      }                                                         \
    return ret;                                                 \
  }                                                             \
  void                                                          \
  adftool_array_##type_in_fname##_free                          \
  (struct adftool_array_##type_in_fname *array)                 \
  {                                                             \
    if (array != NULL)                                          \
      {                                                         \
        free (array->container);                                \
      }                                                         \
    free (array);                                               \
  }                                                             \
  type *                                                        \
  adftool_array_##type_in_fname##_address                       \
  (struct adftool_array_##type_in_fname *array,                 \
   size_t i)                                                    \
  {                                                             \
    return & (array->container[i]);                             \
  }                                                             \
  void                                                          \
  adftool_array_##type_in_fname##_set                           \
  (struct adftool_array_##type_in_fname *array,                 \
   size_t i,                                                    \
   type value)                                                  \
  {                                                             \
    *adftool_array_##type_in_fname##_address (array, i)         \
      = value;                                                  \
  }                                                             \
  type                                                          \
  adftool_array_##type_in_fname##_get                           \
  (const struct adftool_array_##type_in_fname *array,           \
   size_t i)                                                    \
  {                                                             \
    return *adftool_array_##type_in_fname##_address             \
      ((struct adftool_array_##type_in_fname *) array, i);      \
  }

/* *INDENT-OFF* */
ARRAY_TYPE (uint8_t, byte)
ARRAY_TYPE (size_t, size_t)
ARRAY_TYPE (void *, pointer)
ARRAY_TYPE (uint64_t, uint64)
ARRAY_TYPE (long, long)
ARRAY_TYPE (double, double)
/* *INDENT-ON* */

struct timespec *
adftool_timespec_alloc (void)
{
  return malloc (sizeof (struct timespec));
}

void
adftool_timespec_free (struct timespec *time)
{
  free (time);
}

void
adftool_timespec_set_js (struct timespec *time, double milliseconds)
{
  const double seconds = milliseconds / 1000;
  const time_t entire_seconds = seconds;
  const long remainder = milliseconds - (entire_seconds * 1000);
  time->tv_sec = entire_seconds;
  time->tv_nsec = remainder * 1000000;
}

double
adftool_timespec_get_js (const struct timespec *time)
{
  const double seconds = time->tv_sec;
  const double nanoseconds = time->tv_nsec;
  const double milliseconds = nanoseconds / 1000000;
  return seconds * 1000 + milliseconds;
}
