#ifndef _ARDUOUS_H_
#define _ARDUOUS_H_

// #ifdef __cplusplus
// extern "C" {
// #endif


#include <avr/interrupt.h>
#include <avr/io.h>
// #include <stdio.h>
#include "Arduino.h"
#include "assembly.h"


/* Global constants */
#ifndef MAXTHREADS
#define MAXTHREADS 5            /* Max number of threads supported */
#endif
#ifndef THREADMAXSTACKSIZE
#define THREADMAXSTACKSIZE 200  /* Max stacksize for a thread */
#endif
#ifndef TIMERPRESET
#define TIMERPRESET 131
#endif


/* Useful macros */
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
/* Return lower/higher 8 bits of 16 bit value */
#define lower8(X)  ((unsigned char)((unsigned int)(X)))
#define upper8(X) ((unsigned char)((unsigned int)(X) >> 8))
#define ardk_context_switch() do { \
    if (current_thread != NULL && thread_queue != NULL) { \
        current_thread->sp_low = SPL; \
        current_thread->sp_high = SPH; \
        current_thread = thread_queue; \
        thread_queue = thread_queue->next; \
        SPL = current_thread->sp_low; \
        SPH = current_thread->sp_high; \
    } else { Serial.println("NULL ex"); }} while(0)


/* Data structures (ardk_thread == [ard]uous [k]ernel thread) */
struct ardk_thread {                    /* A kernel thread structure */
    int thread_id;                      /* Unique identifier */
    struct ardk_thread *next;           /* Pointer to next thread in queue */
    struct ardk_thread *prev;           /* Pointer to previous thread */
    volatile char sp_low, sp_high;      /* Lower/higher 8 bit of StackPointer */
};


/* Method declarations */
int ardk_start(int ts);                  /* Starts the Arduous kernel */
int ardk_create_thread(void (*runner)(void));   /* Creates a new thread */
void ardk_print_queue(void);
/* Performs a context switch */
void ardk_switch_thread(void) __attribute__ ((naked));
/* Add an element in the back of the queue */
void ardk_enqueue(struct ardk_thread *thread);
/* Return an element from the front of the queue */
struct ardk_thread *ardk_dequeue(struct ardk_thread *thread);

void start_threading(void) __attribute__ ((naked));


// #ifdef __cplusplus
// }
// #endif

#endif /* _ARDUOUS_H_ */
