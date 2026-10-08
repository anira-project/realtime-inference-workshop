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

---

## How the exercises work

1. **Slides** — each step starts here: the problem, and the idea for fixing it
2. **`steps/NN_name/exercise/`** — your code, with TODOs to fill in
3. **`steps/NN_name/solution/`** — the finished step, to compare or to catch up

```plaintext
./build/bin/step01_exercise     your version
./build/bin/step01_solution     the reference
```

Letting an agent solve it gains you nothing — then read the solution directly.
