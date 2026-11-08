/**
 * Speaker view → projected deck. The speaker view's previews are decks of their own, so a
 * click there used to stay there: the timer did not start on the projected deck, a player
 * did not play. Now every click, double-click and input inside a slide of the current-slide
 * preview happens on the same element of the projected deck too: timers, players, toggles,
 * links, whatever listens.
 *
 * The element is found by its place in the slide: the slide's indices, then the child
 * index at every level below it. Both decks are built from the same page.
 *
 * Sound plays only where you clicked. The speaker view has your click, so the browser
 * lets it play; the projected deck may never have had one, and would stay silent or play
 * twice. So the deck replays silently: window.workshopMirror.silent is true while it does,
 * and the timer and the players then run on a clock without sound.
 */

(() => {
    const preview = /receiver/i.test(window.location.search);
    let currentPreview = false;
    try {
        currentPreview = preview && !!window.frameElement?.closest('#current-slide');
    } catch (error) {
        // A frame of another origin: not the speaker view
    }

    window.workshopMirror = { silent: false, preview, currentPreview };

    // The innermost slide around an element, and the way down to it
    const slideOf = element => element.closest('.reveal .slides section:not(:has(section))');
    const pathTo = (slide, element) => {
        const path = [];
        for (let node = element; node !== slide; node = node.parentElement) {
            path.unshift([...node.parentElement.children].indexOf(node));
        }
        return path;
    };
    const follow = (slide, path) => path.reduce((node, index) => node?.children[index], slide);

    if (currentPreview) {
        // The speaker view was opened by the projected deck
        const send = message => {
            try {
                window.parent.opener?.postMessage({ namespace: 'workshop-mirror', ...message }, window.location.origin);
            } catch (error) {
                // The projected deck is gone
            }
        };

        const where = element => {
            const slide = slideOf(element);
            if (!slide) return null;
            const { h, v } = Reveal.getIndices(slide);
            return { h, v, path: pathTo(slide, element) };
        };

        const forward = type => event => {
            if (!(event.target instanceof Element)) return;
            const place = where(event.target);
            if (!place) return;
            const rect = event.target.getBoundingClientRect();
            send({
                type, ...place,
                // Where on it: a click on a spectrogram jumps there
                fx: rect.width ? (event.clientX - rect.left) / rect.width : 0.5,
                fy: rect.height ? (event.clientY - rect.top) / rect.height : 0.5
            });
        };
        document.addEventListener('click', forward('click'));
        document.addEventListener('dblclick', forward('dblclick'));

        // Faders, check boxes, menus
        const forwardValue = type => event => {
            const control = event.target;
            if (!(control instanceof HTMLInputElement || control instanceof HTMLSelectElement)) return;
            const place = where(control);
            if (place) send({ type, ...place, value: control.value, checked: control.checked });
        };
        document.addEventListener('input', forwardValue('input'));
        document.addEventListener('change', forwardValue('change'));
    } else if (!preview) {
        window.addEventListener('message', event => {
            const data = event.data;
            if (event.origin !== window.location.origin || data?.namespace !== 'workshop-mirror') return;
            const slide = Reveal.getSlide(data.h, data.v);
            const target = slide && follow(slide, data.path);
            if (!target) return;

            window.workshopMirror.silent = true;
            try {
                if (data.type === 'input' || data.type === 'change') {
                    target.value = data.value;
                    if ('checked' in target) target.checked = data.checked;
                    target.dispatchEvent(new Event(data.type, { bubbles: true }));
                } else {
                    const rect = target.getBoundingClientRect();
                    target.dispatchEvent(new MouseEvent(data.type, {
                        bubbles: true,
                        cancelable: true,
                        clientX: rect.left + data.fx * rect.width,
                        clientY: rect.top + data.fy * rect.height
                    }));
                }
            } finally {
                window.workshopMirror.silent = false;
            }
        });
    }
})();
