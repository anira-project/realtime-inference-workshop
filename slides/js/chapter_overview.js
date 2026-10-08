/**
 * Chapter overview (Esc): a scrollable row of chapters on top, the slides of the
 * selected chapter below. Replaces Reveal's overview, which only renders the
 * slides near the current one.
 * Click a chapter to show its slides, click a slide to jump to it. While open,
 * the arrow keys still move through the deck and the overview follows.
 */

class ChapterOverview {
    constructor() {
        this.element = null;
        this.chapters = [];
        this.selected = -1;
    }

    isOpen() {
        return this.element !== null && this.element.classList.contains('open');
    }

    toggle() {
        this.isOpen() ? this.close() : this.open();
    }

    open() {
        if (this.element === null) {
            this.build();
        }
        this.element.classList.add('open');
        this.update();
    }

    close() {
        if (this.element !== null) {
            this.element.classList.remove('open');
        }
    }

    build() {
        this.element = document.createElement('div');
        this.element.className = 'chapter-overview';

        const chapterRow = document.createElement('div');
        chapterRow.className = 'chapter-overview-chapters';
        this.element.appendChild(chapterRow);

        this.slidesPanel = document.createElement('div');
        this.slidesPanel.className = 'chapter-overview-slides';
        this.element.appendChild(this.slidesPanel);

        document.querySelectorAll('.reveal .slides > section').forEach((chapter, h) => {
            const slides = chapter.querySelectorAll(':scope > section');

            const card = document.createElement('div');
            card.className = 'chapter-overview-chapter';
            card.appendChild(this.thumbnail(slides[0]));
            const title = document.createElement('div');
            title.className = 'chapter-overview-title';
            const heading = slides[0]?.querySelector('h1, h2');
            title.textContent = heading ? heading.innerText.replace(/\s+/g, ' ') : chapter.id;
            card.appendChild(title);
            card.addEventListener('click', () => this.select(h));
            chapterRow.appendChild(card);

            const list = document.createElement('div');
            list.className = 'chapter-overview-list';
            const thumbs = [...slides].map((slide, v) => {
                const thumb = this.thumbnail(slide);
                thumb.addEventListener('click', () => {
                    Reveal.slide(h, v);
                    this.close();
                });
                list.appendChild(thumb);
                return thumb;
            });
            this.slidesPanel.appendChild(list);

            this.chapters.push({ card, list, thumbs });
        });

        document.querySelector('.reveal').appendChild(this.element);
    }

    // A static copy of the slide with every fragment shown; it lives outside
    // .slides so Reveal never counts it as a slide
    thumbnail(slide) {
        const clone = slide.cloneNode(true);
        clone.removeAttribute('id');
        clone.removeAttribute('hidden');
        clone.removeAttribute('aria-hidden');
        clone.removeAttribute('style');
        clone.classList.remove('past', 'present', 'future');
        clone.querySelectorAll('aside.notes').forEach(notes => notes.remove());
        clone.querySelectorAll('[id]').forEach(element => element.removeAttribute('id'));
        clone.querySelectorAll('.fragment').forEach(fragment => fragment.classList.add('visible'));
        clone.classList.add('chapter-overview-slide');

        const thumb = document.createElement('div');
        thumb.className = 'chapter-overview-thumb';
        thumb.appendChild(clone);
        return thumb;
    }

    select(h) {
        this.selected = h;
        this.chapters.forEach((chapter, i) => {
            chapter.card.classList.toggle('selected', i === h);
            chapter.list.classList.toggle('selected', i === h);
        });
        this.chapters[h].card.scrollIntoView({ block: 'nearest', inline: 'nearest' });
    }

    // Selects the current chapter and marks the current slide
    update() {
        if (!this.isOpen()) return;
        const { h, v } = Reveal.getIndices();
        this.select(h);
        this.chapters.forEach((chapter, i) => {
            chapter.card.classList.toggle('current', i === h);
            chapter.thumbs.forEach((thumb, j) => thumb.classList.toggle('current', i === h && j === v));
        });
    }
}

window.chapterOverview = new ChapterOverview();

window.addEventListener('reveal-ready', () => {
    Reveal.on('slidechanged', () => window.chapterOverview.update());
    document.addEventListener('keydown', event => {
        if (event.key === 'Enter' && window.chapterOverview.isOpen()) {
            window.chapterOverview.close();
        }
    });
});
