/**
 * Spectrogram player: two recordings, their spectrograms stacked, one transport,
 * and a fader that crossfades between them while both loop in sync.
 *
 * <div class="spectro-player" data-a="in.wav" data-b="out.wav"
 *      data-label-a="In" data-label-b="Out"></div>
 */

(() => {
    const FFT_SIZE = 2048;
    const HOP = 256;
    const MIN_FREQUENCY = 40;
    const MAX_FREQUENCY = 20000;
    const DYNAMIC_RANGE_DB = 80;

    // In-place radix-2 FFT on separate real and imaginary arrays.
    function fft(re, im) {
        const n = re.length;
        for (let i = 1, j = 0; i < n; ++i) {
            let bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) {
                [re[i], re[j]] = [re[j], re[i]];
                [im[i], im[j]] = [im[j], im[i]];
            }
        }
        for (let size = 2; size <= n; size <<= 1) {
            const angle = -2 * Math.PI / size;
            const wRe = Math.cos(angle);
            const wIm = Math.sin(angle);
            for (let start = 0; start < n; start += size) {
                let curRe = 1;
                let curIm = 0;
                for (let k = 0; k < size / 2; ++k) {
                    const a = start + k;
                    const b = a + size / 2;
                    const tRe = re[b] * curRe - im[b] * curIm;
                    const tIm = re[b] * curIm + im[b] * curRe;
                    re[b] = re[a] - tRe;
                    im[b] = im[a] - tIm;
                    re[a] += tRe;
                    im[a] += tIm;
                    [curRe, curIm] = [curRe * wRe - curIm * wIm, curRe * wIm + curIm * wRe];
                }
            }
        }
    }

    // Magnitudes in dB, one Float32Array of FFT_SIZE / 2 bins per frame.
    function spectrogram(samples) {
        const window = new Float64Array(FFT_SIZE);
        for (let i = 0; i < FFT_SIZE; ++i) window[i] = 0.5 - 0.5 * Math.cos(2 * Math.PI * i / FFT_SIZE);

        const frames = [];
        const re = new Float64Array(FFT_SIZE);
        const im = new Float64Array(FFT_SIZE);
        for (let start = 0; start + FFT_SIZE <= samples.length; start += HOP) {
            for (let i = 0; i < FFT_SIZE; ++i) {
                re[i] = samples[start + i] * window[i];
                im[i] = 0;
            }
            fft(re, im);
            const frame = new Float32Array(FFT_SIZE / 2);
            for (let k = 0; k < FFT_SIZE / 2; ++k) {
                frame[k] = 20 * Math.log10(Math.hypot(re[k], im[k]) + 1e-9);
            }
            frames.push(frame);
        }
        return frames;
    }

    function hex(name) {
        const value = getComputedStyle(document.documentElement).getPropertyValue(name).trim();
        const m = value.match(/^#([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})$/i);
        return m ? [parseInt(m[1], 16), parseInt(m[2], 16), parseInt(m[3], 16)] : [28, 28, 26];
    }

    // Quiet is transparent, so the card shows through; loud runs through lilac to ink
    let STOPS = [];
    function updateStops() {
        const ink = hex('--ink');
        STOPS = [
            [0.0, [139, 134, 184, 0]],
            [0.45, [139, 134, 184, 110]],
            [0.8, [ink[0], ink[1], ink[2], 210]],
            [1.0, [ink[0], ink[1], ink[2], 255]],
        ];
    }

    function colour(value) {
        for (let i = 1; i < STOPS.length; ++i) {
            const [p1, c1] = STOPS[i];
            const [p0, c0] = STOPS[i - 1];
            if (value <= p1) {
                const t = (value - p0) / (p1 - p0);
                return c0.map((c, j) => c + (c1[j] - c) * t);
            }
        }
        return STOPS[STOPS.length - 1][1];
    }

    // Log frequency axis: one bin per canvas row
    function draw(canvas, frames, sampleRate, maxDb) {
        const width = canvas.width;
        const height = canvas.height;
        const context = canvas.getContext('2d');
        const image = context.createImageData(width, height);
        const binHz = sampleRate / FFT_SIZE;

        const rowBins = new Int32Array(height);
        for (let y = 0; y < height; ++y) {
            const fraction = 1 - y / (height - 1);
            const frequency = MIN_FREQUENCY * Math.pow(MAX_FREQUENCY / MIN_FREQUENCY, fraction);
            rowBins[y] = Math.min(FFT_SIZE / 2 - 1, Math.round(frequency / binHz));
        }

        for (let x = 0; x < width; ++x) {
            const frame = frames[Math.min(frames.length - 1, Math.floor(x / width * frames.length))];
            for (let y = 0; y < height; ++y) {
                const db = frame[rowBins[y]];
                const value = Math.max(0, Math.min(1, (db - (maxDb - DYNAMIC_RANGE_DB)) / DYNAMIC_RANGE_DB));
                const [r, g, b, a] = colour(value);
                const offset = (y * width + x) * 4;
                image.data[offset] = r;
                image.data[offset + 1] = g;
                image.data[offset + 2] = b;
                image.data[offset + 3] = a;
            }
        }
        context.putImageData(image, 0, 0);
    }

    // Min and max per column, mirrored around the centre line, in ink
    function drawWave(canvas, samples) {
        const width = canvas.width;
        const height = canvas.height;
        const context = canvas.getContext('2d');
        const ink = hex('--ink');
        context.clearRect(0, 0, width, height);
        let peak = 0;
        for (const v of samples) peak = Math.max(peak, Math.abs(v));
        const scale = (height / 2 - 12) / (peak || 1);
        context.fillStyle = `rgb(${ink.join(',')})`;
        const perColumn = samples.length / width;
        for (let x = 0; x < width; ++x) {
            let lo = Infinity;
            let hi = -Infinity;
            const end = Math.min(samples.length, Math.floor((x + 1) * perColumn));
            for (let i = Math.floor(x * perColumn); i < end; ++i) {
                lo = Math.min(lo, samples[i]);
                hi = Math.max(hi, samples[i]);
            }
            const top = height / 2 - hi * scale;
            context.fillRect(x, top, 1, Math.max(1, (hi - lo) * scale));
        }
    }

    async function load(url) {
        const data = await (await fetch(url)).arrayBuffer();
        // An offline context decodes without needing a user gesture
        return new OfflineAudioContext(1, 1, 48000).decodeAudioData(data);
    }

    const PLAY = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M8 5.5v13l10.5-6.5z"/></svg>';
    const PAUSE = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M7 5h3.5v14H7zM13.5 5H17v14h-3.5z"/></svg>';

    async function setup(player) {
        player.innerHTML = `
          <div class="spectro-view" role="group" aria-label="View">
            <button class="active" data-view="spectrogram">Spectrogram</button>
            <button data-view="waveform">Waveform</button>
          </div>
          <div class="spectro-body">
            <div class="spectro-mix">
              <span class="spectro-fader-label"></span>
              <input class="spectro-fader" type="range" min="0" max="1" step="0.01" value="0" aria-label="Mix">
              <span class="spectro-fader-label"></span>
            </div>
            <div class="spectro-tracks">
              <div class="spectro-track a"><div class="spectro-label"></div><canvas width="1600" height="230"></canvas></div>
              <div class="spectro-track b"><div class="spectro-label"></div><canvas width="1600" height="230"></canvas></div>
              <div class="spectro-playhead"></div>
            </div>
          </div>
          <div class="spectro-controls">
            <button class="spectro-play" aria-label="Play">${PLAY}</button>
            <span class="spectro-time"></span>
          </div>`;

        const [trackA, trackB] = player.querySelectorAll('.spectro-track');
        const [labelA, labelB] = player.querySelectorAll('.spectro-fader-label');
        trackA.querySelector('.spectro-label').textContent = player.dataset.labelA || 'A';
        trackB.querySelector('.spectro-label').textContent = player.dataset.labelB || 'B';
        labelA.textContent = (player.dataset.labelA || 'A').split(' ')[0];
        labelB.textContent = (player.dataset.labelB || 'B').split(' ')[0];

        const [bufferA, bufferB] = await Promise.all([load(player.dataset.a), load(player.dataset.b)]);
        const framesA = spectrogram(bufferA.getChannelData(0));
        const framesB = spectrogram(bufferB.getChannelData(0));
        // One scale for both, so louder really looks louder
        let maxDb = -Infinity;
        for (const frame of [...framesA, ...framesB]) for (const v of frame) maxDb = Math.max(maxDb, v);
        let view = 'spectrogram';
        const redraw = () => {
            updateStops();
            if (view === 'waveform') {
                drawWave(trackA.querySelector('canvas'), bufferA.getChannelData(0));
                drawWave(trackB.querySelector('canvas'), bufferB.getChannelData(0));
            } else {
                draw(trackA.querySelector('canvas'), framesA, bufferA.sampleRate, maxDb);
                draw(trackB.querySelector('canvas'), framesB, bufferB.sampleRate, maxDb);
            }
        };
        player.querySelectorAll('.spectro-view button').forEach(button => {
            button.addEventListener('click', () => {
                view = button.dataset.view;
                player.querySelectorAll('.spectro-view button').forEach(b => b.classList.toggle('active', b === button));
                redraw();
                button.blur();
            });
        });
        redraw();
        // The colours come from the theme; draw again when it flips
        new MutationObserver(redraw).observe(document.documentElement, { attributes: true, attributeFilter: ['data-theme'] });

        const duration = Math.min(bufferA.duration, bufferB.duration);
        const playButton = player.querySelector('.spectro-play');
        const fader = player.querySelector('.spectro-fader');
        const playhead = player.querySelector('.spectro-playhead');

        let context = null;
        let sources = [];
        let gains = [];
        let offset = 0;      // Seconds into the recording when paused
        let startedAt = 0;   // context.currentTime when playback started

        // Both recordings loop, so the position wraps
        const position = () => (sources.length ? (offset + context.currentTime - startedAt) % duration : offset);
        const timeLabel = player.querySelector('.spectro-time');

        const FADE = 0.04;   // seconds, so starting and stopping do not click
        let master = null;

        const applyFader = () => {
            const mix = Number(fader.value);
            // Equal power: the blend does not dip in the middle
            if (gains.length) {
                const now = context.currentTime;
                gains[0].gain.setTargetAtTime(Math.cos(mix * Math.PI / 2), now, 0.01);
                gains[1].gain.setTargetAtTime(Math.sin(mix * Math.PI / 2), now, 0.01);
            }
            trackA.style.opacity = 0.3 + 0.7 * (1 - mix);
            trackB.style.opacity = 0.3 + 0.7 * mix;
        };

        const stop = () => {
            const end = context.currentTime + FADE;
            master.gain.cancelScheduledValues(context.currentTime);
            master.gain.setValueAtTime(master.gain.value, context.currentTime);
            master.gain.linearRampToValueAtTime(0, end);
            sources.forEach(source => source.stop(end));
            sources = [];
            gains = [];
            playButton.innerHTML = PLAY;
            player.classList.remove('playing');
        };

        const play = () => {
            context = context || new AudioContext();
            context.resume();
            master = context.createGain();
            master.connect(context.destination);
            sources = [bufferA, bufferB].map(buffer => {
                const source = context.createBufferSource();
                source.buffer = buffer;
                source.loop = true;
                source.loopEnd = duration;
                return source;
            });
            gains = sources.map(source => {
                const gain = context.createGain();
                source.connect(gain).connect(master);
                return gain;
            });
            applyFader();
            startedAt = context.currentTime + 0.05;
            master.gain.setValueAtTime(0, startedAt);
            master.gain.linearRampToValueAtTime(1, startedAt + FADE);
            sources.forEach(source => source.start(startedAt, offset));
            playButton.innerHTML = PAUSE;
            player.classList.add('playing');
        };

        playButton.addEventListener('click', () => {
            if (sources.length) {
                offset = position();
                stop();
            } else {
                play();
            }
        });

        fader.addEventListener('input', applyFader);

        // Give the keyboard back to Reveal: a focused range input or button keeps
        // the arrow keys (and the presenter's clicker) from changing slides
        fader.addEventListener('change', () => fader.blur());
        playButton.addEventListener('mouseup', () => playButton.blur());

        // Click a spectrogram to jump there
        player.querySelectorAll('canvas').forEach(canvas => {
            canvas.addEventListener('click', event => {
                const rect = canvas.getBoundingClientRect();
                const wasPlaying = sources.length > 0;
                if (wasPlaying) stop();
                offset = (event.clientX - rect.left) / rect.width * duration;
                if (wasPlaying) play();
            });
        });

        const tick = () => {
            const fraction = Math.max(0, Math.min(1, Math.max(0, position()) / duration));
            const canvas = trackA.querySelector('canvas');
            playhead.style.left = `${canvas.offsetLeft + fraction * canvas.offsetWidth}px`;
            playhead.style.top = `${trackA.offsetTop}px`;
            timeLabel.textContent = `${position().toFixed(1)} / ${duration.toFixed(1)} s`;
            requestAnimationFrame(tick);
        };
        applyFader();
        tick();

        // Leaving the slide stops the sound
        Reveal.on('slidechanged', () => {
            if (sources.length) {
                offset = position();
                stop();
            }
        });
    }

    window.addEventListener('reveal-ready', () => {
        document.querySelectorAll('.reveal .slides .spectro-player').forEach(player => {
            setup(player).catch(error => {
                player.textContent = `Could not load the audio: ${error.message}`;
            });
        });
    });
})();
