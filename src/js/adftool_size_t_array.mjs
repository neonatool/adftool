import gen_numeric_array from './adftool_numeric_array.mjs'

const specialized = gen_numeric_array('size_t');

export const SizeTArray = specialized.type;

export const with_size_t_array = specialized.wrapper;

// Local Variables:
// mode: js
// End:
