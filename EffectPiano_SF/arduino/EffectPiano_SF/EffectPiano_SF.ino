#include <FastLED.h>
#include <IRremote.h>
#include "config.h"
#include "global.h"

using namespace std;

IRrecv irrecv(IRR_PIN);
decode_results results;
CRGB leds[NUM_LEDS];
//bool ledState[NUM_LEDS];

State state;
ColorSet colorSet;
MidiNote inNote;
unsigned long long timeCounter = 0;

void InternalInitColor(){
  colorSet.count = 3;
  colorSet.frontSel = 0;
  colorSet.backSel = 0;
  colorSet.color[0].front = CRGB(100, 48, 127);
  colorSet.color[0].back = CRGB(10, 10, 10);
  colorSet.color[1].front = CRGB(100, 30, 30);
  colorSet.color[1].back = CRGB(10, 5, 15);
  colorSet.color[2].front = CRGB(20, 24, 127);
  colorSet.color[2].back = CRGB(4, 11, 12);
}

void ClearLeds(){
  if(state.fBackLight){
    for(short i = 0; i < NUM_LEDS; i++){
      leds[i] = colorSet.color[colorSet.backSel].back;
    }
  }
  else{
    memset(leds, 0, sizeof(leds));
  }
  //memset(ledState, 0, sizeof(ledState));
  FastLED.show();
}

short Key2Led(short key){
  if(MIDI_KEY_COUNT == 88){
    return (key - MIDI_KEY_OFFSET_88 + state.keyOffset) * 2 + 1;
  }
  else if (MIDI_KEY_COUNT == 61) {
    return (key - MIDI_KEY_OFFSET_61 + state.keyOffset) * 2 + 1;
  }
  return 0;
}

void StartupListen(){
  short startValue = -1;
  short ledBrightness = 0;
  short ledBrightnessD = 1;
  while(startValue != STARTUP_CODE){
    startValue = Serial.read();
    leds[0] = CRGB(ledBrightness/2, ledBrightness/2, ledBrightness/2);
    ledBrightness += ledBrightnessD;
    if(ledBrightness >= 127 || ledBrightness <= 0){
      ledBrightnessD = -ledBrightnessD;
    }
    FastLED.show();
    delay(5);
  }
  leds[0] = CRGB(0, 0, 0);
}

void IRRResponse(){
  digitalWrite(DBG_PIN, 1);
  digitalWrite(BUZZER_PIN, 1);
}

void IRRControl(){
  if(irrecv.decode(&results)){
    //set midi key offset
    if(results.value == IRR_VOLPLUS){
      state.keyOffset += 12;
      IRRResponse();
      irrecv.resume();
      return;
    }
    if (results.value == IRR_VOLMINUS){
      state.keyOffset -= 12;
      IRRResponse();
      irrecv.resume();
      return;
    }
    if(results.value == IRR_LEFT){
      state.keyOffset--;
      IRRResponse();
      irrecv.resume();
      return;
    }
    if(results.value == IRR_RIGHT){
      state.keyOffset++;
      IRRResponse();
      irrecv.resume();
      return;
    }
    if(results.value == IRR_PLAY){
      state.keyOffset = 0;
      IRRResponse();
      irrecv.resume();
      return;
    }
    
    if(results.value == IRR_EQ){
      state.fSwitchFrontColor = !state.fSwitchFrontColor;
      IRRResponse();
      irrecv.resume();
      return;
    }
    if(results.value == IRR_POWER){
      state.fPause = !state.fPause;
      if(state.fPause){
        FastLED.setBrightness(0);
        FastLED.show();
      }
      else{
        FastLED.setBrightness(DEFAULT_BRIGHTNESS);
        FastLED.show();
      }
      IRRResponse();
      irrecv.resume();
      return;
    }
    if(results.value == IRR_UP){
      state.fAutoBrightness = !state.fAutoBrightness;
      IRRResponse();
      irrecv.resume();
      return;
    }
    if(results.value == IRR_DOWN){
      ClearLeds();
      IRRResponse();
      irrecv.resume();
      return;
    }
    if(results.value == IRR_FUNCSTOP){
      state.fExtend = !state.fExtend;
      IRRResponse();
      irrecv.resume();
      return;
    }
    if(results.value == IRR_STREPT){
      state.fBackLight = !state.fBackLight;
      ClearLeds();
      IRRResponse();
      irrecv.resume();
      return;
    }
    for(char i = 0; i < colorSet.count; i++){
      if(results.value == irrecvNum[i]){
        if(!state.fSwitchFrontColor){
          colorSet.frontSel = i;
          colorSet.backSel = i;
          if(state.fBackLight){
            for(short j = 0; j < NUM_LEDS; j++){
              leds[j] = colorSet.color[i].back;
            }
            FastLED.show();
          }
        }
        else{
          state.fSwitchFrontColor = false;
          colorSet.frontSel = i;
        }
        IRRResponse();
      }
    }
    irrecv.resume();
    return;
  }
}

//read message from serial
MidiNote ReadMsg(){
  MidiNote note = MidiNote();
  unsigned char n = 0;
  while(1){
    if(Serial.available() > 0){
      n = Serial.read();
      break;
    }
    if(millis() - timeCounter * LF_DELAY_TIME > 0){
      timeCounter++;
      BrightnessAdjust();
      digitalWrite(BUZZER_PIN, 0);
      digitalWrite(DBG_PIN, 0);
      short pushButtonValue = digitalRead(BUTTON_PIN);
      if(pushButtonValue == HIGH){
        ClearLeds();
      }
    }
    IRRControl();
  }
  if(n == SHUTDOWN_CODE){
    Shutdown();
  }
  note.pitch = n & 0x7F;
  note.dynamic = n >> 7;
  return note;
}

short Lerp(short src, short dest, float lerpRate){
  return src + (dest - src) * lerpRate;
}

//control the leds
void LedControl(MidiNote note){
  short ledPos = Key2Led(note.pitch);
  if(ledPos + 1 < NUM_LEDS && ledPos >= 0){
    if(note.dynamic == 0){
      if(state.fBackLight){
        if(!state.fExtend){
          leds[ledPos] = colorSet.color[colorSet.backSel].back;
          leds[ledPos + 1] = colorSet.color[colorSet.backSel].back;
        }
        else{
          bool fImpact;
          short r, g, b;
          fImpact = false;
          for(short i = 0; i < state.nExtend; i++){
            if(ledPos - i - 1 < 0){
              break;
            }
            if(leds[ledPos - i - 1] == colorSet.color[colorSet.frontSel].front){
              fImpact = true;
              r = Lerp(colorSet.color[colorSet.frontSel].front.r, colorSet.color[colorSet.backSel].back, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              g = Lerp(colorSet.color[colorSet.frontSel].front.g, colorSet.color[colorSet.backSel].back, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              b = Lerp(colorSet.color[colorSet.frontSel].front.b, colorSet.color[colorSet.backSel].back, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              leds[ledPos] = CRGB(r, g, b);
              break;
            }
          }
          if(!fImpact){
            leds[ledPos] = colorSet.color[colorSet.backSel].back;
          }
          fImpact = false;
          for(short i = 0; i < state.nExtend; i++){
            if(ledPos + i + 2 >= NUM_LEDS){
              break;
            }
            if(leds[ledPos + i + 2] == colorSet.color[colorSet.frontSel].front){
              fImpact = true;
              r = Lerp(colorSet.color[colorSet.frontSel].front.r, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              g = Lerp(colorSet.color[colorSet.frontSel].front.g, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              b = Lerp(colorSet.color[colorSet.frontSel].front.b, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              leds[ledPos + 1] = CRGB(r, g, b);
              break;
            }
          }
          if(!fImpact){
            leds[ledPos + 1] = colorSet.color[colorSet.backSel].back;
          }
          for(short i = 0; i < state.nExtend; i++){
            if(ledPos - i - 1 < 0 || ledPos + i + 2 >= NUM_LEDS){
              break;
            }
            if(leds[ledPos - i - 1] != colorSet.color[colorSet.frontSel].front){
              fImpact = false;
              for(short j = 1; j <= state.nExtend; j++){
                if(ledPos - i - 1 - j < 0){
                  break;
                }
                if(leds[ledPos - i - 1 - j] == colorSet.color[colorSet.frontSel].front){
                  fImpact = true;
                  r = Lerp(colorSet.color[colorSet.frontSel].front.r, colorSet.color[colorSet.backSel].back.r, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  g = Lerp(colorSet.color[colorSet.frontSel].front.g, colorSet.color[colorSet.backSel].back.g, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  b = Lerp(colorSet.color[colorSet.frontSel].front.b, colorSet.color[colorSet.backSel].back.b, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  leds[ledPos - i - 1] = CRGB(r, g, b);
                  break;
                }
              }
              if(!fImpact){
                leds[ledPos - i - 1] = colorSet.color[colorSet.backSel].back;
              }
            }
            if(leds[ledPos + i + 2] != colorSet.color[colorSet.frontSel].front){
              fImpact = false;
              for(short j = 1; j <= state.nExtend; j++){
                if(ledPos + i + 2 + j >= NUM_LEDS){
                  break;
                }
                if(leds[ledPos + i + 2 + j] == colorSet.color[colorSet.frontSel].front){
                  fImpact = true;
                  r = Lerp(colorSet.color[colorSet.frontSel].front.r, colorSet.color[colorSet.backSel].back.r, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  g = Lerp(colorSet.color[colorSet.frontSel].front.g, colorSet.color[colorSet.backSel].back.g, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  b = Lerp(colorSet.color[colorSet.frontSel].front.b, colorSet.color[colorSet.backSel].back.b, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  leds[ledPos + i + 2] = CRGB(r, g, b);
                  break;
                }
              }
              if(!fImpact){
                leds[ledPos + i + 2] = colorSet.color[colorSet.backSel].back;
              }
            }
          }
        }
      }
      else{
        if(!state.fExtend){
          leds[ledPos] = CRGB(0, 0, 0);
          leds[ledPos + 1] = CRGB(0, 0, 0);
        }
        else{
          bool fImpact;
          short r, g, b;
          fImpact = false;
          for(short i = 0; i < state.nExtend; i++){
            if(ledPos - i - 1 < 0){
              break;
            }
            if(leds[ledPos - i - 1] == colorSet.color[colorSet.frontSel].front){
              fImpact = true;
              r = Lerp(colorSet.color[colorSet.frontSel].front.r, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              g = Lerp(colorSet.color[colorSet.frontSel].front.g, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              b = Lerp(colorSet.color[colorSet.frontSel].front.b, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              leds[ledPos] = CRGB(r, g, b);
              break;
            }
          }
          if(!fImpact){
            leds[ledPos] = CRGB(0, 0, 0);
          }
          fImpact = false;
          for(short i = 0; i < state.nExtend; i++){
            if(ledPos + i + 2 > NUM_LEDS){
              break;
            }
            if(leds[ledPos + i + 2] == colorSet.color[colorSet.frontSel].front){
              fImpact = true;
              r = Lerp(colorSet.color[colorSet.frontSel].front.r, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              g = Lerp(colorSet.color[colorSet.frontSel].front.g, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              b = Lerp(colorSet.color[colorSet.frontSel].front.b, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
              leds[ledPos + 1] = CRGB(r, g, b);
              break;
            }
          }
          if(!fImpact){
            leds[ledPos + 1] = CRGB(0, 0, 0);
          }
          for(short i = 0; i < state.nExtend; i++){
            if(ledPos - i - 1 < 0 || ledPos + i + 2 >= NUM_LEDS){
              break;
            }
            if(leds[ledPos - i - 1] != colorSet.color[colorSet.frontSel].front){
              fImpact = false;
              for(short j = 1; j <= state.nExtend; j++){
                if(ledPos - i - 1 - j < 0){
                  break;
                }
                if(leds[ledPos - i - 1 - j] == colorSet.color[colorSet.frontSel].front){
                  fImpact = true;
                  r = Lerp(colorSet.color[colorSet.frontSel].front.r, 0, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  g = Lerp(colorSet.color[colorSet.frontSel].front.g, 0, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  b = Lerp(colorSet.color[colorSet.frontSel].front.b, 0, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  leds[ledPos - i - 1] = CRGB(r, g, b);
                  break;
                }
              }
              if(!fImpact){
                leds[ledPos - i - 1] = CRGB(0, 0, 0);
              }
            }
            if(leds[ledPos + i + 2] != colorSet.color[colorSet.frontSel].front){
              fImpact = false;
              for(short j = 1; j <= state.nExtend; j++){
                if(leds[ledPos + i + 2 + j] == colorSet.color[colorSet.frontSel].front){
                  if(ledPos + i + 2 + j >= NUM_LEDS){
                    break;
                  }
                  fImpact = true;
                  r = Lerp(colorSet.color[colorSet.frontSel].front.r, 0, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  g = Lerp(colorSet.color[colorSet.frontSel].front.g, 0, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  b = Lerp(colorSet.color[colorSet.frontSel].front.b, 0, 1.0 / state.nExtend * (j - 1 + 1.0 / state.nExtend));
                  leds[ledPos + i + 2] = CRGB(r, g, b);
                  break;
                }
              }
              if(!fImpact){
                leds[ledPos + i + 2] = CRGB(0, 0, 0);
              }
            }
          }
        }
      }
    }
    else{
      leds[ledPos] = colorSet.color[colorSet.frontSel].front;
      leds[ledPos + 1] = colorSet.color[colorSet.frontSel].front;
      if(state.fExtend){
        short r, g, b;
        for(short i = 0; i < state.nExtend; i++){
          if(ledPos - i - 1 < 0 && ledPos + i + 2 >= NUM_LEDS){
            break;
          }
          if(state.fBackLight){
            r = Lerp(colorSet.color[colorSet.frontSel].front.r, colorSet.color[colorSet.backSel].back.r, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
            g = Lerp(colorSet.color[colorSet.frontSel].front.g, colorSet.color[colorSet.backSel].back.g, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
            b = Lerp(colorSet.color[colorSet.frontSel].front.b, colorSet.color[colorSet.backSel].back.b, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
          }
          else{
            r = Lerp(colorSet.color[colorSet.frontSel].front.r, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
            g = Lerp(colorSet.color[colorSet.frontSel].front.g, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
            b = Lerp(colorSet.color[colorSet.frontSel].front.b, 0, 1.0 / state.nExtend * (i + 1.0 / state.nExtend));
          }
          if((colorSet.color[colorSet.frontSel].front.r >= colorSet.color[colorSet.backSel].back.r ? (leds[ledPos - i - 1].r < r) : (leds[ledPos - i - 1].r > r)) ||
             (colorSet.color[colorSet.frontSel].front.g >= colorSet.color[colorSet.backSel].back.g ? (leds[ledPos - i - 1].g < g) : (leds[ledPos - i - 1].g > g)) ||
             (colorSet.color[colorSet.frontSel].front.b >= colorSet.color[colorSet.backSel].back.b ? (leds[ledPos - i - 1].b < b) : (leds[ledPos - i - 1].b > b))){
            leds[ledPos - i - 1] = CRGB(r, g, b);
          }
          if((colorSet.color[colorSet.frontSel].front.r >= colorSet.color[colorSet.backSel].back.r ? (leds[ledPos + i + 2].r < r) : (leds[ledPos + i + 2].r > r)) ||
             (colorSet.color[colorSet.frontSel].front.g >= colorSet.color[colorSet.backSel].back.g ? (leds[ledPos + i + 2].g < g) : (leds[ledPos + i + 2].g > g)) ||
             (colorSet.color[colorSet.frontSel].front.b >= colorSet.color[colorSet.backSel].back.b ? (leds[ledPos + i + 2].b < b) : (leds[ledPos + i + 2].b > b))){
            leds[ledPos + i + 2] = CRGB(r, g, b);
          }
        }
      }
    }
    FastLED.show();
  }
}

void Shutdown(){
  digitalWrite(DBG_PIN, 0);
  state = State();
  memset(leds, 0, sizeof(leds));
  FastLED.show();
  setup();
}

void BrightnessAdjust(){
  if(state.fPause){
    return;
  }
  short analogIn = 0;
  if(state.fAutoBrightness){
    analogIn = analogRead(ALS_PIN);
    analogIn = ALS_MAP(analogIn);
  }
  else {
    analogIn = 1023 - analogRead(KNOB_PIN);
    analogIn *= ANALOG_INPUT_FACTOR;
  }
  analogIn = min(127, analogIn);
  analogIn = max(16, analogIn);
  short afterLerp = Lerp(state.brightness, analogIn, (state.fAutoBrightness ? SLOW_LERP_RATE : FAST_LERP_RATE));
  if(abs(afterLerp - state.brightness) > 1){
    state.brightness = afterLerp;
    FastLED.setBrightness(state.brightness);
    FastLED.show();
  }
}

//to run once
void setup() {
  state = State();
  colorSet = ColorSet();
  Serial.begin(SIRIAL_BAUDRATE);
  InternalInitColor();
  pinMode(DBG_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);//0~255
  StartupListen();
  irrecv.enableIRIn();
  ClearLeds();
  FastLED.show();
  timeCounter = millis() / LF_DELAY_TIME;
}

//to run repeatedly
void loop() {
  inNote = ReadMsg();
  LedControl(inNote);
}
