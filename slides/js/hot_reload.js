// Hot reload WebSocket connection
// The slide position survives the reload in the URL hash (Reveal's hash: true)
(function() {
  const ws = new WebSocket(`ws://${window.location.host}`);

  ws.onmessage = function(event) {
    if (event.data === 'reload') {
      window.location.reload();
    }
  };

  ws.onopen = function() {
    console.log('🔥 Hot reload connected');
  };

  ws.onclose = function() {
    console.log('🔌 Hot reload disconnected');
  };
})();
