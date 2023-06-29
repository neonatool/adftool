import gen_numeric_array from './adftool_numeric_array.mjs'

const specialized = gen_numeric_array('pointer');

export const PointerArray = specialized.type;

export const with_pointer_array = specialized.wrapper;

// Local Variables:
// mode: js
// End:
