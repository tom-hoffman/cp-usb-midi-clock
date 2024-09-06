// USB MIDI clock for Circuit Playground
// Using micros() for best accuracy.
// Using USB-MIDI library.

// By Tom Hoffman
// Copyright 2024.
// Licensened under the GPL 3.0.

// Instructions:
// Switch left pauses clock messages (neopixels red).
// Switch right sends messages on each toggle of neopixels (blue).
// Left (A) button decreases tempo.
// Right (B) button increases tempo.
// Red led shows active sensing message outputted every 300 ms.

// Notes:
// Using sys ex with manufacturer 0x7D (the official "educational use"
// ID) to send a split 14(!) bit value for the tempo to the display.

// TODO
// make switch left "manual" mode rather than pause.

#include <Adafruit_CircuitPlayground.h>
#include <USB-MIDI.h>

USBMIDI_CREATE_DEFAULT_INSTANCE();
using namespace MIDI_NAMESPACE;


// GLOBAL VARIAGLES (with defaults)

const long BUTTON_DEBOUNCE    = 100000;


uint32_t  tempo               = 90;    // BPM
uint32_t  tempo_delay;                 // 1/24th a beat in milliseconds
uint32_t  last_tick;
uint32_t  last_button;
uint32_t  beat_count          = 0;      // change LED every 24k ticks
bool      pix_on              = false;  // NeoPixels on?
bool      resetting           = false;
uint8_t   red_value           = 0;
uint8_t   blue_value          = 85;
uint8_t   green_value         = 170;

uint32_t calculateTempoDelay(uint32_t tp) {
  // Takes in tempo as bmp, returns delay in micros().
  return (60 * pow(10, 6)) / (tp * 48);
}

void sendTempo(uint16_t t) {
  uint8_t m[] = {0xF0, 0x7D, (t & 0b11111110000000) >> 7, (t & 0b1111111), 0xF7};
  MIDI.sendSysEx(5, m, true);  // true indicates start and stop bytes are included in array.
}

void setAllNeoPixels(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < 10; i++) {
    CircuitPlayground.setPixelColor(i, r, g, b);
  }
}

void updateNeoPixels() {
  if (pix_on) {
    setAllNeoPixels(red_value, green_value, blue_value);
    }
  else {
    setAllNeoPixels(0, 0, 0);
  }
}

void displayPause() {
  setAllNeoPixels(255, 0, 0);
}

void advanceClock() {
  long m = micros();
  if (m - last_tick > tempo_delay) {
    last_tick = last_tick + tempo_delay;
    MIDI.sendRealTime(MidiType::Clock);
    ++beat_count;
    if (beat_count >=24) {
      red_value++;
      blue_value++;
      green_value++;
      updateNeoPixels();
      pix_on = !(pix_on);
      beat_count = 0;
    }
  }
  if (m - last_button > BUTTON_DEBOUNCE) {
    if (CircuitPlayground.rightButton()) {
      ++tempo;
      tempo_delay = calculateTempoDelay(tempo);
      last_button = m;
      sendTempo(tempo);
    }
    if (CircuitPlayground.leftButton() && (tempo > 2)) {
      --tempo;
      tempo_delay = calculateTempoDelay(tempo);
      last_button = m;
      sendTempo(tempo);
    }
  }
}

void setup() {
  Serial.begin(31250);
  MIDI.begin(14);
  delay(1000);
  CircuitPlayground.begin(1);  // arg. is neopixel brightness.
  tempo_delay = calculateTempoDelay(tempo);
  sendTempo(tempo);
  MIDI.sendRealTime(MidiType::Stop);
  MIDI.sendRealTime(MidiType::Start);
  last_tick = micros();
  last_button = last_tick;
}

void loop() {
  if (!(CircuitPlayground.slideSwitch())) { // switch right
    if (resetting) {  // stop resetting
      resetting = false;
      MIDI.sendRealTime(MidiType::Stop);
      MIDI.sendRealTime(MidiType::Start);
      sendTempo(tempo);
      pix_on = false;
      last_tick = micros();

    }
    else {
      advanceClock();  // continue normally
    }
  }
  else { // switch left
    if (!resetting) {  // start resetting
      resetting = true;
      MIDI.sendRealTime(MidiType::Stop);
      displayPause();
      sendTempo(0);
    }
  }

}