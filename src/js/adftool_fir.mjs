import Adftool from './adftool_binding.mjs'

const adftool_fir_alloc = Adftool.cwrap (
    'adftool_fir_alloc',
    '*',
    ['number', 'number']);

const adftool_fir_order = Adftool.cwrap (
    'adftool_fir_order',
    'number',
    ['*']);

const adftool_fir_free = Adftool.cwrap (
    'adftool_fir_free',
    null,
    ['*']);

const adftool_fir_design_bandpass = Adftool.cwrap (
    'adftool_fir_design_bandpass',
    null,
    ['*', 'number', 'number']);

const adftool_fir_apply = (() => {
    const impl = Adftool.cwrap (
	'adftool_fir_apply',
	null,
	['*', 'number', '*', '*']);
    return (filter, signal, f) => {
	const input_pointer = Adftool._malloc (signal.length * 8);
	try {
	    Adftool.HEAPF64.set(signal, input_pointer / 8);
	    const output_pointer = Adftool._malloc (signal.length * 8);
	    try {
		impl (filter, signal.length, input_pointer, output_pointer);
		const view =
		      Adftool.HEAPF64.subarray (output_pointer / 8,
						output_pointer / 8 + signal.length);
		return f (view);
	    } finally {
		Adftool._free (output_pointer);
	    }
	} finally {
	    Adftool._free (input_pointer);
	}
    };
}) ();

export class Fir {
    constructor (sfreq, transition_bandwidth) {
	this._ptr = adftool_fir_alloc (sfreq, transition_bandwidth);
    }
    order () {
	return adftool_fir_order (this._ptr);
    }
    design_bandpass (freq_low, freq_high) {
	adftool_fir_design_bandpass (this._ptr, freq_low, freq_high);
    }
    apply (signal, f) {
	adftool_fir_apply (this._ptr, signal, f);
    }
    _destroy () {
	adftool_fir_free (this._ptr);
    }
}

export function with_fir (sfreq, transition_bandwidth, f) {
    const fir = new Fir (sfreq, transition_bandwidth);
    try {
	return f (fir);
    } finally {
	fir._destroy ();
    }
}

export function with_bandpass (sfreq, transition_bandwidth, freq_low, freq_high, f) {
    return with_fir (sfreq, transition_bandwidth, (fir) => {
	fir.design_bandpass (freq_low, freq_high);
	f (fir);
    });
}

export function with_bandpassed (sfreq, transition_bandwidth, freq_low, freq_high, signal, f) {
    return with_bandpass (sfreq, transition_bandwidth, freq_low, freq_high, (fir) => {
	return fir.apply (signal, (output) => f (output, fir));
    });
}

// Local Variables:
// mode: js
// End:
