import { factory } from './adftool_load_binding.mjs';

export default function () {
    return factory.then (async _ => {
	const { with_timespec } = await import('./adftool_timespec.mjs');
	const { with_long_array } = await import('./adftool_long_array.mjs');
	const { with_size_t_array } = await import('./adftool_size_t_array.mjs');
	const { with_double_array } = await import('./adftool_double_array.mjs');
	const { with_uint64_t_array } = await import('./adftool_uint64_t_array.mjs');
	const { with_pointer_array } = await import('./adftool_pointer_array.mjs');
	const {
	    with_term, with_blank_node, with_named_node,
	    with_literal_node, with_literal_integer,
	    with_literal_double, with_literal_date,
	    with_n3, with_term_copy, with_n_terms
	} = await import('./adftool_term.mjs');
	const { with_string } = await import('./adftool_string.mjs');
	const { with_statement, with_statement_init,
		with_statement_copy, with_n_statements } = await import('./adftool_statement.mjs');
	const { with_fir, with_bandpass, with_bandpassed } = await import('./adftool_fir.mjs');
	const { File, with_file, with_generated_file } = await import('./adftool_file.mjs');
	const { lytonepal } = await import('./adftool_lytonepal.mjs');
	return {
	    File: File,
	    with_bandpass: with_bandpass,
	    with_bandpassed: with_bandpassed,
	    with_blank_node: with_blank_node,
	    with_double_array: with_double_array,
	    with_file: with_file,
	    with_generated_file: with_generated_file,
	    with_fir: with_fir,
	    with_literal_date: with_literal_date,
	    with_literal_double: with_literal_double,
	    with_literal_integer: with_literal_integer,
	    with_literal_node: with_literal_node,
	    with_long_array: with_long_array,
	    with_n3: with_n3,
	    with_n_statements: with_n_statements,
	    with_n_terms: with_n_terms,
	    with_named_node: with_named_node,
	    with_pointer_array: with_pointer_array,
	    with_size_t_array: with_size_t_array,
	    with_statement: with_statement,
	    with_statement_copy: with_statement_copy,
	    with_statement_init: with_statement_init,
	    with_string: with_string,
	    with_term: with_term,
	    with_term_copy: with_term_copy,
	    with_timespec: with_timespec,
	    with_uint64_t_array: with_uint64_t_array,
            lytonepal: lytonepal
	};
    });
}

// Local Variables:
// mode: js
// End:
