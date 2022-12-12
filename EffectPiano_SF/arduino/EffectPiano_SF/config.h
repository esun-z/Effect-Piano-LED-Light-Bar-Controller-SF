#pragma once

//pin
#define LED_PIN 7
#define IRR_PIN 8
#define KNOB_PIN 0
#define ALS_PIN 1
#define DBG_PIN 13
#define BUZZER_PIN 12
#define BUTTON_PIN 6

//serial
#define SIRIAL_BAUDRATE 19200

//count
#define NUM_LEDS 178
#define COLOR_COUNT 10
#define MIDI_KEY_COUNT 61

//offset
#define MIDI_KEY_OFFSET_88 21
#define MIDI_KEY_OFFSET_61 36

//default settings
#define DEFAULT_BRIGHTNESS 64

//trans code
#define STARTUP_CODE 127
#define SHUTDOWN_CODE 126
#define STATE_TRANS_CODE 125
#define COLORSET_TRANS_CODE 124

//time control
#define FAST_LERP_RATE 0.1
#define SLOW_LERP_RATE 0.05
#define LF_DELAY_TIME 100

//map
#define ANALOG_INPUT_FACTOR 0.125
#define ALS_MAP(x) 0.000000642931728*x*x*x+0.000926067680086*x*x+0.475726692295626*x+13.704003630533682
