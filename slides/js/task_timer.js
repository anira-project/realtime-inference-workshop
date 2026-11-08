/**
 * Task timer: <div class="task-timer" data-minutes="5"></div> on a task slide.
 * Click to start, click again to pause; double-click resets. One soft tone
 * with a minute left, three when the time is up.
 */

(() => {
    // The speaker view's previews are decks of their own: only the current slide's
    // preview, where you click, sounds (js/mirror.js)
    const preview = /receiver/i.test(window.location.search);
    const silent = preview && !window.workshopMirror?.currentPreview;
    let audio = null;

    // A short sine with a soft attack and decay, `delay` seconds from now
    const tone = (frequency, delay = 0) => {
        if (!audio) return;
        const start = audio.currentTime + delay;
        const oscillator = audio.createOscillator();
        const gain = audio.createGain();
        oscillator.frequency.value = frequency;
        gain.gain.setValueAtTime(0, start);
        gain.gain.linearRampToValueAtTime(0.25, start + 0.02);
        gain.gain.exponentialRampToValueAtTime(0.001, start + 0.6);
        oscillator.connect(gain).connect(audio.destination);
        oscillator.start(start);
        oscillator.stop(start + 0.65);
    };

    const format = seconds => {
        const s = Math.max(0, Math.ceil(seconds));
        return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
    };

    const reset = timer => {
        clearInterval(timer._interval);
        timer._interval = null;
        timer._remaining = Number(timer.dataset.minutes || 5) * 60;
        // A timer shorter than two minutes gets no one-minute tone
        timer._warned = timer._remaining < 120;
        timer.classList.remove('running', 'done');
        timer.textContent = format(timer._remaining);
    };

    const tick = timer => {
        const remaining = timer._remaining - (Date.now() - timer._started) / 1000;
        timer.textContent = format(remaining);
        if (!timer._warned && remaining <= 60) {
            timer._warned = true;
            if (!timer._mirrored) tone(880);
        }
        if (remaining <= 0) {
            clearInterval(timer._interval);
            timer._interval = null;
            timer._remaining = 0;
            timer.classList.remove('running');
            timer.classList.add('done');
            if (!timer._mirrored) [0, 0.35, 0.7].forEach(delay => tone(880, delay));
        }
    };

    const toggle = timer => {
        if (timer._remaining === undefined) reset(timer);
        if (timer.classList.contains('done')) return;

        if (timer._interval) {
            // Pause: keep what is left
            timer._remaining -= (Date.now() - timer._started) / 1000;
            clearInterval(timer._interval);
            timer._interval = null;
            timer.classList.remove('running');
            return;
        }
        // Started from the speaker view: the speaker view plays the tones (js/mirror.js)
        timer._mirrored = window.workshopMirror?.silent === true;
        // Created on the click that starts it: browsers allow audio only after a gesture
        if (!silent && !timer._mirrored && !audio) audio = new AudioContext();
        audio?.resume();
        timer._started = Date.now();
        timer._interval = setInterval(() => tick(timer), 250);
        timer.classList.add('running');
    };

    // Only the timers on real slides; the copies in the chapter overview stay inert
    const timerFor = event => event.target.closest('.reveal .slides .task-timer');

    document.addEventListener('click', event => {
        const timer = timerFor(event);
        if (timer) toggle(timer);
    });

    document.addEventListener('dblclick', event => {
        const timer = timerFor(event);
        if (timer) reset(timer);
    });

    window.addEventListener('reveal-ready', () => {
        document.querySelectorAll('.reveal .slides .task-timer').forEach(reset);
    });
})();
