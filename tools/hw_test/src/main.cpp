#include <Arduino.h>

/**
 * Hardware Test - RGB LED + Buzzer
 *
 * Cycles through RGB colors, then beeps the buzzer.
 * Upload via: pio run --project-dir tools/hw_test -t upload --upload-port /dev/ttyUSB1
 */

// Pin assignments (matches PCB design)
#define RGB_RED_PIN     21
#define RGB_GREEN_PIN   19
#define RGB_BLUE_PIN    18
#define BUZZER_PIN      15

// PWM channels (ESP32 LEDC)
#define CH_RED    0
#define CH_GREEN  1
#define CH_BLUE   2
#define CH_BUZZER 3

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(CH_RED, r);
    ledcWrite(CH_GREEN, g);
    ledcWrite(CH_BLUE, b);
}

void beep(int freq, int duration_ms) {
    ledcWriteTone(CH_BUZZER, freq);
    delay(duration_ms);
    ledcWriteTone(CH_BUZZER, 0);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Hardware Test: RGB LED + Buzzer ===\n");

    // Setup RGB LED PWM channels
    ledcSetup(CH_RED, 5000, 8);
    ledcSetup(CH_GREEN, 5000, 8);
    ledcSetup(CH_BLUE, 5000, 8);
    ledcAttachPin(RGB_RED_PIN, CH_RED);
    ledcAttachPin(RGB_GREEN_PIN, CH_GREEN);
    ledcAttachPin(RGB_BLUE_PIN, CH_BLUE);

    // Setup buzzer PWM channel
    ledcSetup(CH_BUZZER, 2000, 8);
    ledcAttachPin(BUZZER_PIN, CH_BUZZER);

    // All off
    setRGB(0, 0, 0);
    ledcWriteTone(CH_BUZZER, 0);

    Serial.println("Starting test sequence...\n");

    // --- RGB Test ---
    Serial.println("RED");
    setRGB(255, 0, 0);
    delay(1500);

    Serial.println("GREEN");
    setRGB(0, 255, 0);
    delay(1500);

    Serial.println("BLUE");
    setRGB(0, 0, 255);
    delay(1500);

    Serial.println("YELLOW (R+G)");
    setRGB(255, 255, 0);
    delay(1500);

    Serial.println("CYAN (G+B)");
    setRGB(0, 255, 255);
    delay(1500);

    Serial.println("MAGENTA (R+B)");
    setRGB(255, 0, 255);
    delay(1500);

    Serial.println("WHITE (R+G+B)");
    setRGB(255, 255, 255);
    delay(1500);

    setRGB(0, 0, 0);
    Serial.println("RGB OFF\n");
    delay(500);

    // --- Buzzer Test ---
    Serial.println("BUZZER: Low tone (500Hz)");
    beep(500, 300);
    delay(200);

    Serial.println("BUZZER: Mid tone (1000Hz)");
    beep(1000, 300);
    delay(200);

    Serial.println("BUZZER: High tone (2000Hz)");
    beep(2000, 300);
    delay(200);

    Serial.println("BUZZER: Chirp");
    for (int f = 500; f < 3000; f += 100) {
        ledcWriteTone(CH_BUZZER, f);
        delay(20);
    }
    ledcWriteTone(CH_BUZZER, 0);
    delay(500);

    // --- Combined: success jingle with green ---
    Serial.println("\nSUCCESS jingle");
    setRGB(0, 255, 0);
    beep(1047, 150); // C5
    delay(50);
    beep(1319, 150); // E5
    delay(50);
    beep(1568, 300); // G5
    delay(100);
    setRGB(0, 0, 0);
    ledcWriteTone(CH_BUZZER, 0);

    Serial.println("\n=== Test Complete ===");
    Serial.println("If you saw R/G/B/Y/C/M/W and heard beeps, hardware is good.");
}

void loop() {
    // Gentle breathing on blue LED to show we're alive
    for (int i = 0; i < 255; i++) {
        ledcWrite(CH_BLUE, i);
        delay(5);
    }
    for (int i = 255; i >= 0; i--) {
        ledcWrite(CH_BLUE, i);
        delay(5);
    }
}
