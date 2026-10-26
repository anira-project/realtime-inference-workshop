/**
 * Pipeline (Threads and latency): audio blocks arrive at a steady rate and
 * fill the input ring; whenever a window is full it is enqueued into a slot,
 * the inference threads run the slots, and every callback dequeues the next
 * finished slot into the output ring and pops one block out of it. With a wait
 * ratio the audio thread may block between its enqueue and its dequeue.
 *
 * A small event simulation in host block periods, drawn as rings, slots and
 * threads. Simplified, not to scale. It runs only while its slide is shown and
 * starts over on entry.
 * data-window: blocks per model window; data-inference: block periods per run;
 * data-wait: the wait ratio; data-period: ms per block period on screen;
 * data-threads, data-slots, data-ring: sizes; data-loop: callbacks per loop.
 *
 * Built up on fragments: data-stage 1 to 5 on the fragments inside .pipeline switch on
 * push, enqueue, the threads, dequeue and pop, one after another, while the
 * simulation keeps running; without fragments everything is on.
 */

(function () {
    const NS = 'http://www.w3.org/2000/svg';
    const ENQUEUE_AT = 0.05;   // within a callback, in block periods
    const DEQUEUE_AT = 0.1;
    const POP_AFTER = 0.05;
    const LIT = 0.15;          // how long a step stays lit

    // An arc of the ring from angle a0 to a1, in degrees, clockwise from the top
    function arc(r, a0, a1) {
        const point = a => {
            const rad = (a - 90) * Math.PI / 180;
            return `${100 + r * Math.cos(rad)} ${100 + r * Math.sin(rad)}`;
        };
        return `M${point(a0)} A${r} ${r} 0 0 1 ${point(a1)}`;
    }

    function ring(container, n) {
        const svg = document.createElementNS(NS, 'svg');
        svg.setAttribute('viewBox', '0 0 200 200');
        const step = 360 / n, gap = 6;
        const segments = Array.from({ length: n }, (_, i) => {
            const path = document.createElementNS(NS, 'path');
            path.setAttribute('d', arc(72, i * step + gap / 2, (i + 1) * step - gap / 2));
            path.setAttribute('class', 'pl-seg');
            svg.appendChild(path);
            return path;
        });
        container.appendChild(svg);
        return segments;
    }

    class Pipeline {
        constructor(root) {
            this.root = root;
            const d = root.dataset;
            this.window = Number(d.window || 2);
            this.inference = Number(d.inference || 2.6);
            this.wait = Number(d.wait || 0);
            this.period = Number(d.period || 800);
            this.numThreads = Number(d.threads || 2);
            this.numSlots = Number(d.slots || 4);
            this.ringSize = Number(d.ring || 8);
            this.loop = Number(d.loop || 16);
            this.staged = root.querySelector('.fragment[data-stage]') !== null;
            this.level = this.staged ? 0 : 5;
            this.frame = null;
            this.build();
        }

        build() {
            this.inBlock = this.root.querySelector('.pl-in .pl-audio');
            this.outBlock = this.root.querySelector('.pl-out .pl-audio');
            this.inSegments = ring(this.root.querySelector('.pl-ring.in .pl-circle'), this.ringSize);
            this.outSegments = ring(this.root.querySelector('.pl-ring.out .pl-circle'), this.ringSize);
            const list = this.root.querySelector('.pl-slots .pl-list');
            this.slotCells = Array.from({ length: this.numSlots }, () => list.appendChild(document.createElement('div')));
            const threads = this.root.querySelector('.pl-threads .pl-list');
            this.threadCells = Array.from({ length: this.numThreads }, (_, i) => {
                const cell = threads.appendChild(document.createElement('div'));
                cell.className = 'pl-thread';
                cell.innerHTML = `<i class="pl-thread-bar"></i><span>thread ${i + 1}</span>`;
                return cell;
            });
            const step = name => this.root.querySelector(`.pl-${name}`);
            this.push = step('push');
            this.enqueue = step('enqueue');
            this.then = step('then');
            this.dequeue = step('dequeue');
            this.pop = step('pop');
            // How much of the allowed wait between enqueue and dequeue was used
            this.waitFill = this.root.querySelector('.pl-wait-fill');
        }

        reset() {
            this.time = 0;
            this.inRing = [];                       // blocks waiting for a full window
            this.outRing = [];                      // processed blocks waiting to be played
            this.slots = Array.from({ length: this.numSlots }, () => ({ state: 'free' }));
            this.threads = Array.from({ length: this.numThreads }, () => null);
            this.nextStamp = 0;
            this.nextPop = 0;
            this.played = null;
            this.nextCallback = 0;
            this.current = null;                    // the callback on screen and its times
            this.events = [];                       // the steps of the running callback
        }

        // Free threads take the oldest waiting slot
        takeJobs(t) {
            if (this.level < 3) return;
            this.threads.forEach((job, i) => {
                if (job) return;
                const waiting = this.slots.filter(s => s.state === 'queued').sort((a, b) => a.stamp - b.stamp)[0];
                if (!waiting) return;
                waiting.state = 'running';
                this.threads[i] = { slot: waiting, start: t, end: t + this.inference };
            });
        }

        // The steps of one callback, each at its time: push, enqueue, (wait), dequeue and pop
        doPush(k) {
            // Nothing takes the block before the push is on; a full ring takes no more
            if (this.level >= 1 && this.inRing.length < this.ringSize) this.inRing.push(k);
            this.current = { k, enqueued: false, waitFrom: k + DEQUEUE_AT, dequeueAt: k + DEQUEUE_AT };
            this.events.push({ at: k + ENQUEUE_AT, run: () => this.doEnqueue(k) });
        }

        doEnqueue(k) {
            while (this.level >= 2 && this.inRing.length >= this.window) {
                const slot = this.slots.find(s => s.state === 'free');
                if (!slot) break;
                Object.assign(slot, { state: 'queued', stamp: this.nextStamp++, blocks: this.inRing.splice(0, this.window) });
                this.current.enqueued = true;
            }
            this.takeJobs(k + ENQUEUE_AT);
            // With a wait ratio the audio thread blocks for the next stamp, at most ratio × block period
            let at = k + DEQUEUE_AT;
            const next = this.slots.find(s => s.stamp === this.nextPop && s.state !== 'free');
            if (this.wait > 0 && next && next.state !== 'done') {
                const job = this.threads.find(j => j && j.slot === next);
                const deadline = at + this.wait;
                at = job ? Math.min(deadline, Math.max(at, job.end)) : deadline;
            }
            this.current.dequeueAt = at;
            this.events.push({ at, run: () => this.doDequeue() });
        }

        doDequeue() {
            const ready = this.slots.find(s => s.state === 'done' && s.stamp === this.nextPop);
            if (this.level >= 4 && ready && this.outRing.length + ready.blocks.length <= this.ringSize) {
                this.outRing.push(...ready.blocks);
                this.nextPop++;
                Object.assign(ready, { state: 'free', stamp: undefined, blocks: undefined });
            }
            this.played = this.level >= 5 && this.outRing.length > 0 ? this.outRing.shift() : null;
        }

        // The highest stage whose fragment is shown
        setLevel() {
            if (!this.staged) return;
            const shown = [...this.root.querySelectorAll('.fragment.visible[data-stage]')]
                .map(e => Number(e.dataset.stage));
            this.level = Math.max(0, ...shown);
            this.takeJobs(this.time);
        }

        // Every event up to time t, in time order; a model that ends with a dequeue ends first
        advance(t) {
            for (;;) {
                const finishing = this.threads.filter(Boolean).reduce((a, j) => (!a || j.end < a.end ? j : a), null);
                const event = this.events.reduce((a, e) => (!a || e.at < a.at ? e : a), null);
                const pushAt = this.events.length === 0 ? this.nextCallback : Infinity;
                const at = Math.min(finishing ? finishing.end : Infinity, event ? event.at : Infinity, pushAt);
                if (at > t) break;
                if (finishing && finishing.end === at) {
                    finishing.slot.state = 'done';
                    this.threads[this.threads.indexOf(finishing)] = null;
                    this.takeJobs(at);
                } else if (event && event.at === at) {
                    this.events.splice(this.events.indexOf(event), 1);
                    event.run();
                } else {
                    this.doPush(this.nextCallback++);
                }
            }
        }

        step(now) {
            if (this.startedAt === undefined) this.startedAt = now;
            const t = (now - this.startedAt) / this.period;
            if (t >= this.loop) {
                this.startedAt = now;
                this.reset();
            } else {
                this.time = t;
                this.advance(t);
            }
            this.render();
            this.frame = requestAnimationFrame(next => this.step(next));
        }

        render() {
            const t = this.time, c = this.current;
            const lit = (from, on = true) => Boolean(on && c && t >= from && t < from + LIT);

            this.push.classList.toggle('on', lit(c ? c.k : 0, this.level >= 1));
            this.enqueue.classList.toggle('on', lit(c ? c.k + ENQUEUE_AT : 0, c && c.enqueued));
            this.dequeue.classList.toggle('on', lit(c ? c.dequeueAt : 0, this.level >= 4));
            this.pop.classList.toggle('on', lit(c ? c.dequeueAt + POP_AFTER : 0, this.level >= 5));
            this.inBlock.classList.toggle('on', lit(c ? c.k : 0));
            this.outBlock.classList.toggle('on', lit(c ? c.dequeueAt + POP_AFTER : 0) && this.played !== null);
            this.outBlock.classList.toggle('silence', !c || this.played === null);

            // The way down from enqueue to dequeue, lit while the audio thread is on it
            this.then.classList.toggle('on', Boolean(this.level >= 4 && c && t >= c.waitFrom - 0.05 && t < c.dequeueAt + 0.05));
            if (this.waitFill && this.wait > 0) {
                const waited = c ? Math.max(0, Math.min(t, c.dequeueAt) - c.waitFrom) : 0;
                const shown = c && t < c.k + 0.9 ? waited / this.wait : 0;
                this.waitFill.style.height = `${Math.min(1, shown) * 100}%`;
            }

            const filled = new Set(this.inRing.map(b => b % this.ringSize));
            this.inSegments.forEach((seg, i) => seg.classList.toggle('full', filled.has(i)));
            const out = new Set(this.outRing.map(b => b % this.ringSize));
            this.outSegments.forEach((seg, i) => seg.classList.toggle('full', out.has(i)));

            this.slotCells.forEach((cell, i) => { cell.className = `pl-slot ${this.slots[i].state}`; });

            this.threadCells.forEach((cell, i) => {
                const job = this.threads[i];
                const progress = job ? Math.min(1, (t - job.start) / (job.end - job.start)) : 0;
                cell.querySelector('.pl-thread-bar').style.width = `${progress * 100}%`;
            });
        }

        start() {
            this.stop();
            this.reset();
            this.setLevel();
            this.startedAt = undefined;
            this.frame = requestAnimationFrame(now => this.step(now));
        }

        stop() {
            if (this.frame !== null) cancelAnimationFrame(this.frame);
            this.frame = null;
        }
    }

    const pipelines = new Map();

    function update() {
        const current = Reveal.getCurrentSlide();
        pipelines.forEach((pipeline, root) => {
            if (current && current.contains(root)) {
                if (pipeline.frame === null) pipeline.start();
            } else {
                pipeline.stop();
            }
        });
    }

    window.addEventListener('reveal-ready', () => {
        document.querySelectorAll('.reveal .slides .pipeline').forEach(root => pipelines.set(root, new Pipeline(root)));
        Reveal.on('slidechanged', update);
        const restage = () => pipelines.forEach(pipeline => pipeline.setLevel());
        Reveal.on('fragmentshown', restage);
        Reveal.on('fragmenthidden', restage);
        update();
    });
})();
