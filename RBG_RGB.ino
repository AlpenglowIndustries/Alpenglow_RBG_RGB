/*
Software that controls the RBG_RGB Badge
Makes her RBG LED eyes fade through the rainbow
Would like to add a mode for just red pulsing depending on a slide switch position

Pins:
1 - PB0 - Red LED (thru FET)
2 - GND
3 - PB1 - Blue LED (thru FET)
4 - PB2 - Green LED (thru FET)
5 - Vcc
6 - Reset

Programming the ATTiny4:
** NOTE!  The ATTiny4/5/9/10 use the TPI programming interface, which is different from
other ATTinys like the ATTiny85 which uses PDI, and also different from the ISP interface
used by the ATMega series and most Arduinos.  You will need a USBasp programmer or an
original genuine Atmel AVRISP mk II.  AVRdude does not support TPI on other programmers
(sorry Atmel ICE users, you're currently SOL).**

- Arduino IDE with ATTiny10 Core by technoblogy:
  https://github.com/technoblogy/attiny10core
  http://www.technoblogy.com/show?1YQY (excellent programming instructions and blink code)
- Add http://www.technoblogy.com/package_technoblogy_index.json to Boards Urls in preferences
- Install ATTiny10 core using Boards Manager
- Set "Board" to ATTiny10/9/5/4
- Set "Chip" to ATTiny4
- Power must be 5V
- A switch has been provided on the RBG_RGB board to isolate pins during programming.
  Make sure it has been switched from "RUN" mode to "PRG".
  If you are not using our dev board, note that Pin 1, TPIDATA, cannot be driving anything of
  consequence downstream during programming.  No direct driving LEDs with this pin.
- Must "Upload using programmer" - no room for a bootloader!  No serial port!
- Programmers supported:
  Genuine original Atmel AVRISP mkII using Libusb-win32 driver.  Use Zadig to change driver if
    you plug it in and get a USB device not found error (using the default Atmel driver).  Note
    that many of the new knock-offs of the AVRISP mk II use USBtiny firmware which does NOT
    support TPI at this time.  Note the AVRISP mkII uses MISO for TPIDATA.
  USBasp - firmware must be updated to 2011-05-28 version to support TPI.  Download from
    https://www.fischl.de/usbasp/   Firmware can be updated via Arduino IDE, choose the hex
    file for the processor on your USBasp (different people use different ones).  Use an Arduino
    as ISP or a different programmer (ex: USBtiny).  After the update, should program TPI.  You
    may need to update the driver to libusb-win32 for Windows 10.  Use Zadig.
    Note the USBasp uses MOSI for TPIDATA.

Defaults on reset:
- Clock = 1 MHz (8 MHz internal oscillator / 8)
- Timer Module enabled, normal port operation, OCR0A/B disabled, no clock source (timer stopped),
  clock = system clock no prescaling

*/

//#include <util/atomic.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define MAXBRITE  75
#define MINBRITE  5   // minimum PWM resolution is technically 3, but it hangs if lower

volatile uint32_t counter;

// PB0 = red
// PB1 = green
// PB2 = blue

// Variable count
// 4 counter
// 1 mode
// 1 ddrb
// 1 loop counter
// ===============
// 7 bytes RAM max

ISR(TIM0_COMPA_vect) {
  PINB |= (1 << PB2);   // toggles value of PB2, mirrors the OCR0A PB0 PWM (red)
}

ISR(TIM0_COMPB_vect) {
  PINB |= (1 << PB2);   // toggles value of PB2, mirrors the OCR0B PB1 PWM (green)
}

void delay(uint16_t time) {  // brute force semi-accurate delay routine that doesn't use a timer

  counter = 46 * time;
  do counter--; while (counter != 0);
}

void waitForBottom(void) {  // waits for timer to reach 0 to ensure starting/stopping in correct place
  TIFR0 |= (1 << TOV0);                 // ensures flag is clear to start with
  while ((TIFR0 & (1 << TOV0)) == 0);   // waits for bottom to ensure toggling begins at same place
}

void fade(void) {  // adjusts the PWM to fade the LED colors
  uint8_t i;
  for (i = 0; i < MAXBRITE - MINBRITE; i++){
    OCR0A = i + MINBRITE;      // OCR0A = Red (blue), fades to on
    OCR0B = MAXBRITE - i;      // OCROB = Green, fades to off
    delay(60);
  }
  waitForBottom();
}

void checkMode(void) {
  uint8_t mode = 0;
  uint8_t ddrb;
  ddrb = DDRB;
  DDRB &= ~(1 << DDB2);         // turns blue/PB2 to input
  delay(5);
  mode = PINB & (1 << PINB2);   // saves current value of PB2
  if (mode) {
    PORTB = 0;  // all outputs to zero
    DDRB &= ~(1 << DDB1);       // turns green off
    DDRB |= (1 << DDB0);        // turns red on
    uint8_t i = MINBRITE;
    int8_t change = 1;
    OCR0A = i;

    while (mode) {      // fades back and forth, checks mode
      i += change;
      OCR0A = i;      // red fades
      delay(30);
      if (i == MINBRITE) OCR0A = 0;
      if (i == MINBRITE || i == MAXBRITE) {
        change = -change;
        delay(60);
      }
      mode = PINB & (1 << PINB2);
    }

    PORTB &= (1 << PB0);  // turns red off
  }

  DDRB = ddrb;          // resets DDRB to original value
  waitForBottom();      // to make sure PWM toggling starts at known spot,
                        //   prevents accidental PWM inversion and irregular behavior
}


int main(void) {

  DDRB = (1 << DDB0 | 1 << DDB2);                     // PB0, PB2 outputs enabled (RED, GRN)
  sei();                                              // all interrupts enabled

  TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Phase-correct PWM 8-bit mode, A and B
  TCCR0B = (1 << CS01);                               // clk/8, timer started, 125 kHz timer clock
                                                      // and 245 Hz PWM frequency

  /*
  The General Gist:
  There are only 2 hardware PWM outputs on an ATTiny4, PB0 (OCR0A, RED) and PB1 (OCR0B, GRN).
  We need to generate a third PWM in order to fade the BLUE.
  Fortunately, only 2 have to be running at any one time to fade through the rainbow.
  One color is always off, while the other two fade from off to bright and vice versa.
  So we can mirror one of the PWM outputs by toggling PB2 in the interrupt routine that runs
    when the timer compare flag is set, which happens on each side of the pulse width when
    the timer is in phase correct mode.
  Even though the timer is running and its output is mirrored to PB2, we can turn that timer's
    hardwired LED off by changing its port/pin to an input.  Then no current drives the transistor gate,
    the transistor is OFF, and so is the LED attached to it.
  Pretty slick, eh?
  There are some additional commands to ensure we consistently turn the output mirroring on and off at
    the bottom of the timer, this ensures the mirrored PWM isn't inverted.
  All LEDs are driven through transistors to reduce the load on the processor.
    The LEDs are common-anode, hardwired to V+.  Switching GND toggles the LEDs on/off.  Therefore,
    the transistor-driven LEDs have "normal" logic (high means LED is ON).
  */

  while (1) {

    // RED = fades from ON to OFF
    // GRN = fades from OFF to ON
    // BLU = OFF


                                 // red is already on
    PORTB &= ~(1 << PB2);        // turns blue output to zero
    DDRB &= ~(1 << DDB2);        // turns blue output off
    checkMode();
    DDRB |= (1 << DDB1);         // turns green output on

    uint8_t j;
    for (j = 0; j < MAXBRITE - MINBRITE; j++){
      OCR0A = MAXBRITE - j;     // red (blue)    ON to OFF
      OCR0B = j + MINBRITE;     // green (blue)  OFF to ON
      delay(60);
    }


    // RED = OFF
    // GRN = fades from ON to OFF
    // BLU = fades from OFF to ON

    DDRB &= ~(1 << DDB0);       // turns red output off
    checkMode();
    PORTB |= (1 << PB2);        // PWM is high at bottom, sets blue to high for proper mirroring
    TIFR0 |= (1 << OCF0A);      // clears any remnant flag on A
    TIMSK0 |= (1 << OCIE0A);    // enables PWM mirroring for blue on A (red)
    DDRB |= (1 << DDB2);        // turns blue output on
    fade();                     // Red(blue) fades to on, green fades to off, waits for bottom (PWM is high)
    TIMSK0 &= ~(1 << OCIE0A);   // disables PWM mirroring on A (red)
    checkMode();                // do this while no PWMs are being mirrored
    TIFR0 |= (1 << OCF0B);      // clears any remnant flag on B
    TIMSK0 |= (1 << OCIE0B);    // enables PWM mirroring for blue on B (green)
                                // this happens fast, PWM is still high

    // // RED = fades from OFF to ON
    // // GRN = OFF
    // // BLU = fades from ON to OFF

    DDRB &= ~(1 << DDB1);       // turns green output off
    DDRB |= (1 << DDB0);        // turns red output on
    fade();                     // red fades to on, green(blue) fades to off, waits for bottom
    TIMSK0 &= ~(1 << OCIE0B);   // disables PWM mirroring on B (green)

  }   // end while(1)
}     // end main
