/**
 * Teaser animations for the introduction. Each runs on a canvas, only while its
 * slide is shown, and redraws in the current theme's colours.
 *
 * <div class="teaser" data-teaser="clock"><canvas></canvas></div>
 *     One turn of the clock is one callback of 512 samples. Every 4th turn the network
 *     has its 2048 samples and runs; its time varies, and it does not fit. Time is
 *     counted in samples of audio, like the block sizes.
 * <div class="teaser" data-teaser="codec" data-in="in.wav" data-out="out.wav"><canvas></canvas></div>
 *     The real-time thread delivers 512 samples per callback. An FFT and a neural
 *     network both need 2048; only the FFT takes the same time for every block.
 *
 * Numbers are Step 3's: 44.1 kHz, host block 512, model block 2048, about 13 ms per
 * forward pass on ONNX Runtime, slower calls now and then. The variation shown is an
 * illustration of that, not a measurement.
 */

(() => {
    const SAMPLE_RATE = 44100;
    const MODEL_BLOCK = 2048;
    const HOST_BLOCK = 512;
    const BLOCKS_PER_MODEL = MODEL_BLOCK / HOST_BLOCK;
    const CALLBACK_MS = HOST_BLOCK / SAMPLE_RATE * 1000;   // 11.6
    const FORWARD_MS = 13;
    const PIXEL_RATIO = 2;

    const MONO = '"IBM Plex Mono", monospace';
    const SANS = '"Barlow Semi Condensed", "Helvetica Neue", Arial, sans-serif';

    const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');

    // ---------- helpers ----------

    function colours() {
        const style = getComputedStyle(document.documentElement);
        const token = (name, fallback) => style.getPropertyValue(name).trim() || fallback;
        return {
            ink: token('--ink', '#1c1c1a'),
            paper: token('--paper', '#ffffff'),
            wash: token('--wash', '#f4f4f3'),
            muted: token('--fs-text-muted-color', 'rgba(28,28,26,0.62)'),
            subtle: token('--fs-text-subtle-color', 'rgba(28,28,26,0.45)'),
            hairline: token('--fs-border-strong', 'rgba(28,28,26,0.2)'),
            model: token('--tint-model', '#e7e1d6'),
            over: token('--mark-over', '#c0503a'),
        };
    }

    // Deterministic, so every run of the slide tells the same story
    function random(seed) {
        return () => {
            seed |= 0;
            seed = seed + 0x6d2b79f5 | 0;
            let t = Math.imul(seed ^ seed >>> 15, 1 | seed);
            t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t;
            return ((t ^ t >>> 14) >>> 0) / 4294967296;
        };
    }

    // Forward-pass time in ms: around 13, sometimes slower, now and then much slower
    function forwardPass(rand) {
        const r = rand();
        if (r < 0.18) return 18 + rand() * 4;
        if (r < 0.4) return 14.5 + rand() * 1.5;
        return FORWARD_MS - 0.5 + rand() * 1.2;
    }

    const clamp = (v, lo = 0, hi = 1) => Math.max(lo, Math.min(hi, v));
    const easeOut = t => 1 - Math.pow(1 - clamp(t), 3);
    const easeInOut = t => { t = clamp(t); return t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2; };

    function fit(canvas) {
        const width = canvas.clientWidth;
        const height = canvas.clientHeight;
        if (canvas.width !== width * PIXEL_RATIO || canvas.height !== height * PIXEL_RATIO) {
            canvas.width = width * PIXEL_RATIO;
            canvas.height = height * PIXEL_RATIO;
        }
        const ctx = canvas.getContext('2d');
        ctx.setTransform(PIXEL_RATIO, 0, 0, PIXEL_RATIO, 0, 0);
        ctx.clearRect(0, 0, width, height);
        ctx.globalAlpha = 1;
        return { ctx, width, height };
    }

    function text(ctx, value, x, y, { font = `500 24px ${SANS}`, colour, align = 'left', baseline = 'alphabetic', alpha = 1 } = {}) {
        ctx.save();
        ctx.globalAlpha *= alpha;
        ctx.font = font;
        ctx.fillStyle = colour;
        ctx.textAlign = align;
        ctx.textBaseline = baseline;
        ctx.fillText(value, x, y);
        ctx.restore();
    }

    function arc(ctx, cx, cy, r, from, to, colour, width) {
        if (to <= from) return;
        ctx.beginPath();
        ctx.arc(cx, cy, r, -Math.PI / 2 + from * 2 * Math.PI, -Math.PI / 2 + to * 2 * Math.PI);
        ctx.strokeStyle = colour;
        ctx.lineWidth = width;
        ctx.lineCap = 'butt';
        ctx.stroke();
    }

    // ---------- A: fighting the clock ----------

    const TURN_SECONDS = 1.5;    // one callback on screen
    const LAYERS = [3, 5, 5, 3];

    class Clock {
        constructor(root) {
            this.root = root;
            this.canvas = root.querySelector('canvas');
        }

        enter() {
            this.started = performance.now();
            this.rand = random(5);
            this.work = [];
        }

        leave() {}

        // How many turns callback k needs: a copy, or a whole forward pass
        workOf(k) {
            while (this.work.length <= k) {
                const i = this.work.length;
                this.work.push(i % BLOCKS_PER_MODEL === BLOCKS_PER_MODEL - 1
                    ? forwardPass(this.rand) / CALLBACK_MS
                    : 0.03 + this.rand() * 0.02);
            }
            return this.work[k];
        }

        draw(now) {
            const c = colours();
            const { ctx, width, height } = fit(this.canvas);
            const t = reducedMotion.matches ? TURN_SECONDS * 4.2 : (now - this.started) / 1000;
            const k = Math.floor(t / TURN_SECONDS);
            const phase = t / TURN_SECONDS - k;

            const overflow = k > 0 ? Math.max(0, this.workOf(k - 1) - 1) : 0;
            const ownStart = overflow;
            const ownWork = this.workOf(k);

            // ----- the clock -----
            const cx = width * 0.27;
            const cy = height * 0.52;
            const R = Math.min(height * 0.36, 250);
            const ring = 30;

            arc(ctx, cx, cy, R, 0, 1, c.wash, ring);
            // Last callback's forward pass, still running past the deadline
            arc(ctx, cx, cy, R, 0, Math.min(phase, overflow), c.over, ring);
            // This callback's own work
            arc(ctx, cx, cy, R, ownStart, Math.min(phase, ownStart + ownWork, 1), c.ink, ring);

            // A late callback: the ring flashes
            const lateFlash = overflow > 0 ? 1 - clamp(phase / 0.4) : 0;
            if (lateFlash > 0) {
                ctx.globalAlpha = lateFlash * 0.5;
                arc(ctx, cx, cy, R + ring / 2 + 10, 0, 1, c.over, 8 * lateFlash);
                ctx.globalAlpha = 1;
            }

            // Deadline at twelve o'clock
            ctx.fillStyle = c.ink;
            ctx.fillRect(cx - 1.5, cy - R - ring / 2 - 18, 3, ring + 18);
            text(ctx, 'deadline', cx, cy - R - ring / 2 - 30, { font: `500 24px ${MONO}`, colour: c.muted, align: 'center' });

            // Hand
            const angle = -Math.PI / 2 + phase * 2 * Math.PI;
            ctx.strokeStyle = c.ink;
            ctx.lineWidth = 3;
            ctx.lineCap = 'round';
            ctx.beginPath();
            // From outside the centre text to the ring, so it never crosses the numbers
            ctx.moveTo(cx + Math.cos(angle) * R * 0.62, cy + Math.sin(angle) * R * 0.62);
            ctx.lineTo(cx + Math.cos(angle) * (R - ring / 2 - 10), cy + Math.sin(angle) * (R - ring / 2 - 10));
            ctx.stroke();

            // Centre: the budget, or how late we were
            if (lateFlash > 0.05) {
                text(ctx, `+${Math.round(overflow * HOST_BLOCK)}`, cx, cy - 34,
                     { font: `600 64px ${SANS}`, colour: c.over, align: 'center', baseline: 'middle', alpha: Math.min(1, lateFlash * 2) });
                text(ctx, 'samples too late', cx, cy + 22, { font: `400 30px ${SANS}`, colour: c.over, align: 'center', baseline: 'middle', alpha: Math.min(1, lateFlash * 2) });
            } else {
                text(ctx, 'Host block', cx, cy - 40, { font: `400 32px ${SANS}`, colour: c.muted, align: 'center', baseline: 'middle' });
                text(ctx, `${HOST_BLOCK} samples`, cx, cy + 14, { font: `600 64px ${SANS}`, colour: c.ink, align: 'center', baseline: 'middle' });
            }

            // History: one mark per finished callback
            const history = 16;
            const markSize = 18;
            const markGap = 10;
            const hx = cx - (history * (markSize + markGap) - markGap) / 2;
            const hy = cy + R + ring / 2 + 40;
            for (let i = 0; i < history; ++i) {
                const index = k - history + i;
                ctx.fillStyle = index < 0 ? c.wash : this.workOf(index) > 1 ? c.over : c.ink;
                ctx.globalAlpha = index < 0 ? 1 : 0.35 + 0.65 * (i + 1) / history;
                ctx.fillRect(hx + i * (markSize + markGap), hy, markSize, markSize);
            }
            ctx.globalAlpha = 1;

            // ----- the network -----
            const nx = width * 0.72;
            const ny = cy - 20;
            const layerGap = 130;
            const nodeGap = 74;
            const nodeR = 17;
            const nodes = LAYERS.map((count, li) => Array.from({ length: count }, (_, ni) => ({
                x: nx + (li - (LAYERS.length - 1) / 2) * layerGap,
                y: ny + (ni - (count - 1) / 2) * nodeGap,
            })));

            // Is a forward pass running, and how far is it?
            let progress = -1;
            let elapsedMs = 0;
            if (k % BLOCKS_PER_MODEL === BLOCKS_PER_MODEL - 1 && phase >= ownStart) {
                progress = (phase - ownStart) / ownWork;
                elapsedMs = (phase - ownStart) * CALLBACK_MS;
            } else if (overflow > 0 && phase < overflow) {
                const previous = this.workOf(k - 1);
                progress = (1 + phase) / previous;
                elapsedMs = (1 + phase) * CALLBACK_MS;
            }
            const running = progress >= 0 && progress <= 1;
            const front = progress * (LAYERS.length - 1 + 0.6) - 0.3;
            const glow = li => running ? clamp(1 - Math.abs(li - front) * 1.1) : 0;

            for (let li = 0; li < LAYERS.length - 1; ++li) {
                const g = running ? clamp(1 - Math.abs(li + 0.5 - front) * 1.1) : 0;
                ctx.strokeStyle = g > 0 ? c.ink : c.hairline;
                ctx.globalAlpha = g > 0 ? 0.25 + 0.75 * g : 1;
                ctx.lineWidth = 1 + 2 * g;
                for (const a of nodes[li]) {
                    for (const b of nodes[li + 1]) {
                        ctx.beginPath();
                        ctx.moveTo(a.x, a.y);
                        ctx.lineTo(b.x, b.y);
                        ctx.stroke();
                    }
                }
            }
            ctx.globalAlpha = 1;
            nodes.forEach((layer, li) => {
                const g = glow(li);
                for (const node of layer) {
                    ctx.beginPath();
                    ctx.arc(node.x, node.y, nodeR + 4 * g, 0, Math.PI * 2);
                    ctx.fillStyle = c.paper;
                    ctx.fill();
                    if (g > 0) {
                        ctx.globalAlpha = g;
                        ctx.fillStyle = c.ink;
                        ctx.fill();
                        ctx.globalAlpha = 1;
                    }
                    ctx.strokeStyle = c.ink;
                    ctx.lineWidth = 2;
                    ctx.stroke();
                }
            });

            // Samples collected for the next forward pass: one square per callback
            const collected = running ? BLOCKS_PER_MODEL : (k % BLOCKS_PER_MODEL) + (phase > 0.05 ? 1 : 0);
            const sx = nx - (BLOCKS_PER_MODEL * 52 - 12) / 2;
            const sy = ny - 2.5 * nodeGap - 90;
            for (let i = 0; i < BLOCKS_PER_MODEL; ++i) {
                ctx.fillStyle = i < collected ? c.model : c.wash;
                ctx.fillRect(sx + i * 52, sy, 40, 40);
                ctx.strokeStyle = i < collected ? c.ink : c.hairline;
                ctx.lineWidth = 1.5;
                ctx.strokeRect(sx + i * 52 + 0.75, sy + 0.75, 38.5, 38.5);
            }
            text(ctx, `Model block · ${MODEL_BLOCK} samples`, nx, sy - 22,
                 { font: `600 44px ${SANS}`, colour: c.ink, align: 'center' });
            text(ctx, `${BLOCKS_PER_MODEL} × ${HOST_BLOCK}`, nx + (BLOCKS_PER_MODEL * 52 - 12) / 2 + 16, sy + 28,
                 { font: `500 24px ${MONO}`, colour: c.muted, baseline: 'middle' });

            const labelY = ny + 2.5 * nodeGap + 70;
            text(ctx, 'Neural network', nx, labelY, { font: `600 40px ${SANS}`, colour: c.ink, align: 'center' });
            text(ctx, running ? `forward pass · ${Math.round(elapsedMs * SAMPLE_RATE / 1000)} samples` : 'forward pass · time varies', nx, labelY + 40,
                 { font: `500 26px ${MONO}`, colour: running && elapsedMs > CALLBACK_MS ? c.over : c.muted, align: 'center' });
        }
    }

    // ---------- B: the real-time thread, an FFT and a neural network ----------

    const CYCLE_SECONDS = 2.4;            // one model block (46.4 ms) on screen
    const MODEL_MS = MODEL_BLOCK / SAMPLE_RATE * 1000;
    const SPECTRUM_BARS = 40;
    const LATENT = 8;

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
            for (let start = 0; start < n; start += size) {
                for (let k = 0; k < size / 2; ++k) {
                    const wRe = Math.cos(angle * k);
                    const wIm = Math.sin(angle * k);
                    const a = start + k;
                    const b = a + size / 2;
                    const tRe = re[b] * wRe - im[b] * wIm;
                    const tIm = re[b] * wIm + im[b] * wRe;
                    re[b] = re[a] - tRe;
                    im[b] = im[a] - tIm;
                    re[a] += tRe;
                    im[a] += tIm;
                }
            }
        }
    }

    // Log-spaced band levels of one 2048-sample block, 0..1 over 70 dB
    function bands(samples, start, count) {
        const re = new Float64Array(MODEL_BLOCK);
        const im = new Float64Array(MODEL_BLOCK);
        for (let i = 0; i < MODEL_BLOCK; ++i) {
            re[i] = (samples[start + i] || 0) * (0.5 - 0.5 * Math.cos(2 * Math.PI * i / MODEL_BLOCK));
        }
        fft(re, im);
        const levels = new Float32Array(count);
        for (let b = 0; b < count; ++b) {
            const lo = Math.max(1, Math.round(40 * Math.pow(400, b / count) / SAMPLE_RATE * MODEL_BLOCK));
            const hi = Math.max(lo + 1, Math.round(40 * Math.pow(400, (b + 1) / count) / SAMPLE_RATE * MODEL_BLOCK));
            let power = 0;
            for (let k = lo; k < hi; ++k) power = Math.max(power, re[k] * re[k] + im[k] * im[k]);
            levels[b] = 10 * Math.log10(power + 1e-12);
        }
        return levels;
    }

    async function loadSamples(src) {
        const bytes = await (await fetch(src)).arrayBuffer();
        const context = new (window.OfflineAudioContext || window.webkitOfflineAudioContext)(1, 1, SAMPLE_RATE);
        return (await context.decodeAudioData(bytes)).getChannelData(0);
    }

    // Forward-pass times: around 13 ms, sometimes slower, now and then a stall
    function flakyTimes(count) {
        const rand = random(23);
        return Array.from({ length: count }, (_, i) => {
            if (i % 11 === 7) return i % 33 === 7 ? 43 : 22 + rand() * 8;
            if (rand() < 0.15) return 15 + rand() * 1.2;
            return 12.4 + rand() * 1.4;
        });
    }

    class Codec {
        constructor(root) {
            this.root = root;
            this.canvas = root.querySelector('canvas');
            Promise.all([loadSamples(root.dataset.in), loadSamples(root.dataset.out)]).then(([input, output]) => {
                const blocks = Math.floor(Math.min(input.length, output.length) / MODEL_BLOCK);
                const normalise = (all, peak) => all.map(levels => levels.map(v => clamp(1 + (v - peak) / 70)));
                const spectra = Array.from({ length: blocks }, (_, b) => bands(input, b * MODEL_BLOCK, SPECTRUM_BARS));
                const latents = Array.from({ length: blocks }, (_, b) => bands(output, b * MODEL_BLOCK, LATENT));
                const peakOf = all => Math.max(...all.map(levels => Math.max(...levels)));
                const peak = [input, output].reduce((m, all) => all.reduce((a, s) => Math.max(a, Math.abs(s)), m), 0) || 1;
                this.data = {
                    input, output, blocks, peak,
                    spectra: normalise(spectra, peakOf(spectra)),
                    latents: normalise(latents, peakOf(latents)),
                    times: flakyTimes(blocks),
                };
            });
        }

        enter() {
            this.started = performance.now();
        }

        leave() {}

        // A 512-sample piece of a block as a small waveform in a slot
        slot(ctx, c, samples, start, x, y, w, h, { fill = 1, empty = false } = {}) {
            ctx.fillStyle = c.wash;
            ctx.fillRect(x, y, w, h);
            if (!empty && fill > 0) {
                const columns = Math.floor(w * fill / 2);
                const perColumn = HOST_BLOCK / (w / 2);
                ctx.fillStyle = c.ink;
                for (let col = 0; col < columns; ++col) {
                    let lo = 0;
                    let hi = 0;
                    for (let i = 0; i < perColumn; ++i) {
                        const s = (samples[start + Math.floor(col * perColumn + i)] || 0) / this.data.peak;
                        lo = Math.min(lo, s);
                        hi = Math.max(hi, s);
                    }
                    ctx.fillRect(x + col * 2, y + h / 2 - hi * h / 2 * 0.9, 1.4, Math.max(1, (hi - lo) * h / 2 * 0.9));
                }
            }
            ctx.strokeStyle = c.hairline;
            ctx.lineWidth = 1;
            ctx.strokeRect(x + 0.5, y + 0.5, w - 1, h - 1);
        }

        // Encoder (narrowing) or decoder (widening), its layers lit by `progress`
        trapezoid(ctx, c, x, y, w, h, narrowing, progress, label) {
            const small = h * 0.18;
            const left = narrowing ? h : small;
            const right = narrowing ? small : h;
            const mid = y + h / 2;
            ctx.beginPath();
            ctx.moveTo(x, mid - left / 2);
            ctx.lineTo(x + w, mid - right / 2);
            ctx.lineTo(x + w, mid + right / 2);
            ctx.lineTo(x, mid + left / 2);
            ctx.closePath();
            ctx.fillStyle = c.paper;
            ctx.fill();
            const layers = 5;
            for (let i = 0; i < layers; ++i) {
                const f = (i + 0.5) / layers;
                const lx = x + f * w;
                const lh = left + (right - left) * f;
                const lit = progress >= 0 ? clamp(1 - Math.abs(progress * (layers + 1) - 0.5 - i) * 0.9) : 0;
                ctx.fillStyle = c.ink;
                ctx.globalAlpha = 0.12 + 0.88 * lit;
                ctx.fillRect(lx - 5, mid - lh / 2 + 6, 10, lh - 12);
            }
            ctx.globalAlpha = 1;
            ctx.strokeStyle = c.ink;
            ctx.lineWidth = 2;
            ctx.stroke();
            text(ctx, label, x + w / 2, y + h + 34, { font: `500 24px ${MONO}`, colour: c.muted, align: 'center' });
        }

        // FFT or IFFT: three butterfly stages, lit together
        butterfly(ctx, c, x, y, w, h, progress, label) {
            ctx.fillStyle = c.paper;
            ctx.fillRect(x, y, w, h);
            const lit = progress >= 0 && progress <= 1 ? 1 : 0;
            ctx.strokeStyle = c.ink;
            ctx.globalAlpha = 0.18 + 0.82 * lit;
            ctx.lineWidth = 1.5;
            const rows = 8;
            const stages = 3;
            const rowY = r => y + 14 + r * (h - 28) / (rows - 1);
            for (let s = 0; s < stages; ++s) {
                const x0 = x + 14 + s * (w - 28) / stages;
                const x1 = x + 14 + (s + 1) * (w - 28) / stages;
                const span = 1 << s;
                for (let r = 0; r < rows; ++r) {
                    const partner = r ^ span;
                    ctx.beginPath();
                    ctx.moveTo(x0, rowY(r));
                    ctx.lineTo(x1, rowY(r));
                    ctx.moveTo(x0, rowY(r));
                    ctx.lineTo(x1, rowY(partner));
                    ctx.stroke();
                }
            }
            ctx.globalAlpha = 1;
            ctx.strokeStyle = c.ink;
            ctx.lineWidth = 2;
            ctx.strokeRect(x + 1, y + 1, w - 2, h - 2);
            text(ctx, label, x + w / 2, y + h + 34, { font: `500 24px ${MONO}`, colour: c.muted, align: 'center' });
        }

        draw(now) {
            const c = colours();
            const { ctx, width, height } = fit(this.canvas);
            const data = this.data;
            if (!data) {
                text(ctx, 'Loading …', 0, 40, { colour: c.muted });
                return;
            }
            const u = width / 1720;
            const X = v => v * u;

            const t = reducedMotion.matches ? CYCLE_SECONDS * 3.95 : (now - this.started) / 1000;
            const n = Math.floor(t / CYCLE_SECONDS);
            const phase = t / CYCLE_SECONDS - n;
            const incoming = n % data.blocks;
            const processing = n >= 1 ? (n - 1) % data.blocks : -1;

            const laneHeight = height / 2;
            const lanes = [
                { kind: 'fft', top: 0, title: 'FFT' },
                { kind: 'nn', top: laneHeight, title: 'Neural network' },
            ];

            // ----- the real-time thread: one callback of 512 samples every 11.6 ms -----
            const hostW = X(260);
            const sinceCallback = (phase * BLOCKS_PER_MODEL) % 1;
            const pulse = reducedMotion.matches ? 0 : 1 - clamp(sinceCallback / 0.35);
            ctx.fillStyle = c.wash;
            ctx.fillRect(0, 10, hostW, height - 20);
            ctx.strokeStyle = c.ink;
            ctx.globalAlpha = 0.15 + 0.85 * pulse;
            ctx.lineWidth = 2 + 2 * pulse;
            ctx.strokeRect(1, 11, hostW - 2, height - 22);
            ctx.globalAlpha = 1;
            const hx = 24;
            const hmid = height / 2;
            text(ctx, 'Host', hx, 64, { font: `600 40px ${SANS}`, colour: c.ink });
            text(ctx, 'real-time thread', hx, 100, { font: `400 28px ${SANS}`, colour: c.muted });
            text(ctx, `${HOST_BLOCK}`, hx, hmid + 30, { font: `600 110px ${SANS}`, colour: c.ink });
            text(ctx, 'samples per callback', hx, hmid + 72, { font: `400 28px ${SANS}`, colour: c.muted });
            ctx.beginPath();
            ctx.arc(hx + 14, height - 54, 10 + 4 * pulse, 0, Math.PI * 2);
            ctx.fillStyle = c.ink;
            ctx.globalAlpha = 0.2 + 0.8 * pulse;
            ctx.fill();
            ctx.globalAlpha = 1;
            text(ctx, 'callback', hx + 36, height - 46, { font: `500 24px ${MONO}`, colour: c.muted });

            for (const lane of lanes) {
                const y = lane.top;
                const boxTop = y + 80;
                const boxHeight = laneHeight - 170;
                const mid = boxTop + boxHeight / 2;
                const slotW = X(56);
                const slotGap = X(8);
                const slotTop = boxTop + boxHeight * 0.2;
                const slotHeight = boxHeight * 0.6;
                const inX = X(330);
                const slotsW = BLOCKS_PER_MODEL * slotW + (BLOCKS_PER_MODEL - 1) * slotGap;

                // How long this block takes, as a fraction of the cycle
                const ms = lane.kind === 'fft' ? 0 : data.times[processing] ?? 0;
                const duration = lane.kind === 'fft' ? 0.06 : ms / MODEL_MS;
                const progress = processing >= 0 ? phase / duration : -1;
                const done = progress >= 1;

                text(ctx, lane.title, inX, y + 48, { font: `600 44px ${SANS}`, colour: c.ink });

                // Connector from the host through the lane
                ctx.fillStyle = c.hairline;
                ctx.fillRect(hostW, mid - 1, X(1200) - hostW, 2);

                // In: one slot per callback; the 4th completes the block
                for (let s = 0; s < BLOCKS_PER_MODEL; ++s) {
                    const arrival = (s + 1) / BLOCKS_PER_MODEL;
                    const slotX = inX + s * (slotW + slotGap);
                    this.slot(ctx, c, data.input, incoming * MODEL_BLOCK + s * HOST_BLOCK, slotX, slotTop, slotW, slotHeight,
                              { empty: phase < arrival });
                    const flight = (phase - (arrival - 0.1)) / 0.1;
                    if (flight >= 0 && flight < 1) {
                        const fx = hostW + (slotX - hostW) * easeInOut(flight);
                        this.slot(ctx, c, data.input, incoming * MODEL_BLOCK + s * HOST_BLOCK, fx, slotTop, slotW, slotHeight);
                    }
                }
                text(ctx, `${MODEL_BLOCK} samples`, inX + slotsW / 2, boxTop + boxHeight + 38,
                     { font: `600 32px ${SANS}`, colour: c.ink, align: 'center' });

                const analysis = clamp(progress / 0.45, -1, 2);
                const synthesis = progress >= 0.55 ? clamp((progress - 0.55) / 0.45, -1, 2) : -1;
                const aX = X(640);
                const rX = X(800);
                const rW = X(190);
                const sX = X(1020);
                const boxW = X(130);

                if (lane.kind === 'fft') {
                    this.butterfly(ctx, c, aX, boxTop, boxW, boxHeight, progress >= 0 && progress < 0.55 ? 0.5 : -1, 'FFT');
                    this.butterfly(ctx, c, sX, boxTop, boxW, boxHeight, progress >= 0.45 && progress < 1 ? 0.5 : -1, 'IFFT');
                } else {
                    this.trapezoid(ctx, c, aX, boxTop, boxW, boxHeight, true, progress >= 0 && progress < 0.55 ? analysis : -1, 'encoder');
                    this.trapezoid(ctx, c, sX, boxTop, boxW, boxHeight, false, progress >= 0.45 && progress < 1 ? synthesis : -1, 'decoder');
                }

                // In between: spectrum or latent of the block being processed
                const shown = processing >= 0 && progress >= 0.45 ? processing : processing - 1;
                ctx.fillStyle = c.paper;
                ctx.fillRect(rX, boxTop, rW, boxHeight);
                if (shown >= 0) {
                    const appear = processing === shown ? easeOut((progress - 0.45) / 0.2) : 1;
                    if (lane.kind === 'fft') {
                        const levels = data.spectra[shown % data.blocks];
                        const bw = rW / SPECTRUM_BARS;
                        ctx.fillStyle = c.ink;
                        levels.forEach((v, i) => {
                            const bh = v * appear * boxHeight;
                            ctx.fillRect(rX + i * bw + 1, boxTop + boxHeight - bh, bw - 2, bh);
                        });
                    } else {
                        const levels = data.latents[shown % data.blocks];
                        const cell = Math.min(rW / LATENT, boxHeight * 0.5);
                        const cx0 = rX + (rW - cell * LATENT) / 2;
                        levels.forEach((v, i) => {
                            ctx.fillStyle = c.ink;
                            ctx.globalAlpha = (0.08 + 0.92 * v) * appear;
                            ctx.fillRect(cx0 + i * cell + 2, mid - cell / 2, cell - 4, cell);
                            ctx.globalAlpha = 1;
                            ctx.strokeStyle = c.ink;
                            ctx.lineWidth = 1.5;
                            ctx.strokeRect(cx0 + i * cell + 2.75, mid - cell / 2 + 0.75, cell - 5.5, cell - 1.5);
                        });
                    }
                }
                text(ctx, lane.kind === 'fft' ? 'spectrum' : 'latent', rX + rW / 2, boxTop + boxHeight + 34,
                     { font: `500 24px ${MONO}`, colour: c.muted, align: 'center' });

                // While the network runs: how long it has been running
                if (lane.kind === 'nn' && processing >= 0) {
                    const elapsed = Math.min(phase, duration) * MODEL_MS;
                    const running = progress >= 0 && progress < 1;
                    text(ctx, `${Math.round(elapsed * SAMPLE_RATE / 1000)}`, rX + rW / 2, boxTop + 34,
                         { font: `600 36px ${SANS}`, colour: elapsed > CALLBACK_MS ? c.over : running ? c.ink : c.muted, align: 'center' });
                }

                // Time per block in samples of audio, the last ten, against the callback's 512
                const tx = X(1260);
                const tw = width - tx;
                const bars = 10;
                const barW = tw / bars;
                const scale = boxHeight / 25;
                const base = boxTop + boxHeight;
                const deadlineY = base - CALLBACK_MS * scale;
                const finished = done ? processing : processing - 1;
                for (let i = 0; i < bars; ++i) {
                    const block = finished - (bars - 1 - i);
                    if (block < 0) continue;
                    const value = lane.kind === 'fft' ? 0 : data.times[block % data.blocks];
                    const bh = lane.kind === 'fft' ? 3 : Math.min(value, 25) * scale;
                    const fresh = block === finished && done ? 1 - clamp((phase - duration) / 0.15) : 0;
                    ctx.fillStyle = c.ink;
                    ctx.fillRect(tx + i * barW + 3, base - Math.min(bh, base - deadlineY), barW - 6, Math.min(bh, base - deadlineY));
                    if (bh > base - deadlineY) {
                        ctx.fillStyle = c.over;
                        ctx.fillRect(tx + i * barW + 3, base - bh, barW - 6, bh - (base - deadlineY));
                    }
                    if (fresh > 0) {
                        ctx.globalAlpha = fresh * 0.4;
                        ctx.fillStyle = c.ink;
                        ctx.fillRect(tx + i * barW, boxTop - 10, barW, boxHeight + 10);
                        ctx.globalAlpha = 1;
                    }
                }
                ctx.fillStyle = c.hairline;
                ctx.fillRect(tx, base, tw, 2);
                ctx.fillStyle = c.ink;
                for (let dx = 0; dx < tw; dx += 14) ctx.fillRect(tx + dx, deadlineY - 1.5, 8, 3);
                text(ctx, `${HOST_BLOCK}`, width, deadlineY - 10, { font: `500 24px ${MONO}`, colour: c.ink, align: 'right' });
                text(ctx, 'time per block, in samples', tx + tw / 2, base + 38, { font: `500 24px ${MONO}`, colour: c.muted, align: 'center' });
            }

            ctx.fillStyle = c.hairline;
            ctx.fillRect(X(330), laneHeight, width - X(330), 1);
        }
    }

    // ---------- wiring ----------

    const KINDS = { clock: Clock, codec: Codec };

    window.addEventListener('reveal-ready', () => {
        const pieces = new Map();
        for (const root of document.querySelectorAll('.teaser[data-teaser]')) {
            const Kind = KINDS[root.dataset.teaser];
            if (Kind) pieces.set(root, new Kind(root));
        }
        if (pieces.size === 0) return;

        let active = [];
        let frame = 0;

        const tick = now => {
            for (const piece of active) piece.draw(now);
            frame = requestAnimationFrame(tick);
        };

        const update = () => {
            cancelAnimationFrame(frame);
            active.forEach(piece => piece.leave());
            const slide = Reveal.getCurrentSlide();
            active = [...pieces].filter(([root]) => slide.contains(root)).map(([, piece]) => piece);
            active.forEach(piece => piece.enter());
            if (active.length) frame = requestAnimationFrame(tick);
        };

        Reveal.on('slidechanged', update);
        update();
    });
})();
