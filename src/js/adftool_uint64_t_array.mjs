import gen_numeric_array from './adftool_numeric_array.mjs'
import Adftool from './adftool_binding.mjs';

const specialized = gen_numeric_array('uint64_t');

const Base = specialized.type;

function specialized_getter (what) {
    const impl =
	  Adftool.cwrap (`adftool_array_uint64_t_get_js_${what}`,
			 'number',
			 ['*', 'number']);
    return (ptr, i) => {
	return impl (ptr, i);
    };
}

const adftool_get_low = specialized_getter ('low');
const adftool_get_high = specialized_getter ('high');
const adftool_set_js =
      Adftool.cwrap ('adftool_array_uint64_t_set_js',
		     null,
		     ['*', 'number', 'number', 'number']);

export class Uint64TArray extends Base {
    constructor (n) {
	super (n);
    }
    get_low (i) {
	return adftool_get_low (this._ptr, i);
    }
    get_high (i) {
	return adftool_get_high (this._ptr, i);
    }
    set (i, high, low) {
	return adftool_set_js (this._ptr, i, high, low);
    }
}

export function with_uint64_t_array (n, f) {
    const a = new Uint64TArray (n);
    try {
	return f (a);
    } finally {
	a._destroy ();
    }
}

// Local Variables:
// mode: js
// End:
