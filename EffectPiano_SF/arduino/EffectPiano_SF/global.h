#include "config.h"
typedef unsigned char uchar;

struct State {
    bool fExtend;
    uchar nExtend;
    bool fBackLight;
    bool fSwitchFrontColor;
    bool irrAvailable;
    bool fPause;
    bool fAutoBrightness;
    uchar brightness;
    char keyOffset;
    State(){
      fExtend = false;
      nExtend = 2;
      fBackLight = true;
      fSwitchFrontColor = false;
      irrAvailable = true;
      fPause = false;
      fAutoBrightness = true;
      brightness = DEFAULT_BRIGHTNESS;
      keyOffset = 0;
    }
};

struct Color {
    CRGB front;
    CRGB back;
};

struct ColorSet {
    char count;
    char frontSel;
    char backSel;
    Color color[COLOR_COUNT];
    ColorSet() {
        count = 0;
        frontSel = 0;
        backSel = 0;
        memset(color, 0, sizeof(color));
    }
};

struct MidiNote {
    short pitch;
    short dynamic;
    MidiNote() {
        pitch = -1;
        dynamic = -1;
    }
};

const long irrecvNum[10] = {
  0xFD30CF,
  0xFD08F7,
  0xFD8877,
  0xFD48B7,
  0xFD28D7,
  0xFDA857,
  0xFD6897,
  0xFD18E7,
  0xFD9867,
  0xFD58A7
};

#define IRR_EQ 0xFDB04F
#define IRR_POWER 0xFD00FF
#define IRR_STREPT 0xFD708F
#define IRR_PLAY 0xFDA05F
#define IRR_FUNCSTOP 0xFD40BF
#define IRR_VOLPLUS 0xFD807F
#define IRR_VOLMINUS 0xFD906F
#define IRR_LEFT 0xFD20DF
#define IRR_RIGHT 0xFD609F
#define IRR_UP 0xFD50AF
#define IRR_DOWN 0xFD10EF
