import gen_numeric_array from './adftool_numeric_array.mjs'

const specialized = gen_numeric_array('long');

export const LongArray = specialized.type;

export const with_long_array = specialized.wrapper;

// Local Variables:
// mode: js
// End:
