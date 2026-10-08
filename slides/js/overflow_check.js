/**
 * Overflow check (dev server only): lists every slide whose content reaches into
 * the bottom of the slide, where the footer and the progress bar sit, or past
 * its right edge.
 */

// In slide pixels: the slide is 1080 tall, the footer covers roughly the last 50
const OVERFLOW_LIMIT = 1030;
const WIDTH_LIMIT = 1920;

// How far the content reaches down and right, in slide pixels.
function measureSlideExtent(slide, scale) {
    const origin = slide.getBoundingClientRect();
    let bottom = 0;
    let right = 0;
    slide.querySelectorAll('*').forEach(element => {
        if (element.closest('aside.notes')) return;
        const rect = element.getBoundingClientRect();
        if (rect.height > 0) {
            bottom = Math.max(bottom, rect.bottom);
            right = Math.max(right, rect.right);
        }
    });
    return { bottom: (bottom - origin.top) / scale, right: (right - origin.left) / scale };
}

function checkOverflow() {
    const scale = Reveal.getScale();
    const overflows = [];

    document.querySelectorAll('.reveal .slides > section').forEach((chapter, h) => {
        chapter.querySelectorAll(':scope > section').forEach((slide, v) => {
            // Hidden slides have no layout; show them for the measurement only
            const chapterDisplay = chapter.style.display;
            const slideDisplay = slide.style.display;
            chapter.style.display = 'block';
            slide.style.display = 'block';
            const { bottom, right } = measureSlideExtent(slide, scale);
            chapter.style.display = chapterDisplay;
            slide.style.display = slideDisplay;

            if (bottom > OVERFLOW_LIMIT || right > WIDTH_LIMIT) {
                const heading = slide.querySelector('h1, h2');
                const title = heading ? heading.innerText.replace(/\s+/g, ' ') : '(no heading)';
                const size = `${Math.round(right)} × ${Math.round(bottom)}px`;
                overflows.push({ h, v, size, label: `${chapter.id} / ${v + 1}: ${title}` });
            }
        });
    });

    document.querySelector('.overflow-check')?.remove();
    if (overflows.length === 0) return;

    console.warn(`${overflows.length} slide(s) reach below ${OVERFLOW_LIMIT}px or past ${WIDTH_LIMIT}px:`,
        overflows.map(o => `${o.label} (${o.size})`));

    const panel = document.createElement('div');
    panel.className = 'overflow-check';
    panel.textContent = `${overflows.length} slide(s) reach the footer or the right edge:`;
    for (const overflow of overflows) {
        const link = document.createElement('a');
        link.textContent = `${overflow.label} (${overflow.size})`;
        link.addEventListener('click', () => Reveal.slide(overflow.h, overflow.v));
        panel.appendChild(link);
    }
    document.body.appendChild(panel);
}

window.addEventListener('reveal-ready', async () => {
    await document.fonts.ready;
    checkOverflow();
});
