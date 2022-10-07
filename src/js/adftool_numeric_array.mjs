import Adftool from './adftool_binding.mjs'

export default function (type) {
    const adftool_array_numeric_alloc = Adftool.cwrap (
	`adftool_array_${type}_alloc`,
	'*',
	['number']);
    const adftool_array_numeric_free = Adftool.cwrap (
	`adftool_array_${type}_free`,
	null,
	['*']);
    const adftool_array_numeric_address = Adftool.cwrap (
	`adftool_array_${type}_address`,
	'*',
	['*', 'number']);
    const adftool_array_numeric_set = Adftool.cwrap (
	`adftool_array_${type}_set`,
	null,
	['*', 'number', 'number']);
    const adftool_array_numeric_get = Adftool.cwrap (
	`adftool_array_${type}_get`,
	'number',
	['*', 'number']);
    class NumericArray {
	constructor (n) {
	    this._ptr = adftool_array_numeric_alloc (n);
	}
	address (i) {
	    return adftool_array_numeric_address (this._ptr, i);
	}
	set (i, value) {
	    adftool_array_numeric_set (this._ptr, i, value);
	}
	get (i) {
	    return adftool_array_numeric_get (this._ptr, i);
	}
	_destroy () {
	    adftool_array_numeric_free (this._ptr);
	}
    }
    const with_numeric_array = (n, f) => {
	const a = new NumericArray (n);
	try {
	    return f (a);
	} finally {
	    a._destroy ();
	}
    }
    return { "type": NumericArray, "wrapper": with_numeric_array };
}

// Local Variables:
// mode: js
// End:
