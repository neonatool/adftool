import { Uint64TArray, with_uint64_t_array } from './adftool_uint64_t_array.mjs';
import { PointerArray, with_pointer_array } from './adftool_pointer_array.mjs';
import { Term, with_term } from './adftool_term.mjs';
import { with_string } from './adftool_string.mjs';
import Adftool from './adftool_binding.mjs';

const adftool_statement_alloc = Adftool.cwrap (
    'adftool_statement_alloc',
    '*',
    []);

const  adftool_statement_free = Adftool.cwrap (
    'adftool_statement_free',
    null,
    ['*']);

const adftool_statement_set = (() => {
    const impl = Adftool.cwrap (
	'adftool_statement_set',
	null,
	['*',
	 '*', '*', '*', '*',
	 '*']);
    return (statement, subject, predicate, object, graph, deletion_date) => {
	const once_date_set = (date_ptr) => {
	    with_pointer_array (4, (a) => {
		const addresses = [a.address(0), a.address(1),
				   a.address(2), a.address(3)];
		[subject, predicate, object, graph].forEach ((term, i) => {
		    if (term === false) {
			/* Delete that term => set it to an allocated NULL. */
			a.set (i, 0);
		    } else if (!term) {
			/* null, or undefined: we don’t want to set
			 * that term. */
			addresses[i] = 0;
		    } else {
			a.set (i, term);
		    }
		});
		return impl (statement, ...addresses, date_ptr);
	    });
	};
	return with_uint64_t_array (1, (a) => {
	    let address = a.address (0);
	    if (deletion_date === false) {
		a.set (0, 4294967295, 4294967295);
	    } else if (!deletion_date) {
		address = 0;
	    } else {
		a.set (0, 0, deletion_date);
	    }
	    return once_date_set (address);
	});
    };
})();

const adftool_statement_get = (() => {
    const impl = Adftool.cwrap (
	'adftool_statement_get',
	null,
	['*',
	 '*', '*', '*', '*',
	 '*']);
    return (statement, f) => {
	return with_uint64_t_array (1, (d) => {
	    return with_pointer_array (4, (terms) => {
		impl (statement,
		      terms.address (0),
		      terms.address (1),
		      terms.address (2),
		      terms.address (3),
		      d.address (0));
		const h = d.get_high (0);
		const l = d.get_low (0);
		const whole = {};
		if (h != 4294967295 && l != 4294967295) {
		    whole['deletion_date'] = h * 4294967296 + l;
		}
		if (terms.get (0) != 0) {
		    whole.subject = new Term (terms.get (0));
		}
		if (terms.get (1) != 0) {
		    whole.predicate = new Term (terms.get (1));
		}
		if (terms.get (2) != 0) {
		    whole.object = new Term (terms.get (2));
		}
		if (terms.get (3) != 0) {
		    whole.graph = new Term (terms.get (3));
		}
		return f (whole);
	    });
	});
    };
})();

const adftool_statement_copy =
      Adftool.cwrap ('adftool_statement_copy',
		     null, ['*', '*']);

const adftool_statement_compare = (() => {
    const impl =
	  Adftool.cwrap ('adftool_statement_compare',
			 'number', ['*', '*', '*']);
    return (s1, s2, order) => {
	return with_string (order, (ptr) => {
	    return impl (s1, s2, ptr);
	});
    };
})();

export class Statement {
    constructor () {
	this._t = adftool_statement_alloc ();
    }
    set (subject, predicate, object, graph, deletion_date) {
	/* The pattern (thing && call(thing)) returns false if thing
	 * is false, null or undefined if thing is null or undefined,
	 * or call(thing) if thing is truthy. */
	const date = deletion_date && (deletion_date - new Date (0));
	adftool_statement_set (this._t,
			       (subject && subject._t),
			       (predicate && predicate._t),
			       (object && object._t),
			       (graph && graph._t),
			       date);
    }
    get (f) {
	return adftool_statement_get (this._t, f);
    }
    copy (other) {
	adftool_statement_copy (this._t, other._t);
    }
    compare (other, order) {
	return adftool_statement_compare (this._t, other._t, order);
    }
    _destroy () {
	adftool_statement_free (this._t);
    }
}

export function with_statement (f) {
    const t = new Statement();
    try {
	return f (t);
    } finally {
	t._destroy();
    }
}

export function with_statement_init (subject, predicate, object, graph, deletion_date, f) {
    return with_statement ((t) => {
	t.set (subject, predicate, object, graph, deletion_date);
	return f (t);
    });
}

export function with_statement_copy (original, f) {
    return with_statement ((statement) => {
	statement.copy (original);
	return f (statement);
    });
}

export function with_n_statements (n, f) {
    return with_pointer_array (n, (pa) => {
	const objects = [];
	try {
	    for (let i = 0; i < n; i++) {
		const s = new Statement ();
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
