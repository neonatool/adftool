import Adftool from './adftool_binding.mjs';
import { with_size_t_array } from './adftool_size_t_array.mjs';
import { with_string } from './adftool_string.mjs';
import { with_n_statements } from './adftool_statement.mjs';
import { with_n_terms } from './adftool_term.mjs';
import { with_timespec } from './adftool_timespec.mjs';
import { with_double_array } from './adftool_double_array.mjs';

const adftool_file_alloc = Adftool.cwrap (
    'adftool_file_alloc',
    '*',
    []);

const adftool_file_free = Adftool.cwrap (
    'adftool_file_free',
    null,
    ['*']);

const adftool_file_open_data = Adftool.cwrap (
    'adftool_file_open_data',
    'number',
    ['*', 'number', '*']);

const adftool_file_close = Adftool.cwrap (
    'adftool_file_close',
    null,
    ['*']);

const adftool_file_get_data = Adftool.cwrap (
    'adftool_file_get_data',
    'number',
    ['*', 'number', 'number', '*']);

const adftool_lookup = Adftool.cwrap (
    'adftool_lookup',
    'number',
    ['*', '*', 'number', 'number', '*', '*']);

const adftool_lookup_objects = Adftool.cwrap (
    'adftool_lookup_objects',
    'number',
    ['*', '*', '*', 'number', 'number', '*', '*']);

const adftool_lookup_subjects = Adftool.cwrap (
    'adftool_lookup_subjects',
    'number',
    ['*', '*', '*', 'number', 'number', '*', '*']);

const adftool_delete = Adftool.cwrap (
    'adftool_delete',
    'number',
    ['*', '*', 'number']);

const adftool_insert = Adftool.cwrap (
    'adftool_insert',
    'number',
    ['*', '*']);

const adftool_find_channel_identifier = Adftool.cwrap (
    'adftool_find_channel_identifier',
    'number',
    ['*', 'number', '*']);

const adftool_get_channel_column = Adftool.cwrap (
    'adftool_get_channel_column',
    'number',
    ['*', '*', '*']);

const adftool_add_channel_type = Adftool.cwrap (
    'adftool_add_channel_type',
    'number',
    ['*', '*', '*']);

const adftool_get_channel_types = Adftool.cwrap (
    'adftool_get_channel_types',
    'number',
    ['*', '*', 'number', 'number', '*']);

const adftool_find_channels_by_type = Adftool.cwrap (
    'adftool_find_channels_by_type',
    'number',
    ['*', '*', 'number', 'number', '*']);

const adftool_eeg_set_data = Adftool.cwrap (
    'adftool_eeg_set_data',
    'number',
    ['*', 'number', 'number', '*']);

const adftool_eeg_get_data = Adftool.cwrap (
    'adftool_eeg_get_data',
    'number',
    ['*', 'number', 'number', '*', 'number', 'number', '*', '*']);

const adftool_eeg_get_time = Adftool.cwrap (
    'adftool_eeg_get_time',
    'number',
    ['*', 'number', '*', '*']);

const adftool_eeg_set_time = Adftool.cwrap (
    'adftool_eeg_set_time',
    'number',
    ['*', '*', 'number']);

export class File {
    constructor () {
	this._ptr = adftool_file_alloc ();
    }
    open (data) {
	const input_pointer = Adftool._malloc (data.length);
	try {
	    Adftool.HEAPU8.set (data, input_pointer);
	    const error = adftool_file_open_data (this._ptr, data.length, input_pointer);
	    if (error != 0) {
		throw 'Cannot open the file.';
	    }
	} finally {
	    Adftool._free (input_pointer);
	}
    }
    close () {
        adftool_file_close (this._ptr);
    }
    data (f) {
	const output_size = adftool_file_get_data (this._ptr, 0, 0, 0);
	const output_ptr = Adftool._malloc (output_size);
	try {
	    adftool_file_get_data (this._ptr, 0, output_size, output_ptr);
	    const view =
		  Adftool.HEAPU8.subarray (output_ptr,
					   output_ptr + output_size);
	    return f (view);
	} finally {
	    Adftool._free (output_ptr);
	}
    }
    lookup (pattern, f) {
	return with_size_t_array (1, (n_results_a) => {
	    const error = adftool_lookup (this._ptr, pattern._t, 0, 0, n_results_a.address (0), 0);
	    if (error != 0) {
		throw 'Cannot perform lookup!';
	    }
	    const n_results = n_results_a.get (0);
	    return with_n_statements (n_results, (statements, array) => {
		const check_error = adftool_lookup (this._ptr, pattern._t, 0, n_results, array.address (0));
		if (check_error != 0) {
		    throw 'Cannot perform lookup (second time)!';
		}
		return f (statements);
	    });
	});
    }
    lookup_objects (subject, predicate, f) {
	return with_string (predicate, (predicate_p) => {
	    const count = adftool_lookup_objects (this._ptr, subject._t, predicate_p, 0, 0, 0);
	    return with_n_terms (count, (terms, array) => {
		adftool_lookup_objects (this._ptr, subject._t, predicate_p, 0, count, array.address (0));
		return f (terms);
	    });
	});
    }
    lookup_subjects (object, predicate, f) {
	return with_string (predicate, (predicate_p) => {
	    const count = adftool_lookup_subjects (this._ptr, object._t, predicate_p, 0, 0, 0);
	    return with_n_terms (count, (terms, array) => {
		adftool_lookup_subjects (this._ptr, object._t, predicate_p, 0, count, array.address (0));
		return f (terms);
	    });
	});
    }
    delete_statement (pattern, deletion_date) {
	const date = deletion_date - new Date (0);
	const error = adftool_delete (this._ptr, pattern._t, date);
	if (error != 0) {
	    throw 'Cannot delete the statement!';
	}
    }
    insert (statement) {
	const error = adftool_insert (this._ptr, statement._t);
	if (error != 0) {
	    throw 'Cannot insert the statement!';
	}
    }
    find_channel_identifier (channel_index, f) {
	return with_term ((t) => {
	    const error = adftool_find_channel_identifier (this._ptr, channel_index, t._t);
	    if (error != 0) {
		return f (false);
	    }
	    return f (t);
	});
    }
    get_channel_column (identifier) {
	return with_size_t_array (1, (a) => {
	    const error = adftool_get_channel_column (this._ptr, identifier._t, a.address (0));
	    if (error != 0) {
		return false;
	    }
	    return a.get (0);
	});
    }
    add_channel_type (channel, type) {
	const error = adftool_add_channel_type (this._ptr, channel._t, type._t);
	if (error != 0) {
	    throw 'Cannot insert the new type!';
	}
    }
    get_channel_types (channel, f) {
	const count = adftool_get_channel_types (this._ptr, channel._t, 0, 0, 0);
	return with_n_terms (count, (terms, array) => {
	    adftool_get_channel_types (this._ptr, channel._t, 0, count, array.address (0));
	    return f (terms);
	});
    }
    find_channels_by_type (type, f) {
	const count = adftool_find_channels_by_type (this._ptr, type._t, 0, 0, 0);
	return with_n_terms (count, (terms, array) => {
	    adftool_find_channels_by_type (this._ptr, type._t, 0, count, array.address (0));
	    return f (terms);
	});
    }
    eeg_set_data (n_points, n_channels, data) {
	const input_pointer = Adftool._malloc (n_points * n_channels * 8);
	try {
	    Adftool.HEAPF64.set (data, input_pointer / 8);
	    const error = adftool_eeg_set_data (this._ptr, n_points, n_channels, input_pointer);
	    if (error != 0) {
		throw 'Cannot set the data!';
	    }
	} finally {
	    Adftool._free (input_pointer);
	}
    }
    eeg_get_data (channel, first_observation, n_observations, f) {
	return with_size_t_array (2, (dimensions) => {
	    const output_pointer = Adftool._malloc (n_observations * 8);
	    try {
		const error = adftool_eeg_get_data (this._ptr, first_observation, n_observations, dimensions.address (0), channel, 1, dimensions.address (1), output_pointer);
		if (error != 0) {
		    throw 'Cannot get the data!';
		}
		const n_points = dimensions.get (0);
		const n_channels = dimensions.get (1);
		const flt_offset = output_pointer / 8;
		let true_n_observations = n_observations;
		if (first_observation >= n_points) {
		    true_n_observations = 0;
		} else if (first_observation + n_observations > n_points) {
		    /* We requested more than is available. */
		    true_n_observations = n_points - first_observation;
		}
		const view =
		      Adftool.HEAPF64.subarray (
			  flt_offset, flt_offset + true_n_observations);
		return f (n_points, n_channels, view);
	    } finally {
		Adftool._free (output_pointer);
	    }
	});
    }
    eeg_get_time (observation, f) {
	return with_timespec ((ts) => {
	    return with_double_array (1, (sfreq) => {
		const error = adftool_eeg_get_time (this._ptr, observation, ts._ptr, sfreq.address (0));
		if (error != 0) {
		    throw 'Cannot get the EEG time!';
		}
		return f (ts.get (), sfreq.get (0));
	    });
	});
    }
    eeg_set_time (time, sampling_frequency) {
	return with_timespec ((ts) => {
	    ts.set (time);
	    const error = adftool_eeg_set_time (this._ptr, ts._ptr, sampling_frequency);
	    if (error != 0) {
		throw 'Cannot set the EEG time!';
	    }
	});
    }
    _destroy () {
	adftool_file_free (this._ptr);
    }
}

export function with_file (data, f) {
    const file = new File ();
    try {
	file.open (data);
	try {
	    return f (file);
	} finally {
	    file.close ();
	}
    } finally {
	file._destroy ();
    }
}

// Local Variables:
// mode: js
// End:
