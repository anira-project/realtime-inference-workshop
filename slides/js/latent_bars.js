/**
 * Latent bars: the eight numbers between encoder and decoder, moving frame by
 * frame, and a toggle that adds an offset per dimension. The movement is a
 * simulation (smooth, per dimension), not the encoder's real values: a picture
 * of what the latent space does, with no sound.
 *
 * <div class="latent-live" data-offsets="1.6,2.8,2.3,0.7,-2.0,2.7,-2.9,-1.2"></div>
 *
 * The bars step 21.5 times a second, one latent frame per 2048 samples.
 */

(() => {
    const RANGE = 5;          // the bars show -RANGE .. +RANGE
    const FRAME_RATE = 44100 / 2048;

    // A smooth, repeatable path per dimension: a few slow sines, more movement in the first ones
    function simulated(dimension, t) {
        const amount = 1.6 / (1 + 0.35 * dimension);
        const base = [0.4, -0.8, 0.2, -0.3, 0.6, -0.2, 0.1, -0.5][dimension];
        const a = Math.sin(t * (0.9 + 0.13 * dimension) + dimension * 1.7);
        const b = Math.sin(t * (2.3 + 0.21 * dimension) + dimension * 0.6);
        return base + amount * (0.7 * a + 0.3 * b);
    }

    function setup(widget) {
        const offsets = widget.dataset.offsets.split(',').map(Number);
        widget.innerHTML = `
          <div class="lb-bars">${offsets.map((offset, i) => `
            <div class="lb-col"><div class="lb-value">0.00</div>
              <div class="lb-track"><div class="lb-zero"></div><div class="lb-ghost"></div><div class="lb-bar"></div></div>
              <div class="lb-name">z${i}</div><div class="lb-offset">${offset >= 0 ? '+' : ''}${offset.toFixed(1)}</div></div>`).join('')}
          </div>
          <div class="lb-controls">
            <button class="lb-toggle" aria-pressed="false">offset off</button>
            <span class="lb-frame">frame 0 · 21.5 a second</span>
          </div>`;
        const bars = [...widget.querySelectorAll('.lb-bar')];
        const ghosts = [...widget.querySelectorAll('.lb-ghost')];
        const values = [...widget.querySelectorAll('.lb-value')];
        const toggle = widget.querySelector('.lb-toggle');
        const frameLabel = widget.querySelector('.lb-frame');
        let offsetOn = false;

        toggle.addEventListener('click', () => {
            offsetOn = !offsetOn;
            toggle.setAttribute('aria-pressed', String(offsetOn));
            toggle.textContent = offsetOn ? 'offset on' : 'offset off';
            widget.classList.toggle('is-offset', offsetOn);
            toggle.blur();   // the arrow keys belong to Reveal again
        });

        const place = (element, from, to) => {
            const y = v => 50 - (Math.max(-RANGE, Math.min(RANGE, v)) / RANGE) * 50;
            element.style.top = `${Math.min(y(from), y(to))}%`;
            element.style.height = `${Math.abs(y(from) - y(to))}%`;
        };

        const start = performance.now();
        const tick = () => {
            if (widget.closest('section.present')) {
                const frame = Math.floor((performance.now() - start) / 1000 * FRAME_RATE);
                const time = frame / FRAME_RATE;   // the bars step once per latent frame
                offsets.forEach((offset, i) => {
                    const z = simulated(i, time);
                    const shown = offsetOn ? z + offset : z;
                    place(bars[i], 0, shown);
                    place(ghosts[i], 0, z);
                    values[i].textContent = `${shown >= 0 ? '+' : ''}${shown.toFixed(2)}`;
                });
                frameLabel.textContent = `frame ${frame} · 21.5 a second`;
            }
            requestAnimationFrame(tick);
        };
        tick();
    }

    function init() {
        document.querySelectorAll('.reveal .slides .latent-live').forEach(setup);
    }
    if (window.Reveal) Reveal.on('ready', init);
})();
