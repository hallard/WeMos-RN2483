// **********************************************************************************
// RGB Led source file for WeMos Lora RN2483 Shield
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
#include "RGBLed.h"

MyPixelBus rgb_led(RGB_LED_COUNT, RGB_LED_PIN);
uint8_t rgb_luminosity = 20 ; // Luminosity from 0 to 100% 
uint16_t wifi_led_color ; // Wifi Led Color dependinf on connexion type
uint16_t anim_speed;

// one entry per pixel to match the animation timing manager
NeoPixelAnimator animations(RGB_LED_COUNT); 
MyAnimationState animationState[RGB_LED_COUNT];

/* ======================================================================
Function: LedRGBBlinkAnimUpdate
Purpose : Blink Anim update for RGB Led
Input   : -
Output  : - 
Comments: grabbed from NeoPixelBus library examples
====================================================================== */
void LedRGBBlinkAnimUpdate(const AnimationParam& param)
{
  // this gets called for each animation on every time step
  // progress will start at 0.0 and end at 1.0
  rgb_led.SetPixelColor(param.index, RgbColor(0));

  // 25% on so 75% off
  if (param.progress < 0.25f) {
    rgb_led.SetPixelColor(param.index, animationState[param.index].RgbNoEffectColor);
  }

  // done, time to restart this position tracking animation/timer
  if (param.state == AnimationState_Completed)
    animations.RestartAnimation(param.index);
}

/* ======================================================================
Function: LedRGBFadeAnimUpdate
Purpose : Fade in and out effect for RGB Led
Input   : -
Output  : - 
Comments: grabbed from NeoPixelBus library examples
====================================================================== */
void LedRGBFadeAnimUpdate(const AnimationParam& param)
{
  // this gets called for each animation on every time step
  // progress will start at 0.0 and end at 1.0
  // apply a exponential curve to both front and back
  float progress = param.progress;

  if (animationState[param.index].RgbEffectState==RGB_ANIM_FADE_IN)
    progress =  NeoEase::QuadraticOut(param.progress) ;

  if (animationState[param.index].RgbEffectState==RGB_ANIM_FADE_OUT)
    progress =  NeoEase::QuadraticIn(param.progress) ;

  // we use the blend function on the RgbColor to mix
  // color based on the progress given to us in the animation
  #ifdef RGBW_LED
    RgbwColor updatedColor = RgbwColor::LinearBlend( 
                                        animationState[param.index].RgbStartingColor, 
                                        animationState[param.index].RgbEndingColor, 
                                        progress);
    rgb_led.SetPixelColor(param.index, updatedColor);
  #else
    RgbColor updatedColor = RgbColor::LinearBlend( 
                                      animationState[param.index].RgbStartingColor, 
                                      animationState[param.index].RgbEndingColor, 
                                      progress);
    rgb_led.SetPixelColor(param.index, updatedColor);
  #endif
}

/* ======================================================================
Function: LedRGBAnimate
Purpose : Manage RGBLed Animations
Input   : true if forcing setup
Output  : - 
Comments: 
====================================================================== */
void LedRGBAnimate(bool force)
{
  static unsigned long ctx;

  if ( animations.IsAnimating() && !force ) {
    // the normal loop just needs these two to run the active animations
    animations.UpdateAnimations();
    rgb_led.Show();

  } else {

    ctx++;

    // Loop trough animations
    for (uint8_t i=0 ; i<RGB_LED_COUNT; i++) {
      bool restart = true;
      RgbEffectState_e effect;

      // We start animation with current led color
      animationState[i].RgbStartingColor = rgb_led.GetPixelColor(i);

      // We have a counter on animations 
      if (animationState[i].AnimCount>1) {
        animationState[i].AnimCount--;
        //Debugf("%d Animation(%d) Count=%d", ctx, i, animationState[i].AnimCount);

        // Last one ?
        if (animationState[i].AnimCount==1) {
          // Stop this animation
          animationState[i].RgbEffectState=RGB_ANIM_NONE;
          animationState[i].AnimTime=0;
          animations.StopAnimation(i);
          restart = false;
          //Debugf(" Stopping effect=%d", animationState[i].RgbEffectState );
        }

        //Debugln("");
      }

      effect = animationState[i].RgbEffectState;

      // And a time value
      if (animationState[i].AnimTime == 0 )
        restart = false;

      // Fade in 
      if ( effect==RGB_ANIM_FADE_IN ) {
        animationState[i].RgbEndingColor = animationState[i].RgbNoEffectColor; // selected color
        animationState[i].RgbEffectState=RGB_ANIM_FADE_OUT; // Next
        if (restart)
          animations.StartAnimation(i, animationState[i].AnimTime, LedRGBFadeAnimUpdate);

      // Fade out
      } else if ( effect==RGB_ANIM_FADE_OUT ) {
        animationState[i].RgbEndingColor = RgbColor(0); // off
        animationState[i].RgbEffectState=RGB_ANIM_FADE_IN; // Next
        if (restart)
          animations.StartAnimation(i, animationState[i].AnimTime, LedRGBFadeAnimUpdate);

      // Blink ON
      } else if ( effect==RGB_ANIM_BLINK_ON ) {
        animationState[i].RgbEffectState=RGB_ANIM_BLINK_OFF; // Next
        if (restart)
          animations.StartAnimation(i, animationState[i].AnimTime, LedRGBBlinkAnimUpdate);

      // Blink OFF
      } else if ( effect==RGB_ANIM_BLINK_OFF ) {
        animationState[i].RgbEffectState=RGB_ANIM_BLINK_ON; // Next
        if (restart)
          animations.StartAnimation(i, animationState[i].AnimTime, LedRGBBlinkAnimUpdate);
      }
      //Debugf("%d Animation(%d) restart=%d, duration=%ld, count=%d, effect=%d\r\n", ctx, i, restart, animationState[i].AnimTime,  animationState[i].AnimCount, animationState[i].RgbEffectState );
    }
  }
}

/* ======================================================================
Function: LedRGBSetAnimation
Purpose : Manage RGBLed Animations
Input   : animation duration (in ms)
          led number (from 1 to ...), if 0 then all leds
          number of animation count (0 = infinite)
          animation type
Input   : -
Output  : - 
Comments: 
====================================================================== */
void LedRGBSetAnimation(uint16_t duration, uint16_t led, uint8_t count, RgbEffectState_e effect)
{
  uint8_t start = 0;
  uint8_t end   = RGB_LED_COUNT-1; // Start at 0
  
  // just one LED ?
  // Strip start 0 not 1
  if (led) {
    led--;
    start = led ;
    end   = start ;
  } 

  // Specific counter for animation ?
  if (count) {
    // Counting stopping to 1 (0 used for infinite) so add 1
    // Counting is decremented before doing, so add 1 again
    count+=2;
  }

  for (uint8_t i=start ; i<=end; i++) {
    animationState[i].RgbEffectState = effect;
    animationState[i].AnimCount      = count ;
    animationState[i].AnimTime       = duration;
    //Debugf("SetAnimation(%d) duration=%ld, count=%d, effect=%d\r\n", i, duration, count, effect );
  }
}

/* ======================================================================
Function: LedRGBON
Purpose : Set RGB LED strip color, but does not lit it
Input   : Hue of LED (0..360)
          led number (from 1 to ...), if 0 then all leds
          if led should be lit immediatly
Output  : - 
Comments: 
====================================================================== */
void LedRGBON (uint16_t hue, uint16_t led, bool doitnow)
{
  uint8_t start = 0;
  uint8_t end   = RGB_LED_COUNT-1; // Start at 0

  // Convert to neoPixel API values
  // H (is color from 0..360) should be between 0.0 and 1.0
  // S is saturation keep it to 1
  // L is brightness should be between 0.0 and 0.5
  // rgb_luminosity is between 0 and 100 (percent)
  RgbColor target = HslColor( hue / 360.0f, 1.0f, 0.005f * rgb_luminosity);

  // just one LED ?
  // Strip start 0 not 1
  if (led) {
    led--;
    start = led ;
    end   = start ;
  } 

  for (uint8_t i=start ; i<=end; i++) {
    animationState[i].RgbNoEffectColor = target;
    // do it now ?
    if (doitnow) {
      // Stop animation
      animations.StopAnimation(i);
      animationState[i].RgbEndingColor  = RgbColor(0);
      rgb_led.SetPixelColor(i, target);
      rgb_led.Show();  
    }
  }
}

/* ======================================================================
Function: LedRGBOFF 
Purpose : light off the RGB LED strip
Input   : Led number starting at 1, if 0=>all leds
Output  : - 
Comments: -
====================================================================== */
void LedRGBOFF(uint16_t led)
{
  uint8_t start = 0;
  uint8_t end   = RGB_LED_COUNT-1; // Start at 0

  // just one LED ?
  if (led) {
    led--;
    start = led ;
    end   = start ;
  }

  // stop animation, reset params
  for (uint8_t i=start ; i<=end; i++) {
    animations.StopAnimation(i);
    animationState[i].RgbStartingColor = RgbColor(0);
    animationState[i].RgbEndingColor   = RgbColor(0);
    animationState[i].RgbNoEffectColor = RgbColor(0);
    animationState[i].RgbEffectState   = RGB_ANIM_NONE;

    // clear the led strip
    rgb_led.SetPixelColor(i, RgbColor(0));
    rgb_led.Show();  
  }
}

