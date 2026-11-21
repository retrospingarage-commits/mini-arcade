// Atelier pixel art : la borne crée son propre petit réseau Wi-Fi (sans internet, protégé par un mot de passe).
// Le téléphone s'y connecte (QR code sur l'OLED), la page de dessin s'ouvre toute seule et chaque pixel dessiné
// s'affiche en direct sur la matrice. « Enregistrer » range le dessin dans la Galerie.
// Tant que l'Atelier est ouvert, la mise à jour sans câble (OTA) est possible avec le même mot de passe.
// Bouton du joystick : fermer l'Atelier (le Wi-Fi s'éteint).
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include "hw.h"
#include "gallery.h"
#include "activities.h"
#if __has_include("qrcode.h")
#include "qrcode.h"  // générateur de QR code fourni avec l'ESP32
#define HAS_QRCODE 1
#endif

namespace {

const char* const SSID = "Mini-arcade";
const IPAddress   AP_IP(192, 168, 4, 1);

// Page de dessin envoyée au téléphone (HTML + JavaScript, sans rien à installer)
const char PAGE[] PROGMEM = R"rawliteral(<!doctype html><html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"><title>Atelier pixel art</title>
<style>body{margin:0;background:#111;color:#eee;font-family:sans-serif;text-align:center}
h2{margin:10px 0 4px}#g{display:grid;grid-template-columns:repeat(16,1fr);width:min(96vw,62vh);margin:6px auto;
gap:1px;background:#333;touch-action:none;user-select:none}#g div{aspect-ratio:1;background:#000}
#p{display:flex;flex-wrap:wrap;justify-content:center;gap:6px;margin:8px;max-width:420px;margin:auto}
#p b{width:34px;height:34px;border-radius:50%;border:3px solid #444;box-sizing:border-box}#p b.s{border-color:#fff}
button{font-size:17px;margin:8px 4px;padding:10px 16px;border-radius:10px;border:0;background:#f4a;color:#fff}
#m{min-height:1.2em}</style></head><body><h2>Atelier pixel art</h2><div id="g"></div><div id="p"></div>
<button onclick="fill(0)">Effacer</button><button onclick="save()">Enregistrer</button><p id="m"></p>
<script>
const g=document.getElementById('g'),p=document.getElementById('p'),m=document.getElementById('m');
let cur=1,pal=[],px=[],down=false,q=[];
fetch('/state').then(r=>r.json()).then(s=>{pal=s.palette;px=s.pixels;build()});
function build(){for(let i=0;i<256;i++){const d=document.createElement('div');d.style.background=pal[px[i]];g.appendChild(d)}
pal.forEach((c,i)=>{const b=document.createElement('b');b.style.background=c;b.onclick=()=>{cur=i;
[...p.children].forEach(x=>x.className='');b.className='s'};p.appendChild(b)});p.children[1].className='s'}
function paint(e){const el=document.elementFromPoint(e.clientX,e.clientY);const i=[...g.children].indexOf(el);
if(i<0||px[i]==cur)return;px[i]=cur;el.style.background=pal[cur];q.push(i)}
g.addEventListener('pointerdown',e=>{down=true;paint(e)});addEventListener('pointerup',()=>down=false);
g.addEventListener('pointermove',e=>{if(down)paint(e)});
setInterval(()=>{if(!q.length)return;const b=q.splice(0,40).map(i=>i+'.'+px[i]).join(',');
fetch('/px?d='+b,{method:'POST'})},120);
function fill(c){px.fill(c);[...g.children].forEach(d=>d.style.background=pal[c]);fetch('/fill?c='+c,{method:'POST'})}
function save(){fetch('/save',{method:'POST'}).then(r=>r.text()).then(t=>m.textContent=t)}
</script></body></html>)rawliteral";

WebServer   server(80);
DNSServer   dns;
Preferences prefs;
String      password;
bool        running = false, dirty = true, otaBusy = false, otaReady = false;
int         lastStations = -1;
uint8_t     lastColor = 1;

// Mot de passe tiré au hasard au premier lancement et gardé dans la borne (jamais écrit dans le code)
String loadPassword() {
  prefs.begin("wifi", false);
  String pass = prefs.getString("pass", "");
  if (pass.length() < 8) {
    const char* alphabet = "abcdefghjkmnpqrstuvwxyz23456789";  // sans 0/o ni 1/l, faciles à confondre
    pass = "";
    for (int i = 0; i < 8; i++) pass += alphabet[esp_random() % 31];
    prefs.putString("pass", pass);
  }
  prefs.end();
  return pass;
}

void sendState() {
  String s = "{\"palette\":[";
  for (int i = 0; i < availableColors(); i++) { if (i) s += ','; s += '"'; s += paletteCss(i); s += '"'; }
  s += "],\"pixels\":[";
  for (int i = 0; i < 256; i++) { if (i) s += ','; s += canvas[i]; }
  s += "]}";
  server.send(200, "application/json", s);
}

void handlePixels() {  // d = "indice.couleur,indice.couleur,…"
  String d = server.arg("d");
  int start = 0;
  while (start < (int)d.length()) {
    int comma = d.indexOf(',', start);
    if (comma < 0) comma = d.length();
    int dot = d.indexOf('.', start);
    if (dot > start && dot < comma) {
      int i = d.substring(start, dot).toInt(), c = d.substring(dot + 1, comma).toInt();
      if (i >= 0 && i < 256 && c >= 0 && c < availableColors()) { canvas[i] = c; lastColor = c; }
    }
    start = comma + 1;
  }
  server.send(204);
}

void handleFill() {
  int c = server.arg("c").toInt();
  if (c >= 0 && c < availableColors()) memset(canvas, c, sizeof(canvas));
  server.send(204);
}

void handleSave() {
  int n = gallerySave(canvas);
  server.send(200, "text/plain; charset=utf-8", String("Enregistré ! ") + n + " dessin(s) dans la Galerie.");
  tone(1568, 120);
}

void redirectHome() {  // portail captif : toute adresse inconnue renvoie vers la page de dessin
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

#ifdef HAS_QRCODE
void drawQr(esp_qrcode_handle_t qr) {  // QR code de connexion au Wi-Fi, 2 pixels par module
  int size = esp_qrcode_get_size(qr);
  int scale = size * 2 <= 64 ? 2 : 1;
  int off = (64 - size * scale) / 2;
  for (int y = 0; y < size; y++)
    for (int x = 0; x < size; x++)
      if (esp_qrcode_get_module(qr, x, y)) oled.drawBox(off + x * scale, off + y * scale, scale, scale);
}
#endif

void drawOled(int stations) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  int textX = 0;
#ifdef HAS_QRCODE
  String wifi = String("WIFI:T:WPA;S:") + SSID + ";P:" + password + ";;";
  esp_qrcode_config_t cfg = {};
  cfg.display_func = drawQr;
  cfg.max_qrcode_version = 5;
  cfg.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;
  esp_qrcode_generate(&cfg, wifi.c_str());
  textX = 66;
#endif
  oled.setCursor(textX, 10); oled.print(textX ? "Wi-Fi :" : "Réseau Wi-Fi :");
  oled.setCursor(textX, 21); oled.print(SSID);
  oled.setCursor(textX, 35); oled.print("Mot de passe");
  oled.setCursor(textX, 46); oled.print(password);
  oled.setCursor(textX, 62);
  if (otaBusy) oled.print("Mise à jour...");
  else if (stations) oled.printf("%d connecté(s)", stations);
  else oled.print("Scanne-moi !");
  oled.sendBuffer();
}

void startPortal() {
  password = loadPassword();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID, password.c_str());
  dns.start(53, "*", AP_IP);

  static bool routesReady = false;  // les pages ne s'enregistrent qu'une fois, même si on rouvre l'Atelier
  if (!routesReady) {
    routesReady = true;
    server.on("/", HTTP_GET, [] { server.send_P(200, "text/html; charset=utf-8", PAGE); });
    server.on("/state", HTTP_GET, sendState);
    server.on("/px", HTTP_POST, handlePixels);
    server.on("/fill", HTTP_POST, handleFill);
    server.on("/save", HTTP_POST, handleSave);
    server.onNotFound(redirectHome);
  }
  server.begin();

  // Mise à jour sans câble : même mot de passe que le Wi-Fi
  ArduinoOTA.setHostname("mini-arcade");
  ArduinoOTA.setPassword(password.c_str());
  if (!otaReady) {
    otaReady = true;
    ArduinoOTA.onStart([] { otaBusy = true; drawOled(0); FastLED.clear(); FastLED.show(); });
    ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {  // barre de progression sur la matrice
      int n = total ? done * NUM_LEDS / total : 0;
      for (int i = 0; i < n; i++) leds[XY(i % 16, i / 16)] = CRGB(0, 60, 0);
      FastLED.show();
    });
  }
  ArduinoOTA.begin();
  running = true;
}

void stopPortal() {
  ArduinoOTA.end();
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  running = false;
}

}  // namespace

void atelierBegin() {
  if (!running) startPortal();
  lastStations = -1;
  dirty = true;
}

GameResult atelierLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  dns.processNextRequest();
  server.handleClient();
  ArduinoOTA.handle();
  if (btn2.pressed()) { stopPortal(); return GAME_QUIT; }
  btn1.pressed();
  btnEnc.pressed();

  int stations = WiFi.softAPgetStationNum();
  if (stations != lastStations) { lastStations = stations; dirty = true; }
  if (now - lastFrame >= 40) {
    lastFrame = now;
    FastLED.clear();
    drawDrawing(canvas, now);
    FastLED.show();
    setRgb(drawingColor(lastColor, 8, 8, now));
  }
  if (dirty) { drawOled(stations); dirty = false; }
  return GAME_RUNNING;
}
