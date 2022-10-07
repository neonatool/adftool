import Adftool from './adftool_binding.mjs'

export function with_string (str, f) {
    const required = Adftool.lengthBytesUTF8 (str);
    const ptr = Adftool._malloc (required + 1);
    try {
	Adftool.stringToUTF8 (str, ptr, required + 1);
	return f (ptr, required);
    } finally {
	Adftool._free (ptr);
    }
}

// Local Variables:
// mode: js
// End:
