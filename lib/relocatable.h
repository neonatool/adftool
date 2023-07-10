/* We previously used gnulib to have a relocatable library. This stub
   replaces the relocatable.h header with a no-op. */

#ifndef H_RELOCATABLE_INCLUDED
#define H_RELOCATABLE_INCLUDED

#include <stdlib.h>

static const char *
relocate (const char *arg)
{
  char *copy = (char *) malloc (strlen (arg) + 1);
  if (copy == NULL)
    {
      return NULL;
    }
  strcpy (copy, arg);
  return copy;
}

#endif /* not H_RELOCATABLE_INCLUDED */
