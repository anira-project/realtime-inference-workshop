// Every slide restarts its animations when it is shown. A Task pipeline (.run) then plays
// its cards one after the other, one card per TODO: each card gets .is-on when it starts
// (its own animations are keyed to that) and .is-current while it is the active one.
// After the last card the pipeline gets .is-checked, holds, and starts over.
const RUN_START = 400;
const RUN_HOLD = 2600;

window.addEventListener('reveal-ready', () => {
  const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');
  let timers = [];

  const later = (callback, delay) => timers.push(setTimeout(callback, delay));

  const play = run => {
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
    later(() => play(run), time + RUN_HOLD);
  };

  const enter = ({ currentSlide }) => {
    timers.forEach(clearTimeout);
    timers = [];
    for (const animation of currentSlide.getAnimations({ subtree: true })) {
      animation.currentTime = 0;
    }
    currentSlide.querySelectorAll('.run').forEach(play);
  };

  Reveal.on('slidechanged', enter);
  enter({ currentSlide: Reveal.getCurrentSlide() });
});
