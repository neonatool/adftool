#include <adftool_private.h>
#include <time.h>

#define ARRAY_TYPE(type, type_in_fname)                                 \
  size_t                                                                \
  adftool_sizeof_##type_in_fname (void)                                 \
  {                                                                     \
    return sizeof (type);                                               \
  }                                                                     \
                                                                        \
  type                                                                  \
  adftool_array_get_##type_in_fname (const char *array, size_t i)       \
  {                                                                     \
    type ret;                                                           \
    memcpy (&ret, array + i * sizeof (type), sizeof (type));            \
    return ret;                                                         \
  }                                                                     \
                                                                        \
  void                                                                  \
  adftool_array_set_##type_in_fname (char *array,                       \
                                     size_t i,                          \
                                     type value)                        \
  {                                                                     \
    memcpy (&array + i * sizeof (type), &value, sizeof (type));         \
  }

/* *INDENT-OFF* */
ARRAY_TYPE (uint8_t, byte)
ARRAY_TYPE (size_t, size_t)
ARRAY_TYPE (struct timespec, timespec_)
ARRAY_TYPE (void *, pointer)
ARRAY_TYPE (uint64_t, uint64)
ARRAY_TYPE (long, long)
ARRAY_TYPE (double, double)
/* *INDENT-ON* */

size_t
adftool_sizeof_timespec (void)
{
  return adftool_sizeof_timespec_ ();
}

time_t
adftool_array_get_tv_sec (const char *time_array, size_t i)
{
  struct timespec ts = adftool_array_get_timespec_ (time_array, i);
  return ts.tv_sec;
}

long
adftool_array_get_tv_nsec (const char *time_array, size_t i)
{
  struct timespec ts = adftool_array_get_timespec_ (time_array, i);
  return ts.tv_nsec;
}

void
adftool_array_set_timespec (char *time_array, size_t i, time_t tv_sec,
			    long tv_nsec)
{
  struct timespec ts = {.tv_sec = tv_sec,.tv_nsec = tv_nsec };
  adftool_array_set_timespec_ (time_array, i, ts);
}
