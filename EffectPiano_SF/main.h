#pragma once

#include "rtmidi/RtMidi.h"
#include "SerialPort/SerialPort.hpp"
#include "arduino/EffectPiano_SF/config.h"

#include <iostream>
#include <conio.h>

#define MAX_MIDI_KEY_COUNT 128
const unsigned char StartupCode = STARTUP_CODE;
const unsigned char ShutdownCode = SHUTDOWN_CODE;

int SelMidiIn(RtMidiIn* m);
void SelSerialPort();
unsigned char EncodeSerialMessage(int pitch, int dynamic);
bool SendSerialMessage(int pitch, int dynamic);
bool ReadState();
bool ReadColorSet();
bool SendState();
bool SendColorSet();
void MidiInCallback(double deltatime, std::vector< unsigned char >* message, void* userData);
void MainMessageLoop();

struct STATE {
	int midiInSeq;
};

struct ArduinoState {
    bool fExtend;
    short nExtend;
    bool fBackLight;
    bool fSwitchBackColor;
    bool irrAvailable;
    bool fPause;
    bool fAutoBrightness;
    short brightness;
    ArduinoState() {
        fExtend = false;
        nExtend = 2;
        fBackLight = true;
        fSwitchBackColor = false;
        irrAvailable = true;
        fPause = false;
        fAutoBrightness = true;
        brightness = DEFAULT_BRIGHTNESS;
    }
};

struct COLOR {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

struct Color {
    COLOR back;
    COLOR front;
};

struct ColorSet {
    short count;
    short frontSel;
    short backSel;
    Color color[COLOR_COUNT];
    ColorSet() {
        count = 0;
        frontSel = 0;
        backSel = 0;
        memset(color, 0, sizeof(color));
    }
};

struct MIDINOTE {
	double deltatime;
	int pitch;
	int dynamic;
	int channel;
};