// Test Program for ATTiny4

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define MAXBRITE  75

volatile uint32_t counter;

ISR(TIM0_COMPA_vect) {
  PINB |= (1 << PB2);
}

void delay (uint16_t time) {
  counter = 46 * time;
  do counter--; while (counter != 0);
}

int main (void) {
  DDRB = (1 << DDB0 | 1 << DDB1 | 1 << DDB2);  // PB0, 1, 2 as outputs

  TIMSK0 |= (1 << OCIE0A);  // enables output compare A match interrupt for PWM mirroring
  sei();

  OCR0A = 4;
  OCR0B = 255;
  PORTB |= (1 << PB2);
  TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Phase-correct PWM 8-bit mode, A and B
  TCCR0B = (1 << CS01);  // clk/8, timer started

  while (1) {

    uint8_t i;
    for (i = 4; i < MAXBRITE; i++){
      OCR0A = i;
      OCR0B = MAXBRITE-i;
      delay(50);
    }

    OCR0A = MAXBRITE;
    OCR0B = 4;
    delay(50);


    for (i = MAXBRITE; i > 4; i--){
      OCR0A = i;
      OCR0B = MAXBRITE-i;
      delay(50);
    }

    OCR0A = 4;
    OCR0B = MAXBRITE;
    delay(50);

  }
}
