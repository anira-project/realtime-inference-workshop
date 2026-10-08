/**
 * Task timer: <div class="task-timer" data-minutes="5"></div> on a task slide.
 * Click to start, click again to pause; double-click resets.
 */

(() => {
    const format = seconds => {
        const s = Math.max(0, Math.ceil(seconds));
        return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
    };

    const reset = timer => {
        clearInterval(timer._interval);
        timer._interval = null;
        timer._remaining = Number(timer.dataset.minutes || 5) * 60;
        timer.classList.remove('running', 'done');
        timer.textContent = format(timer._remaining);
    };

    const tick = timer => {
        const remaining = timer._remaining - (Date.now() - timer._started) / 1000;
        timer.textContent = format(remaining);
        if (remaining <= 0) {
            clearInterval(timer._interval);
            timer._interval = null;
            timer._remaining = 0;
            timer.classList.remove('running');
            timer.classList.add('done');
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
