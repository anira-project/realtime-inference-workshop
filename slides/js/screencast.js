/**
 * Live screencast of the IDE, for the room: while a screen share runs, every
 * slide tagged "Solution" shows it in place of its static solution, so the
 * exercises are solved live. Without a share, as for anyone reading the slides
 * later, the solution slides stay as they are.
 *
 * Started from a hidden button in the top right corner of the first slide,
 * or with the C key; the browser then asks which window to share. Stopping the
 * share in the browser brings the solution slides back.
 *
 * While the share runs, a run of solution slides (one per TODO) is one stop:
 * the live picture is the same on each, so navigation skips the rest of the
 * run, forwards and backwards.
 */
(function () {
    let stream = null;
    const video = document.createElement('video');
    video.className = 'screencast-video';
    video.muted = true;
    video.autoplay = true;
    video.playsInline = true;

    const isSolution = slide => slide
        && [...slide.querySelectorAll(':scope > .tag')].some(tag => tag.textContent.trim() === 'Solution');

    // A solution slide that follows another one: skipped while the share runs
    const continuesRun = slide => isSolution(slide) && isSolution(slide.previousElementSibling);

    // Where navigation lands instead of a skipped slide: the run's first slide
    // going back, the slide after the run going forward
    function skip(slide, forward) {
        const chapters = [...document.querySelectorAll('.reveal .slides > section')];
        const h = chapters.indexOf(slide.parentElement);
        const siblings = [...slide.parentElement.children];
        let target = slide;
        if (forward) {
            while (target && isSolution(target)) { target = target.nextElementSibling; }
            if (target) { Reveal.slide(h, siblings.indexOf(target)); } else { Reveal.slide(h + 1, 0); }
        } else {
            while (continuesRun(target)) { target = target.previousElementSibling; }
            Reveal.slide(h, siblings.indexOf(target));
        }
    }

    let last = { h: 0, v: 0 };
    function changed(event) {
        const forward = event.indexh > last.h || (event.indexh === last.h && (event.indexv || 0) > last.v);
        last = { h: event.indexh, v: event.indexv || 0 };
        if (stream && continuesRun(event.currentSlide)) {
            // After Reveal has finished this change
            setTimeout(() => skip(event.currentSlide, forward));
            return;
        }
        place();
    }

    // The live picture covers the whole window, header and footer included, when
    // the current slide is a solution slide. It sits on the page, not in the
    // slide: the slides are scaled to fit, and inside them it could only be as
    // large as a slide.
    function place() {
        document.querySelectorAll('.screencast-on').forEach(slide => slide.classList.remove('screencast-on'));
        const slide = Reveal.getCurrentSlide();
        if (stream && isSolution(slide)) {
            document.body.appendChild(video);
            // Taking a video out of the page pauses it, and autoplay doesn't restart it
            video.play().catch(() => {});
            slide.classList.add('screencast-on');
        } else {
            video.remove();
        }
    }

    function stop() {
        if (stream) stream.getTracks().forEach(track => track.stop());
        stream = null;
        video.srcObject = null;
        place();
    }

    async function toggle() {
        if (stream) return stop();
        try {
            stream = await navigator.mediaDevices.getDisplayMedia({ video: { frameRate: 30 }, audio: false });
        } catch (error) {
            stream = null;  // The share was declined: nothing changes
            return;
        }
        video.srcObject = stream;
        // Ending the share from the browser's own bar
        stream.getVideoTracks()[0].addEventListener('ended', stop);
        place();
    }

    window.addEventListener('reveal-ready', () => {
        // The speaker view's previews are decks of their own: no share there
        if (/receiver/i.test(window.location.search)) return;

        const button = document.createElement('button');
        button.className = 'screencast-start';
        button.setAttribute('aria-label', 'Start or stop the live screencast');
        button.addEventListener('click', toggle);
        document.querySelector('.reveal .slides > section > section')?.appendChild(button);

        Reveal.addKeyBinding({ keyCode: 67, key: 'C', description: 'Live screencast of the IDE' }, toggle);
        Reveal.on('slidechanged', changed);
    });
})();
