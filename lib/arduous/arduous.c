#include "arduous.h"

/* Pointer to head of thread queue (NULL if empty) */
static struct ardk_thread *thread_queue = NULL;
/* Pointer to currently running thread (NULL if none) */
static struct ardk_thread *current_thread = NULL;
/* Timeslice */
static int time_slice; /* Number of msecs given to each thread */
static int time_count;


/**
 * Insert *thread into back of queue by modifying pointers of the head and tail
 * of the queue, and the pointers of *thread itself.
 * @param head   Head of the queue
 * @param thread The thread to insert into the queue
 */
static void ardk_enqueue(struct ardk_thread *thread) {
    struct ardk_thread *head = thread_queue;
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
    if (thread->prev == thread->next) /* If 1 element in queue => now empty */
        thread_queue = NULL;
    thread->prev->next = thread->next;
    thread->next->prev = thread->prev;
    return thread;
}

/**
 * Creates a new thread by allocating a new stack, clear it, save SP and
 * add it to the queue of threads
 * @param  runner Pointer to function where the thread resides
 * @return        0 on success; -1 on error
 */
int ardk_create_thread(void (*runner)(void)) {
    int i;
    char *stack;
    struct ardk_thread *new_thread;

    if (current_thread) return -1; /* Not allowed to create while running */

    new_thread = malloc( sizeof(struct ardk_thread) );
    if (new_thread == NULL) return -1;

    stack = malloc(THREADMAXSTACKSIZE);
    if (stack == NULL) return -1;

    *(stack--) = 0x00;              /* Safety distance */
    *(stack--) = lower8(runner);
    *(stack--) = higher8(runner);
    /* Mega 2560 use 3 byte for call/ret addresses the rest only 2 */
    #if defined (__AVR_ATmega2560__)
        *(stack--) = EIND;
    #endif
    *(stack--) = 0x00; /* R1 */
    *(stack--) = 0x00; /* R0 */
    *(stack--) = 0x00; /* SREG */
    /* Mega 1280 and 2560 need to save rampz reg just in case */
    #if defined (__AVR_ATmega2560__) || defined (__AVR_ATmega1280__)
        *(stack--) = RAMPZ;
        *(stack--) = EIND;
    #endif
    for (i=0; i<30; i++)    /* R2-R31 = 30 registers */
        *(stack--) = 0x00;

    /* *stack is now the stack pointer. Add the thread to the queue */
    new_thread->sp_low = lower8(stack);
    new_thread->sp_high = higher8(stack);
    ardk_enqueue(new_thread);

    return 0;
}

/**
 * Initializes the kernel
 * Timer interrupt regards:
 *     http://popdevelop.com/2010/04/mastering-timer-interrupts-on-the-arduino/
 * @return  0 on success; -1 on error
 */
int ardk_start(int ts) {
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
    TCNT2 = TIMERPRESET;
    TIMSK2 |= (1<<TOIE2);

    /* We need to calculate a proper value to load the timer counter.
     * The following loads the value 131 into the Timer 2 counter register
     * The math behind this is:
     * (CPU frequency = 16E6) / (prescaler value = 128) = 125000 Hz = 8 us.
     * (desired period = 1000 us) / 8 us = 125.
     * MAX(uint8) + 1 - 125 = 131;
    */
   
   time_slice = ts;

    return 0;
}

/**
 * Performs a context switch and invokes a new thread to execute
 */
static void ardk_switch_thread(void) {
    if (current_thread == thread_queue)
        return 0; /* If current thread is next => skip context switch */

    /* Save registers on stack */
    PUSHREGISTERS();

    /* Save stack pointer */
    current_thread->sp_low = SPL;
    current_thread->sp_high = SPH;
    /* Issue next thread and put in the back of the queue */
    current_thread = thread_queue;
    ardk_enqueue(ardk_dequeue(current_thread));
    /* Restore stack pointer */
    SPL = current_thread->sp_low;
    SPH = current_thread->sp_high;

    /* Restore registers from stack */
    POPREGISTERS();

    /* (Enable interrupt and) return to address at stack pointer */
    RET();
}

/**
 * Some helpful comment here
 */
ISR(TIMER2_OVF_vect, ISR_NAKED) {
    /*ISR run with 1 kHz*/
    TCNT2  = TIMERPRESET;

    if (time_count == time_slice) {
        time_count = 0;
        DISABLE_INTERRUPTS();
        ardk_switch_thread();
        ENABLE_INTERRUPTS();
    } else
        time_count++;
}
