#include "SyncServer.hpp"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>
#include "Personality.hpp"
#include "Eyes.hpp"

static WebServer server(80);
static bool serverStarted = false;
static bool wifiStarted = false;

static void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{background:#09090b;color:#f4f4f5;font-family:system-ui,sans-serif;display:flex;flex-direction:column;align-items:center;padding:32px 16px;margin:0}
.card{background:#18181b;border:1px solid #27272a;border-radius:18px;padding:20px;width:100%;max-width:360px;margin-bottom:16px}
h2{margin:0 0 12px;font-size:18px;color:#00d4ff}
input,button{width:100%;box-sizing:border-box;padding:12px;border-radius:12px;border:1px solid #3f3f46;background:#09090b;color:#fff;margin-bottom:10px;font-size:15px}
button{background:#00d4ff;color:#000;font-weight:700;border:none;cursor:pointer}
</style></head><body>
<div class="card"><h2>Charlie Wi-Fi Setup</h2>
<form action="/wifi" method="POST">
<input name="ssid" placeholder="Home Wi-Fi Name (SSID)" required>
<input name="pass" type="password" placeholder="Wi-Fi Password">
<button type="submit">Save & Connect</button></form></div>
<div class="card"><h2>Wireless Update (OTA)</h2>
<form action="/ota" method="POST" enctype="multipart/form-data">
<input type="file" name="firmware" accept=".bin" required>
<button type="submit">Upload Firmware</button></form></div>
</body></html>)rawliteral";
  server.send(200, "text/html", html);
}

static void handleWifiSave() {
  Preferences p;
  p.begin("charlie", false);
  p.putString("ssid", server.arg("ssid"));
  p.putString("pass", server.arg("pass"));
  p.end();
  server.send(200, "text/html", "<html><body style='background:#09090b;color:#00d4ff;font-family:sans-serif;text-align:center;padding-top:40%'>Saved! Rebooting Charlie...</body></html>");
  delay(800);
  ESP.restart();
}

static void handleSync() {
  bool changed = false;

  if (server.hasArg("curiosity")) {
    float v[PERSONALITY_COUNT];
    v[CURIOSITY]       = server.arg("curiosity").toFloat();
    v[PLAYFULNESS]     = server.arg("playfulness").toFloat();
    v[AFFECTION]       = server.arg("affection").toFloat();
    v[SENSITIVITY]     = server.arg("sensitivity").toFloat();
    v[ENERGY_BASELINE] = server.arg("energy").toFloat();
    v[STUBBORNNESS]    = server.arg("stubbornness").toFloat();
    personalityApplyFromApp(v);
    changed = true;
  }

  if (server.hasArg("color")) {
    String c = server.arg("color");
    c.replace("#", "");
    gEyeColor = strtoul(c.c_str(), nullptr, 16);
    gLeft.color = gRight.color = gEyeColor;
    changed = true;
  }

  if (server.hasArg("owner") || server.hasArg("name")) {
    Preferences p;
    p.begin("charlie", false);
    if (server.hasArg("owner")) p.putString("owner", server.arg("owner"));
    if (server.hasArg("name"))  p.putString("name", server.arg("name"));
    p.end();
  }

  if (server.hasArg("treat") && server.arg("treat").toInt() > 0) {
    gMood.happiness = min(1.0f, gMood.happiness + 0.30f);
    eyesBlink();
  }

  if (server.hasArg("toy") && server.arg("toy").toInt() > 0) {
    gMood.happiness = min(1.0f, gMood.happiness + 0.20f);
    gMood.energy    = min(1.0f, gMood.energy + 0.15f);
  }

  uint32_t claimedSteps = 0;
  if (server.hasArg("claim_steps") && server.arg("claim_steps").toInt() > 0) {
    claimedSteps = gSteps;
    gSteps = 0;
    changed = true;
  }

  if (changed) personalitySaveToNVS();

  String redirectUrl = "https://lovable.app" + String(claimedSteps);
  String html = "<html><body style='background:#000'></body><script>"
                "setTimeout(()=>{ window.close(); setTimeout(()=>{ window.location.href='" + redirectUrl + "'; }, 200); }, 120);"
                "</script></html>";
  server.send(200, "text/html", html);
}

void syncServerBegin() {
  if (serverStarted) return;
  server.on("/", handleRoot);
  server.on("/wifi", HTTP_POST, handleWifiSave);
  server.on("/sync", handleSync);
  server.on("/ota", HTTP_POST,
    []() {
      server.send(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
      delay(500);
      ESP.restart();
    },
    []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Update.begin(UPDATE_SIZE_UNKNOWN);
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        Update.write(upload.buf, upload.currentSize);
      } else if (upload.status == UPLOAD_FILE_END) {
        Update.end(true);
      }
    }
  );
  server.begin();
  serverStarted = true;
}

void syncServerLoop() {
  if (gWifiOn && !wifiStarted) {
    Preferences p;
    p.begin("charlie", true);
    String ssid = p.getString("ssid", "");
    String pass = p.getString("pass", "");
    p.end();

    if (ssid.length() == 0) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("Charlie-Setup");
    } else {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), pass.c_str());
      uint32_t t0 = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) delay(100);
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Charlie-Setup");
      } else {
        MDNS.begin("charlie");
      }
    }
    syncServerBegin();
    wifiStarted = true;
  } else if (!gWifiOn && wifiStarted) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiStarted = false;
  }

  if (gWifiOn && serverStarted) {
    server.handleClient();
  }
}
