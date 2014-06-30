#ifndef _ARDUOUS_H_
#define _ARDUOUS_H_


/* Global constants */
#ifndef MAXTHREADS
#define MAXTHREADS 10           /* Max number of threads supported */
#endif
#ifndef THREADMAXSTACKSIZE
#define THREADMAXSTACKSIZE 200  /* Max stacksize for a thread */
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
int ardk_start(void);                           /* Start the Arduous kernel */
int ardk_create_thread(void (*runner)(void));   /* Create a new thread */
static int ardk_switch_thread();                /* Performs context switch */

#endif /* _ARDUOUS_H_ */