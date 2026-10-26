/**
 * Combinations (outlook): hardware, execution provider and engine in three
 * columns, every node joined to every node of the next column. The chains in
 * data-chains ("hardware,provider,engine;...") are drawn again on a fragment,
 * each with a question mark: the combinations still to be benchmarked.
 * Positions come from the laid-out nodes, so the lines are drawn when the
 * slide is shown and again when the fonts have loaded.
 */

(function () {
    const NS = 'http://www.w3.org/2000/svg';

    // A horizontal S-curve from the right edge of a to the left edge of b
    function segment(a, b) {
        const x1 = a.offsetLeft + a.offsetWidth, y1 = a.offsetTop + a.offsetHeight / 2;
        const x2 = b.offsetLeft, y2 = b.offsetTop + b.offsetHeight / 2, m = (x1 + x2) / 2;
        return `M${x1} ${y1}C${m} ${y1} ${m} ${y2} ${x2} ${y2}`;
    }

    function addPath(parent, d, className) {
        const path = document.createElementNS(NS, 'path');
        path.setAttribute('d', d);
        path.setAttribute('class', className);
        parent.appendChild(path);
        return path;
    }

    function build(combos) {
        // Hidden slides have no layout; try again when this one is shown
        if (combos.offsetWidth === 0) return;
        const svg = combos.querySelector('.combos-svg');
        const base = svg.querySelector('.combos-base');
        const hot = svg.querySelector('.combos-hot');
        base.innerHTML = '';
        hot.innerHTML = '';
        svg.setAttribute('viewBox', `0 0 ${combos.offsetWidth} ${combos.offsetHeight}`);

        const columns = [...combos.querySelectorAll('.combos-col')]
            .map(column => [...column.querySelectorAll('.combos-node')]);
        for (let c = 0; c < columns.length - 1; c++) {
            for (const a of columns[c]) {
                for (const b of columns[c + 1]) addPath(base, segment(a, b), 'combos-line');
            }
        }

        const chains = (combos.dataset.chains || '').split(';').filter(Boolean)
            .map(chain => chain.split(',').map(Number));
        // Every line first, then every mark, so no line is drawn over a question mark
        const marks = [];
        chains.forEach((chain, i) => {
            for (let c = 0; c < chain.length - 1; c++) {
                const path = addPath(hot, segment(columns[c][chain[c]], columns[c + 1][chain[c + 1]]), 'combos-hot-line');
                // Spread the question marks along the curves so they do not stack
                marks.push(path.getPointAtLength(path.getTotalLength() * [0.5, 0.3, 0.7, 0.4, 0.6][i % 5]));
            }
        });
        for (const point of marks) {
            const mark = document.createElementNS(NS, 'g');
            mark.setAttribute('class', 'combos-q');
            mark.innerHTML = `<rect x="${point.x - 16}" y="${point.y - 16}" width="32" height="32" rx="6"/>` +
                `<text x="${point.x}" y="${point.y}">?</text>`;
            hot.appendChild(mark);
        }
    }

    function buildCurrent() {
        const slide = window.Reveal && Reveal.getCurrentSlide();
        slide?.querySelectorAll('.combos').forEach(build);
    }

    window.addEventListener('reveal-ready', () => {
        Reveal.on('slidechanged', buildCurrent);
        buildCurrent();
        document.fonts?.ready.then(buildCurrent);
    });
})();
