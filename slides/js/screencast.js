/**
 * Live screencast of the IDE, for the room: while a screen share runs, every
 * slide tagged "Solution" shows it in place of its static solution, so the
 * exercises are solved live. Without a share, as for anyone reading the slides
 * later, the solution slides stay as they are.
 *
 * Started from a hidden button in the top right corner of the first slide,
 * or with the C key; the browser then asks which window to share. Stopping the
 * share in the browser brings the solution slides back.
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

    // The live picture goes into the current slide when it is a solution slide
    function place() {
        document.querySelectorAll('.screencast-on').forEach(slide => slide.classList.remove('screencast-on'));
        const slide = Reveal.getCurrentSlide();
        if (stream && isSolution(slide)) {
            slide.appendChild(video);
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
        Reveal.on('slidechanged', place);
    });
})();
