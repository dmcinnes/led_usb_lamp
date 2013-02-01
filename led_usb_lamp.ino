#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>     /* for _delay_ms() */
#include <usbdrv.h>

#include <avr/io.h>


#define set(x) |= (1<<x)
#define clr(x) &=~(1<<x)
#define inv(x) ^=(1<<x)

#define BLUE_CLEAR (pinlevelB &= ~(1 << BLUE)) // map BLUE to PB2
#define GREEN_CLEAR (pinlevelB &= ~(1 << GREEN)) // map BLUE to PB2
#define RED_CLEAR (pinlevelB &= ~(1 << RED)) // map BLUE to PB2
#define PORTB_MASK  (1 << PB0) | (1 << PB1) | (1 << PB2)
#define BLUE PB2
#define GREEN PB1
#define RED PB0


#define STATE_CYCLE   0
#define STATE_HOLD    1
#define STATE_BEDTIME 2
#define STATE_SLEEP   3


unsigned char DigisparkPWMcompare[3];
volatile unsigned char DigisparkPWMcompbuff[3];

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
  DigisparkRGBBegin();

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

  chooseNextColor();
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
      // wake up
      chooseNextColor();
      state = STATE_CYCLE;
    }

    lastSof = usbSofCount;
  }

  if (currentMillis - lastRefresh > 50) {
    lastRefresh = currentMillis;
    usbPoll();
  }

  if (state != STATE_SLEEP) {
    DigisparkRGBUpdate();
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
  if (currentMillis - holdMillis > 2000) { // 2 seconds
    holdMillis = currentMillis;
    chooseNextColor();
    state = STATE_CYCLE;
  }
}

void shuttingDown() {
  if (currentMillis - holdMillis > 30) {
    holdMillis = currentMillis;

    shiftColors();

    if (match()) {
      state = STATE_SLEEP;
    }
  }
}

// Digispark RGB code

void DigisparkRGBBegin() {

  pinMode(2, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(0, OUTPUT);
  //CLKPR = (1 << CLKPCE);        // enable clock prescaler update
  //CLKPR = 0;                    // set clock to maximum (= crystal)


  DigisparkPWMcompare[0] = 0x00;// set default PWM values
  DigisparkPWMcompare[1] = 0x00;// set default PWM values
  DigisparkPWMcompare[2] = 0x00;// set default PWM values
  DigisparkPWMcompbuff[0] = 0x00;// set default PWM values
  DigisparkPWMcompbuff[1] = 0x00;// set default PWM values
  DigisparkPWMcompbuff[2] = 0x00;// set default PWM values


  TIFR = (1 << TOV0);           // clear interrupt flag
  TIMSK = (1 << TOIE0);         // enable overflow interrupt
  TCCR0B = (1 << CS00);         // start timer, no prescale

  sei();
}

void DigisparkRGB(int pin,int value){
	DigisparkPWMcompbuff[pin] = value;
}

void DigisparkRGBDelay(int ms) {
  while (ms) {
    _delay_ms(1);
    ms--;
  }
}

void DigisparkRGBUpdate() {
  static unsigned char pinlevelB=PORTB_MASK;
  static unsigned char softcount=0xFF;

  PORTB = pinlevelB;            // update outputs

  if(++softcount == 0){         // increment modulo 256 counter and update
                                // the compare values only when counter = 0.
    DigisparkPWMcompare[0] = DigisparkPWMcompbuff[0]; // verbose code for speed
    DigisparkPWMcompare[1] = DigisparkPWMcompbuff[1]; // verbose code for speed
    DigisparkPWMcompare[2] = DigisparkPWMcompbuff[2]; // verbose code for speed

    pinlevelB = PORTB_MASK;     // set all port pins high
  }
  // clear port pin on compare match (executed on next interrupt)
  if(DigisparkPWMcompare[0] == softcount) RED_CLEAR;
  if(DigisparkPWMcompare[1] == softcount) GREEN_CLEAR;
  if(DigisparkPWMcompare[2] == softcount) BLUE_CLEAR;
}
