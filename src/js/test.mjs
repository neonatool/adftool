import * as Adftool from './adftool.mjs';

console.log('Now:', Adftool.with_literal_date (new Date(Date.now ()), (t) => t.to_n3 ()));

console.log('42 = ', Adftool.with_literal_integer (42, (t) => t.to_n3 ()));
console.log('49.3 = ', Adftool.with_literal_double (49.3, (t) => t.to_n3 ()));

import { strict as assert } from 'node:assert';

const not_n3 = "Hello, world!";
const is_n3 = "<hello>, <world>!";

Adftool.with_n3 (not_n3, (term, rest) => {
    assert.deepStrictEqual (term, null);
    assert.deepStrictEqual (rest, not_n3);
});

Adftool.with_n3 (is_n3, (term, rest) => {
    assert.deepStrictEqual (term.to_n3 (), "<hello>");
    assert.deepStrictEqual (rest, ", <world>!");
});

Adftool.with_n3 ("<a>", (subject) => {
    Adftool.with_n3 ("<b>", (predicate) => {
	Adftool.with_n3 ("<c>", (object) => {
	    Adftool.with_statement_init (subject, predicate, object, null, null, (statement) => {
		/* The statement should have a subject, a predicate,
		 * an object, but no graph and no deletion date. */
		assert.deepStrictEqual (statement.get (s => {
		    assert ('subject' in s);
		    assert.deepStrictEqual (s.subject.to_n3 (), "<a>");
		    assert ('predicate' in s);
		    assert.deepStrictEqual (s.predicate.to_n3 (), "<b>");
		    assert ('object' in s);
		    assert.deepStrictEqual (s.object.to_n3 (), "<c>");
		    assert (! ('graph' in s));
		    assert (! ('deletion_date' in s));
		    return 42;
		}),
					42);
		statement.set (null, null, null, null, new Date(42));
		/* The statement should still have a subject, a
		 * predicate, an object, still no graph, but 42 as the
		 * deletion date. */
		assert.deepStrictEqual (statement.get (s => {
		    assert ('subject' in s);
		    assert.deepStrictEqual (s.subject.to_n3 (), "<a>");
		    assert ('predicate' in s);
		    assert.deepStrictEqual (s.predicate.to_n3 (), "<b>");
		    assert ('object' in s);
		    assert.deepStrictEqual (s.object.to_n3 (), "<c>");
		    assert (! ('graph' in s));
		    assert ('deletion_date' in s);
		    assert.deepStrictEqual (s.deletion_date, 42);
		    return 42;
		}),
					42);
		statement.set (null, null, null, null, false);
		/* The statement should still have a subject, a
		 * predicate, an object, still no graph, and no
		 * deletion date. */
		assert.deepStrictEqual (statement.get (s => {
		    assert ('subject' in s);
		    assert.deepStrictEqual (s.subject.to_n3 (), "<a>");
		    assert ('predicate' in s);
		    assert.deepStrictEqual (s.predicate.to_n3 (), "<b>");
		    assert ('object' in s);
		    assert.deepStrictEqual (s.object.to_n3 (), "<c>");
		    assert (! ('graph' in s));
		    assert (! ('deletion_date' in s));
		    return 42;
		}),
					42);
		statement.set (null, false, null, null, null);
		/* The statement should not have a predicate
		 * anymore. */
		assert.deepStrictEqual (statement.get (s => {
		    assert ('subject' in s);
		    assert.deepStrictEqual (s.subject.to_n3 (), "<a>");
		    assert (! ('predicate' in s));
		    assert ('object' in s);
		    assert.deepStrictEqual (s.object.to_n3 (), "<c>");
		    assert (! ('graph' in s));
		    assert (! ('deletion_date' in s));
		    return 42;
		}),
					42);
	    });
	});
    });
});

Adftool.with_n3 ("<a>", (a) => {
    Adftool.with_n3 ("<b>", (b) => {
	Adftool.with_n3 ("<c>", (c) => {
	    Adftool.with_n3 ("<d>", (d) => {
		Adftool.with_n3 ("<e>", (e) => {
		    Adftool.with_statement_init (a, b, c, null, null, (statement1) => {
			Adftool.with_statement_copy (statement1, (statement2) => {
			    statement1.set (null, null, e);
			    statement2.set (null, c, d);
			    /* 1: a b e */
			    /* 2: a c d */
			    assert (statement1.compare (statement1, "SPOG") == 0);
			    assert (statement1.compare (statement2, "S") == 0);
			    assert (statement2.compare (statement1, "S") == 0);
			    assert (statement1.compare (statement2, "SP") < 0);
			    assert (statement2.compare (statement1, "SP") > 0);
			    assert (statement1.compare (statement2, "SO") > 0);
			    assert (statement2.compare (statement1, "SO") < 0);
			});
		    });
		});
	    });
	});
    });
});

function make_sinusoid (sampling_frequency, sine_frequency, length) {
    const output = new Float64Array (length);
    const period = 1 / sine_frequency;
    for (let i = 0; i < length; i++) {
	const time = i / sampling_frequency;
	output[i] = Math.sin (2 * Math.PI * (time / period));
    }
    return output;
}

const sampling_frequency = 256;
const total_length = 20;

const quick_sine = make_sinusoid (sampling_frequency, 2, total_length * sampling_frequency);
const slow_sine = make_sinusoid (sampling_frequency, 4, total_length * sampling_frequency);

const mix = (() => {
    const output = new Float64Array (total_length * sampling_frequency);
    for (let i = 0; i < output.length; i++) {
	output[i] = quick_sine[i] + slow_sine[i];
    }
    return output;
})();

function count_roots (signal) {
    let last_sign = Math.sign (signal[0]);
    let changes = 0;
    for (let i = 0; i < signal.length; i++) {
	const new_sign = Math.sign (signal[i]);
	if (new_sign * last_sign < 0) {
	    changes++;
	}
	last_sign = new_sign;
    }
    return changes;
}

Adftool.with_bandpassed (sampling_frequency, 0.5, 1, 3, mix, (filter_quick, fir1) => {
    Adftool.with_bandpassed (sampling_frequency, 0.5, 3, 5, mix, (filter_slow, fir2) => {
	console.log('Filter orders:', fir1.order (), fir2.order ());
	console.log('Roots of quick:', count_roots (quick_sine));
	console.log('Roots of slow:', count_roots (slow_sine));
	console.log('Roots of quick filtered:', count_roots (filter_quick));
	console.log('Roots of slow filtered:', count_roots (filter_slow));
	const root_1 = count_roots (filter_quick);
	const root_2 = count_roots (filter_slow);
	assert (root_1 >= count_roots (quick_sine) - 2);
	assert (root_1 <= count_roots (quick_sine) + 2);
	assert (root_2 >= count_roots (slow_sine) - 2);
	assert (root_2 <= count_roots (slow_sine) + 2);
    });
});

const data =
    Adftool.with_file (new Uint8Array (0), (f) => {
	return Adftool.with_named_node ("a", (subject) => {
	    return Adftool.with_named_node ("b", (predicate) => {
		return Adftool.with_named_node ("c", (object) => {
		    return Adftool.with_statement_init (subject, predicate, object, null, null, (statement) => {
			f.insert (statement);
                        return f.data ((data) => {
                            return data;
                        });
		    });
		});
	    });
	});
    });

const file_ok =
    Adftool.with_file (data, (f) => {
        return Adftool.with_named_node ("c", (object) => {
	    return f.lookup_subjects (object, "b", (subjects) => {
	        return (subjects.length == 1
		        && subjects[0].to_n3 () == "<a>");
	    });
	});
    });

assert (file_ok);

// Local Variables:
// mode: js
// End:
