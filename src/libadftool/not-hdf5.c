#include <adftool_private.h>

#include <stdio.h>

#define _(String) dgettext (PACKAGE, String)
#define N_(String) (String)

void
adftool_bplus_parameters_set_fetch_from_hdf5 (struct adftool_bplus_parameters
					      *parameters, hid_t dataset)
{
  (void) parameters;
  (void) dataset;
  ensure_init ();
  fprintf (stderr, _("Error: adftool has not been compiled with HDF5.\n\
This was a bad idea and now you understand why."));
}
