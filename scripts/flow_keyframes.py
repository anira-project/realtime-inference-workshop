# Usage: python3 scripts/flow_keyframes.py slides/css/exercise.css
# Step 6 flow keyframes: a callback a second, 512 in and 512 out every
# callback; the worker pops each block at once, runs the model on every
# fourth, and pushes the result; a block that comes in meanwhile waits in
# m_input. The start, with the prefill, plays once (0..S); from S the state
# repeats every P seconds, and that part loops forever.
import sys
S, P = 5.0, 20.0           # P: 4 blocks per model run, 5 slots in m_output
EPS = 0.02
N_IN = int(P)              # elements per class: block k uses element k % P
HOST_IN = (8, 8); HOST_OUT = (1628, 8); MODEL = (895, 198)
def m_in(j): return (228 + 88 * j, 8)
def blk(j): return (428 + 88 * j, 198)
def m_out(s): return (1128 + 88 * s, 8)
def model_start(g): return 4 * g + 3.9
def model_end(g): return 4 * g + 4.6
def push_end(g): return 4 * g + 4.85
G = 60  # groups simulated, far past S + P

def busy_until(t):
    for g in range(G):
        if model_start(g) <= t < push_end(g): return push_end(g)
    return t

def in_block(k):
    g, j = divmod(k, 4)
    a = float(k); pop = busy_until(a + 0.5); s = model_start(g)
    return [(a - EPS, HOST_IN, 0), (a, HOST_IN, 1), (a + 0.33, m_in(j), 1), (pop, m_in(j), 1),
            (pop + 0.33, blk(j), 1), (s, blk(j), 1), (s + 0.3, MODEL, 1), (s + 0.4, MODEL, 0)]

def pop_out(c, slot):
    return [(c, m_out(slot), 1), (c + 0.33, HOST_OUT, 1), (c + 0.67, HOST_OUT, 1), (c + 0.8, HOST_OUT, 0)]

def result(k):
    g = k // 4; c = k + 5; slot = c % 5
    return [(model_end(g) - EPS, MODEL, 0), (model_end(g), MODEL, 1), (push_end(g), m_out(slot), 1)] + pop_out(float(c), slot)

def prefill(s):
    return [(0.0, m_out(s), 1)] + pop_out(float(s), s)

def at(pts, t):
    if t <= pts[0][0]: return pts[0][1], pts[0][2]
    for (t0, p0, o0), (t1, p1, o1) in zip(pts, pts[1:]):
        if t0 <= t <= t1:
            f = 0 if t1 == t0 else (t - t0) / (t1 - t0)
            return (round(p0[0] + f * (p1[0] - p0[0]), 1), round(p0[1] + f * (p1[1] - p0[1]), 1)), round(o0 + f * (o1 - o0), 3)
    return pts[-1][1], pts[-1][2]

def frames(name, pts, a, b):
    ts = sorted({a, b} | {t for t, _, _ in pts if a < t < b})
    out = [f"@keyframes {name} {{"]
    for t in ts:
        (x, y), o = at(pts, t)
        out.append(f"  {(t - a) / (b - a) * 100:.2f}% {{ transform: translate({x:g}px, {y:g}px); opacity: {o:g}; }}")
    return "\n".join(out + ["}"])

def scalar_frames(name, prop, spans, on, off, a, b):
    # spans: absolute (start, end) where prop = on
    ts = {a, b}
    for s, e in spans:
        for t in (s - EPS, s, e, e + EPS):
            if a < t < b: ts.add(t)
    def val(t):
        return on if any(s <= t <= e for s, e in spans) else off
    out = [f"@keyframes {name} {{"]
    for t in sorted(ts):
        out.append(f"  {(t - a) / (b - a) * 100:.2f}% {{ {prop}: {val(t)}; }}")
    return "\n".join(out + ["}"])

kf, rules, n = [], [], 0
def two_phase(name, pts):
    global n
    n += 1
    kf.append(frames(name + "-a", pts, 0, S))
    kf.append(frames(name + "-b", pts, S, S + P))
    rules.append(f"html.tanh .reveal section.present .flow i:nth-of-type({n}) {{ animation: {name}-a {S:g}s linear 1, {name}-b {P:g}s linear {S:g}s infinite; }}")

def merged(fn, e):
    pts = []
    for k in range(e, 4 * G, N_IN): pts += fn(k)
    return pts

for e in range(N_IN): two_phase(f"flow-in-{e}", merged(in_block, e))
for s in range(5):
    n += 1
    kf.append(frames(f"flow-pre-{s}-a", prefill(s), 0, S))
    rules.append(f"html.tanh .reveal section.present .flow i:nth-of-type({n}) {{ animation: flow-pre-{s}-a {S:g}s linear 1 forwards; }}")
for e in range(N_IN): two_phase(f"flow-res-{e}", merged(result, e))

model = [(model_start(g), model_end(g)) for g in range(G)]
busy = [(model_start(g), push_end(g)) for g in range(G)]
push = [(model_end(g), model_end(g) + 0.8) for g in range(G)]
for nm, prop, spans, on, off in [("flow-model", "background", model, "var(--mark-model)", "transparent"),
                                 ("flow-push-say", "opacity", push, "1", "0")]:
    kf.append(scalar_frames(nm + "-a", prop, spans, on, off, 0, S))
    kf.append(scalar_frames(nm + "-b", prop, spans, on, off, S, S + P))
def anim(nm): return f"{nm}-a {S:g}s linear 1, {nm}-b {P:g}s linear {S:g}s infinite"

css = "/* flow:begin, made by scripts/flow_keyframes.py slides/css/exercise.css: edit the script, not this */\n\n" + "\n\n".join(kf) + """

@keyframes flow-tick {
  0% { box-shadow: 0 0 0 6px var(--mark-check); }
  30%, 100% { box-shadow: 0 0 0 0 transparent; }
}

@keyframes flow-blink {
  0% { opacity: 1; }
  100% { opacity: 0.25; }
}

@keyframes flow-rotate {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

""" + "\n".join(rules) + f"""
html.tanh .reveal section.present .flow-host {{ animation: flow-tick 1s linear infinite; }}
html.tanh .reveal section.present .flow-model {{ animation: {anim('flow-model')}; }}
html.tanh .reveal section.present .flow-say.spin .blink {{ display: inline-block; animation: flow-blink 0.35s steps(2) infinite; }}
html.tanh .reveal section.present .flow-spinner {{ animation: flow-rotate 0.6s linear infinite; }}
html.tanh .reveal section.present .flow-say.push {{ animation: {anim('flow-push-say')}; }}
/* flow:end */"""

p = sys.argv[1]
s = open(p).read()
if "/* flow:begin" in s:
    a = s.index("/* flow:begin"); b = s.index("/* flow:end */") + len("/* flow:end */")
else:
    a = s.index("@keyframes flow-in-0 {")
    m = "html.tanh .reveal section.present .flow-say.push { animation: flow-push-say 13s linear infinite; }"
    b = s.index(m) + len(m)
s = s[:a] + css + s[b:]
s = s.replace("while the model runs: that block waits in m_input. 13 s, a callback a second.",
              "while the model runs: that block waits in m_input. A callback a second: the\n * start with the prefill plays once, 5 s, then 20 s that loop forever.")
open(p, "w").write(s)
print("elements:", n)
