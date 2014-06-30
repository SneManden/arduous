#ifndef _ARDUOUS_H_
#define _ARDUOUS_H_

// #ifdef __cplusplus
// extern "C" {
// #endif


#include <avr/interrupt.h>
#include <avr/io.h>
#include "Arduino.h"
#include "assembly.h"


/* Global constants */
#ifndef MAXTHREADS
#define MAXTHREADS 10           /* Max number of threads supported */
#endif
#ifndef THREADMAXSTACKSIZE
#define THREADMAXSTACKSIZE 100  /* Max stacksize for a thread */
#endif
#ifndef TIMERPRESET 
#define TIMERPRESET 131
#endif


/* Useful macros */
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
/* Return lower/higher 8 bits of 16 bit value */
#define lower8(X)  ((unsigned char)((unsigned int)(X)))
#define higher8(X) ((unsigned char)((unsigned int)(X) >> 8))


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
static void ardk_switch_thread(void) __attribute__ ((naked));           /* Performs a context switch */
/* Add an element in the back of the queue */
static void ardk_enqueue(struct ardk_thread *thread);
/* Return an element from the front of the queue */
static struct ardk_thread *ardk_dequeue(struct ardk_thread *thread);


// #ifdef __cplusplus
// }
// #endif

#endif /* _ARDUOUS_H_ */
