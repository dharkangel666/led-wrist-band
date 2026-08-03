/*
 * LED Wrist Band — XIAO ESP32 smoke test
 * Blinks the onboard LED and prints heartbeat on USB serial.
 * Optional: cyan chase on strip if STRIP_TEST is 1.
 */

#include <FastLED.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 21  // Seeed XIAO ESP32-S3 yellow LED
#endif

#define STRIP_TEST      1
#define LED_PIN         D0
#define LEDS_PER_ROW    24
#define ROWS            8
#define NUM_LEDS        (ROWS * LEDS_PER_ROW)
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BRIGHTNESS      48

#if STRIP_TEST
CRGB leds[NUM_LEDS];
uint16_t chase = 0;
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("led-wrist-band xiao-smoke-test OK");
  Serial.printf("chip=%s  rev=%d  LED_BUILTIN=%d\n",
                ESP.getChipModel(), ESP.getChipRevision(), LED_BUILTIN);

#if STRIP_TEST
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);
  Serial.printf("strip: %u x %u = %u LEDs on D0\n", ROWS, LEDS_PER_ROW, NUM_LEDS);
#endif
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED on");

#if STRIP_TEST
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  leds[chase % NUM_LEDS] = CRGB::Cyan;
  leds[(chase + LEDS_PER_ROW) % NUM_LEDS] = CRGB(0, 40, 80);
  FastLED.show();
  chase++;
#endif

  delay(400);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("LED off");
  delay(400);
}
