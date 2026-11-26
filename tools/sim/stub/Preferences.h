#pragma once
#include <map>
#include <string>
#include <cstdint>
#include "Arduino.h"
#include <cstring>
#include <algorithm>
struct Preferences {
  static std::map<std::string,uint8_t>& store(){ static std::map<std::string,uint8_t> m; return m; }
  bool begin(const char*, bool){ return true; } void end(){}
  uint8_t getUChar(const char* k, uint8_t d){ auto it=store().find(k); return it==store().end()?d:it->second; }
  uint16_t getUShort(const char* k, uint16_t d){ auto it=store16().find(k); return it==store16().end()?d:it->second; }
  size_t putUShort(const char* k, uint16_t v){ store16()[k]=v; return 2; }
  static std::map<std::string,uint16_t>& store16(){ static std::map<std::string,uint16_t> m; return m; }
  static std::map<std::string,std::string>& blobs(){ static std::map<std::string,std::string> m; return m; }
  size_t getBytes(const char* k, void* out, size_t n){ auto it=blobs().find(k); if(it==blobs().end()) return 0; memcpy(out,it->second.data(),std::min(n,it->second.size())); return std::min(n,it->second.size()); }
  size_t putBytes(const char* k, const void* in, size_t n){ blobs()[k]=std::string((const char*)in,n); return n; }
  static std::map<std::string,std::string>& strs(){ static std::map<std::string,std::string> m; return m; }
  String getString(const char* k, const char* d){ String r; auto it=strs().find(k); r.s = it==strs().end()? d : it->second; return r; }
  size_t putString(const char* k, const char* v){ strs()[k]=v; return 1; }
  bool isKey(const char* k){ return store().count(k) || blobs().count(k) || store16().count(k) || strs().count(k); }
  bool remove(const char* k){ store().erase(k); blobs().erase(k); store16().erase(k); strs().erase(k); return true; }
  size_t putUChar(const char* k, uint8_t v){ store()[k]=v; return 1; }
};
