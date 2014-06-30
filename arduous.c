#include "arduous.h"

int ardk_start(void) {
    /*Configure TIMER2 in normal mode (pure counting, no PWM)*/
    TCCR2A = 0x00;

    /*Set prescaler to 1024*/
    TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);

    /*Disable Compare Match A interrupt enable*/
    TIMSK2 &= ~(1 << OCIE2A);

    /*Overflow interrupt enable*/
    TIMSK2 = 0x01;

    /*
     * f_CPU / prescaler = 16E6 / 1024 = 15625 Hz ~= 64 us
     * 100 Hz => 10 ms = 10000 us
     * 10000 us / 64 us = 156,25
     * Hvad skal vi da initialiserer timer reg. med for at få 100 Hz?
     * => 8-bit reg. minus antal counts på 10 ms
     * 2^8 - 156,25 ~= 100
     * Så hvis timer reg. sættes til 100, så kører timer med freq på 100 Hz
     * Tilsvarende; 1 ms = 15,625 counts
     * 2^8 - 15,625 ~= 240
     * Så hvis timer reg. sættes til 240, så kører timer med freq på 1 kHz
     */

    TCNT2 = 240;
}

ISR (TIMER2_OVF_vect, ISR_NAKED) {
    /*Nu burde ISR gerne køre med frekvens på 1 kHz*/
}
