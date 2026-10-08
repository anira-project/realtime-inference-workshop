/**
 * Overflow check (dev server only): lists every slide whose content reaches into
 * the bottom of the slide, where the footer and the progress bar sit.
 */

// In slide pixels: the slide is 1080 tall, the footer covers roughly the last 50
const OVERFLOW_LIMIT = 1030;

function measureSlideBottom(slide, scale) {
    const top = slide.getBoundingClientRect().top;
    let bottom = 0;
    slide.querySelectorAll('*').forEach(element => {
        if (element.closest('aside.notes')) return;
        const rect = element.getBoundingClientRect();
        if (rect.height > 0) {
            bottom = Math.max(bottom, rect.bottom);
        }
    });
    return (bottom - top) / scale;
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
            const bottom = measureSlideBottom(slide, scale);
            chapter.style.display = chapterDisplay;
            slide.style.display = slideDisplay;

            if (bottom > OVERFLOW_LIMIT) {
                const heading = slide.querySelector('h1, h2');
                const title = heading ? heading.innerText.replace(/\s+/g, ' ') : '(no heading)';
                overflows.push({ h, v, bottom: Math.round(bottom), label: `${chapter.id} / ${v + 1}: ${title}` });
            }
        });
    });

    document.querySelector('.overflow-check')?.remove();
    if (overflows.length === 0) return;

    console.warn(`${overflows.length} slide(s) reach below ${OVERFLOW_LIMIT}px:`,
        overflows.map(o => `${o.label} (${o.bottom}px)`));

    const panel = document.createElement('div');
    panel.className = 'overflow-check';
    panel.textContent = `${overflows.length} slide(s) reach the footer:`;
    for (const overflow of overflows) {
        const link = document.createElement('a');
        link.textContent = `${overflow.label} (${overflow.bottom}px)`;
        link.addEventListener('click', () => Reveal.slide(overflow.h, overflow.v));
        panel.appendChild(link);
    }
    document.body.appendChild(panel);
}

window.addEventListener('reveal-ready', async () => {
    await document.fonts.ready;
    checkOverflow();
});
