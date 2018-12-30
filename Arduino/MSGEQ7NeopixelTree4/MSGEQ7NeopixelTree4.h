//Header file for NeopixelRGBVisualizer v2

//#define DEBUG
#define NEOPIXEL_ENABLE

#ifdef NEOPIXEL_ENABLE
#include <Adafruit_NeoPixel.h>
#define STRIP_PIN 2
#endif

// Import NicoHood's MSGEQ7 driver
#include <MSGEQ7.h>
// Number of channels of MSGEQ7 output
#define MSGEQ7_FREQCOUNT 7
// Analog input connected to the MSGEQ7 DC voltage pin
#define MSGEQ7_PINDCIN A0
// Digital output connected to the MSGEQ7 reset pin
#define MSGEQ7_PINRESET 4
// Digital output connected to the MSGEQ7 strobe pin
#define MSGEQ7_PINSTROBE 3
// Sample Interval of the MSGEQ7
#define MSGEQ7_INTERVAL 33333 // 1000000/30
// MSGEQ7 output mean filter rank
#define MSGEQ7_SMOOTH 10
// Reduction of mean volume for silence:
#define GAIN_OFFSET -1

// Button count
#define BUTTONCOUNT 4
// Digital input switched low by the "Mode" button
#define BUTTON_MODE 5
// Digital input switched low by the "-" button
#define BUTTON_MINUS 8
// Digital input switched low by the "+" button
#define BUTTON_PLUS 7
// Digital input switched low by the "I/O" header pins
#define BUTTON_CUSTOM 6
