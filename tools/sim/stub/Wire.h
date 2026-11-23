#pragma once
#include <cstdint>
struct TwoWire { void begin(int,int){} void beginTransmission(uint8_t){} uint8_t endTransmission(){ return 0; } };
extern TwoWire Wire;
