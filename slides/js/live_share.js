/**
 * Live share: <button class="live-share">…</button> on a slide shares a window (the anira
 * demo, say) and shows it as a popup over the projected deck. The × closes it and stops
 * the share; so does the button again, or leaving the slide.
 *
 * The browser asks for the window to share only right after a click, in the window that
 * got the click. Clicked in the speaker view, the speaker view asks and hands the picture
 * to the projected deck; the deck's replay of that click (js/mirror.js) does nothing.
 */

(() => {
    const mirror = window.workshopMirror || {};
    let overlay = null;
    let stream = null;

    function close() {
        stream?.getTracks().forEach(track => track.stop());
        stream = null;
        overlay?.remove();
        overlay = null;
    }

    function show(shared) {
        close();
        stream = shared;
        overlay = document.createElement('div');
        overlay.className = 'live-share-overlay';
        overlay.innerHTML = `
          <video autoplay muted playsinline></video>
          <button class="live-share-close" aria-label="Close the shared window">×</button>`;
        overlay.querySelector('video').srcObject = shared;
        overlay.querySelector('.live-share-close').addEventListener('click', close);
        document.body.appendChild(overlay);
        // Ended from the browser's own "Stop sharing" bar
        shared.getVideoTracks()[0]?.addEventListener('ended', close);
    }

    // The projected deck's side, for the speaker view to call
    window.liveShare = { show, close, isOpen: () => overlay !== null };

    async function share(from) {
        try {
            return await from.navigator.mediaDevices.getDisplayMedia({ video: { frameRate: 30 }, audio: false });
        } catch (error) {
            return null;  // Declined: nothing changes
        }
    }

    document.addEventListener('click', async event => {
        const button = event.target.closest('.reveal .slides .live-share');
        if (!button) return;
        button.blur();  // the arrow keys belong to Reveal again
        if (mirror.silent) return;  // replayed from the speaker view, which shares itself

        if (mirror.currentPreview) {
            // The speaker view: it got the click, the deck that opened it shows the picture
            const deck = window.parent.opener?.liveShare;
            if (!deck) return;
            if (deck.isOpen()) return deck.close();
            const shared = await share(window.parent);
            if (shared) deck.show(shared);
        } else if (!mirror.preview) {
            if (overlay) return close();
            const shared = await share(window);
            if (shared) show(shared);
        }
    });

    window.addEventListener('reveal-ready', () => {
        if (!mirror.preview) Reveal.on('slidechanged', close);
    });
})();
