#include "AudioBeat.hpp"
#include "Eyes.hpp"
#include "Personality.hpp"
#include <Wire.h>
#include <driver/i2s.h>
#include <math.h>

// Waveshare 1.75" AMOLED ES7210 Mic ADC (I2C 0x40) + I2S0 pins
#define ES7210_ADDR 0x40
#define I2S_MCLK    42
#define I2S_SCLK    9
#define I2S_LRCK    45
#define I2S_DIN     10

static bool audioReady = false;
static bool music = false;
static float phase = 0.0f;
static float bpmHz = 2.0f; // default 120 BPM
static float envAvg = 0.0f;
static uint32_t lastBeatMs = 0;
static int beatStreak = 0;
static uint32_t earwormUntil = 0;
static bool earwormShaking = false;

static void esWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ES7210_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void audioInit() {
  Wire.begin(15, 14);
  Wire.beginTransmission(ES7210_ADDR);
  if (Wire.endTransmission() != 0) return;

  // Initialize ES7210 dual microphones
  esWrite(0x00, 0xFF); delay(10);
  esWrite(0x00, 0x32);
  esWrite(0x0D, 0x09);
  esWrite(0x01, 0x20);
  esWrite(0x03, 0x06);
  esWrite(0x04, 0x03);
  esWrite(0x06, 0x00);
  esWrite(0x07, 0x20);
  esWrite(0x08, 0x10);
  esWrite(0x02, 0xC1);
  esWrite(0x43, 0x1B); // MIC1 gain
  esWrite(0x44, 0x1B); // MIC2 gain
  esWrite(0x4B, 0x0F);
  esWrite(0x4C, 0x0F);
  esWrite(0x00, 0x71);
  esWrite(0x00, 0x41);

  i2s_config_t i2s_cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = true
  };
  i2s_pin_config_t pin_cfg = {
    .mck_io_num = I2S_MCLK,
    .bck_io_num = I2S_SCLK,
    .ws_io_num = I2S_LRCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_DIN
  };

  if (i2s_driver_install(I2S_NUM_0, &i2s_cfg, 0, nullptr) == ESP_OK) {
    i2s_set_pin(I2S_NUM_0, &pin_cfg);
    audioReady = true;
  }
}

void audioUpdate(float dt) {
  uint32_t now = millis();
  float rms = 0.0f;

  if (audioReady) {
    int16_t buf[128];
    size_t bytesRead = 0;
    if (i2s_read(I2S_NUM_0, buf, sizeof(buf), &bytesRead, 0) == ESP_OK && bytesRead > 0) {
      int samples = bytesRead / 2;
      double sumSq = 0;
      for (int i = 0; i < samples; i++) {
        float s = buf[i] / 32768.0f;
        sumSq += s * s;
      }
      rms = sqrtf(sumSq / samples);
    }
  }

  float spike = rms - envAvg;
  envAvg += (rms - envAvg) * 0.12f;

  // 1. Sudden loud noise -> Startle reaction
  float startleThresh = 0.28f - gPersonality[SENSITIVITY] * 0.12f;
  if (spike > startleThresh && rms > 0.18f) {
    gMood.annoyance = min(1.0f, gMood.annoyance + 0.18f);
    eyesBlink();
    eyesLook(0.0f, -0.3f);
  }
  // 2. Rhythmic beat detection (music)
  else if (spike > 0.035f && rms > 0.045f) {
    uint32_t gap = now - lastBeatMs;
    if (gap >= 330 && gap <= 1000) { // 60 to 180 BPM
      beatStreak++;
      float detectedHz = 1000.0f / gap;
      bpmHz += (detectedHz - bpmHz) * 0.35f;
      if (beatStreak >= 3) {
        music = true;
        earwormUntil = now + 50000; // 50s "song stuck in head" after music stops
        earwormShaking = false;
      }
    } else if (gap > 1100) {
      beatStreak = 1;
    }
    lastBeatMs = now;
  }
  // 3. Normal talking volume -> Curious tilt
  else if (rms > 0.025f && !music) {
    gLeft.lidAngle = -10.0f * gPersonality[CURIOSITY];
    gRight.lidAngle = -10.0f * gPersonality[CURIOSITY];
  }

  if (music && (now - lastBeatMs > 2500)) {
    music = false; // Music stopped -> enter earworm memory phase
  }

  phase += dt * bpmHz;
  if (phase >= 1.0f) phase -= 1.0f;

  if (music) {
    float bounce = fabsf(sinf(phase * 3.14159f));
    eyesLook(sinf(phase * 6.283f) * 0.18f, -bounce * (0.28f + gPersonality[PLAYFULNESS] * 0.15f));
  } else if (now < earwormUntil) {
    float rem = (earwormUntil - now) / 50000.0f;
    if (rem > 0.04f) {
      // Gently bouncing to remembered beat with fading energy
      float bounce = fabsf(sinf(phase * 3.14159f)) * rem;
      eyesLook(sinf(phase * 6.283f) * 0.14f * rem, -bounce * 0.22f);
    } else {
      // Final 2 seconds: quick head/eye shake to "clear the song out of his head"
      earwormShaking = true;
      eyesLook(sinf(now * 0.055f) * 0.75f, 0.0f);
    }
  } else if (earwormShaking) {
    earwormShaking = false;
    eyesLook(0.0f, 0.0f);
  }
}

bool isMusicPlaying() { return music; }
float getBeatPhase() { return phase; }
