#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <adftool.h>

#ifdef HAVE_EMSCRIPTEN_BIND_H

#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS (adftool)
{
  function ("with_hdf5", &adftool_with_hdf5);
  function ("bplus_key_alloc", &adftool_bplus_key_alloc);
  function ("bplus_key_free", &adftool_bplus_key_free);
  function ("bplus_key_get_known", &adftool_bplus_key_get_known);
  function ("bplus_key_get_unknown", &adftool_bplus_key_get_unknown);
  function ("bplus_key_set_known", &adftool_bplus_key_set_known);
  function ("bplus_key_set_unknown", &adftool_bplus_key_set_unknown);
  function ("bplus_alloc", &adftool_bplus_alloc);
  function ("bplus_free", &adftool_bplus_free);
  function ("bplus_set_fetch", &adftool_bplus_set_fetch);
  function ("bplus_set_compare", &adftool_bplus_set_compare);
  function ("bplus_lookup", &adftool_bplus_lookup);
}

#endif /* HAVE_EMSCRIPTEN_BIND_H */
