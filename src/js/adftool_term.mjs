import { Timespec, with_timespec } from './adftool_timespec.mjs';
import { LongArray, with_long_array } from './adftool_long_array.mjs';
import { SizeTArray, with_size_t_array } from './adftool_size_t_array.mjs';
import { DoubleArray, with_double_array } from './adftool_double_array.mjs';
import { with_pointer_array } from './adftool_pointer_array.mjs';
import { with_string } from './adftool_string.mjs';
import Adftool from './adftool_binding.mjs';

const adftool_term_alloc = Adftool.cwrap (
    'adftool_term_alloc',
    '*',
    []);

const  adftool_term_free = Adftool.cwrap (
    'adftool_term_free',
    null,
    ['*']);

const adftool_term_copy = Adftool.cwrap (
    'adftool_term_copy',
    null,
    ['*', '*']);

const adftool_term_set_blank = Adftool.cwrap (
    'adftool_term_set_blank',
    null,
    ['*', 'string']);

const adftool_term_set_named = Adftool.cwrap (
    'adftool_term_set_named',
    null,
    ['*', 'string']);

const adftool_term_set_literal = (() => {
    const only_value = Adftool.cwrap (
	'adftool_term_set_literal',
	null,
	['*', 'string', '*', '*']);
    const with_type = Adftool.cwrap (
	'adftool_term_set_literal',
	null,
	['*', 'string', 'string', '*']);
    const with_langtag = Adftool.cwrap (
	'adftool_term_set_literal',
	null,
	['*', 'string', '*', 'string']);
    return (t, value, type, langtag) => {
	if (type) {
	    return with_type (t, value, type, null);
	} else if (langtag) {
	    return with_langtag (t, value, null, langtag);
	} else {
	    return only_value (t, value, null, null);
	}
    };
})();

const adftool_term_set_integer = Adftool.cwrap (
    'adftool_term_set_integer',
    null,
    ['*', 'number']);

const adftool_term_set_double = Adftool.cwrap (
    'adftool_term_set_double',
    null,
    ['*', 'number']);

const adftool_term_set_date = (() => {
    const impl = Adftool.cwrap (
	'adftool_term_set_date',
	null,
	['*', '*']);
    return (t, date) => {
	return with_timespec ((ts) => {
	    ts.set (date);
	    return impl (t, ts._ptr);
	});
    };
})();

function wrap_type_checker (name) {
    const impl = Adftool.cwrap (
	`adftool_term_is_${name}`,
	'number',
	['*']);
    return (t) => {
	const ret = impl (t);
	return (ret != 0);
    };
}

const adftool_term_is_typed_literal = wrap_type_checker ('typed_literal');
const adftool_term_is_langstring = wrap_type_checker ('langstring');
const adftool_term_is_literal = wrap_type_checker ('literal');
const adftool_term_is_named = wrap_type_checker ('named');
const adftool_term_is_blank = wrap_type_checker ('blank');

function wrap_string_output (name) {
    const impl = Adftool.cwrap (
	`adftool_term_${name}`,
	'number',
	['*', 'number', 'number', '*']);
    return (t) => {
	const size = impl (t, 0, 0, 0);
	const mem = Adftool._malloc (size + 1);
	const check = impl (t, 0, size + 1, mem);
	if (check != size) {
	    throw "Impossible";
	}
	const js = Adftool.UTF8ToString (mem);
	Adftool._free (mem);
	return js;
    };
}

const adftool_term_value = wrap_string_output ('value');
const adftool_term_meta = wrap_string_output ('meta');
const adftool_term_to_n3 = wrap_string_output ('to_n3');

const adftool_term_as_integer = (() => {
    const impl = Adftool.cwrap (
	'adftool_term_as_integer',
	'number',
	['*', '*']);
    return (term) => {
	return with_long_array (1, (a) => {
	    const ptr = a.address (0);
	    const ret = impl (term, ptr);
	    if (ret == 0) {
		return a.get (0);
	    }
	    return null;
	});
    };
})();

const adftool_term_as_double = (() => {
    const impl = Adftool.cwrap (
	'adftool_term_as_double',
	'number',
	['*', '*']);
    return (term) => {
	return with_double_array (1, (a) => {
	    const ptr = a.address (0);
	    const ret = impl (term, ptr);
	    if (ret == 0) {
		return a.get (0);
	    }
	    return null;
	});
    };
})();

const adftool_term_as_date = (() => {
    const impl = Adftool.cwrap (
	'adftool_term_as_date',
	'number',
	['*', '*']);
    return (term) => {
	return with_timespec ((ts) => {
	    const ret = impl (term, ts._ptr);
	    if (ret == 0) {
		return ts.get ();
	    }
	    return null;
	});
    };
})();

const adftool_term_compare = Adftool.cwrap (
    'adftool_term_compare',
    'number',
    ['*', '*']);

const adftool_term_parse_n3 = (() => {
    const impl = Adftool.cwrap (
	'adftool_term_parse_n3',
	'number',
	['*', 'number', '*', '*']);
    return (str, term) => {
	return with_string (str, (ptr, size) => {
	    return with_size_t_array (1, (a) => {
		const error = impl (ptr, size, a.address (0), term);
		if (error == 0) {
		    const consumed = a.get (0);
		    const remaining = ptr + consumed;
		    return Adftool.UTF8ToString (remaining, size - consumed);
		} else {
		    return null;
		}
	    });
	});
    };
})();

export class Term {
    constructor (ptr) {
	if (ptr) {
	    this._t = ptr;
	    this.owned = false;
	} else {
	    this._t = adftool_term_alloc ();
	    this.owned = true;
	}
    }
    copy (other) {
	adftool_term_copy (this._t, other._t);
    }
    set_blank (id) {
	adftool_term_set_blank (this._t, id);
    }
    set_named (id) {
	adftool_term_set_named (this._t, id);
    }
    set_literal (value, type, language) {
	adftool_term_set_literal (this._t, value, type, language);
    }
    set_integer (value) {
	adftool_term_set_integer (this._t, value);
    }
    set_double (value) {
	adftool_term_set_double (this._t, value);
    }
    set_date (value) {
	adftool_term_set_date (this._t, value);
    }
    is_blank () {
	return adftool_term_is_blank (this._t);
    }
    is_named () {
	return adftool_term_is_named (this._t);
    }
    is_literal () {
	return adftool_term_is_literal (this._t);
    }
    is_langstring () {
	return adftool_term_is_langstring (this._t);
    }
    is_typed_literal () {
	return adftool_term_is_typed_literal (this._t);
    }
    value () {
	return adftool_term_value (this._t);
    }
    meta () {
	return adftool_term_meta (this._t);
    }
    as_integer () {
	return adftool_term_as_integer (this._t);
    }
    as_double () {
	return adftool_term_as_double (this._t);
    }
    as_date () {
	return adftool_term_as_date (this._t);
    }
    compare (other) {
	return adftool_term_compare (this._t, other._t);
    }
    parse_n3 (text) {
	return adftool_term_parse_n3 (text, this._t);
    }
    to_n3 () {
	return adftool_term_to_n3 (this._t);
    }
    _destroy () {
	if (this.owned) {
	    adftool_term_free (this._t);
	}
    }
}

export function with_term (f) {
    const t = new Term();
    try {
	return f (t);
    } finally {
	t._destroy();
    }
}

export function with_blank_node (id, f) {
    return with_term ((t) => {
	t.set_blank (id);
	return f (t);
    });
}

export function with_named_node (id, f) {
    return with_term ((t) => {
	t.set_named (id);
	return f (t);
    });
}

export function with_literal_node (value, type, langtag, f) {
    return with_term ((t) => {
	t.set_literal (value, type, langtag);
	return f (t);
    });
}

export function with_literal_integer (value, f) {
    return with_term ((t) => {
	t.set_integer (value);
	return f (t);
    });
}

export function with_literal_double (value, f) {
    return with_term ((t) => {
	t.set_double (value);
	return f (t);
    });
}

export function with_literal_date (value, f) {
    return with_term ((t) => {
	t.set_date (value);
	return f (t);
    });
}

export function with_n3 (text, f) {
    return with_term ((t) => {
	const rest = t.parse_n3 (text);
	if (rest === null) {
	    return f (null, text);
	} else {
	    return f (t, rest);
	}
    });
}

export function with_term_copy (original, f) {
    return with_term ((t) => {
	t.copy (original);
	return f (t);
    });
}

export function with_n_terms (n, f) {
    return with_pointer_array (n, (pa) => {
	const objects = [];
	try {
	    for (let i = 0; i < n; i++) {
		const s = new Term ();
		objects.push (s);
		pa.set (i, s._t);
	    }
	    return f (objects, pa);
	} finally {
	    objects.forEach ((s) => s._destroy ());
	}
    });
}

// Local Variables:
// mode: js
// End:
