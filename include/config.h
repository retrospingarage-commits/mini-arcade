#pragma once
// Brochage : carte "Shield capteurs v2.1" + ESP32 DevKit 30 broches (WROOM-32D)

// Matrice WS2812B 16x16 (connecteur RELAIS : GND + D16, alimentée en direct)
constexpr uint8_t  PIN_LED_DATA = 16;
constexpr uint16_t MATRIX_W = 16;
constexpr uint16_t MATRIX_H = 16;
constexpr uint16_t NUM_LEDS = MATRIX_W * MATRIX_H;
constexpr bool     MATRIX_SERPENTINE = true;   // à passer à false si l'image est déformée
constexpr uint16_t MAX_MILLIAMPS = 1500;       // limite logicielle du courant (chargeur 2,4 A partagé avec l'ESP32)

// Encodeur KY-040 / HW-040 (connecteur LORA : 3V3, GND, D18, D19, D23)
constexpr uint8_t PIN_ENC_A  = 18;  // CLK
constexpr uint8_t PIN_ENC_B  = 19;  // DT
constexpr uint8_t PIN_ENC_SW = 23;  // bouton de l'axe

// Buzzer passif HW-508 (connecteur LORA, même câble que l'encodeur : D25 + GND)
constexpr uint8_t PIN_BUZZER = 25;

// Boutons de jeu (connecteurs PLUVIO 1 et 2, 10K vers 3V3 + 100nF si soudés)
constexpr uint8_t PIN_BTN1 = 15;    // PLUVIO 1 : bouton HW-483 (strapping : OK avec pull-up)
constexpr uint8_t PIN_BTN2 = 36;    // PLUVIO 2 (VP) : bouton du joystick. PAS de pull-up interne, la 10K est obligatoire

// Joystick analogique (connecteurs HUM SOL 1 et 2, via les 1K soudées) : à alimenter en 3V3, PAS en 5V
constexpr uint8_t PIN_JOY_X = 32;   // HUM SOL 1 (S1)
constexpr uint8_t PIN_JOY_Y = 34;   // HUM SOL 2 (S2), entrée seule

// LED RVB HW-478 à cathode commune (connecteur RELAIS, même câble que la matrice : D17, D27, D33 + GND)
constexpr uint8_t PIN_RGB_R = 17;
constexpr uint8_t PIN_RGB_G = 27;
constexpr uint8_t PIN_RGB_B = 33;

// OLED SSD1306 128x64 en I2C (connecteur T H AIR 2) : SDA = 21, SCL = 22, adresse 0x3C
