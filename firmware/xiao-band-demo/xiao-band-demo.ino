/*
 * LED Wrist Band — cylinder look demo (no mic)
 *
 * Own effect language: Orbit, Twins, Heartbeat, Scanner, Ripple, Ember, Aurora.
 * Cycles looks with 4 s crossfades + demo beat.
 *
 * D0 → level shifter → strip DIN
 * D1 → button → GND  (short = skip to next look)
 */

#include <FastLED.h>
#include <math.h>

#define LED_PIN         D0
#define BTN_PIN         D1
#define ROWS            8
#define LEDS_PER_ROW    24
#define NUM_LEDS        (ROWS * LEDS_PER_ROW)
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BRIGHTNESS      64
#define HOLD_MS         4000
#define FADE_MS         4000
#define MAX_RIPPLES     4

#ifndef LED_BUILTIN
#define LED_BUILTIN 21
#endif

enum Look : uint8_t {
  LOOK_ORBIT = 0,
  LOOK_TWINS,
  LOOK_HEARTBEAT,
  LOOK_SCANNER,
  LOOK_RIPPLE,
  LOOK_EMBER,
  LOOK_AURORA,
  LOOK_SCOPE,
  LOOK_COUNT
};

static const char* LOOK_NAME[] = {
  "Orbit",
  "Twins",
  "Heartbeat",
  "Scanner",
  "Ripple",
  "Ember",
  "Aurora",
  "Scope",
};

CRGB leds[NUM_LEDS];
CRGB bufA[NUM_LEDS];
CRGB bufB[NUM_LEDS];

float orbit = 0;
float pulse = 0;
float huePhase = 0;
float emberPhase = 0;
float demoBass = 0;
float demoKick = 0;
float demoMids = 0;
float demoHighs = 0;
float heartEnv = 0;
float heartEnv2 = 0;
bool heartDubPending = false;
uint32_t lastHeartTrig = 0;

struct Ripple {
  float col;
  float age;
  bool active;
};
Ripple ripples[MAX_RIPPLES];
uint32_t lastKickMs = 0;

uint8_t lookIndex = 0;
bool fading = false;
uint32_t holdUntil = 0;
uint32_t fadeStart = 0;
uint8_t fromLook = 0;
uint8_t toLook = 0;

bool btnHeld = false;
uint8_t lowStreak = 0, highStreak = 0;
uint32_t lastBtnAction = 0;

inline uint16_t ledIndex(uint8_t row, uint8_t col) {
  return (uint16_t)row * LEDS_PER_ROW + col;
}

inline float tOf(uint8_t row) {
  return (row + 0.5f) / (float)ROWS;
}

CRGB hsvByte(float h, float s, float v) {
  h = fmodf(h, 360.0f);
  if (h < 0) h += 360.0f;
  CHSV hsv((uint8_t)(h * 255.0f / 360.0f),
           (uint8_t)constrain(s * 255.0f, 0, 255),
           (uint8_t)constrain(v * 255.0f, 0, 255));
  CRGB rgb;
  hsv2rgb_rainbow(hsv, rgb);
  return rgb;
}

float angDist(float a, float b, float n) {
  float d = fabsf(a - b);
  return min(d, n - d);
}

void spawnRipple(float col) {
  for (uint8_t i = 0; i < MAX_RIPPLES; i++) {
    if (!ripples[i].active) {
      ripples[i].col = col;
      ripples[i].age = 0;
      ripples[i].active = true;
      return;
    }
  }
  // reuse oldest
  uint8_t oldest = 0;
  for (uint8_t i = 1; i < MAX_RIPPLES; i++) {
    if (ripples[i].age > ripples[oldest].age) oldest = i;
  }
  ripples[oldest].col = col;
  ripples[oldest].age = 0;
  ripples[oldest].active = true;
}

void tickDemoAudio(uint32_t now) {
  const uint32_t period = 500;
  uint32_t ph = now % period;
  demoKick = ph < 70 ? (1.0f - ph / 70.0f) : 0.0f;
  float swell = 0.35f + 0.25f * (0.5f + 0.5f * sinf(now / 180.0f));
  demoBass = max(demoKick, swell * 0.55f);
  demoMids = 0.35f + 0.2f * sinf(now / 140.0f);
  demoHighs = 0.2f + 0.15f * sinf(now / 90.0f);

  orbit = fmodf(orbit + 0.012f, 1.0f);
  if (lookIndex == LOOK_SCANNER && demoKick > 0.85f) {
    orbit = fmodf(orbit + 0.08f, 1.0f);
  }
  pulse = fmodf(pulse + 0.016f, 1.0f);
  huePhase = fmodf(huePhase + 0.35f, 360.0f);
  emberPhase = fmodf(emberPhase + 0.004f, 1.0f);

  // Heartbeat envelopes from bass kicks only
  bool heartTrig = demoKick > 0.42f || (demoBass > 0.62f && demoKick > 0.2f);
  if (heartTrig && (now - lastHeartTrig) > 170) {
    lastHeartTrig = now;
    heartEnv = min(1.0f, 0.8f + demoKick * 0.35f);
    heartEnv2 = 0;
    heartDubPending = true;
  }
  if (heartDubPending && (now - lastHeartTrig) > 110) {
    heartEnv2 = 0.55f;
    heartDubPending = false;
  }
  heartEnv = max(0.0f, heartEnv - 0.055f);
  heartEnv2 = max(0.0f, heartEnv2 - 0.055f);
  if (demoBass < 0.1f && demoKick < 0.06f) {
    heartEnv *= 0.8f;
    heartEnv2 *= 0.8f;
  }

  for (uint8_t i = 0; i < MAX_RIPPLES; i++) {
    if (!ripples[i].active) continue;
    ripples[i].age += 0.018f;
    if (ripples[i].age >= 1.2f) ripples[i].active = false;
  }

  if (demoKick > 0.75f && (now - lastKickMs) > 180) {
    lastKickMs = now;
    if (lookIndex == LOOK_RIPPLE || toLook == LOOK_RIPPLE || fromLook == LOOK_RIPPLE) {
      spawnRipple(orbit * LEDS_PER_ROW);
    }
  }
}

float baseHue(uint8_t col, bool twin, uint8_t colorMode) {
  // 0 fixed, 1 cycle, 2 rainbow, 3 dual
  if (colorMode == 0) return twin ? fmodf(200.0f + 180.0f, 360.0f) : 200.0f;
  if (colorMode == 1) return twin ? fmodf(huePhase + 140.0f, 360.0f) : huePhase;
  if (colorMode == 2) return fmodf(huePhase + (col / (float)LEDS_PER_ROW) * 360.0f, 360.0f);
  return twin ? fmodf(200.0f + 160.0f + huePhase * 0.2f, 360.0f)
              : fmodf(200.0f + huePhase * 0.15f, 360.0f);
}

void renderLook(uint8_t look, CRGB* out) {
  fill_solid(out, NUM_LEDS, CRGB::Black);
  const float floorL = 0.05f;
  const float pos = orbit * LEDS_PER_ROW;
  const float b = demoBass;
  const float kick = demoKick;

  for (uint8_t r = 0; r < ROWS; r++) {
    float tRow = tOf(r);
    for (uint8_t c = 0; c < LEDS_PER_ROW; c++) {
      float energy = floorL;
      bool twin = false;
      uint8_t colorMode = 0;
      float hue = 200;
      float sat = 1.0f;

      switch (look) {
        case LOOK_ORBIT: {
          colorMode = 0;
          float d = angDist((float)c, pos, (float)LEDS_PER_ROW);
          float trail = 2.4f + demoMids * 1.8f;
          float head = expf(-(d * d) / 0.45f) * (d < 0.9f ? 1.15f : 1.0f);
          float wake = d < trail ? expf(-(d * d) / (trail * 0.55f)) * 0.75f : 0.0f;
          float punch = 0.85f + 0.15f * max(kick, b);
          energy = min(1.0f, floorL + max(head, wake) * punch * (1.0f - floorL));
          break;
        }
        case LOOK_TWINS: {
          colorMode = 3;
          float p1 = pos;
          float p2 = fmodf(pos + LEDS_PER_ROW / 2.0f, (float)LEDS_PER_ROW);
          float d1 = angDist((float)c, p1, (float)LEDS_PER_ROW);
          float d2 = angDist((float)c, p2, (float)LEDS_PER_ROW);
          float trail = 2.2f + demoMids * 1.5f;
          float e1 = (d1 < trail) ? expf(-(d1 * d1) / (trail * 0.35f)) : 0.0f;
          float e2 = (d2 < trail) ? expf(-(d2 * d2) / (trail * 0.35f)) : 0.0f;
          twin = e2 > e1;
          float punch = 0.85f + 0.15f * max(kick, b);
          energy = min(1.0f, floorL + max(e1, e2) * punch * (1.0f - floorL));
          break;
        }
        case LOOK_HEARTBEAT: {
          colorMode = 0;
          hue = 330.0f;
          float thump = max(heartEnv, heartEnv2 * 0.65f);
          if (thump < 0.03f) {
            energy = 0;
            break;
          }
          float bloomR = 0.12f + thump * 0.95f;
          float edge = max(0.0f, 1.0f - max(0.0f, tRow - bloomR) * 4.8f);
          float body = thump * (0.55f + 0.45f * (1.0f - tRow * 0.35f));
          energy = min(1.0f, body * edge);
          break;
        }
        case LOOK_SCANNER: {
          colorMode = 0;
          hue = 185;
          float d = angDist((float)c, pos, (float)LEDS_PER_ROW);
          float gate = 0;
          if (d < 0.55f) gate = 1.0f;
          else if (d < 1.2f) gate = 1.0f - (d - 0.55f) / 0.65f;
          energy = min(1.0f, floorL + gate * (0.9f + 0.1f * kick) * (1.0f - floorL));
          break;
        }
        case LOOK_RIPPLE: {
          colorMode = 2;
          float rip = 0;
          for (uint8_t i = 0; i < MAX_RIPPLES; i++) {
            if (!ripples[i].active) continue;
            float radius = ripples[i].age * LEDS_PER_ROW * 0.55f;
            float d = angDist((float)c, ripples[i].col, (float)LEDS_PER_ROW);
            float ring = expf(-powf((d - radius) * 2.4f, 2));
            float fade = max(0.0f, 1.0f - ripples[i].age);
            rip = max(rip, ring * fade);
          }
          energy = min(1.0f, floorL + rip * (0.85f + 0.15f * b));
          break;
        }
        case LOOK_EMBER: {
          colorMode = 0;
          float crawl = fmodf(tRow + emberPhase + sinf(c * 0.7f + orbit * 6.0f) * 0.08f, 1.0f);
          if (crawl < 0) crawl += 1.0f;
          float hot = expf(-powf((crawl - 0.55f) * 4.5f, 2));
          float wander = 0.35f + 0.65f * expf(-powf(angDist((float)c, pos, (float)LEDS_PER_ROW) / (LEDS_PER_ROW * 0.35f), 2));
          float flick = 0.75f + 0.25f * (0.5f + 0.5f * sinf(c + huePhase));
          energy = min(1.0f, floorL + hot * wander * flick * (0.7f + 0.3f * b));
          hue = 10.0f + energy * 35.0f;
          break;
        }
        case LOOK_AURORA: {
          colorMode = 2;
          sat = 0.75f;
          float shear = sinf((c / (float)LEDS_PER_ROW) * PI * 2.0f + orbit * PI * 2.0f) * 0.5f + 0.5f;
          float curtain = 0.35f + 0.65f * sinf(tRow * PI + shear * 2.0f + huePhase * 0.02f);
          float soft = max(0.0f, curtain) * (0.55f + 0.45f * (0.5f + 0.5f * sinf(pulse * PI * 2.0f)));
          energy = min(1.0f, floorL + soft * 0.85f);
          hue = fmodf(180.0f + (c / (float)LEDS_PER_ROW) * 100.0f + huePhase + r * 8.0f, 360.0f);
          break;
        }
        case LOOK_SCOPE:
        default: {
          // Flat mid-line until mic path exists (amp = 0). Mic fills samples later.
          colorMode = 0;
          hue = 125.0f;
          sat = 0.95f;
          float amp = 0.0f;
          float targetRow = (0.5f - amp * 0.48f) * (ROWS - 1);
          float dist = fabsf((float)r - targetRow);
          float trace = expf(-powf(dist * 2.4f, 2));
          energy = dist < 1.25f ? min(1.0f, 0.08f * floorL + trace * 0.98f) : floorL * 0.15f;
          break;
        }
      }

      if (look != LOOK_EMBER && look != LOOK_AURORA && look != LOOK_SCANNER
          && look != LOOK_SCOPE && look != LOOK_HEARTBEAT) {
        hue = baseHue(c, twin, colorMode);
      } else if (look == LOOK_SCANNER) {
        // keep fixed cool
      }

      if (energy < 0.03f) continue;
      out[ledIndex(r, c)] = hsvByte(hue, sat, min(1.0f, energy));
    }
  }
}

void startFade() {
  fromLook = lookIndex;
  toLook = (lookIndex + 1) % LOOK_COUNT;
  fading = true;
  fadeStart = millis();
  Serial.printf("fade %s -> %s\n", LOOK_NAME[fromLook], LOOK_NAME[toLook]);
}

void finishFade() {
  lookIndex = toLook;
  fading = false;
  holdUntil = millis() + HOLD_MS;
  Serial.printf("hold %s\n", LOOK_NAME[lookIndex]);
}

void skipNext() {
  if (fading) finishFade();
  else startFade();
  lastBtnAction = millis();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("led-wrist-band band-demo");
  Serial.printf("%u x %u = %u LEDs  hold=%ums fade=%ums\n",
                ROWS, LEDS_PER_ROW, NUM_LEDS, HOLD_MS, FADE_MS);
  for (uint8_t i = 0; i < LOOK_COUNT; i++) {
    Serial.printf("  %u) %s\n", i, LOOK_NAME[i]);
  }

  for (uint8_t i = 0; i < MAX_RIPPLES; i++) ripples[i].active = false;

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);

  holdUntil = millis() + HOLD_MS;
  Serial.printf("hold %s\n", LOOK_NAME[lookIndex]);
}

void loop() {
  uint32_t now = millis();
  tickDemoAudio(now);

  bool rawLow = digitalRead(BTN_PIN) == LOW;
  if (rawLow) { lowStreak++; highStreak = 0; }
  else { highStreak++; lowStreak = 0; }
  if (!btnHeld && lowStreak >= 2) {
    btnHeld = true;
    if (now - lastBtnAction > 280) skipNext();
  }
  if (btnHeld && highStreak >= 2) btnHeld = false;

  if (!fading && (int32_t)(now - holdUntil) >= 0) startFade();
  if (fading && (now - fadeStart) >= FADE_MS) finishFade();

  if (fading) {
    float blendAmt = (float)(now - fadeStart) / (float)FADE_MS;
    if (blendAmt > 1) blendAmt = 1;
    renderLook(fromLook, bufA);
    renderLook(toLook, bufB);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      leds[i] = blend(bufA[i], bufB[i], (fract8)(blendAmt * 255));
    }
  } else {
    renderLook(lookIndex, leds);
  }

  FastLED.show();
  digitalWrite(LED_BUILTIN, (now / 400) & 1 ? HIGH : LOW);
  delay(12);
}
