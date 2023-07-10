import { with_string } from './adftool_string.mjs';
import Adftool from './adftool_binding.mjs';

const adftool_lytonepal = Adftool.cwrap (
    'adftool_lytonepal',
    'number',
    ['string', 'number', 'number', '*']);

export function lytonepal (concept) {
    const size = adftool_lytonepal (concept, 0, 0, 0);
    const mem = Adftool._malloc (size + 1);
    const check = adftool_lytonepal (concept, 0, size + 1, mem);
    if (check != size) {
        throw "Impossible";
    }
    const js = Adftool.UTF8ToString (mem);
    Adftool._free (mem);
    return js;
}

// Local Variables:
// mode: js
// End:
