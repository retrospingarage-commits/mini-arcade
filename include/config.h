#pragma once
// Brochage : carte "Shield capteurs v2.1" + ESP32 DevKit 30 broches (WROOM-32D)

// Matrice WS2812B 16x16 (connecteur RELAIS : GND + D16, alimentée en direct)
constexpr uint8_t  PIN_LED_DATA = 16;
constexpr uint16_t MATRIX_W = 16;
constexpr uint16_t MATRIX_H = 16;
constexpr uint16_t NUM_LEDS = MATRIX_W * MATRIX_H;
constexpr bool     MATRIX_SERPENTINE = true;   // à passer à false si l'image est déformée
constexpr uint16_t MAX_MILLIAMPS = 2000;       // limite logicielle du courant

// Encodeur KY-040 (connecteur LORA : 3V3, GND, D18, D19, D23)
constexpr uint8_t PIN_ENC_A  = 18;  // CLK
constexpr uint8_t PIN_ENC_B  = 19;  // DT
constexpr uint8_t PIN_ENC_SW = 23;  // bouton de l'axe

// Boutons de jeu (connecteurs PLUVIO 1 et 2, 10K vers 3V3 + 100nF si soudés)
constexpr uint8_t PIN_BTN1 = 15;    // PLUVIO 1 (strapping : OK avec pull-up)
constexpr uint8_t PIN_BTN2 = 36;    // PLUVIO 2 (VP) : PAS de pull-up interne, la 10K est obligatoire

// OLED SSD1306 128x64 en I2C (connecteur T H AIR 2) : SDA = 21, SCL = 22, adresse 0x3C
