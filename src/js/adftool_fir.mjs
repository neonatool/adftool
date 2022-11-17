import Adftool from './adftool_binding.mjs'
import { with_double_array} from './adftool_double_array.mjs';

const adftool_fir_auto_bandwidth = (() => {
    const impl = Adftool.cwrap (
	'adftool_fir_auto_bandwidth',
	null,
	['number', 'number', 'number', '*', '*']);
    return (sfreq, freq_low, freq_high) => {
	return with_double_array (2, (a) => {
	    impl (sfreq, freq_low, freq_high, a.address (0), a.address (1));
	    return [ a.get(0), a.get (1) ];
	});
    };
})();

const adftool_fir_auto_order = Adftool.cwrap (
    'adftool_fir_auto_order',
    'number',
    ['number', 'number']);

const adftool_fir_alloc = Adftool.cwrap (
    'adftool_fir_alloc',
    '*',
    ['number']);

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
    ['*', 'number', 'number', 'number', 'number', 'number']);

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
    constructor (order) {
	this._ptr = adftool_fir_alloc (order);
    }
    order () {
	return adftool_fir_order (this._ptr);
    }
    design_bandpass (sfreq, freq_low, freq_high, trans_low, trans_high) {
	adftool_fir_design_bandpass (this._ptr, sfreq, freq_low, freq_high, trans_low, trans_high);
    }
    apply (signal, f) {
	return adftool_fir_apply (this._ptr, signal, f);
    }
    _destroy () {
	adftool_fir_free (this._ptr);
    }
    static auto_bandwidth (sfreq, freq_low, freq_high) {
	return adftool_fir_auto_bandwidth (sfreq, freq_low, freq_high);
    }
    static auto_order (sfreq, tightest_transition_bandwidth) {
	return adftool_fir_auto_order (sfreq, tightest_transition_bandwidth);
    }
}

export function with_fir (order, f) {
    const fir = new Fir (order);
    try {
	return f (fir);
    } finally {
	fir._destroy ();
    }
}

export function with_bandpass (sfreq, freq_low, freq_high, f) {
    const [trans_low, trans_high] = Fir.auto_bandwidth (sfreq, freq_low, freq_high);
    let tightest = trans_low;
    if (trans_high < tightest) {
	tightest = trans_high;
    }
    const order = Fir.auto_order (sfreq, tightest);
    return with_fir (order, (fir) => {
	fir.design_bandpass (sfreq, freq_low, freq_high, trans_low, trans_high);
	return f (fir);
    });
}

export function with_bandpassed (sfreq, freq_low, freq_high, signal, f) {
    return with_bandpass (sfreq, freq_low, freq_high, (fir) => {
	return fir.apply (signal, (output) => f (output, fir));
    });
}

// Local Variables:
// mode: js
// End:
