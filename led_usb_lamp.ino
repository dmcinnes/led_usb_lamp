#include <avr/interrupt.h>
#include <DigisparkRGB.h>
/*
   Digispark RGB

   This example shows how to use soft PWM to fade 3 colors.
Note: This is only necessary for PB2 (pin 2) - Blue, as Red (pin 0) and Green (pin 1) as well as pin 4 support the standard Arduino analogWrite() function.

This example code is in the public domain.
*/
byte RED = 0;
byte BLUE = 2;
byte GREEN = 1;
byte COLORS[] = {RED, BLUE, GREEN};

boolean dir = true;
int currentColor = 0;

unsigned long counter         = 0;
unsigned long previousCounter = 1;

bool off   = false;
bool sleep = false;

// the setup routine runs once when you press reset:
void setup() {
  DigisparkRGBBegin();

  // Enable interrupts only on pins 3 and 4
  PCMSK = (1<<PCINT3) | (1<<PCINT4);
  // Enable PCINT interrupts
  GIMSK = (1<<PCIE);

  // Turn on global interrupts
  sei();
}

void loop () {
  if (off) {
    // do nothing
  } else if (sleep) {
    fade(0, false);
    fade(1, false);
    fade(2, false);
    off = true;
    dir = true; // so it fades back up
  } else {
    fade(COLORS[currentColor], dir);
    currentColor = (currentColor+1) % 3;
    dir = !dir;
  }
}

bool checkSleep() {
  if (counter == previousCounter && !sleep) {
    sleep = true;
    return true;
  }
  previousCounter = counter;
  return false;
}

void fade(byte Led, boolean dir) {
  int i;

  //if fading up
  if (dir) {
    for (i = 0; i < 256; i++) {
      DigisparkRGB(Led, i);
      DigisparkRGBDelay(25);
      if (checkSleep()) {
        break;
      }
    }
  } else {
    for (i = 255; i >= 0; i--) {
      DigisparkRGB(Led, i);
      DigisparkRGBDelay(25);
      if (checkSleep()) {
        break;
      }
    }
  }
}

// called when data appears on the USB pins
ISR(PCINT0_vect) {
  counter++;
  off = false;
  sleep = false;
}
