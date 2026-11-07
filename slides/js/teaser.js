/**
 * Teaser animations for the introduction. Each runs on a canvas, only while its
 * slide is shown, and redraws in the current theme's colours.
 *
 * <div class="teaser" data-teaser="clock" data-work="fft"><canvas></canvas></div>
 * <div class="teaser" data-teaser="clock" data-work="nn"><canvas></canvas></div>
 *     One turn of the clock is one callback of 512 samples. Every 4th turn a block of
 *     2048 is complete and gets processed: by an FFT and IFFT ("fft"), which takes about
 *     one sample's worth of time and the same for every block, or by the neural network
 *     ("nn"), whose time varies and does not fit. Time is counted in samples of audio,
 *     like the block sizes. Both slides draw the same clock; only the work changes.
 *
 * Numbers: 44.1 kHz, host block 512, block 2048. The network takes about 13 ms per
 * forward pass on ONNX Runtime (Step 3), slower calls now and then; the variation shown
 * is an illustration of that, not a measurement. The FFT's 1 sample is an upper bound:
 * numpy's 2048-point rfft, multiply and irfft took 22 µs on an Apple Silicon laptop
 * under Asahi Linux, Python's call overhead included. Copying a block costs next to
 * nothing. Both are drawn as the thinnest visible tick.
 */

(() => {
    const SAMPLE_RATE = 44100;
    const MODEL_BLOCK = 2048;
    const HOST_BLOCK = 512;
    const BLOCKS_PER_MODEL = MODEL_BLOCK / HOST_BLOCK;
    const CALLBACK_MS = HOST_BLOCK / SAMPLE_RATE * 1000;   // 11.6
    const FORWARD_MS = 13;
    const FFT_SAMPLES = 1;
    const COPY_SAMPLES = 0;
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

    // FFT or IFFT: three butterfly stages, drawn darker and thicker by `lit` (0..1)
    function butterfly(ctx, c, x, y, w, h, lit, label) {
        ctx.fillStyle = c.paper;
        ctx.fillRect(x, y, w, h);
        const rows = 8;
        const stages = 3;
        const pad = 18;
        const rowY = r => y + pad + r * (h - 2 * pad) / (rows - 1);
        ctx.strokeStyle = c.ink;
        ctx.globalAlpha = 0.18 + 0.82 * lit;
        ctx.lineWidth = 1.5 + 1.5 * lit;
        for (let s = 0; s < stages; ++s) {
            const x0 = x + pad + s * (w - 2 * pad) / stages;
            const x1 = x + pad + (s + 1) * (w - 2 * pad) / stages;
            const span = 1 << s;
            for (let r = 0; r < rows; ++r) {
                ctx.beginPath();
                ctx.moveTo(x0, rowY(r));
                ctx.lineTo(x1, rowY(r));
                ctx.moveTo(x0, rowY(r));
                ctx.lineTo(x1, rowY(r ^ span));
                ctx.stroke();
            }
        }
        ctx.globalAlpha = 1;
        ctx.lineWidth = 2;
        ctx.strokeRect(x + 1, y + 1, w - 2, h - 2);
        text(ctx, label, x + w / 2, y + h + 34, { font: `500 24px ${MONO}`, colour: c.muted, align: 'center' });
    }

    // ---------- the clock: one callback per turn, an FFT or a neural network every 4th ----------

    const TURN_SECONDS = 1.5;    // one callback on screen
    const TICK = 0.008;          // thinnest arc drawn for work, in turns
    const LAYERS = [3, 5, 5, 3];

    class Clock {
        constructor(root) {
            this.root = root;
            this.canvas = root.querySelector('canvas');
            this.fft = root.dataset.work === 'fft';
        }

        enter() {
            this.started = performance.now();
            this.rand = random(5);
            this.work = [];
        }

        leave() {}

        // How many turns callback k needs: a copy, or the work on a whole block
        workOf(k) {
            while (this.work.length <= k) {
                const blockComplete = this.work.length % BLOCKS_PER_MODEL === BLOCKS_PER_MODEL - 1;
                this.work.push(!blockComplete ? COPY_SAMPLES / HOST_BLOCK
                    : this.fft ? FFT_SAMPLES / HOST_BLOCK
                    : forwardPass(this.rand) / CALLBACK_MS);
            }
            return this.work[k];
        }

        draw(now) {
            const c = colours();
            const { ctx, width, height } = fit(this.canvas);
            // Without motion: the FFT just ran, or the network has just missed
            const still = this.fft ? 3.15 : 4.2;
            const t = reducedMotion.matches ? TURN_SECONDS * still : (now - this.started) / 1000;
            const k = Math.floor(t / TURN_SECONDS);
            const phase = t / TURN_SECONDS - k;
            const blockComplete = k % BLOCKS_PER_MODEL === BLOCKS_PER_MODEL - 1;

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
            // This callback's own work, at least a visible tick
            arc(ctx, cx, cy, R, ownStart, Math.min(phase, ownStart + Math.max(ownWork, TICK), 1), c.ink, ring);

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

            // ----- the work on a block: an FFT or the network -----
            const nx = width * 0.72;
            const ny = cy - 20;
            const nodeGap = 74;
            const labelY = ny + 2.5 * nodeGap + 70;

            let running = false;
            if (this.fft) {
                // The FFT takes about one sample: light it up when its block is complete, then fade
                const lit = blockComplete && phase >= 0.05 ? 1 - clamp((phase - 0.15) / 0.3) : 0;
                const boxW = 170;
                const boxH = 300;
                const boxGap = 90;
                const left = nx - boxW - boxGap / 2;
                ctx.fillStyle = c.hairline;
                ctx.fillRect(left - 20, ny - 1, 2 * boxW + boxGap + 40, 2);
                butterfly(ctx, c, left, ny - boxH / 2, boxW, boxH, lit, 'FFT');
                butterfly(ctx, c, nx + boxGap / 2, ny - boxH / 2, boxW, boxH, lit, 'IFFT');

                text(ctx, 'Fourier transform', nx, labelY, { font: `600 40px ${SANS}`, colour: c.ink, align: 'center' });
                text(ctx, `FFT + IFFT · ≈ ${FFT_SAMPLES} sample · every block`, nx, labelY + 40,
                     { font: `500 26px ${MONO}`, colour: c.muted, align: 'center' });
            } else {
                const layerGap = 130;
                const nodeR = 17;
                const nodes = LAYERS.map((count, li) => Array.from({ length: count }, (_, ni) => ({
                    x: nx + (li - (LAYERS.length - 1) / 2) * layerGap,
                    y: ny + (ni - (count - 1) / 2) * nodeGap,
                })));

                // Is a forward pass running, and how far is it?
                let progress = -1;
                let elapsedMs = 0;
                if (blockComplete && phase >= ownStart) {
                    progress = (phase - ownStart) / ownWork;
                    elapsedMs = (phase - ownStart) * CALLBACK_MS;
                } else if (overflow > 0 && phase < overflow) {
                    const previous = this.workOf(k - 1);
                    progress = (1 + phase) / previous;
                    elapsedMs = (1 + phase) * CALLBACK_MS;
                }
                running = progress >= 0 && progress <= 1;
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

                text(ctx, 'Neural network', nx, labelY, { font: `600 40px ${SANS}`, colour: c.ink, align: 'center' });
                text(ctx, running ? `forward pass · ${Math.round(elapsedMs * SAMPLE_RATE / 1000)} samples` : 'forward pass · time varies', nx, labelY + 40,
                     { font: `500 26px ${MONO}`, colour: running && elapsedMs > CALLBACK_MS ? c.over : c.muted, align: 'center' });
            }

            // Samples collected for the next block: one square per callback
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
            text(ctx, `${this.fft ? 'FFT' : 'Model'} block · ${MODEL_BLOCK} samples`, nx, sy - 22,
                 { font: `600 44px ${SANS}`, colour: c.ink, align: 'center' });
            text(ctx, `${BLOCKS_PER_MODEL} × ${HOST_BLOCK}`, nx + (BLOCKS_PER_MODEL * 52 - 12) / 2 + 16, sy + 28,
                 { font: `500 24px ${MONO}`, colour: c.muted, baseline: 'middle' });
        }
    }

    // ---------- wiring ----------

    const KINDS = { clock: Clock };

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
