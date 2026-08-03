# LED Wrist Band — Firmware Effect Spec

Cylinder LED band worn on the wrist. Simulator: `band-sim.html`.

This is its own look language — not a port of the jacket spoke effects.

## Hardware context

- **Form:** short LED **cylinder** — up to **8 rings** stacked up the arm (see `DESIGN.md`)
- **Rows** = rings along the arm: `0` = wrist-edge ring → `ROWS-1` = outer-cuff ring
- **Columns** = position around each ring: `0` → `LEDS_PER_ROW-1`
- **Defaults (tunable):** `ROWS = 8`, `LEDS_PER_ROW = 24` → 192 LEDs
- **MCU:** Seeed XIAO ESP32 + WS2812B (GRB) + 1 mode button
- **Power:** single 5V bank; brightness capped ~20–30%
- **Wiring v1:** `index = row * LEDS_PER_ROW + col` (serpentine optional later)

## Pixel map

```
row r, col c  →  index = r * LEDS_PER_ROW + col
float orbit   // 0→1 position around the band (azimuth)
float pulse   // 0→1 soft heartbeat / bloom amount
```

Expose in firmware:

- `ROWS` (1–8)
- `LEDS_PER_ROW` (~16–32 typical)
- `NUM_LEDS = ROWS * LEDS_PER_ROW`
- `float orbit`, `float pulse`, `float huePhase`

## Shared parameters

| Param | Range | Role |
|-------|--------|------|
| `floor` | 0–20% | Idle glow between motion |
| `brightness` | master | Overall cap |
| `orbitSpeed` | | How fast patterns travel around the wrist |
| `pulseSpeed` | | Heartbeat / bloom rate |
| `bass / mids / highs` | 0–1 | Mic FFT or demo beat |
| `colorMode` | enum | Fixed / Cycle / Rainbow / Dual |
| `hueSpeed` | | Colour drift rate |

**Music mapping (band-native):**

- **Bass** → kick flash, ripple spawn, orbit punch
- **Mids** → trail length / body glow
- **Highs** → sparkle / tip glitter on the cuff rows

---

## Layouts (sim + firmware looks)

### 1. Orbit
A bright head travels around the wrist with a soft trailing wake. All rows share the same azimuth (reads as a vertical bar sliding around the arm). Bass kick briefly boosts head brightness.

### 2. Twin Orbit
Two heads on opposite sides of the band, counter-rotating. Favourite “jewelry in motion” look.

### 3. Heartbeat
Whole band soft-breathes. On each beat, a quick radial bloom: wrist rows flash first, then expand toward the cuff and fall back — like a pulse on the skin.

### 4. Scanner
Thin bright column sweeps around; the rest stays at `floor`. Clean tech / radar feel. Speed tracks `orbitSpeed`; kick can jump the scanner ahead.

### 5. Ripple
Kick drops a “stone” at the current orbit column. Rings expand left/right around the circumference and fade. Multiple overlapping ripples allowed.

### 6. Ember
Warm palette crawl: hot spots drift slowly upward through rows (wrist → cuff) while wandering in column. Flicker on highs. No cold teals — fire/amber/magenta.

### 7. Aurora
Soft vertical curtains of colour that slowly shear around the band. Low contrast, dreamy; good for chill / breathe moments.

### 8. Off / dim
Floor-only or near-black.

## Colour modes

| Mode | Behaviour |
|------|-----------|
| **Fixed** | One HSV (cool or warm depending on preset) |
| **Cycle** | Whole band shares one drifting hue |
| **Rainbow** | Hue varies by column around the wrist |
| **Dual** | Head A and head B (or left/right ripples) use opposite hues |

## User controls (real band)

Sim has sliders. Firmware stays dance-simple.

### Hardware
- **1× mode button** — short press cycles presets
- **Long-press** — brightness Low / Med / High (under safety cap)
- No app for v1

### Presets (short-press cycle)

| # | Name | Layout + colour |
|---|------|-----------------|
| 1 | **Orbit** | Orbit, Fixed cool, low floor |
| 2 | **Twins** | Twin Orbit, Dual colour |
| 3 | **Heartbeat** | Heartbeat, Cycle, soft |
| 4 | **Scanner** | Scanner, Fixed cool |
| 5 | **Ripple** | Ripple + mic/demo kicks, Rainbow |
| 6 | **Ember** | Ember, warm Fixed/Cycle |
| 7 | **Party** | Twin Orbit + mic punch + sparkle highs |
| 8 | **Off / dim** | Floor only |

Bake `floor`, speeds, and FFT gains into each preset — not user-facing.

### Mic
On for Ripple / Party (and optional kick accents on Orbit). Auto-gain: relative bass rises vs slow noise floor so clubs and quiet rooms both work.

### Rule of thumb
If it needs a slider in the sim, it becomes a **preset or a long-press**, not another button.

## Implementation notes

- HSV → RGB per LED; apply `floor` and master brightness last
- Drive `orbit` / `pulse` / `huePhase` from `millis()` every frame
- Keep ripple list small (e.g. max 4 active) for MCU headroom
- Support `ROWS` 1–8 without rewriting patterns

## Simulator reference

`band-sim.html` — match firmware visuals to that file when porting.
