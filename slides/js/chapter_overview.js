/**
 * Chapter overview (Esc): every chapter on top, one block per part, wrapped into rows, the slides of the
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
        document.documentElement.classList.add('overview-open');
        this.update();
    }

    close() {
        if (this.element !== null) {
            this.element.classList.remove('open');
            document.documentElement.classList.remove('overview-open');
        }
    }

    build() {
        this.element = document.createElement('div');
        this.element.className = 'chapter-overview';

        const chapterSection = document.createElement('div');
        chapterSection.className = 'chapter-overview-section';
        chapterSection.appendChild(this.heading('Chapters'));
        const chapterRow = document.createElement('div');
        chapterRow.className = 'chapter-overview-chapters';
        chapterSection.appendChild(chapterRow);
        this.element.appendChild(chapterSection);

        // "Slides · <the selected chapter>", the chapter part set on select
        const slideSection = document.createElement('div');
        slideSection.className = 'chapter-overview-section';
        const slideHeading = this.heading('Slides');
        this.slideHeadingChapter = document.createElement('span');
        this.slideHeadingChapter.className = 'chapter-overview-heading-chapter';
        slideHeading.appendChild(this.slideHeadingChapter);
        slideSection.appendChild(slideHeading);
        this.slidesPanel = document.createElement('div');
        this.slidesPanel.className = 'chapter-overview-slides';
        slideSection.appendChild(this.slidesPanel);
        this.element.appendChild(slideSection);

        let part = null;
        document.querySelectorAll('.reveal .slides > section').forEach((chapter, h) => {
            const slides = chapter.querySelectorAll(':scope > section');

            // One block per part, as in the agenda; the introduction opens the first
            const partName = chapter.dataset.part;
            if (part === null || (partName && part.name && partName !== part.name)) {
                part = this.addPart(chapterRow);
            }
            if (partName && !part.name) {
                part.name = partName;
                part.label.textContent = `Part ${chapterRow.children.length} · ${partName}`;
            }

            const card = document.createElement('div');
            card.className = 'chapter-overview-chapter';
            card.appendChild(this.thumbnail(slides[0]));
            const title = document.createElement('div');
            title.className = 'chapter-overview-title';
            const heading = slides[0]?.querySelector('h1, h2')?.cloneNode(true);
            // Hidden slides render no line breaks, so innerText would glue "Step 1" to the title
            heading?.querySelectorAll('br').forEach(br => br.replaceWith(' '));
            title.textContent = heading ? heading.textContent.replace(/\s+/g, ' ').trim() : chapter.id;
            card.appendChild(title);
            card.addEventListener('click', () => this.select(h));
            part.chapters.appendChild(card);

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

            this.chapters.push({ card, list, thumbs, title: title.textContent });
        });

        document.querySelector('.reveal').appendChild(this.element);
    }

    heading(text) {
        const heading = document.createElement('div');
        heading.className = 'chapter-overview-heading';
        heading.textContent = text;
        return heading;
    }

    // A labelled block of chapter cards; the label is set by its first chapter with a part
    addPart(chapterRow) {
        const element = document.createElement('div');
        element.className = 'chapter-overview-part';
        const label = document.createElement('div');
        label.className = 'chapter-overview-part-label';
        const chapters = document.createElement('div');
        chapters.className = 'chapter-overview-part-chapters';
        element.append(label, chapters);
        chapterRow.appendChild(element);
        return { name: null, label, chapters };
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
        this.slideHeadingChapter.textContent = this.chapters[h].title;
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
