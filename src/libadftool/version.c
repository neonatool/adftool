#include <config.h>
#include <attribute.h>
#include <adftool.h>
#include "gettext.h"
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
