import AdftoolFactory from './index.mjs';

export const factory = AdftoolFactory ();

export let immediate = () => {
    throw 'Patience, the adftool code has not been compiled yet.';
};

factory.then ((loaded) => { immediate = () => loaded });

// Local Variables:
// mode: js
// End:
