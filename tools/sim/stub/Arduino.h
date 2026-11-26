#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cstdarg>
using std::min; using std::max;
#define IRAM_ATTR
#define INPUT 0
#define INPUT_PULLUP 1
#define CHANGE 2
extern uint32_t fakeMillis; extern bool fakePin[64];
inline uint32_t millis(){return fakeMillis;}
inline int digitalRead(int p){return fakePin[p];}
inline void pinMode(int,int){} inline void attachInterrupt(int,void(*)(),int){}
inline void noInterrupts(){} inline void interrupts(){}
template<class T,class A,class B> T constrain(T v,A a,B b){return v<a?a:v>b?b:v;}
inline long random(long a,long b){return a+rand()%(b-a);}
inline long random(long b){return rand()%b;}
struct String{ std::string s; const char* c_str()const{return s.c_str();} String& operator+=(char c){s+=c;return *this;} String substring(int i)const{String r; r.s=i<(int)s.size()?s.substr(i):"";return r;} float toFloat()const{return atof(s.c_str());} char operator[](int i)const{return i<(int)s.size()?s[i]:0;} String& operator=(const char*c){s=c;return *this;}};
extern std::string oledText; struct Print{ void printf(const char*f,...){} void print(const char* t){ oledText += t; oledText += "|"; } void print(uint32_t){} void print(int){} void println(const char*){} };
struct SerialT:Print{void begin(int){} int available(){return 0;} int read(){return 0;}};
extern SerialT Serial;
struct FakeAnalogInit{ int v[64]; FakeAnalogInit(){ for(int&x:v) x=2048; } };
inline FakeAnalogInit fakeAnalogInit;
inline int analogRead(int pin){return fakeAnalogInit.v[pin&63];}
inline void delay(int){}
inline bool ledcAttach(int,int,int){return true;}
inline void ledcWriteTone(int,int){}
extern int ledcVal[64]; inline void ledcWrite(int p,int v){ledcVal[p]=v;}
inline void ledcDetach(int){} inline void digitalWrite(int,int){} inline void delayMicroseconds(int){}
#define OUTPUT 3
#define LOW 0
inline uint32_t esp_random(){ return (uint32_t)rand()*2654435761u + rand(); }
