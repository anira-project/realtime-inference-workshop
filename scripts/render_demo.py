# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy", "onnxruntime", "soundfile"]
# ///
"""Render the sound demo for the slides: a synthesized phrase, and what the model makes of it.

The input is generated here, so the demo carries no third-party audio. The model
runs block by block on the stateless ONNX export with the state carried by hand,
the same reference path the C++ steps are checked against.

Usage:
    uv run scripts/render_demo.py [--model models/forward.onnx] [--out slides/assets/audio]
"""

import argparse
import pathlib

import numpy as np
import onnxruntime as ort
import soundfile as sf

SAMPLE_RATE = 48000
BLOCK = 2048
ROOT = pathlib.Path(__file__).resolve().parent.parent

# (MIDI note, start in beats, length in beats), at 120 bpm
PHRASE = [
    (45, 0, 1), (57, 1, 0.5), (55, 1.5, 0.5), (52, 2, 1), (48, 3, 1),
    (45, 4, 1), (57, 5, 0.5), (60, 5.5, 0.5), (59, 6, 1.5), (52, 7.5, 0.5),
    (45, 8, 2),
]
BEAT = 0.5


def synthesize(duration):
    """Saw notes through a decaying one-pole lowpass, with a short attack and release."""
    out = np.zeros(int(duration * SAMPLE_RATE), dtype=np.float64)
    for note, start, length in PHRASE:
        frequency = 440.0 * 2.0 ** ((note - 69) / 12)
        n = int(length * BEAT * SAMPLE_RATE)
        t = np.arange(n) / SAMPLE_RATE
        saw = 2.0 * ((frequency * t) % 1.0) - 1.0
        envelope = np.minimum(1.0, t / 0.005) * np.minimum(1.0, (n - np.arange(n)) / (0.02 * SAMPLE_RATE))
        cutoff = 300.0 + 4000.0 * np.exp(-t * 6.0)
        coefficient = np.exp(-2.0 * np.pi * cutoff / SAMPLE_RATE)
        filtered = np.empty(n)
        state = 0.0
        for i in range(n):
            state = (1.0 - coefficient[i]) * saw[i] + coefficient[i] * state
            filtered[i] = state
        begin = int(start * BEAT * SAMPLE_RATE)
        out[begin:begin + n] += 0.5 * envelope * filtered
    return out.astype(np.float32)


def run_model(model_path, audio):
    session = ort.InferenceSession(str(model_path), providers=["CPUExecutionProvider"])
    inputs = session.get_inputs()
    out_names = [o.name for o in session.get_outputs()]
    # Batch size forced to 1; the first input is the audio, the rest is state
    states = [np.zeros([d if isinstance(d, int) and d > 0 else 1 for d in i.shape], dtype=np.float32)
              for i in inputs[1:]]

    padded = np.concatenate([audio, np.zeros((-len(audio)) % BLOCK, dtype=np.float32)])
    out = []
    for begin in range(0, len(padded), BLOCK):
        block = padded[begin:begin + BLOCK].reshape(1, 1, BLOCK)
        result = session.run(out_names, dict(zip([i.name for i in inputs], [block] + states)))
        out.append(result[0].reshape(-1))
        states = result[1:]
    return np.concatenate(out)[:len(audio)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", type=pathlib.Path, default=ROOT / "models/forward.onnx")
    parser.add_argument("--out", type=pathlib.Path, default=ROOT / "slides/assets/audio")
    args = parser.parse_args()

    duration = (max(start + length for _, start, length in PHRASE) * BEAT) + 0.5
    dry = synthesize(duration)
    wet = run_model(args.model, dry)

    args.out.mkdir(parents=True, exist_ok=True)
    sf.write(args.out / "demo_input.wav", dry, SAMPLE_RATE, subtype="PCM_16")
    sf.write(args.out / "demo_output.wav", wet, SAMPLE_RATE, subtype="PCM_16")
    print(f"{duration:.1f} s, peak in {np.abs(dry).max():.2f}, peak out {np.abs(wet).max():.2f}")


if __name__ == "__main__":
    main()
