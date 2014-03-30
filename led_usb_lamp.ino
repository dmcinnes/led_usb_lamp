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

uint8_t state = STATE_CYCLE;

unsigned char color[]     = {255, 255, 255};
unsigned char nextColor[] = {0, 0, 0};

unsigned int counter;
unsigned int previousCounter;


// the setup routine runs once when you press reset:
void setup() {
  counter         = 1;
  previousCounter = 0;

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

  checkSleep();
}

void shuttingDown() {
  shiftColors();

  if (match()) {
    state = STATE_SLEEP;
  }
}

void checkSleep() {
  if (counter == previousCounter) {
    // counter's not updating, go to sleep
    state = STATE_BEDTIME;

    // fade to zero
    nextColor[0] = 0;
    nextColor[1] = 0;
    nextColor[2] = 0;
  }

  previousCounter = counter;
}

void checkForWake() {
  if (counter != previousCounter) {
    // We're asleep and counter is now updating, wake up
    chooseNextColor();
    state = STATE_CYCLE;

    // wait 5 seconds for things to mellow out
    DigisparkRGBDelay(5000);

    // --- Reboot the Digispark so it can get in a good state again
    reboot();
  }
}

void reboot(void) {
  noInterrupts(); // disable interrupts which could mess with changing prescaler
  CLKPR = 0b10000000; // enable prescaler speed change
  CLKPR = 0; // set prescaler to default (16mhz) mode required by bootloader
  void (*ptrToFunction)(); // allocate a function pointer
  ptrToFunction = 0x0000; // set function pointer to bootloader reset vector
  (*ptrToFunction)(); // jump to reset, which bounces in to bootloader
}

// called when data appears on the USB pins
ISR(PCINT0_vect) {
  counter++;
}
