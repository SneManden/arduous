#ifndef _ARDUOUS_H_
#define _ARDUOUS_H_

#include <avr/interrupt.h>
#include <avr/io.h>
#include "Arduino.h"
#include "assembly.h"


/* Global constants */
#ifndef MAXTHREADS
#define MAXTHREADS 5            /* Max number of threads supported */
#endif
#ifndef THREADMAXSTACKSIZE
#define THREADMAXSTACKSIZE 150  /* Max stacksize for a thread */
#endif
/* We need to calculate a proper value to load the timer counter.
 * The following loads the value 131 into the Timer 2 counter register
 * The math behind this is:
 * (CPU frequency = 16E6) / (prescaler value = 128) = 125000 Hz = 8 us.
 * (desired period = 1000 us) / 8 us = 125.
 * MAX(uint8) + 1 - 125 = 131;
 */
#ifndef TIMERPRESET
#define TIMERPRESET 131
#endif


/**
 * MACROS
 */
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
/* Return lower/higher 8 bits of 16 bit value */
#define lower8(X)  ((unsigned char)((unsigned int)(X)))
#define upper8(X) ((unsigned char)((unsigned int)(X) >> 8))
/* Performs a context switch by setting stack pointer among threads */
#define ardk_context_switch() do { \
    if (current_thread != NULL && thread_queue != NULL && \
        current_thread != thread_queue) { \
        current_thread->sp_low = SPL; \
        current_thread->sp_high = SPH; \
        current_thread = thread_queue; \
        thread_queue = thread_queue->next; \
        SPL = current_thread->sp_low; \
        SPH = current_thread->sp_high; \
    } } while(0)
#if defined (__AVR_ATmega2560__) || defined (__AVR_ATmega1280__)
#define SERIAL Serial1
#else
#define SERIAL Serial
#endif


/**
 * Data structures (ardk_thread == [ard]uous [k]ernel thread)
 */
struct ardk_thread {                    /* A kernel thread structure */
    int thread_id;                      /* Unique identifier */
    struct ardk_thread *next;           /* Pointer to next thread in queue */
    struct ardk_thread *prev;           /* Pointer to previous thread */
    volatile char sp_low, sp_high;      /* Lower/higher 8 bit of StackPointer */
};


/**
 * Method declarations
 */
/* Starts the Arduous kernel */
int ardk_start(int ts);
/* Creates a new thread */
int ardk_create_thread(void (*runner)(void));
/* Switches between threads */
void ardk_switch_thread(void) __attribute__ ((naked));

/* Add an element in the back of the queue */
void ardk_enqueue(struct ardk_thread *thread);
/* Return an element from the front of the queue */
struct ardk_thread *ardk_dequeue(struct ardk_thread *thread);
/* Prints the thread queue */
void ardk_print_queue(void);
/* Prints the thread stack */
void ardk_print_stack(char *stack, int bytes);


#endif /* _ARDUOUS_H_ */