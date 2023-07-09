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

size_t
adftool_lytonepal (const char *cncept, size_t start, size_t max, char *dest)
{
  const size_t length = strlen (LYTONEPAL_ONTOLOGY_PREFIX) + strlen (cncept);
  for (size_t i = 0; i < length + 1; i++)
    {
      char next = '\0';
      if (i < strlen (LYTONEPAL_ONTOLOGY_PREFIX))
	{
	  next = LYTONEPAL_ONTOLOGY_PREFIX[i];
	}
      else if (i < length)
	{
	  next = cncept[i - strlen (LYTONEPAL_ONTOLOGY_PREFIX)];
	}
      if (i >= start && i < start + max)
	{
	  dest[i - start] = next;
	}
    }
  return length;
}
