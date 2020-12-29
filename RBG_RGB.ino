// Test Program for ATTiny4

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define MAXBRITE  75

volatile uint32_t counter;

// PB0 = red
// PB1 = blue
// PB2 = green

ISR(TIM0_COMPA_vect) {
  PINB |= (1 << PB2);   // toggles value of PB2, mirrors the OCR0A PB0 PWM
}

ISR(TIM0_COMPB_vect) {
  PINB |= (1 << PB2);
}

void delay (uint16_t time) {
  counter = 46 * time;
  do counter--; while (counter != 0);
}

void cycle (void) {
  uint8_t i;
  for (i = 4; i < MAXBRITE; i++){
    OCR0A = MAXBRITE-i;     // red
    OCR0B = i;              // blue
    delay(50);
  }
}

int main (void) {

  DDRB = (1 << DDB0 | 1 << DDB1 | 1 << DDB2);  // PB0, 1, 2 as outputs
  sei();

  TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Phase-correct PWM 8-bit mode, A and B
  TCCR0B = (1 << CS01);  // clk/8, timer started

  while (1) {

    PORTB |= (1 << PB2);  // turns green off (direct drive)
    uint8_t j;
    for (j = 4; j < MAXBRITE; j++){
      OCR0A = j;
      OCR0B = MAXBRITE-j;
      delay(50);
    }

    DDRB &= ~(1 << DDB1);       // turns blue off
    while (!(TIFR0 &= (1 << TOV0)));
    PORTB &= ~(1 << PB2);
    TIMSK0 |= (1 << OCIE0B);    // enables output compare B match interrupt for green PWM mirroring on red
    TIFR0 |= (1 << TOV0);
    cycle();
    while (!(TIFR0 &= (1 << TOV0)));
    TIMSK0 &= ~(1 << OCIE0B);   // disables PWM mirroring
    TIFR0 |= (1 << TOV0);
//    OCR0B = 4;                  // sets blue to low
    DDRB |= (1 << DDB1);        // turns blue output on
    PORTB &= ~(1 << PB2);

    DDRB &= ~(1 << DDB0);       // turns red off
    while (!(TIFR0 &= (1 << TOV0)));
    TIMSK0 |= (1 << OCIE0A);    // enables output compare A match interrupt for green PWM mirroring on blue
    TIFR0 |= (1 << TOV0);
    cycle();
    while (!(TIFR0 &= (1 << TOV0)));
    TIMSK0 &= ~(1 << OCIE0A);   // disables PWM mirroring
    TIFR0 |= (1 << TOV0);
//    OCR0A = 4;                  // sets red to low
    DDRB |= (1 << DDB0);        // turns red output on

  }   // end while(1)
}     // end main
