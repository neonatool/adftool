import gen_numeric_array from './adftool_numeric_array.mjs'

const specialized = gen_numeric_array('double');

export const DoubleArray = specialized.type;

export const with_double_array = specialized.wrapper;

// Local Variables:
// mode: js
// End:
