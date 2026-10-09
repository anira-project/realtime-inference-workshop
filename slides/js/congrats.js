// Confetti from the check mark whenever a slide with .congrats is shown.
window.addEventListener('reveal-ready', () => {
  // The deck's four tints, deepened so they read on paper, plus ink
  const COLOURS = ['#b9a582', '#7fa592', '#9aab76', '#8b86b8', 'var(--ink)'];
  const PIECES = 90;

  const burst = ({ currentSlide }) => {
    const origin = currentSlide.querySelector('.congrats');
    if (!origin || matchMedia('(prefers-reduced-motion: reduce)').matches) return;
    origin.querySelectorAll('.confetti').forEach(piece => piece.remove());

    for (let i = 0; i < PIECES; i++) {
      const piece = document.createElement('i');
      piece.className = 'confetti';
      piece.style.background = COLOURS[i % COLOURS.length];
      origin.appendChild(piece);

      const angle = Math.random() * Math.PI * 2;
      const distance = 260 + Math.random() * 520;
      const x = Math.cos(angle) * distance;
      // Mostly upwards, so the fall ends above the footer
      const y = -Math.abs(Math.sin(angle)) * distance * 0.5;
      const spin = (Math.random() - 0.5) * 1440;
      piece.animate([
        { transform: 'translate(-50%, -50%) rotate(0deg) scale(0.4)', opacity: 1 },
        { transform: `translate(calc(-50% + ${x}px), calc(-50% + ${y}px)) rotate(${spin / 2}deg) scale(1)`, opacity: 1, offset: 0.35 },
        { transform: `translate(calc(-50% + ${x * 1.15}px), calc(-50% + ${y + 300}px)) rotate(${spin}deg) scale(1)`, opacity: 0 },
      ], {
        duration: 2200 + Math.random() * 1200,
        delay: 600 + Math.random() * 150,
        easing: 'cubic-bezier(0.2, 0.7, 0.3, 1)',
        fill: 'both',
      }).finished.then(() => piece.remove());
    }
  };

  Reveal.on('slidechanged', burst);
  burst({ currentSlide: Reveal.getCurrentSlide() });
});
