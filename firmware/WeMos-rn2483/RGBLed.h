// **********************************************************************************
// RGB Led header file for WeMos Lora RN2483 Shield
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/4.0/
//
// Written by Charles-Henri Hallard http://ch2i.eu
//
// History : V1.20 2016-06-11 - Creation
//
// All text above must be included in any redistribution.
//
// **********************************************************************************

// Master Project File
#include "WeMos-rn2483.h"

#ifndef RGBLed_H
#define RGBLed_H

// value for HSL color
// see http://www.workwithcolor.com/blue-color-hue-range-01.htm
#define COLOR_RED              0
#define COLOR_ORANGE          30
#define COLOR_ORANGE_YELLOW   45
#define COLOR_YELLOW          60
#define COLOR_YELLOW_GREEN    90
#define COLOR_GREEN          120
#define COLOR_GREEN_CYAN     165
#define COLOR_CYAN           180
#define COLOR_CYAN_BLUE      210
#define COLOR_BLUE           240
#define COLOR_BLUE_MAGENTA   275
#define COLOR_MAGENTA        300
#define COLOR_PINK           350

// The RGB animation state machine 
typedef enum {
  RGB_ANIM_NONE,
  RGB_ANIM_FADE_IN,
  RGB_ANIM_FADE_OUT,
  RGB_ANIM_BLINK_ON,
  RGB_ANIM_BLINK_OFF,
} 
RgbEffectState_e;

// RGB Led on GPIO0
#define RGB_LED_PIN 	0
#define RGB_LED_COUNT 2
#define RGBW_LED 	/* I'm using a RGBW WS2812 led */

// Master RDB LED
#define RGB_LED1      1

#ifdef RGBW_LED
  typedef NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> MyPixelBus;

  // what is stored for state is specific to the need, in this case, the colors.
  // basically what ever you need inside the animation update function
  struct MyAnimationState {
    RgbwColor         RgbStartingColor;
    RgbwColor         RgbEndingColor;
    RgbwColor         RgbNoEffectColor;
    RgbEffectState_e  RgbEffectState;  // current effect of RGB LED 
    uint16_t          AnimTime;
    uint8_t           AnimCount; // Animation counter 
    //uint8_t   IndexPixel;   // general purpose variable used to store pixel index
  };
#else
  typedef NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBang800KbpsMethod> MyPixelBus;

  // what is stored for state is specific to the need, in this case, the colors.
  // basically what ever you need inside the animation update function
  struct MyAnimationState  {
    RgbColor          RgbStartingColor;
    RgbColor          RgbEndingColor;
    RgbColor          RgbNoEffectColor;
    RgbEffectState_e  RgbEffectState;  // current effect of RGB LED 
    uint16_t          AnimTime;
    uint8_t           AnimCount; // Animation counter 
    //uint8_t   IndexPixel;   // general purpose variable used to store pixel index
  };
#endif 

void LedRGBFadeAnimUpdate(const AnimationParam& param);
void LedRGBAnimate(bool force=false);
void LedRGBSetAnimation(uint16_t duration, uint16_t led=0, uint8_t count=0, RgbEffectState_e effect=RGB_ANIM_FADE_IN);
void LedRGBOFF(uint16_t led=0);
void LedRGBON (uint16_t hue, uint16_t led=0, bool doitnow=false);

extern uint16_t wifi_led_color ; 
extern uint8_t rgb_luminosity; // Luminosity from 0 to 100% 
extern uint16_t anim_speed;
extern MyPixelBus rgb_led;
extern MyAnimationState animationState[];


#endif
