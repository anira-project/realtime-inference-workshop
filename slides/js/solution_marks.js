/**
 * Solution marks: in a code block, «...» marks what the participant filled
 * in, so a solution slide shows what changed against the exercise file.
 * The « and » are removed and the text between them is wrapped in
 * .solution-mark. Runs once highlight.js has split the code into tokens:
 * a mark may span several tokens, so every text node is walked in order
 * and the parts inside a mark are wrapped one by one.
 */

(function () {
    const OPEN = '«';   // «
    const CLOSE = '»';  // »

    function mark(code) {
        if (!code.textContent.includes(OPEN)) { return; }
        const walker = document.createTreeWalker(code, NodeFilter.SHOW_TEXT);
        const nodes = [];
        while (walker.nextNode()) { nodes.push(walker.currentNode); }

        let inside = false;
        let pieces = [];  // the spans of the mark being built, to round its two ends
        const close = () => {
            if (pieces.length) {
                pieces[0].classList.add('solution-start');
                pieces[pieces.length - 1].classList.add('solution-end');
            }
            pieces = [];
        };
        nodes.forEach((node) => {
            const text = node.nodeValue;
            if (!inside && !text.includes(OPEN) && !text.includes(CLOSE)) { return; }
            const parts = document.createDocumentFragment();
            let run = '';
            const flush = () => {
                if (!run) { return; }
                if (inside) {
                    const span = document.createElement('span');
                    span.className = 'solution-mark';
                    span.textContent = run;
                    parts.appendChild(span);
                    pieces.push(span);
                } else {
                    parts.appendChild(document.createTextNode(run));
                }
                run = '';
            };
            for (const ch of text) {
                if (ch === OPEN || ch === CLOSE) {
                    flush();
                    if (ch === CLOSE) { close(); }
                    inside = ch === OPEN;
                } else {
                    run += ch;
                }
            }
            flush();
            node.parentNode.replaceChild(parts, node);
        });
    }

    if (window.Reveal) {
        Reveal.on('ready', () => setTimeout(() => {
            document.querySelectorAll('.reveal pre code').forEach(mark);
        }, 0));
    }
})();
