// Test Program for ATTiny4

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define MAXBRITE  75
#define MINBRITE  4   // under 4 and shit gets wiggy

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

void fade (void) {
  uint8_t j;
  for (j = MINBRITE; j < MAXBRITE; j++){
    OCR0A = MAXBRITE-j;     // red (green)
    OCR0B = j;              // blue (green)
    delay(50);
  }
}

int main (void) {

  DDRB = (1 << DDB0 | 1 << DDB1 | 1 << DDB2);  // PB0, 1, 2 outputs enabled
  sei();

  TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Phase-correct PWM 8-bit mode, A and B
  TCCR0B = (1 << CS01);  // clk/8, timer started

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
  We purposefully invert the green PWM by setting the pin high or low before starting the interrupt toggling.
  */

  while (1) {

    // RED = fades from OFF to ON
    // BLU = fades from ON to OFF
    // GRN = OFF
    PORTB |= (1 << PB2);        // turns green off (direct drive)
    DDRB &= ~(1 << DDB2);       // turns green output off
    DDRB |= (1 << DDB0);        // turns red output on
    uint8_t i;
    for (i = MINBRITE; i < MAXBRITE; i++){
      OCR0A = i;                // red
      OCR0B = MAXBRITE-i;       // blue
      delay(50);
    }

    // RED = fades from ON to OFF
    // BLU = OFF
    // GRN = fades from OFF to ON
    DDRB &= ~(1 << DDB1);       // turns blue output off (transistor drive)
    TIFR0 |= (1 << TOV0);       // clears flag
    while (!(TIFR0 & (1 << TOV0)));  // waits for bottom to ensure toggling begins at same place
    PORTB &= ~(1 << PB2);       // starts green output from known low state (ON)
    TIMSK0 |= (1 << OCIE0B);    // enables PWM mirroring for green on B (blue)
    DDRB |= (1 << DDB2);        // turns green output on
    fade();                     // fades red to off, green to on
    TIFR0 |= (1 << TOV0);       // clears flag
    while (!(TIFR0 & (1 << TOV0)));  // waits for bottom to ensure toggling ends/begins at same place
    TIMSK0 &= ~(1 << OCIE0B);   // disables PWM mirroring on B (blue)
    TIMSK0 |= (1 << OCIE0A);    // enables PWM mirroring on A (red)

    // // RED = OFF
    // // BLU = fades from OFF to ON
    // // GRN = fades from ON to OFF
    DDRB &= ~(1 << DDB0);       // turns red output off (transistor drive)
    DDRB |= (1 << DDB1);        // turns blue output on
    fade();                     // fades green to off, blue to on
    TIFR0 |= (1 << TOV0);       // clears flag
    while (!(TIFR0 & (1 << TOV0)));  // waits for bottom to ensure toggling ends at same place
    TIMSK0 &= ~(1 << OCIE0A);   // disables PWM mirroring on A (red)

  }   // end while(1)
}     // end main
