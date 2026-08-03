# LED Wrist Band — Electronics

Bench wiring and power notes for the cylinder band (up to 8 rows × N LEDs/row).

Product decisions (grid pitch, wear, diffusion fabric): see [`../DESIGN.md`](../DESIGN.md).

## Defaults

| Item | Value |
|------|--------|
| MCU | Seeed XIAO ESP32 (S3 or C3) |
| LEDs | WS2812B, GRB |
| Grid | `ROWS = 8`, `LEDS_PER_ROW = 24` → **192 LEDs** |
| Data | `D0` → level shifter → strip DIN |
| Button | `D1` → button → GND (`INPUT_PULLUP`) |
| Brightness | Cap ~20–30% in firmware (`BRIGHTNESS` 48–72 typical) |

## Power budget

WS2812B worst case ≈ **60 mA** per LED at full white; design with ~**40 mA** average headroom for pink noise, then run at ~25% brightness.

| Grid | LEDs | Full white (~40 mA) | At 25% brightness |
|------|------|---------------------|-------------------|
| 8 × 24 | 192 | ~7.7 A | ~1.9 A |
| 6 × 20 | 120 | ~4.8 A | ~1.2 A |
| 4 × 16 | 64 | ~2.6 A | ~0.65 A |

**Recommendations**

- Size the 5V supply for the **25% operating** column plus ~30% margin, not theoretical full white
- Inject 5V at **both ends** of a long strip (or every ~1 m) if using one continuous chain of rows
- Common **GND** between PSU, XIAO, and strip is mandatory
- Prefer a dedicated 5V pack / buck; do not power 100+ LEDs from the XIAO 5V pin

## Wiring (v1 straight index)

```
index = row * LEDS_PER_ROW + col
row 0 = wrist edge → ROWS-1 = outer cuff
col wraps the circumference
```

```
5V PSU ----+---- strip 5V (inject as needed)
           |
           +---- XIAO 5V (MCU only)  [or USB for bench MCU]

GND  ------+---- strip GND
           +---- XIAO GND
           +---- button other side

XIAO D0 → 74AHCT125 (or similar) → strip DIN
XIAO D1 → button → GND
```

Level shifter: ESP32 is 3.3 V logic; WS2812B data prefers 5 V. A single-gate buffer powered at 5 V is enough for one strip DIN.

## Wear layout — XIAO at the wrist

**Decision:** the XIAO lives on the wrist in a small **3D-printed box**. LED data stays local. The battery bank stays elsewhere on the body; a long-sleeved shirt hides the power cable.

```
[5V bank]  --sleeve cable (5V + GND only)--  [wrist box: XIAO + shifter + button]
                                              |-- short local runs --> LED rows
```

| Piece | Where | Notes |
|-------|--------|------|
| Battery bank | Pocket / belt / upper-arm pouch | Sized for ~2 A @ 25% on 8×24 |
| Sleeve harness | Under long sleeve | **Power only** — 5V + GND; use wire thick enough for current (not jumper wire) |
| Wrist box (3D print) | On-band / cuff | XIAO, level shifter, mode button; USB access for flashing; vents if needed |
| LED cylinder | Band itself | DIN from box; inject 5V/GND from the same sleeve feed at both ends of the strip if the chain is long |

**Why this split**
- Shortest possible data path (fewer glitches)
- Sleeve cable is only two conductors → easier to hide and strain-relieve
- Band can use a cuff pigtail / connector so the shirt harness unplugs for washing

**Box wishlist (v1)**
- XIAO USB-C opening
- Button on a reachable face
- Strain relief for sleeve power in and strip out
- Mount that doesn’t spin freely on the wrist (strap slot / sew tabs)

## Diffusion fabric

Full rationale in [`../DESIGN.md`](../DESIGN.md). Short version:

- **Primary:** white milliskin / dance lycra / matte spandex over the matrix  
- **Optional:** 2–3 mm white foam under the lycra if pixels still poke through  
- **Optional outer:** fashion fabric on top (keep a white diffuser underneath)  
- **Avoid:** fur/neoprene bulk, black over LEDs, clear vinyl alone  

Physical target: **rings** around the wrist (~**16 mm** pitch on each ring, ~**8 mm** between rings; aligned grid, not staggered). See [`../DESIGN.md`](../DESIGN.md).

## Mic (later)

Ripple / Party presets expect a mic + FFT path with relative auto-gain.
Not required for smoke test or band-demo (demo beat is baked in).

## Bring-up order

1. Flash `firmware/xiao-smoke-test` — onboard blink + optional cyan chase
2. Confirm row/column count matches the physical strip length
3. Flash `firmware/xiao-band-demo` — layout cycle + button skip
4. Tune `ROWS` / `LEDS_PER_ROW` / `BRIGHTNESS` before sewing into the band
