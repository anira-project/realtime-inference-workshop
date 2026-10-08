<h1>Real-Time Neural<br>Inference</h1>

From an exported neural model to a real-time-safe audio plugin

<!-- .slide: data-state="no-header" -->
---

## Who this is for

- You write **real-time safe C++ DSP**: no allocations, no locks, no waiting on the audio thread
- Today: how to bring a **neural network** into that environment
- Neural network basics help, but are not required

---

## Agenda

{{AGENDA}}

Each step **fixes the last step's problem and exposes the next one**.
