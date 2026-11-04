#include <Arduino.h>

// AirPiano - play notes by moving your hand over an HC-SR04
// (same as the original, plus a live "AP,..." telemetry line for the web piano)
const int TRIG = 7, ECHO = 6, BUZZER = 8;
const int LED_R = 9, LED_G = 10, LED_B = 5;
const int REC_BTN = 2, PLAY_BTN = 4;

const float MIN_CM = 5.0, MAX_CM = 45.0, BAND = 5.0;

const char* NAMES[8] = {"C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"};
const int FREQ[8]    = {262, 294, 330, 349, 392, 440, 494, 523};
// rainbow: red, orange, yellow, green, cyan, blue, indigo, violet
const byte COLORS[8][3] = {
  {255, 0, 0}, {255, 100, 0}, {255, 220, 0}, {0, 255, 0},
  {0, 220, 220}, {0, 0, 255}, {75, 0, 200}, {160, 0, 255}
};

const int MAX_NOTES = 32;
int8_t noteSeq[MAX_NOTES];
unsigned int noteLen[MAX_NOTES];
int count = 0;

bool recording = false;
int8_t curNote = -1;
unsigned long noteStart = 0;

// --- telemetry for the web piano ---
int8_t lastReported = -2;
unsigned long lastReport = 0;

void setColor(int idx, float brightness) {
  if (idx < 0) {
    analogWrite(LED_R, 0); analogWrite(LED_G, 0); analogWrite(LED_B, 0);
    return;
  }
  analogWrite(LED_R, COLORS[idx][0] * brightness);
  analogWrite(LED_G, COLORS[idx][1] * brightness);
  analogWrite(LED_B, COLORS[idx][2] * brightness);
}

// returns distance in cm, or -1 if no echo
float readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long t = pulseIn(ECHO, HIGH, 30000UL);
  if (t == 0) return -1;
  return t * 0.0343 / 2.0;
}

void closeNote() {
  if (curNote >= 0 && count < MAX_NOTES) {
    unsigned long len = millis() - noteStart;
    if (len >= 40) {                       // ignore tiny glitches
      noteSeq[count] = curNote;
      noteLen[count] = len;
      count++;
      Serial.print(NAMES[curNote]);
      Serial.print(" ");
      Serial.print(len);
      Serial.println("ms");
    }
  }
}

void playback() {
  Serial.println("--- Playback ---");
  for (int i = 0; i < count; i++) {
    tone(BUZZER, FREQ[noteSeq[i]]);
    setColor(noteSeq[i], 1.0);
    Serial.print(NAMES[noteSeq[i]]);
    Serial.print(" ");
    Serial.print(noteLen[i]);
    Serial.println("ms");
    delay(noteLen[i]);
  }
  noTone(BUZZER);
  setColor(-1, 0);
  Serial.println("--- Done ---");
  while (digitalRead(PLAY_BTN) == LOW) {}   // wait for release
}

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(REC_BTN, INPUT_PULLUP);
  pinMode(PLAY_BTN, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("AirPiano ready");
}

void loop() {
  bool recHeld = digitalRead(REC_BTN) == LOW;
  bool playHeld = digitalRead(PLAY_BTN) == LOW;

  if (playHeld && !recHeld && count > 0) {
    playback();
    return;
  }

  float d = readDistance();
  int8_t idx = -1;
  float brightness = 0;

  if (d >= MIN_CM && d < MAX_CM) {
    idx = (int)((d - MIN_CM) / BAND);
    if (idx > 7) idx = 7;
    float pos = (d - MIN_CM - idx * BAND) / BAND;      // 0..1 inside band
    float centre = 1.0 - fabs(pos - 0.5) * 2.0;        // 1 in middle, 0 at edges
    brightness = 0.1 + 0.9 * centre;
  }

  if (idx >= 0) tone(BUZZER, FREQ[idx]);
  else noTone(BUZZER);
  setColor(idx, brightness);

  // >>> NEW: tell the web piano what's happening: AP,<note index or -1>,<cm>,<brightness %>
  if (idx != lastReported || millis() - lastReport > 100) {
    Serial.print("AP,");
    Serial.print(idx);
    Serial.print(",");
    Serial.print(d, 1);
    Serial.print(",");
    Serial.println((int)(brightness * 100));
    lastReport = millis();
    lastReported = idx;
  }

  if (recHeld) {
    if (!recording) {
      recording = true;
      count = 0;
      curNote = -1;
      Serial.println("--- Recording ---");
    }
    if (idx != curNote) {
      closeNote();
      curNote = idx;
      noteStart = millis();
    }
  } else if (recording) {
    closeNote();
    recording = false;
    curNote = -1;
    Serial.println("--- Saved ---");
  }

  delay(30);
}
