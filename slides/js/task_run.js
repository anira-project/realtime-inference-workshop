// Every slide restarts its animations when it is shown. A Task pipeline (.run) then plays
// its cards one after the other, one card per TODO: each card gets .is-on when it starts
// (its own animations are keyed to that) and .is-current while it is the active one.
// After the last card the pipeline gets .is-checked, holds, and starts over.
//
// In steps 1 to 8 every loop repeats only every LOOP_EVERY ms: a looping CSS animation
// plays once, holds its end, and starts over then; a pipeline holds until then.
const RUN_START = 400;
const RUN_HOLD = 10600;
const LOOP_EVERY = 20000;
const slowLoops = slide => /^0[1-8]-/.test(slide.closest('section[data-file]')?.dataset.file || '');

window.addEventListener('reveal-ready', () => {
  const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');
  let timers = [];
  let loops = [];

  const later = (callback, delay) => timers.push(setTimeout(callback, delay));

  const play = (run, slow) => {
    const steps = [...run.querySelectorAll('.run-step')];
    steps.forEach(step => step.classList.remove('is-on', 'is-current'));
    run.classList.remove('is-checked');
    // Restart the CSS animations: they come back with the classes
    void run.offsetWidth;

    if (reducedMotion.matches) {
      steps.forEach(step => step.classList.add('is-on'));
      run.classList.add('is-checked');
      return;
    }

    let time = RUN_START;
    for (const step of steps) {
      later(() => {
        steps.forEach(other => other.classList.remove('is-current'));
        step.classList.add('is-on', 'is-current');
      }, time);
      time += Number(step.dataset.seconds || 2) * 1000;
    }
    later(() => {
      steps.forEach(step => step.classList.remove('is-current'));
      run.classList.add('is-checked');
    }, time);
    later(() => play(run, slow), slow ? Math.max(time + 2000, LOOP_EVERY) : time + RUN_HOLD);
  };

  const enter = ({ currentSlide }) => {
    timers.forEach(timer => { clearTimeout(timer); clearInterval(timer); });
    timers = [];
    const slow = slowLoops(currentSlide);
    loops = [];
    for (const animation of currentSlide.getAnimations({ subtree: true })) {
      if (slow && animation.effect.getTiming().iterations === Infinity) {
        animation.effect.updateTiming({ iterations: 1, fill: 'forwards' });
        loops.push(animation);
      }
      animation.currentTime = 0;
    }
    if (loops.length > 0) {
      timers.push(setInterval(() => loops.forEach(restart), LOOP_EVERY));
    }
    currentSlide.querySelectorAll('.run').forEach(run => play(run, slow));
  };

  const restart = animation => {
    animation.currentTime = 0;
    animation.play();
  };

  // A loop inside a fragment may have played out before the click: start it when it shows
  Reveal.on('fragmentshown', ({ fragment }) => {
    fragment.getAnimations({ subtree: true })
      .filter(animation => loops.includes(animation))
      .forEach(restart);
  });

  Reveal.on('slidechanged', enter);
  enter({ currentSlide: Reveal.getCurrentSlide() });
});
