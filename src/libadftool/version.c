#include <adftool_private.h>
#include "relocatable.h"

void
_adftool_ensure_init (void)
{
  static volatile int is_initialized = 0;
  if (!is_initialized)
    {
      bindtextdomain (PACKAGE, relocate (LOCALEDIR));
      is_initialized = 1;
    }
}

int
adftool_with_hdf5 (void)
{
#ifdef LIBADFTOOL_WITHOUT_HDF5
  return 0;
#else /* not LIBADFTOOL_WITHOUT_HDF5 */
  return 1;
#endif /* not LIBADFTOOL_WITHOUT_HDF5 */
}
