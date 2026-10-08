# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy", "onnxruntime", "soundfile"]
# ///
"""Render the sound demo for the slides: synthesized slow chords, and what the model makes of them.

The input is generated here, so the demo carries no third-party audio. The model
runs block by block on the stateless ONNX export with the state carried by hand,
the same reference path the C++ steps are checked against.

The progression is played several times in a row so the state has settled, and the middle
pass is kept: input and output are cut to exactly one loop, the output shifted back
by the model's measured latency, so both loop seamlessly and line up in time.

Usage:
    uv run scripts/render_demo.py [--model models/mrp_strengjavera_forward.onnx] [--out slides/assets/audio]
"""

import argparse
import pathlib

import numpy as np
import onnxruntime as ort
import soundfile as sf

SAMPLE_RATE = 44100
BLOCK = 2048
ROOT = pathlib.Path(__file__).resolve().parent.parent

# Slow chords, the way the magnetic resonator piano sounds: no hammer, a swell, a long ring.
# (MIDI notes, start in beats, length in beats), at 120 bpm; one pass is LOOP_BEATS long
CHORDS = [
    ((45, 52, 57, 60, 64), 0, 3),     # A minor
    ((41, 48, 57, 60, 65), 3, 3),     # F
    ((48, 55, 60, 64, 67), 6, 3),     # C
    ((43, 50, 55, 59, 62), 9, 3),     # G
]
BEAT = 0.5
LOOP_BEATS = 12          # 6 s, one pass of the progression
PASSES = 3
ATTACK, RELEASE = 0.6, 0.9
FADE = int(0.08 * SAMPLE_RATE)   # loop crossfade


def synthesize(passes):
    """Chords of decaying sine partials with a slow swell and a release that rings into the next chord.

    Rendered as one continuous signal, so a release that crosses the end of a pass lands at the
    start of the next one and any single pass loops without a click.
    """
    loop = int(LOOP_BEATS * BEAT * SAMPLE_RATE)
    out = np.zeros(loop * passes + int(2 * RELEASE * SAMPLE_RATE))
    for p in range(passes):
        for notes, start, length in CHORDS:
            hold = length * BEAT
            n = int((hold + RELEASE) * SAMPLE_RATE)
            t = np.arange(n) / SAMPLE_RATE
            swell = np.clip(t / ATTACK, 0, 1) ** 2
            release = np.clip((hold + RELEASE - t) / RELEASE, 0, 1)
            envelope = swell * release
            tone = np.zeros(n)
            for note in notes:
                frequency = 440.0 * 2.0 ** ((note - 69) / 12)
                for k in range(1, 7):
                    if frequency * k > SAMPLE_RATE / 2:
                        break
                    # Higher partials quieter and dying faster, as on a string
                    tone += np.sin(2 * np.pi * frequency * k * t) * np.exp(-t * 0.4 * k) / k ** 1.3
            begin = p * loop + int(start * BEAT * SAMPLE_RATE)
            out[begin:begin + n] += envelope * tone
    out *= 0.5 / np.abs(out).max()
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


def seamless(audio):
    """One loop from a slightly longer cut: the overhang is crossfaded into the start.

    The model's output after one pass is not exactly its output at the start of the
    next, so a plain cut clicks at the loop point; an equal-power crossfade hides it.
    """
    loop = len(audio) - FADE
    t = np.linspace(0.0, np.pi / 2, FADE)
    out = audio[:loop].copy()
    out[:FADE] = audio[:FADE] * np.sin(t) + audio[loop:] * np.cos(t)
    return out


def latency(model_path, hop=64):
    """How long the model takes to answer: a tone burst after silence, onset in vs. onset out.

    Measured at four positions within a block (the model works in whole blocks) and
    the median taken; periodic material like the chords would fool a cross-correlation.
    """
    onsets = []
    for phase in (0, BLOCK // 4, BLOCK // 2, 3 * BLOCK // 4):
        start = 20 * BLOCK + phase
        x = np.zeros(40 * BLOCK, dtype=np.float32)
        n = int(0.3 * SAMPLE_RATE)
        x[start:start + n] = 0.6 * np.sin(2 * np.pi * 220 * np.arange(n) / SAMPLE_RATE)
        y = run_model(model_path, x)
        e = np.sqrt((y[:len(y) // hop * hop].reshape(-1, hop) ** 2).mean(axis=1))
        quiet = e[5 * BLOCK // hop:19 * BLOCK // hop]
        onsets.append(int(np.argmax(e[start // hop:] > quiet.mean() + 6 * quiet.std())) * hop)
    return int(np.median(onsets))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", type=pathlib.Path, default=ROOT / "models/mrp_strengjavera_forward.onnx")
    parser.add_argument("--out", type=pathlib.Path, default=ROOT / "slides/assets/audio")
    args = parser.parse_args()

    loop = int(LOOP_BEATS * BEAT * SAMPLE_RATE)
    dry_all = synthesize(PASSES + 1)
    wet_all = run_model(args.model, dry_all)
    lag = latency(args.model)

    # The middle pass: the state has settled, and the output's tail from the pass before is in it
    begin = loop
    dry = seamless(dry_all[begin:begin + loop + FADE])
    wet = seamless(wet_all[begin + lag:begin + lag + loop + FADE])
    duration = loop / SAMPLE_RATE

    args.out.mkdir(parents=True, exist_ok=True)
    sf.write(args.out / "demo_input.wav", dry, SAMPLE_RATE, subtype="PCM_16")
    sf.write(args.out / "demo_output.wav", wet, SAMPLE_RATE, subtype="PCM_16")
    print(f"{duration:.1f} s loop, latency {lag} samples ({1000 * lag / SAMPLE_RATE:.1f} ms), "
          f"peak in {np.abs(dry).max():.2f}, peak out {np.abs(wet).max():.2f}")


if __name__ == "__main__":
    main()
