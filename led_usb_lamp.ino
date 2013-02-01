#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>     /* for _delay_ms() */
#include <usbdrv.h>
#include <DigisparkRGB.h>


#define STATE_CYCLE   0
#define STATE_HOLD    1
#define STATE_BEDTIME 2
#define STATE_SLEEP   3

uint8_t color[]     = {255, 255, 255};
uint8_t nextColor[] = {0, 0, 0};
uint8_t sample;
uint8_t i, j;
unsigned long currentMillis = 0;
unsigned long lastMillis    = 0;
unsigned long holdMillis    = 0;
unsigned long lastRefresh   = 0;

unsigned int counter = 0;

uint8_t state = STATE_CYCLE;

// update count from the USB host
extern volatile unsigned char usbSofCount = 0;
unsigned char lastSof = -1;


// the setup routine runs once when you press reset:
void setup()  { 
  // disable timer 0 overflow interrupt (used for millis)
  TIMSK&=!(1<<TOIE0);

  cli();

  usbInit();

  usbDeviceDisconnect();
  uchar   i;
  i = 0;
  while(--i){             /* fake USB disconnect for > 250 ms */
    _delay_ms(1);
  }
  usbDeviceConnect();

  sei();

  usbPoll();

  digitalWrite(0, HIGH);
  digitalWrite(1, HIGH);
  digitalWrite(2, HIGH);

  DigisparkRGBBegin();
} 


void loop () {
  // fake the millisecond counter
  counter++;
  if (counter % 100 == 0) {
    currentMillis++;
  }

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
  }

  if (currentMillis - lastMillis > 1000) {
    lastMillis = currentMillis;

    if (lastSof == usbSofCount) {
      state = STATE_BEDTIME;

      // fade to zero
      nextColor[0] = 0;
      nextColor[1] = 0;
      nextColor[2] = 0;
    } else if (state == STATE_SLEEP) {
      state = STATE_CYCLE;
    }

    lastSof = usbSofCount;
  }

  if (currentMillis - lastRefresh > 50) {
    lastRefresh = currentMillis;
    usbPoll();
  }
}

bool match() {
  return color[0] == nextColor[0] &&
         color[1] == nextColor[1] &&
         color[2] == nextColor[2];
}

void shiftColors() {
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
  for (i = 0; i < 3; i++) {
    nextColor[i] = random(255);
  }
}

// *** STATES ***

void cycleToNextColor() {
  if (match()) {
    state = STATE_HOLD;
  } else {
    if (currentMillis - holdMillis > 10) {
      holdMillis = currentMillis;
      shiftColors();
    }
  }
}

void holdColor() {
  if (currentMillis - holdMillis > 10000) { // 10 seconds
    holdMillis = currentMillis;
    chooseNextColor();
    state = STATE_CYCLE;
  }
}

void shuttingDown() {
  if (currentMillis - holdMillis > 50) {
    holdMillis = currentMillis;

    shiftColors();

    if (match()) {
      state = STATE_SLEEP;
    }
  }
}
