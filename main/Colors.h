#pragma once

#include <stdint.h>

class RGBColor;

class HSVColor {
public:
  uint8_t Hue;
  uint8_t Saturation;
  uint8_t Value;

public:
  HSVColor() : Hue(0), Saturation(0), Value(0) {}
  HSVColor(uint8_t Hue, uint8_t Saturation, uint8_t Value)
      : Hue(Hue), Saturation(Saturation), Value(Value) {}

public:
  RGBColor toRGBColor() const;
};

class RGBColor {
public:
  uint8_t Red;
  uint8_t Green;
  uint8_t Blue;

public:
  RGBColor() : Red(0), Green(0), Blue(0) {}
  RGBColor(uint8_t Red, uint8_t Green, uint8_t Blue)
      : Red(Red), Green(Green), Blue(Blue) {}

public:
  HSVColor toHSVColor() const;
};
