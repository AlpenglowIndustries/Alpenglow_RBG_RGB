/*
Software that controls the RBG_RGB Badge
Makes her RBG LED eyes fade through the rainbow
Would like to add a mode for just red pulsing depending on slide switch position

Programming the ATTiny4:
- Arduino IDE with ATTiny10 Core by technoblogy:
  https://github.com/technoblogy/attiny10core
  http://www.technoblogy.com/show?1YQY (excellent programming instructions and blink code)
- Set "Board" to ATTiny10/9/5/6
- Set "Chip" to ATTiny4
- Power must be 5V
- Pin 1, TPIDATA, cannot be driving anything of consequence downstream.
- A switch to isolate power and Pin 1 during programming is a good idea.
- Must "Upload using programmer" - no room for a bootloader!  No serial port!
- Programmers -
  AVRISP mkII using Libusb-win32 driver.  Use Zadig to change driver if
    you plug it in and get a USB device not found error (using the default Atmel driver).
  USBasp - haven't tried this yet.

Defaults on reset:
- Clock = 1 MHz (8 MHz internal oscillator / 8)
- Timer Module enabled, normal port operation, OCR0A/B disabled, no clock source (timer stopped)

Thoughts:
- need to enable atomic read/writes for timer?

*/
#include <util/atomic.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define MAXBRITE  75
#define MINBRITE  4   // under 4 and shit gets wiggy, interrupts seem to hang processor

volatile uint32_t counter;

// PB0 = red
// PB1 = blue
// PB2 = green

ISR(TIM0_COMPA_vect) {
  PINB |= (1 << PB2);   // toggles value of PB2, mirrors the OCR0A PB0 PWM (red)
}

ISR(TIM0_COMPB_vect) {
  PINB |= (1 << PB2);   // toggles value of PB2, mirrors the OCR0B PB1 PWM (blue)
}

void delay (uint16_t time) {  // brute force semi-accurate delay routine that doesn't use a timer
  counter = 46 * time;
  do counter--; while (counter != 0);
}

void waitForBottom (void) {  // waits for timer to reach 0 to ensure starting/stopping in correct place
  TIFR0 |= (1 << TOV0);            // clears flag
  while (!(TIFR0 & (1 << TOV0)));  // waits for bottom to ensure toggling begins at same place
}

void fade (void) {  // adjusts the PWM to fade the LED colors
  uint8_t j;
  for (j = 0; j < MAXBRITE - MINBRITE; j++){
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
      OCR0A = MAXBRITE - j;     // red (green)
      OCR0B = j + MINBRITE;     // blue (green)
    }
    delay(60);
  }
  waitForBottom();
}

int main (void) {

  DDRB = (1 << DDB0 | 1 << DDB1 | 1 << DDB2);         // PB0, 1, 2 outputs enabled
  sei();                                              // all interrupts enabled

  TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Phase-correct PWM 8-bit mode, A and B
  TCCR0B = (1 << CS01);                               // clk/8, timer started

//  uint8_t slideSw;
  /*
  The General Gist:
  There are only 2 hardware PWM outputs on an ATTiny4, PB0 (OCR0A, RED) and PB1 (OCR0B, BLUE).
  We need to generate a third PWM in order to fade the GREEN.
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
  Red and blue are driven through a transistor, green is driven directly from the processor pin.
    The LEDs are common-anode, hardwired to V+.  Switching GND toggles the LEDs on/off.  Therefore,
    the transistor-driven LEDs have "normal" logic (high means LED is ON), but the green one is inverted
    (high means GREEN is OFF).
  We purposefully invert the green PWM by setting the pin low before starting the interrupt toggling.
  */

  while (1) {

    // OCR0A = Red
    // OCROB = Blue

    // RED = fades from OFF to ON
    // BLU = fades from ON to OFF
    // GRN = OFF
                                 // blue is already on
    PORTB &= ~(1 << PB2);        // turns green off (direct drive)
    DDRB &= ~(1 << DDB2);        // turns green output off
    DDRB |= (1 << DDB0);         // turns red output on
    uint8_t i;
    for (i = 0; i < MAXBRITE - MINBRITE; i++){
      ATOMIC_BLOCK(ATOMIC_FORCEON) {
        OCR0A = i + MINBRITE;      // red fades to on
        OCR0B = MAXBRITE - i;      // blue fades to off
      }
      delay(60);
    }
//    slideSw = PINB & (1 << PB2);


    // RED = fades from ON to OFF
    // BLU = OFF
    // GRN = fades from OFF to ON
    DDRB &= ~(1 << DDB1);       // turns blue output off (transistor drive)
    waitForBottom();            // to make sure PWM toggling starts at known spot,
                                //   prevents accidental PWM inversion and irregular behavior
    TIMSK0 |= (1 << OCIE0B);    // enables PWM mirroring for green on B (blue)
    DDRB |= (1 << DDB2);        // turns green output on
    fade();                     // fades red to off, green to on, waits for bottom
    TIMSK0 &= ~(1 << OCIE0B);   // disables PWM mirroring on B (blue)
    waitForBottom();
    PORTB &= ~(1 << PB2);       // starts green output from known low state (OFF), this inverts it
    TIMSK0 |= (1 << OCIE0A);    // enables PWM mirroring for green on A (red)

    // // RED = OFF
    // // BLU = fades from OFF to ON
    // // GRN = fades from ON to OFF
    DDRB &= ~(1 << DDB0);       // turns red output off (transistor drive)
    DDRB |= (1 << DDB1);        // turns blue output on
    fade();                     // fades green to off, blue to on, waits for bottom
    TIMSK0 &= ~(1 << OCIE0A);   // disables PWM mirroring on A (red)

  }   // end while(1)
}     // end main
