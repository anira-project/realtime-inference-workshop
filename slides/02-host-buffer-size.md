<h1>Step 2<br>Model input size<br>vs. host buffer size</h1>

Two block sizes that have nothing to do with each other

<!-- .slide: data-state="no-header" -->

Note:
    - Ask the room what buffer sizes they ship with. Collect the spread.

---

## Two numbers, two owners

**The model's block** comes from the export — 2048 samples, fixed at trace time.

**The host's block** comes from the device, the user's settings, the sample
rate — 64, 128, 480, 512 — and it can change while you are running.

They match by accident, once.

---

## What that means in a callback

```
host:   |64|64|64|64|64|64|64| ...        32 callbacks
model:  |------ 2048 ------|              1 forward pass
```

- 31 callbacks with **not enough** to run the model
- 1 callback that has to run it — and then produce 64 samples
- A host block of 480 does not even divide 2048

---

## The shape of the fix

```
host in  →  [ ring buffer ]  →  model (2048)  →  [ ring buffer ]  →  host out
```

Two buffers, because both sides set their own pace. The pump is four lines:

```cpp
input.push(samples, num_samples);

while (input.available() >= model_input_size) {
    input.pop(block.data(), model_input_size);
    engine.process(block.data(), model_input_size);
    output.push(block.data(), model_input_size);
}
```

---

## Your job

1. **Size the buffers** — what is the worst moment?
2. **Write the pump** — whole model blocks only
3. **Hand samples back** — including when there are none yet

```bash
./build/bin/step02_exercise
```

It runs at 64, 128, 480, 512, 1024 and 2048 and checks all of them.

Note:
    - The overflow/underflow exceptions are the teaching tool here; let them hit both.

---

## What we check

**What the model produced** — not what the host received.

```
  host block 64            max abs diff 2.21e-06   OK
  host block 480           max abs diff 2.21e-06   OK
  host block 2048          max abs diff 2.21e-06   OK
```

Same stream as step 1, whatever the host does with the chopping.

---

## What broke?

Two things, quietly:

- The host got **silence** at the start — the model cannot answer before it has
  2048 samples. That is latency; we come back to it.
- Every callback now either does nothing, or runs a **whole forward pass**.

How long does that take? And how long may it take?

<!-- .slide: data-state="no-footer" -->

Note:
    - Straight into step 3: measure it, then compare with the budget.
