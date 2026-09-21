# Step 6: On the audio thread

**Goal:** Call inference directly in the audio callback and build with RTSan.

**Problem it reveals:** Allocations, locks and syscalls inside the engine: inference engines are not real-time safe.

## What to do

TODO(workshop)

## Expected output

TODO(workshop)

## If you're stuck

TODO(workshop)
