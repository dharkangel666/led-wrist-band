# LED Wrist Band — Design decisions

Summary of the concept locked so far. Effects language is wrist-native (see `FIRMWARE-EFFECTS.md`); this file covers physical build and wear.

## Concept

- Short LED **cylinder** on the wrist — up to **8 parallel rows**
- Own looks (Orbit, Twins, Heartbeat, Scanner, Ripple, Ember, Aurora) — **not** a port of the fur jacket spoke effects
- Simple dancing UX later: one mode button, presets; sim keeps the design sliders

## LED layout

| Choice | Decision |
|--------|----------|
| Physical build | **Rings** around the wrist, stacked up the arm — not parallel strips running wrist→cuff |
| Why rings | Orbit / Twins / Scanner / Ripple move around the arm; LEDs in that direction stay neighbours on the strip |
| Grid | **Aligned** rings/columns (rectangular matrix) — not staggered / brick |
| Why aligned | Cleaner mapping for motion **and** future glyphs; staggered only softens diagonals a little and complicates firmware |
| Column pitch | ~**16 mm** along each ring (typical 60 LED/m strip spacing) |
| Row / ring pitch | ~**8 mm** between ring centres — keeps band width wearable |
| Pixel shape | Rectangular (~16×8 mm). Effects fine; glyphs need aspect-aware bitmaps or 2-col logical pixels later |
| Glyphs | **Deferred** — heart/smiley were examples only; pick icons later |

**8 rings @ 8 mm:** centre-to-centre span ≈ **56 mm** along the arm (much shorter than 16 mm ring spacing ≈ 112 mm).

**LEDs per ring** from wrist size:

```text
LEDS_PER_ROW ≈ wrist_circumference_mm / 16
```

Tune sim/firmware to the measured count (default 24 in code is a design placeholder, not a sew target).

**Index (v1 — one ring after another):**

```text
index = row * LEDS_PER_ROW + col
row 0 = wrist-edge ring → ROWS-1 = outer-cuff ring
col   = position around that ring
```

Sew as circumferential channels in the cuff; snake data ring 0 → ring 1 → … (or one continuous serpentine). Heartbeat bloom (along the arm) is just adjacent rings — still fine.

## Power and wear

| Piece | Where |
|-------|--------|
| Battery bank | Pocket / belt / upper-arm pouch |
| Cable | **Under a long-sleeved shirt** — only way that scales for this LED count |
| Sleeve harness | **5V + GND only** (power feed sized for current, not jumper wire) |
| Control box | **3D-printed box on the wrist** — XIAO + level shifter + mode button |
| Data | Stays local at the wrist (short DIN runs to the strip) |

Bank → sleeve → wrist box → short runs to LED rows. Optional cuff connector so the shirt harness unplugs for washing.

Details: [`electronics/README.md`](electronics/README.md).

## Diffusion fabric

Need soft glow **without** jacket-style faux fur bulk.

**v1 stack**

1. LED matrix on the band  
2. Optional **2–3 mm white foam / wadding** only if dots still show through  
3. **White milliskin / dance lycra / matte spandex** (one layer; two if needed)  
4. Optional outer fashion layer (can be coloured/patterned if the diffuser under it is white)

| Fabric | Role |
|--------|------|
| White milliskin / lycra | **Primary diffuser** — thin, stretchy, strong blur |
| White jersey | Backup — easy sew, slightly less smooth |
| White tricot lining | Very thin under-layer |
| Soft white power mesh (2×) | Lighter diffusion, more “dotty” |
| Thin white felt | Strong blur patch; less stretch |

**Avoid over the LEDs:** black fabric, clear vinyl alone, thick neoprene/fur, very shiny materials that turn hotspots into glare.

Stretch the white layer slightly taut over the matrix. At 8 mm row / 16 mm column, one lycra layer is usually enough.

## Firmware / sim defaults (software)

- Grid tunable: rows 1–8, LEDs/row adjustable  
- Physical sew target should follow **16 mm col / 8 mm row** and real circumference  
- Glyph preset: add when icons are chosen  

## Live simulator

- Local: `band-sim.html` (sync to `deploy/index.html` before upload)
- Live: https://dharkangel.com/led-wrist-band/ (also https://dharkangel.com/led-wristband/)
- Deploy: upload `deploy/index.html` + `deploy/.htaccess` via main Bluehost FTP (`jezthumy` → `public_html/…`)
- If the page looks stale, hard-refresh — the title should show **v12** when the current build is loaded

## Out of scope for now

- Specific glyph set  
- Mic path on hardware (demo beat in sim/firmware first)  
- Sync with the jacket  
