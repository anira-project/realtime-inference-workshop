/**
 * Notes editor (dev server only): a pencil in the header and in the speaker
 * view opens the current slide's notes as their markdown source. Saving writes
 * them back into the chapter's slides/*.md through the dev server
 * (/api/notes); the rebuild and the hot reload then show them, and the speaker
 * view reconnects by itself.
 */

// Self-contained, because the speaker view runs it as source in its own window.
// mode 'deck': this window's slides. mode 'speaker': the slides of the window
// that opened the speaker view.
function installNotesEditor(mode) {
    const deck = () => (mode === 'speaker' ? window.opener : window);
    const origin = deck().location.origin;
    const PENCIL = '<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M12 20h9"/><path d="M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4Z"/></svg>';

    // The chapter file and the slide's index in it, for the slide on screen
    function target() {
        const w = deck();
        const { h, v } = w.Reveal.getIndices();
        const chapter = w.document.querySelectorAll('.reveal .slides > section')[h];
        return { file: chapter && chapter.dataset.file, index: v || 0 };
    }

    const style = document.createElement('style');
    style.textContent = `
        .notes-editor { position: fixed; inset: 5% 5% auto auto; width: min(760px, 90vw); z-index: 1000;
            background: #fff; color: #111; border: 1px solid #ccc; border-radius: 8px; padding: 14px;
            box-shadow: 0 10px 40px rgba(0, 0, 0, 0.25); font: 15px/1.4 system-ui, sans-serif; }
        .notes-editor-title { font-weight: 600; margin-bottom: 8px; }
        .notes-editor textarea { width: 100%; height: 50vh; box-sizing: border-box; padding: 10px;
            font: 15px/1.5 ui-monospace, Menlo, monospace; border: 1px solid #ccc; border-radius: 6px; resize: vertical; }
        .notes-editor-row { display: flex; align-items: center; gap: 8px; margin-top: 10px; }
        .notes-editor-row span { flex: 1; color: #666; }
        .notes-editor button { font: inherit; padding: 6px 14px; border-radius: 6px; border: 1px solid #bbb;
            background: #f4f4f4; cursor: pointer; }
        .notes-editor button.save { background: #111; color: #fff; border-color: #111; }
        .notes-editor-pencil { background: none; border: 0; padding: 0 0 0 8px; cursor: pointer;
            color: inherit; vertical-align: middle; }`;
    document.head.appendChild(style);

    let panel = null;

    function close() {
        if (panel) panel.remove();
        panel = null;
    }

    async function open() {
        if (panel) return;
        const { file, index } = target();
        if (!file) return;
        panel = document.createElement('div');
        panel.className = 'notes-editor';
        panel.innerHTML = `
            <div class="notes-editor-title">Notes · ${file}, slide ${index + 1}</div>
            <textarea spellcheck="true" disabled>Loading…</textarea>
            <div class="notes-editor-row">
              <span>⌘S / Ctrl+S saves · Esc closes</span>
              <button class="cancel">Cancel</button><button class="save">Save</button>
            </div>`;
        document.body.appendChild(panel);
        const area = panel.querySelector('textarea');
        const status = panel.querySelector('span');

        const response = await fetch(`${origin}/api/notes?file=${encodeURIComponent(file)}&index=${index}`);
        const data = await response.json();
        if (!response.ok) {
            area.value = data.error || 'Could not load the notes.';
            return;
        }
        area.value = data.notes;
        area.disabled = false;
        area.focus();

        async function save() {
            status.textContent = 'Saving…';
            const result = await fetch(`${origin}/api/notes`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ file, index, notes: area.value }),
            });
            if (result.ok) {
                close();  // The rebuild reloads the deck with the new notes
            } else {
                status.textContent = (await result.json()).error || 'Saving failed.';
            }
        }

        panel.querySelector('.save').addEventListener('click', save);
        panel.querySelector('.cancel').addEventListener('click', close);
        // The deck's own keys (arrows, Esc for the overview) stay out of the editor
        panel.addEventListener('keydown', event => {
            event.stopPropagation();
            if ((event.metaKey || event.ctrlKey) && event.key === 's') {
                event.preventDefault();
                save();
            } else if (event.key === 'Escape') {
                close();
            }
        });
    }

    if (mode === 'speaker') {
        // Next to the "Notes" label, once the speaker view has built it
        const place = () => {
            const label = document.querySelector('.speaker-controls-notes .label');
            if (!label) return setTimeout(place, 200);
            const button = document.createElement('button');
            button.className = 'notes-editor-pencil';
            button.title = 'Edit the notes of this slide';
            button.innerHTML = PENCIL;
            button.addEventListener('click', open);
            label.appendChild(button);
            // A slide without notes hides the notes area; the pencil stays reachable.
            // Only a class that is there is removed: removing an absent one still
            // rewrites the attribute, and the observer would call itself forever.
            const notes = document.querySelector('.speaker-controls-notes');
            const unhide = () => {
                if (notes.classList.contains('hidden')) notes.classList.remove('hidden');
            };
            unhide();
            new MutationObserver(unhide).observe(notes, { attributes: true, attributeFilter: ['class'] });
        };
        place();
    } else {
        const button = document.createElement('button');
        button.className = 'header-button';
        button.title = 'Edit the notes of this slide';
        button.setAttribute('aria-label', 'Edit the notes of this slide');
        button.innerHTML = PENCIL;
        button.addEventListener('click', open);
        document.querySelector('.header-logo-box')?.prepend(button);
    }
}

installNotesEditor('deck');

// The speaker view is written into a blank window by Reveal's notes plugin: the
// editor is run there as source, once its notes area exists.
(function () {
    const open = window.open;
    window.open = function (url, name, specs) {
        const speaker = open.call(this, url, name, specs);
        if (speaker && name === 'reveal.js - Notes') {
            const inject = () => {
                const doc = speaker.document;
                if (!doc || !doc.querySelector('.speaker-controls-notes')) return setTimeout(inject, 200);
                const script = doc.createElement('script');
                script.textContent = `(${installNotesEditor.toString()})('speaker');`;
                doc.body.appendChild(script);
            };
            inject();
        }
        return speaker;
    };
})();
