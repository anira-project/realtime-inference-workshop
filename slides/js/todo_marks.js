/**
 * TODO marks: in every code block, a blank of the exercise files (TODO1,
 * "TODO2", ...) is marked, so a slide that shows a file as handed out shows
 * where the work is. Runs once highlight.js has split the code into tokens:
 * a blank may sit in a token of its own or inside plain text.
 */

(function () {
    const BLANK = /"?TODO\d+"?/g;

    function wrap(textNode) {
        const text = textNode.nodeValue;
        BLANK.lastIndex = 0;
        if (!BLANK.test(text)) { return; }
        const parts = document.createDocumentFragment();
        let last = 0;
        text.replace(BLANK, (match, offset) => {
            parts.appendChild(document.createTextNode(text.slice(last, offset)));
            const mark = document.createElement('span');
            mark.className = 'todo-mark';
            mark.textContent = match;
            parts.appendChild(mark);
            last = offset + match.length;
            return match;
        });
        parts.appendChild(document.createTextNode(text.slice(last)));
        textNode.parentNode.replaceChild(parts, textNode);
    }

    function mark() {
        document.querySelectorAll('.reveal pre code').forEach((code) => {
            const walker = document.createTreeWalker(code, NodeFilter.SHOW_TEXT, {
                // Not inside a comment: the hints name the blanks too
                acceptNode: (node) => node.parentElement.closest('.hljs-comment, .todo-mark')
                    ? NodeFilter.FILTER_REJECT : NodeFilter.FILTER_ACCEPT,
            });
            const nodes = [];
            while (walker.nextNode()) { nodes.push(walker.currentNode); }
            nodes.forEach(wrap);
        });
    }

    if (window.Reveal) {
        Reveal.on('ready', () => setTimeout(mark, 0));
    }
})();
