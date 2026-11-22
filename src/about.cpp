// À propos : un QR code qui ouvre le guide pour reprogrammer la borne avec une IA (ABOUT_URL dans config.h).
// N'importe quel bouton : retour au menu.
#include "hw.h"
#include "save.h"
#include "pictures.h"
#include "activities.h"
#if __has_include("qrcode.h")
#include "qrcode.h"  // générateur de QR code fourni avec l'ESP32
#define HAS_QRCODE 1
#endif

namespace {

bool dirty = true;
int  qrSize = 0;

#ifdef HAS_QRCODE
void drawQr(esp_qrcode_handle_t qr) {  // 1 pixel par module, entièrement dans la partie bleue de l'OLED
  qrSize = esp_qrcode_get_size(qr);
  int offY = 16 + (48 - qrSize) / 2;
  for (int y = 0; y < qrSize; y++)
    for (int x = 0; x < qrSize; x++)
      if (esp_qrcode_get_module(qr, x, y)) oled.drawPixel(2 + x, offY + y);
}
#endif

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 11);  // bande jaune
  oled.print("À propos");
  int textX = 0;
#ifdef HAS_QRCODE
  esp_qrcode_config_t cfg = {};
  cfg.display_func = drawQr;
  cfg.max_qrcode_version = 6;
  cfg.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;
  esp_qrcode_generate(&cfg, ABOUT_URL);
  textX = qrSize + 8;
#endif
  const char* lines[] = {"Mini-arcade", "Code libre", "Guide pour", "ton IA :", "scanne-moi !"};
  for (int i = 0; i < 5; i++) { oled.setCursor(textX, 25 + i * 9); oled.print(lines[i]); }
  oled.sendBuffer();
}

}  // namespace

void aboutBegin() { dirty = true; }

GameResult aboutLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  bool any = btn1.pressed();
  any |= btnEnc.pressed();
  any |= btn2.pressed();
  if (any) return GAME_QUIT;
  if (now - lastFrame >= 50) {
    lastFrame = now;
    FastLED.clear();
    drawPicture(PIC_CHAT);  // Mochi
    for (int i = 0; i < 3; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 150, 255);
    FastLED.show();
    setRgb(CHSV(now / 20, 200, 120));
  }
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
