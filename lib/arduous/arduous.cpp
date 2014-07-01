#include "arduous.h"

/* Stack pool */
static char stack_pool[MAXTHREADS][THREADMAXSTACKSIZE];
/* Thread pool */
static struct ardk_thread thread_pool[MAXTHREADS];
/* Pointer to head of thread queue (NULL if empty) */
static struct ardk_thread *thread_queue = NULL;
/* Pointer to currently running thread (NULL if none) */
static struct ardk_thread *current_thread = NULL;
static struct ardk_thread dummy;
/* Timeslice */
static int time_slice; /* Number of msecs given to each thread */
static int time_count;
static int thread_count = 0;


void ardk_print_queue(void) {
    Serial.println("Queue:");
    struct ardk_thread *iter = thread_queue;
    do {
        Serial.print("  thread ");
        Serial.println(iter->thread_id);
        iter = iter->next;
    } while (iter != thread_queue);
    Serial.println("----------");
}


void ardk_print_stack(char *stack, int bytes) {
    Serial.print("Stack: (starting at ");
    Serial.print( upper8(stack) , HEX);
    Serial.print( lower8(stack) , HEX);
    Serial.println(")");
    int i;
    for (i=0; i<bytes; i++) {
        Serial.print("  ");
        Serial.print(upper8(stack+i), HEX);
        Serial.print(lower8(stack+i), HEX);
        Serial.print(": ");
        Serial.println( *(stack+i), HEX );
    }
}


/**
 * Insert *thread into back of queue by modifying pointers of the head and tail
 * of the queue, and the pointers of *thread itself.
 * @param head   Head of the queue
 * @param thread The thread to insert into the queue
 */
void ardk_enqueue(struct ardk_thread *thread) {
    if (thread_queue == NULL) {
        thread_queue = thread;
        thread->next = thread;
        thread->prev = thread;
    } else {
        struct ardk_thread *head = thread_queue;
        struct ardk_thread *tail = head->prev;
        head->prev = thread;
        tail->next = thread;
        thread->prev = tail;
        thread->next = head;
    }
}


/**
 * Removes an element from the queue by updating pointers. Returns the element
 * @param  thread The thread to remove from the queue
 * @return        Pointer to the element
 */
struct ardk_thread *ardk_dequeue(struct ardk_thread *thread) {
    if (thread_queue->prev == thread_queue->next) /* If 1 element in queue => now empty */
        thread_queue = NULL;
    else if (thread == thread_queue)
        thread_queue = thread->next;
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

    // Serial.println("ardk_create_thread():");

    /* Get a new thread from the thread pool */
    new_thread = &thread_pool[thread_count];
    new_thread->thread_id = thread_count++;

    /* stack is starting address of stack, from the stack pool */
    stack = (char*) &stack_pool[new_thread->thread_id];

    // Serial.print(" => got stack at address: ");
    // Serial.print( upper8(stack) , HEX);
    // Serial.println( lower8(stack) , HEX);
    // Serial.print(" => top of stack at address: ");
    // Serial.print( upper8(stack + THREADMAXSTACKSIZE - 1) , HEX);
    // Serial.println( lower8(stack + THREADMAXSTACKSIZE - 1) , HEX);

    /* Prepare the stack */
    stack = stack + THREADMAXSTACKSIZE - 1;
    *(stack--) = 0x00;              /* Safety distance */
    *(stack--) = lower8(runner);
    *(stack--) = upper8(runner);
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
    for (i=2; i<=31; i++)    /* R2-R31 = 30 registers */
        *(stack--) = 0x00;

    /* *stack is now the stack pointer. Add the thread to the queue */
    new_thread->sp_low = lower8(stack);
    new_thread->sp_high = upper8(stack);
    ardk_enqueue(new_thread);

    Serial.print(" => thread created with id ");
    Serial.println(new_thread->thread_id);
    // Serial.print("    starts at address: ");
    // Serial.print(upper8(runner), HEX);
    // Serial.print(" ");
    // Serial.println(lower8(runner), HEX);
    // ardk_print_stack(stack+1, 36);

    return 0;
}


/**
 * Initializes the kernel
 * Timer interrupt regards:
 *     http://popdevelop.com/2010/04/mastering-timer-interrupts-on-the-arduino/
 * @return  0 on success; -1 on error
 */
int ardk_start(int ts) {
    if (thread_queue == NULL)
        return -1;

    DISABLE_INTERRUPTS();

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
    
    current_thread = &dummy;

    DISABLE_INTERRUPTS();
    ardk_switch_thread();
    ENABLE_INTERRUPTS();

    while (1);
    return 0;
}

/**
 * Interrupt service routine
 */
ISR(TIMER2_OVF_vect, ISR_NAKED) {
    /*ISR run with 1 kHz*/
    /* Save registers on stack */
    PUSHREGISTERS();

    TCNT2 = TIMERPRESET;

    if (time_count == time_slice) {
        // ardk_print_queue();
        time_count = 0;
        // Serial.print("Switching from thread ");
        // Serial.println(current_thread->thread_id);
        ardk_context_switch();
        // Serial.print("... to thread ");
        // Serial.println(current_thread->thread_id);
    } else
        time_count++;

    /* Restore registers from stack and return */
    POPREGISTERS();
    RET();
}

/**
 * Performs a context switch and invokes a new thread to execute
 */
void __attribute__ ((naked, noinline)) ardk_switch_thread(void) {
    // if (current_thread->thread_id == 0) {
    //     digitalWrite(12, HIGH);
    //     digitalWrite(13, LOW);
    // } else {
    //     digitalWrite(13, HIGH);
    //     digitalWrite(12, LOW);
    // }

    // Serial.print("Current thread is ");
    // Serial.println(current_thread->thread_id);

    /* Save registers on stack */
    PUSHREGISTERS();

    ardk_context_switch();

    /* Restore registers from stack and return */
    POPREGISTERS();
    RET();
}




// void __attribute__ ((naked, noinline)) start_threading(void) {
//     PUSHREGISTERS();
//     SPL = current_thread->sp_low;
//     SPH = current_thread->sp_high;
//     POPREGISTERS();
//     RET();
// }