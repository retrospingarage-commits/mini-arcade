#pragma once
#include "Arduino.h"
#define U8G2_R0 0
#define U8X8_PIN_NONE 255
extern const uint8_t u8g2_font_6x10_tf[], u8g2_font_logisoso24_tn[], u8g2_font_10x20_tf[];
struct U8G2_SSD1306_128X64_NONAME_F_HW_I2C:Print{U8G2_SSD1306_128X64_NONAME_F_HW_I2C(int,int){} void begin(){} void setBusClock(uint32_t){} void setI2CAddress(uint8_t){} void clearBuffer(){ oledText.clear(); } void sendBuffer(){} void setFont(const uint8_t*){} void setCursor(int,int){} void drawDisc(int,int,int){} void enableUTF8Print(){} void setPowerSave(int){} void setFontMode(int){} void setDrawColor(int){}
 void drawBox(int,int,int,int){} void drawFrame(int,int,int,int){} void drawHLine(int,int,int){} void drawPixel(int,int){}
 int getUTF8Width(const char* s){ int n=0; for(;*s;s++) if((*s&0xC0)!=0x80) n++; return n*6; }};
using U8G2_SH1106_128X64_NONAME_F_HW_I2C = U8G2_SSD1306_128X64_NONAME_F_HW_I2C;
