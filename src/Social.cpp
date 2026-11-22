#include "Social.hpp"
#include "Config.h"
#include "Eyes.hpp"
#include "Personality.hpp"
#include <WiFi.h>
#include <esp_now.h>
#include <Preferences.h>

typedef struct {
  char magic[8];      // "CHARLIE"
  char owner[24];
  float traits[PERSONALITY_COUNT];
  uint8_t animPhase;
} SocialPacket;

static bool espNowActive = false;
static float baseTraits[PERSONALITY_COUNT];
static bool baseSaved = false;
static uint32_t lastBeaconMs = 0;
static uint32_t lastPeerSeenMs = 0;
static bool peerIsBrother = false;
static uint8_t peerPhase = 0;
static uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(SocialPacket)) return;
  const SocialPacket *pkt = (const SocialPacket *)data;
  if (strncmp(pkt->magic, "CHARLIE", 7) != 0) return;

  Preferences p;
  p.begin("charlie", true);
  String myOwner = p.getString("owner", "");
  p.end();

  lastPeerSeenMs = millis();
  peerIsBrother = (myOwner.length() > 0 && myOwner == String(pkt->owner));
  peerPhase = pkt->animPhase;

  // Rub off on each other's personality slightly
  float blendRate = peerIsBrother ? 0.015f : 0.007f;
  for (int i = 0; i < PERSONALITY_COUNT; i++) {
    gPersonality[i] += (pkt->traits[i] - gPersonality[i]) * blendRate;
  }
  gMood.happiness = min(1.0f, gMood.happiness + (peerIsBrother ? 0.02f : 0.01f));
}

void socialInit() {
  for (int i = 0; i < PERSONALITY_COUNT; i++) baseTraits[i] = gPersonality[i];
  baseSaved = true;
}

void socialUpdate() {
  uint32_t now = millis();

  // Turn ESP-NOW on whenever normal Wi-Fi Connect mode is OFF
  if (!gWifiOn && !espNowActive) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK) {
      esp_now_register_recv_cb(onDataRecv);
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, broadcastMac, 6);
      peerInfo.channel = 1;
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
      espNowActive = true;
    }
  } else if (gWifiOn && espNowActive) {
    esp_now_deinit();
    espNowActive = false;
  }

  // Send presence beacon every 600ms
  if (espNowActive && (now - lastBeaconMs > 600)) {
    lastBeaconMs = now;
    Preferences p;
    p.begin("charlie", true);
    String myOwner = p.getString("owner", "");
    p.end();

    SocialPacket pkt = {};
    strcpy(pkt.magic, "CHARLIE");
    strncpy(pkt.owner, myOwner.c_str(), sizeof(pkt.owner) - 1);
    for (int i = 0; i < PERSONALITY_COUNT; i++) pkt.traits[i] = gPersonality[i];
    pkt.animPhase = (now / 600) % 8;
    esp_now_send(broadcastMac, (uint8_t *)&pkt, sizeof(pkt));
  }

  // Synced visual interaction while near another Charlie
  if (espNowActive && lastPeerSeenMs > 0 && (now - lastPeerSeenMs < 2500)) {
    float wave = sinf((now % 1200) / 1200.0f * 6.283f);
    if (peerIsBrother) {
      // Brothers: excited synchronized bounce + affectionate squint
      eyesLook(wave * 0.25f, -fabsf(wave) * 0.35f);
    } else {
      // Friends: curious call-and-response glances
      eyesLook((peerPhase % 2 == 0) ? 0.35f : -0.35f, -0.15f);
    }
  } else if (baseSaved) {
    // Elastic pullback to original quiz personality when alone
    float kPull = 0.0005f + gPersonality[STUBBORNNESS] * 0.0025f;
    for (int i = 0; i < PERSONALITY_COUNT; i++) {
      gPersonality[i] += (baseTraits[i] - gPersonality[i]) * kPull;
    }
  }
}
