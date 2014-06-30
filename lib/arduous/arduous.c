#include "arduous.h"


/**
 * Insert *thread into back of queue by modifying pointers of the head and tail
 * of the queue, and the pointers of *thread itself.
 * @param head   Head of the queue
 * @param thread The thread to insert into the queue
 */
static void ardk_enqueue(struct ardk_thread *head, struct ardk_thread *thread) {
    struct ardk_thread *tail = head->prev;
    head->prev = thread;
    tail->next = thread;
    thread->prev = tail;
    thread->next = head;
}

/**
 * Removes an element from the queue by updating pointers. Returns the element
 * @param  thread The thread to remove from the queue
 * @return        Pointer to the element
 */
static struct ardk_thread *ardk_dequeue(struct ardk_thread *thread) {
    thread->prev->next = thread->next;
    thread->next->prev = thread->prev;
    return thread;
}

int ardk_start(void) {

    /*
    Timer interrupt regards:
    http://popdevelop.com/2010/04/mastering-timer-interrupts-on-the-arduino/
    */

    /* First disable the timer overflow interrupt while we're configuring */
    TIMSK2 &= ~(1<<TOIE2);

    /* Configure timer2 in normal mode (pure counting, no PWM etc.) */
    TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
    TCCR2B &= ~(1<<WGM22);

    /* Select clock source: internal I/O clock */
    ASSR &= ~(1<<AS2);

    /* Disable Compare Match A interrupt enable (only want overflow) */
    TIMSK2 &= ~(1<<OCIE2A);

    /* Now configure the prescaler to CPU clock divided by 128 */
    TCCR2B |= (1<<CS22)  | (1<<CS20); // Set bits
    TCCR2B &= ~(1<<CS21);             // Clear bit

    /* Finally load end enable the timer */
    TCNT2 = 131;
    TIMSK2 |= (1<<TOIE2);

    /* We need to calculate a proper value to load the timer counter.
     * The following loads the value 131 into the Timer 2 counter register
     * The math behind this is:
     * (CPU frequency = 16E6) / (prescaler value = 128) = 125000 Hz = 8 us.
     * (desired period = 1000 us) / 8 us = 125.
     * MAX(uint8) + 1 - 125 = 131;
    */

    return 0;
}

ISR (TIMER2_OVF_vect, ISR_NAKED) {
    /*ISR run with 1 kHz*/
    TCNT2  = 131;
}
