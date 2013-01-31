#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>     /* for _delay_ms() */

#include <oddebug.h>
#include "usbconfig.h"
#include <usbdrv.h>
#include <usbportability.h>

//#include <DigisparkRGB.h>

//#include <DigiUSB.h>

//PROGMEM const char usbHidReportDescriptor[22] = { /* USB report descriptor */
//    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
//    0x09, 0x01,                    // USAGE (Vendor Usage 1)
//    0xa1, 0x01,                    // COLLECTION (Application)
//    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
//    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
//    0x75, 0x08,                    //   REPORT_SIZE (8)
//    0x95, 0x01,                    //   REPORT_COUNT (1)
//    0x09, 0x00,                    //   USAGE (Undefined)
//    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
//    0xc0                           // END_COLLECTION
//};

#define BODS 7                   //BOD Sleep bit in MCUCR
#define BODSE 2                  //BOD Sleep enable bit in MCUCR

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


uint8_t mcucr1, mcucr2;

extern volatile unsigned char usbSofCount = 0;
unsigned char lastSof = -1;


void goToSleep(void) {
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);

  /*digitalWrite(3, HIGH); // set pin 3 high*/

  /* GIMSK |= _BV(INT0);                       //enable INT0 */
  GIMSK |= _BV(PCIE);                       // enable Pin Change Interrupt Enable
  PCMSK |= _BV(PCINT4);                     // only enable pin 4

  MCUCR &= ~(_BV(ISC01) | _BV(ISC00));      //INT0 on low level
  ACSR |= _BV(ACD);                         //disable the analog comparator
  ADCSRA &= ~_BV(ADEN);                     //disable ADC
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  //turn off the brown-out detector.
  //must have an ATtiny45 or ATtiny85 rev C or later for software to be able to disable the BOD.
  //current while sleeping will be <0.5uA if BOD is disabled, <25uA if not.
  cli();
  mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
  mcucr2 = mcucr1 & ~_BV(BODSE);
  MCUCR = mcucr1;
  MCUCR = mcucr2;
  sei();                         //ensure interrupts enabled so we can wake up again
  sleep_cpu();                   //go to sleep
  cli();                         //wake up here, disable interrupts
  GIMSK = 0x00;                  //disable INT0
  sleep_disable();               
  sei();                         //enable interrupts again (but INT0 is disabled from above)

  /*digitalWrite(3, LOW); // set pin 3 low again*/
}


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

  digitalWrite(0, HIGH);
  digitalWrite(1, HIGH);
  digitalWrite(2, HIGH);

  usbPoll();
} 

unsigned int i = 0;
unsigned int j = 0;

unsigned long currentMillis = 0;
unsigned long lastMillis = 0;
unsigned long lastRefresh = 0;

void loop () {
  currentMillis++;

  if (currentMillis - lastMillis > 500000) {
    lastMillis = currentMillis;

    i++;

    for (j = 0; j < 3; j++) {
      digitalWrite(j, i%3 == j ? HIGH : LOW);
    }

    if (lastSof == usbSofCount) {
      lastSof = -1;
      usbDeviceDisconnect();
      goToSleep();
    }

    lastSof = usbSofCount;
  }

  if (currentMillis - lastRefresh > 10000) {
    lastRefresh = currentMillis;
    usbPoll();
  }
}

//void fade(byte Led, boolean dir) {
//  int i;
//
//  if (dir) {
//    for (i = 0; i < 256; i++) {
//      DigisparkRGB(Led, i);
//      DigisparkRGBDelay(25);
//    }
//  }
//  else {
//    for (i = 255; i >= 0; i--) {
//      DigisparkRGB(Led, i);
//      DigisparkRGBDelay(25);
//    }
//  }
//}
//
//
//
