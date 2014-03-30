#include <avr/interrupt.h>
#include <DigisparkRGB.h>

#define STATE_CYCLE   0
#define STATE_HOLD    1
#define STATE_BEDTIME 2
#define STATE_SLEEP   3

// in milliseconds
// time between 1/255 color changes
#define SHIFT_TIMEOUT      25
// time it holds a chosen color
#define HOLD_COLOR_TIMEOUT 2000
// how long to wait for the pin changes to stop
#define WAIT_TIMEOUT       10000

uint8_t state = STATE_SLEEP;

unsigned char color[]     = {0, 0, 0};
unsigned char nextColor[] = {0, 0, 0};

unsigned int counter = 0;


// the setup routine runs once when you press reset:
void setup() {
  DigisparkRGBBegin();

  // Enable interrupts only on pins 3 and 4
  PCMSK = (1<<PCINT3) | (1<<PCINT4);
  // Enable PCINT interrupts
  GIMSK = (1<<PCIE);

  // Turn on global interrupts
  sei();

  chooseNextColor();
}

void loop () {
  switch (state) {
    case STATE_CYCLE:
      cycleToNextColor();
      break;
    case STATE_HOLD:
      holdColor();
      break;
    case STATE_BEDTIME:
      shuttingDown();
      break;
    case STATE_SLEEP:
      checkForWake();
      break;
  }
}

bool match() {
  return color[0] == nextColor[0] &&
         color[1] == nextColor[1] &&
         color[2] == nextColor[2];
}

void shiftColors() {
  unsigned int i;

  DigisparkRGBDelay(SHIFT_TIMEOUT);

  for (i = 0; i < 3; i++) {
    if (color[i] < nextColor[i]) {
      color[i]++;
    } else if (color[i] > nextColor[i]) {
      color[i]--;
    }
  }

  DigisparkRGB(0, color[0]);
  DigisparkRGB(1, color[1]);
  DigisparkRGB(2, color[2]);
}

void chooseNextColor() {
  unsigned int i;

  for (i = 0; i < 3; i++) {
    nextColor[i] = random(255);
    if (nextColor[i] < 50) {
      nextColor[i] = 0;
    }
  }

  if (nextColor[0] == 0 &&
      nextColor[1] == 0 &&
      nextColor[2] == 0) {
    chooseNextColor(); // try again
  }
}

// *** STATES ***

void cycleToNextColor() {
  shiftColors();

  if (match()) {
    state = STATE_HOLD;
  }
}

void holdColor() {
  DigisparkRGBDelay(HOLD_COLOR_TIMEOUT);
  chooseNextColor();
  state = STATE_CYCLE;

  checkForBedtime();
}

void shuttingDown() {
  shiftColors();

  if (match()) {
    goToSleep();
  }
}

void checkForBedtime() {
  if (counter != 0) {
    // counter flipped
    state = STATE_BEDTIME;

    // fade to zero
    nextColor[0] = 0;
    nextColor[1] = 0;
    nextColor[2] = 0;
  }
}

void checkForWake() {
  if (counter != 0) {
    // We're asleep and counter flipped, wake up
    chooseNextColor();
    state = STATE_CYCLE;

    clearCounter();
  }
}

void goToSleep() {
  state = STATE_SLEEP;
  clearCounter();
}

void clearCounter() {
  // wait for things to mellow out
  DigisparkRGBDelay(WAIT_TIMEOUT);

  counter = 0;
}

// called when data appears on the USB pins
ISR(PCINT0_vect) {
  counter++;
}
