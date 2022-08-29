#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <adftool.h>

#ifdef HAVE_EMSCRIPTEN_BIND_H

#include <emscripten/bind.h>

#define EXPORT(f) function (#f, &f)

EMSCRIPTEN_BINDINGS (adftool)
{
  EXPORT (adftool_file_alloc);
  EXPORT (adftool_file_free);
  EXPORT (adftool_file_open);
  EXPORT (adftool_file_close);
  EXPORT (adftool_file_open_data);
  EXPORT (adftool_file_get_data);
  EXPORT (adftool_term_alloc);
  EXPORT (adftool_term_free);
  EXPORT (adftool_term_copy);
  EXPORT (adftool_term_set_blank);
  EXPORT (adftool_term_set_named);
  EXPORT (adftool_term_set_literal);
  EXPORT (adftool_term_is_blank);
  EXPORT (adftool_term_is_named);
  EXPORT (adftool_term_is_literal);
  EXPORT (adftool_term_is_typed_literal);
  EXPORT (adftool_term_is_langstring);
  EXPORT (adftool_term_value);
  EXPORT (adftool_term_meta);
  EXPORT (adftool_term_compare);
  EXPORT (adftool_term_decode);
  EXPORT (adftool_term_encode);
  EXPORT (adftool_term_parse_n3);
  EXPORT (adftool_statement_alloc);
  EXPORT (adftool_statement_free);
  EXPORT (adftool_statement_set);
  EXPORT (adftool_statement_get);
  EXPORT (adftool_statement_copy);
  EXPORT (adftool_statement_compare);
  EXPORT (adftool_quads_get);
  EXPORT (adftool_quads_delete);
  EXPORT (adftool_quads_insert);
  EXPORT (adftool_results_alloc);
  EXPORT (adftool_results_free);
  EXPORT (adftool_results_count);
  EXPORT (adftool_results_get);
  EXPORT (adftool_results_resize);
  EXPORT (adftool_results_set);
  EXPORT (adftool_lookup);
  EXPORT (adftool_delete);
  EXPORT (adftool_insert);
}

#endif /* HAVE_EMSCRIPTEN_BIND_H */
