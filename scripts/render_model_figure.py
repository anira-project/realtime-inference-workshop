"""
Renders the "Our model" figure from the model's real activations.

Runs a 220 Hz sine through the ONNX encoder and decoder block by block (state
carried), taps the output of every resolution stage, and writes one PNG per
stage plus the inline SVG between the model-figure markers in
slides/00-neural-networks.md.

    uv run --with onnx --with onnxruntime --with numpy --with pillow \
        scripts/render_model_figure.py <models-dir>
"""

import base64
import io
import re
import sys
from pathlib import Path

import numpy as np
import onnx
import onnxruntime as ort
from onnx import TensorProto, helper
from PIL import Image

SAMPLE_RATE = 44100
BLOCK = 2048
SHOWN_BLOCK = 5          # after the state has filled
ROOT = Path(__file__).resolve().parent.parent
SLIDE = ROOT / "slides" / "00-neural-networks.md"

INK = np.array([28, 28, 26])
PAPER = np.array([255, 255, 255])
NEGATIVE = np.array([139, 134, 184])


def stride(node):
    attributes = {a.name: helper.get_attribute_value(a) for a in node.attribute}
    return max(attributes.get("strides", [1]))


def tapped_session(path, keep):
    """The model with the output of every node `keep` accepts added as a graph output."""
    model = onnx.load(path)
    shapes = {i.name: list(i.dims) for i in model.graph.initializer}
    taps = [n.output[0] for n in model.graph.node
            if n.op_type in ("Conv", "ConvTranspose") and keep(n, shapes)]
    for name in taps:
        model.graph.output.append(helper.make_tensor_value_info(name, TensorProto.FLOAT, None))
    return ort.InferenceSession(model.SerializeToString()), taps


def heatmap(values, width, height):
    """Channels top to bottom, time left to right; ink for positive, lilac for negative."""
    scale = np.abs(values).max() or 1.0
    # Square-root scale, sign kept, so quiet structure next to a loud band stays visible
    v = np.sign(values) * np.sqrt(np.abs(values) / scale)
    rgb = np.where(v[..., None] >= 0,
                   PAPER + (INK - PAPER) * v[..., None],
                   PAPER + (NEGATIVE - PAPER) * -v[..., None])
    image = Image.fromarray(rgb.astype(np.uint8))
    # Nearest in time keeps the steps visible; channels are averaged down to the box
    image = image.resize((width * 2, values.shape[0]), Image.NEAREST).resize((width * 2, height * 2), Image.BOX)
    buffer = io.BytesIO()
    image.save(buffer, "PNG", optimize=True)
    return "data:image/png;base64," + base64.b64encode(buffer.getvalue()).decode()


def wave_path(samples, x, y, width, amplitude):
    scale = np.abs(samples).max() or 1.0
    points = [f"{x + i * width / (len(samples) - 1):.1f} {y - s / scale * amplitude:.1f}"
              for i, s in enumerate(samples) if i % 4 == 0]
    return '<path class="mf-wave" d="M' + " L".join(points) + '"/>'


def main(models):
    encoder, encoder_taps = tapped_session(
        models / "mrp_strengjavera_encoder.onnx",
        lambda n, s: stride(n) > 1 or s.get(n.input[1]) == [96, 16, 7])
    decoder, decoder_taps = tapped_session(
        models / "mrp_strengjavera_decoder.onnx",
        lambda n, s: n.op_type == "ConvTranspose" or s.get(n.input[1], [0])[0] in (1536, 16))

    t = np.arange(BLOCK * (SHOWN_BLOCK + 1)) / SAMPLE_RATE
    signal = (0.5 * np.sin(2 * np.pi * 220 * t)).astype(np.float32)
    encoder_state = np.zeros(encoder.get_inputs()[1].shape, np.float32)
    decoder_state = np.zeros(decoder.get_inputs()[1].shape, np.float32)
    for k in range(SHOWN_BLOCK + 1):
        block = signal[k * BLOCK:(k + 1) * BLOCK].reshape(1, 1, BLOCK)
        encoded = encoder.run(None, {"audio": block, "state_in": encoder_state})
        latents, encoder_state = encoded[0], encoded[1]
        decoded = decoder.run(None, {"latents": latents, "state_in": decoder_state})
        decoder_state = decoded[1]

    encoder_stages = [a[0] for a in encoded[2:]]                  # 16x128, 96x128, 192x32, 384x8, 768x2, 1536x1
    decoder_stages = [a[0] for a in decoded[2:]]                  # 1536x1, 768, 384, 192, 96 (padded), 16x128
    z = latents[0, :, 0]
    output = decoded[0][0, 0]
    # Transposed convolutions return their overlap too; keep the block's own steps
    nominal = {768: 2, 384: 8, 192: 32, 96: 128}
    decoder_stages = [s[:, -nominal[s.shape[0]]:] if s.shape[0] in nominal else s for s in decoder_stages]

    # One column per resolution, shared by both rows, so each decoder stage sits under its encoder twin
    columns = [("wave", 230), ((16, 128), 170), ((96, 128), 170), ((192, 32), 110),
               ((384, 8), 64), ((768, 2), 34), ((1536, 1), 22)]
    height_for = {16: 40, 96: 80, 192: 100, 384: 120, 768: 145, 1536: 170}
    gap = 46
    x_of, x = {}, 40
    for key, width in columns:
        x_of[key] = (x, width)
        x += width + gap
    last = x - gap

    def arrow(x1, x2, y, leftward):
        tip, tail = (x1, x2) if leftward else (x2, x1)
        head = tip + 9 if leftward else tip - 9
        return (f'<path class="mf-arrow" d="M{tail} {y} L{head} {y}"/>'
                f'<path class="mf-head" d="M{tip} {y} L{head} {y - 5} L{head} {y + 5}Z"/>')

    def row(wave, stages, y, title, leftward):
        x0, w = x_of["wave"]
        parts = [wave_path(wave, x0, y, w, 44),
                 f'<text x="{x0 + w / 2}" y="{y - 70}" class="mf-title">{title}</text>']
        previous_end = x0 + w
        for stage in stages:
            channels, steps = stage.shape
            sx, sw = x_of[(channels, steps)]
            h = height_for[channels]
            parts.append(arrow(previous_end + 10, sx - 10, y, leftward))
            parts.append(f'<image class="mf-map" x="{sx}" y="{y - h / 2:.0f}" width="{sw}" height="{h}" '
                         f'preserveAspectRatio="none" href="{heatmap(stage, sw, h)}"/>')
            parts.append(f'<rect class="mf-frame" x="{sx}" y="{y - h / 2:.0f}" width="{sw}" height="{h}" rx="2"/>')
            parts.append(f'<text x="{sx + sw / 2}" y="{y + 108}" class="mf-shape">{channels}×{steps}</text>')
            previous_end = sx + sw
        return "".join(parts)

    top = row(signal[SHOWN_BLOCK * BLOCK:(SHOWN_BLOCK + 1) * BLOCK], encoder_stages, 140, "in", False)
    # The decoder runs right to left: 1536x1 back up to 16 bands, then the inverse PQMF to samples
    bottom = row(output, decoder_stages[::-1], 450, "out", True)

    cells = []
    zx, cell_h = last + 90, 30
    z_top = 295 - (len(z) * (cell_h + 4)) / 2
    for i, value in enumerate(z):
        y = z_top + i * (cell_h + 4)
        shade = heatmap(np.array([[value]]) / np.abs(z).max(), 1, 1)
        cells.append(f'<image x="{zx}" y="{y:.0f}" width="52" height="{cell_h}" preserveAspectRatio="none" href="{shade}"/>'
                     f'<rect class="mf-frame" x="{zx}" y="{y:.0f}" width="52" height="{cell_h}" rx="2"/>'
                     f'<text x="{zx + 64}" y="{y + 21:.0f}" class="mf-value">{value:+.2f}</text>')
    z_bottom = z_top + len(z) * (cell_h + 4)

    svg = f"""<svg class="mf" viewBox="0 0 1600 600" aria-hidden="true">
  <text x="40" y="24" class="mf-row">encoder: time shrinks, channels grow</text>
  {top}
  <path class="mf-link" d="M{last + 10} 140 C {zx - 20} 140, {zx + 26} {z_top - 40:.0f}, {zx + 26} {z_top - 30:.0f}"/>
  <text x="{zx + 26}" y="{z_top - 8:.0f}" class="mf-title">z</text>
  {"".join(cells)}
  <path class="mf-link" d="M{zx + 26} {z_bottom + 8:.0f} C {zx + 26} {z_bottom + 40:.0f}, {zx - 20} 450, {last + 10} 450"/>
  {bottom}
  <text x="40" y="592" class="mf-row">decoder: back to 2048 samples</text>
  <text x="1470" y="290" class="mf-ratio">2048 → {len(z)}</text>
  <text x="1470" y="330" class="mf-note">{BLOCK // len(z)}× fewer numbers</text>
  <text x="1470" y="358" class="mf-note">for {1000 * BLOCK / SAMPLE_RATE:.1f} ms of audio</text>
</svg>"""

    text = SLIDE.read_text()
    text = re.sub(r"(<!-- model-figure -->\n).*?(\n<!-- /model-figure -->)",
                  lambda m: m.group(1) + svg + m.group(2), text, flags=re.S)
    SLIDE.write_text(text)
    print(f"z: {np.round(z, 2)}  stages: {[s.shape for s in encoder_stages]}")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
