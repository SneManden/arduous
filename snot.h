/******************************************************
 * snot.h                                             *
 *                                                    *
 *  Created on: Apr 15, 2013                          *
 *      Author: jdn                                   *
 **                                                   *
 ******************************************************
 *                                                    *
 *            (simple not - not ?! :-) )              *
 * my own small KeRNeL adapted for Arduino            *
 *                                                    *
 * this version adapted for Arduino                   *
 *                                                    *
 * (C) 2012,2013,2014                                 *
 *                                                    *
 * Jens Dalsgaard Nielsen <jdn@es.aau.dk>             *
 * http://www.control.aau.dk/~jdn                     *
 * Studentspace/Satlab                                *
 * Section of Automation & Control                    *
 * Aalborg University,                                *
 * Denmark                                            *
 *                                                    *
 * "THE BEER-WARE LICENSE" (frit efter PHK)           *
 * <jdn@es.aau.dk> wrote this file. As long as you    *
 * retain this notice you can do whatever you want    *
 * with this stuff. If we meet some day, and you think*
 * this stuff is worth it ...                         *
 *  you can buy me a beer in return :-)               *
 * or if you are real happy then ...                  *
 * single malt will be well received :-)              *
 *                                                    *
 * Use it at your own risk - no warranty              *
 *                                                    *
 * tested with duemilanove w/328, uno R3,             *
 * seeduino 1280 and mega2560                         *
 *****************************************************/


#ifndef SNOT
#define SNOT

#ifdef __cplusplus
extern "C" {
#endif

// remember to update in snot.c !!!
#define SNOT_VRS 1240


// if you are using k_mutex with prio inheritance
// #define MUTEX
// for guessing on architecture ...

#if defined(__AVR_ATmega168__)
#define ARCH_SLCT 1
#endif

#if defined(__AVR_ATmega328P__)
#define ARCH_SLCT 2
#endif

#if defined(__AVR_ATmega32U4__)
#define ARCH_SLCT 3
#endif

#if defined (__AVR_ATmega1280__)
#define ARCH_SLCT 4
#endif

#if  defined(__AVR_ATmega2560__)
#define ARCH_SLCT 6
#endif

#ifndef ARCH_SLCT
#error Failing unknown architecture - JDN
#endif

// DEBUGGING
//#define DMYBLINK     // ifdef then led (pin13) will light when dummy is running


extern int k_task;
extern int k_sem;
extern int k_msg;

#define QHD_PRIO 100            // Queue head prio - for sentinel use
#define DMY_PRIO (QHD_PRIO-2)           // dummy task prio (0== highest prio)
#define DMY_STK_SZ  90          // staksize for dummy
#define MAIN_PRIO   50          // main task prio
#define STAK_HASH   0x5c        // just a hashcode
#define MAX_SEM_VAL 50                  // NB is also max for nr elem in msgQ !
#define MAX_INT 0x7FFF
/***** KeRNeL data types *****/
struct k_t {
  struct k_t
      *next,  // double chain lists ptr
      *pred,
      *elm;   // ptr to owner of mutex etc
  volatile char
  sp_lo,
  sp_hi,    // HW: for saving task stak stk ptr
  prio;     // priority
  volatile int
  deadline, perTime;
  volatile int
  cnt1,    // counter for sem value etc , task: ptr to stak
  cnt2,   // dyn part of time counter for semaphores, task: timeout
  cnt3,   // preset timer value for semaphores, task: ptr to Q we are hanging in
    maxv,   // max value for sem, org priority for task
    clip;  // for lost signals etco
};

struct k_msg_t { // msg type
  struct k_t
      *sem;
  char
  *pBuf;    // ptr to user supplied ringbuffer
  volatile int
  nr_el,
    el_size,
    lost_msg;
  volatile int
  r,
  w,
  cnt;
};

/***** KeRNeL variables *****/
extern struct k_t
    *task_pool,
    *sem_pool,
    AQ,         // activeQ
    main_el,
    *pAQ,
    *pDmy,      // ptr to dummy task descriptor
    *pRun,      // ptr to running task
    *pSleepSem;

extern struct k_msg_t
    *send_pool;

extern char nr_task, nr_sem, nr_send;

extern volatile char k_running;  // no running

extern volatile char bugblink;
extern volatile char
k_err_cnt;  // every time an error occurs cnt is incr by one

#define lo8(X) ((unsigned char)((unsigned int)(X)))
#define hi8(X) ((unsigned char)((unsigned int)(X) >> 8))

#define K_CHG_STAK()    \
if (pRun != AQ.next) {  \
  if (bugblink)         \
    PORTB=0x00;         \
  pRun->sp_lo = SPL;    \
  pRun->sp_hi = SPH;    \
  pRun = AQ.next;       \
  SPL = pRun->sp_lo;    \
  SPH = pRun->sp_hi;    \
}

/**
 * MACROS MACROS
 *
 * PUSHREGS AND POPREGS
 * is actual staklayout plus task address at top
 * A push/pop takes 2 cycles
 * a call takes 3 cycles
 * ret/iret 3-4 cycles
 * So a PUSHREGS is 33 instructions(@ 2 cycles) = 66  cycles ~= 66 cycles /"16 MHz" ~= 4.1 usec
 * So an empty iSR which preserves all registers takes 2*4.1usec + 8-10 cycles (intr + iret) ~= 9 usec
 * So max isr speed with all regs saved is around 100 kHz but then with no code inside ISR !
 *
 * WARNING
 * The 2560 series has 3 bytes PC the rest only 2 bytes PC !!! (PC. program counter)
 * and no tall has rampz and eind register
 *
 *
 * REGISTER NAMING AND INTERNAL ADRESSING
 *
 * Register addresses
 * 0x3f SREG
 * 0x3e SPH
 * 0x3d SPL
 * 0x3c EIND
 * 0x3b RAMPZ
 * ...
 * 0x1f R31
 * etc
 * 0x01 R1
 * 0x00 R0
 *
 * PC is NOT available
 */

// MISSING no code 1284p

#define DI()   asm volatile ("cli")
#define EI()   asm volatile ("sei")
#define RETI() asm volatile ("reti")

#if defined (__AVR_ATmega2560__) || defined (__AVR_ATmega1280__)

#define PUSHREGS() asm volatile ( \
"push r1  \n\t" \
"push r0  \n\t" \
"in r0, __SREG__ \n\t" \
"cli \n\t" \
"push r0  \n\t" \
"in r0 , 0x3b \n\t" \
"push r0 \n\t" \
"in r0 , 0x3c \n\t" \
"push r0 \n\t" \
"push r2  \n\t" \
"push r3  \n\t" \
"push r4  \n\t" \
"push r5  \n\t" \
"push r6  \n\t" \
"push r7  \n\t" \
"push r8  \n\t" \
"push r9  \n\t" \
"push r10 \n\t" \
"push r11 \n\t" \
"push r12 \n\t" \
"push r13 \n\t" \
"push r14 \n\t" \
"push r15 \n\t" \
"push r16 \n\t" \
"push r17 \n\t" \
"push r18 \n\t" \
"push r19 \n\t" \
"push r20 \n\t" \
"push r21 \n\t" \
"push r22 \n\t" \
"push r23 \n\t" \
"push r24 \n\t" \
"push r25 \n\t" \
"push r26 \n\t" \
"push r27 \n\t" \
"push r28 \n\t" \
"push r29 \n\t" \
"push r30 \n\t" \
"push r31 \n\t" \
)

#define POPREGS() asm volatile ( \
"pop r31 \n\t" \
"pop r30 \n\t" \
"pop r29 \n\t" \
"pop r28 \n\t" \
"pop r27 \n\t" \
"pop r26 \n\t" \
"pop r25 \n\t" \
"pop r24 \n\t" \
"pop r23 \n\t" \
"pop r22 \n\t" \
"pop r21 \n\t" \
"pop r20 \n\t" \
"pop r19 \n\t" \
"pop r18 \n\t" \
"pop r17 \n\t" \
"pop r16 \n\t" \
"pop r15 \n\t" \
"pop r14 \n\t" \
"pop r13 \n\t" \
"pop r12 \n\t" \
"pop r11 \n\t" \
"pop r10 \n\t" \
"pop r9  \n\t" \
"pop r8  \n\t" \
"pop r7  \n\t" \
"pop r6  \n\t" \
"pop r5  \n\t" \
"pop r4  \n\t" \
"pop r3  \n\t" \
"pop r2  \n\t" \
"pop r0  \n\t" \
"out 0x3c , r0 \n\t " \
"pop r0  \n\t" \
"out 0x3b , r0 \n\t " \
"pop r0  \n\t" \
"out __SREG__ , r0 \n\t " \
"pop r0  \n\t" \
"pop r1  \n\t" \
)

#else

#define PUSHREGS() asm volatile ( \
"push r1 \n\t" \
"push r0 \n\t" \
"in r0, __SREG__ \n\t" \
"cli \n\t" \
"push r0  \n\t" \
"push r2  \n\t" \
"push r3  \n\t" \
"push r4  \n\t" \
"push r5  \n\t" \
"push r6  \n\t" \
"push r7  \n\t" \
"push r8  \n\t" \
"push r9  \n\t" \
"push r10 \n\t" \
"push r11 \n\t" \
"push r12 \n\t" \
"push r13 \n\t" \
"push r14 \n\t" \
"push r15 \n\t" \
"push r16 \n\t" \
"push r17 \n\t" \
"push r18 \n\t" \
"push r19 \n\t" \
"push r20 \n\t" \
"push r21 \n\t" \
"push r22 \n\t" \
"push r23 \n\t" \
"push r24 \n\t" \
"push r25 \n\t" \
"push r26 \n\t" \
"push r27 \n\t" \
"push r28 \n\t" \
"push r29 \n\t" \
"push r30 \n\t" \
"push r31 \n\t" \
)

#define POPREGS() asm volatile ( \
"pop r31 \n\t" \
"pop r30 \n\t" \
"pop r29 \n\t" \
"pop r28 \n\t" \
"pop r27 \n\t" \
"pop r26 \n\t" \
"pop r25 \n\t" \
"pop r24 \n\t" \
"pop r23 \n\t" \
"pop r22 \n\t" \
"pop r21 \n\t" \
"pop r20 \n\t" \
"pop r19 \n\t" \
"pop r18 \n\t" \
"pop r17 \n\t" \
"pop r16 \n\t" \
"pop r15 \n\t" \
"pop r14 \n\t" \
"pop r13 \n\t" \
"pop r12 \n\t" \
"pop r11 \n\t" \
"pop r10 \n\t" \
"pop r9  \n\t" \
"pop r8  \n\t" \
"pop r7  \n\t" \
"pop r6  \n\t" \
"pop r5  \n\t" \
"pop r4  \n\t" \
"pop r3  \n\t" \
"pop r2  \n\t" \
"pop r0  \n\t" \
"out __SREG__ , r0 \n\t " \
"pop r0  \n\t" \
"pop r1  \n\t" \
)

#endif  // 1280/2560

// function prototypes
// naming convention
// k_... function do a DI/EI and can impose task shift
// ki_... expects interrupt to be disablet and do no task shift
// rest is internal functions

/**
 * Eats CPU time in 1 msec quants
 * @param[in] eatTime  number of milliseconds to eay (<= 10000
 */
char k_eat_time(unsigned int eatTime);

/**
* issues a task shift - handle with care
* Not to be used by normal user
*/
void ki_task_shift(void) __attribute__ ((naked));

/**
* Set task asleep for a number of ticks.
* @param[in] time nr of ticks to sleep < 0
* @remark only to be called after start of SNOT
*/
int k_sleep(int time);

/**
* creates a task and put it in the active Q
* @param[in] pTask pointer to function for code ( void task(void) ...)
* @param[in] prio Priority 1: highest (QHEAD_PRIO-1): lowest
* @param[in] pStk Reference to data area to be used as stak
* @param[in] stkSize size of data area(in bytes) to be used for stak
* @return: pointer to task handle or NULL if no success
* @remark only to be called before start of SNOT but after k_init
*/
struct k_t * k_crt_task(void (*pTask)(void), char prio, char *pStk, int stkSize);

/**
* change priority of calling task)
* @param[in] prio new prio, Priority 1: highest (QHEAD_PRIO-1): lowest
* @return: 0: ok, -1: SNOT not running, -2: illegal value
* @remark only to be called after start of SNOT
*/
int k_set_prio(char prio);

/**
* returns nr of unbytes bytes on stak.
* For chekking if stak is too big or to small...
* @param[in] t Reference to task (by task handle)
* @return: nr of unused bytes on stak (int)
* @remark only to be called after start of SNOT
*/
int k_stk_chk(struct k_t *t);

/**
* creates a standard Dijkstra semaphore. It can be initialized to values in range [0..maxvalue]
* @param[in] init_val startvalue of semaphore 0,1,2,... maxvalue
* @param[in] maxvalue which is maxvalue semaphore can reach
* @return handle to semaphore or NULL pointer
* @remark only to be called before start of SNOT
*/
struct k_t * k_crt_sem(char init_val, int maxvalue);

/**
* attach a timer to the semaphore so SNOT will signal the semaphore with regular intervals.
* Can be used for cyclic real time run of a task.
* @param[in] sem semaphore handle
* @param[in] val interval in quant of SNOT ticks (0: disable cyclic timer, 1,2,3... cyclic quant)
* @return -1: negative val, 0. ok
* @remark only to be called after start of SNOT
*/
int k_set_sem_timer(struct k_t * sem, int val);

/**
* Signal a semaphore. Can be called from an ISR when interrupt is disabled. No task shift will occur - only queue manipulation.
* @param[in] sem semaphore handle
* @return 0: ok , -1: max value of semaphore reached
* @remark only to be called after start of SNOT
*/
int ki_signal(struct k_t * sem);

/**
* Signal a semaphore. Task shift will task place if a task is started by the signal and has higher priority than you.
* @param[in] sem semaphore handle
* @return 0: ok , -1: max value of semaphore reached
* @remark The ki_ indicates that interrups is NOT enabled when leaving ki_signal
* @remark only to be called after start of SNOT
*/
int k_signal(struct k_t * sem);

/**
* Wait on a semaphore. Task shift will task place if you are blocked.
* @param[in] sem semaphore handle
* @param[in] timeout "<0" you will be started after timeout ticks, "=0" wait forever "-1" you will not wait
* @return 0: ok , -1: timeout has occured, -2 no wait bq timeout was -1 and semaphore was negative
* @remark only to be called after start of SNOT
*/
int k_wait(struct k_t * sem, int timeout);

/**
* Wait on a semaphore. Task shift will task place if you are blocked.
* @param[in] sem semaphore handle
* @param[in] timeout "<0" you will be started after timeout ticks, "=0" wait forever "-1" you will not wait
* @param[out] lost if  not eq NULL it resturns how many signals has been lost
* @return 0: ok , -1: timeout has occured, -2 no wait bq timeout was -1 and semaphore was negative
* @remark only to be called after start of SNOT
*/
    int k_wait_lst(struct k_t * sem, int timeout, int *lost);

/**
* Do a wait if no blocking will occur.
* @param[in] sem semaphore handle
* @return 0: ok , -1: could do wait bw blocking would have taken place
* @remark The ki_ indicates that interrups is NOT enabled when leaving ki_nowait
* @remark only to be called after start of SNOT
*/
int ki_nowait(struct k_t * sem);

/**
* Like k_wait with the exception interrupt is NOT enabled when leaving
* @param[in] sem semaphore handle
* @param[in] timeout "<0" you will be started after timeout ticks, "=0" wait forever "-1" you will not wait
* @return 0: ok , -1: could do wait bw blocking would have taken place
* @remark The ki_ indicates that interrups is NOT enabled when leaving ki_wait
* @remark only to be called after start of SNOT
*/
int ki_wait(struct k_t * sem, int timeout);

/**
* returns value of semaphore
* @param[in] sem semaphore handle
* @return 0: semaphore value, negative: tasks are waiting, 0: nothing, positive: ...
* @remark only to be called after start of SNOT
*/
int ki_semval(struct k_t * sem);

#ifdef MUTEX
/**
* Priority inheritance based wait on semaphore for mutex use
* It is  advised only to access one mutex at time. Inheritance protocol is not designed for more than one mutex.
* @param[in] sem semaphore handle
* @return 0: semaphore value, negative: tasks are waiting, 0: nothing, positive: ...
* @remark only to be called after start of SNOT
*/
char k_mutex_entry(struct k_t * sem, int timeout);

/**
* Priority inheritance based signal on semaphore for mutex use
* It is  advised only to access one mutex at time. Inheritance protocol is not designed for more than one mutex.
* @param[in] sem semaphore handle
* @remark only to be called after start of SNOT
*/

char k_mutex_leave(struct k_t * sem);

/**
* Creates a circular message buffer with elements of equal size.
* @param[in] nr_el Number of elements in the ringbuffer
* @param[in] el_size Size of each element in bytes
* @param[in] pBuf Pointer to memory for holding the buffer (size nr_el*el_size). Caller has to come with the memory for the buffer.
* @return Reference to message buffer or NULL if if was not possible to create the buffer.
* @remark only to be called after start of SNOT
*/

#endif
struct k_msg_t * k_crt_send_Q(int nr_el, int el_size, void *pBuf);

/**
* Put data (one element of el_size)in the ringbuffer if there are room for it.
* Intended for ISR use
* DONE BY COPY !
* @param[in] pB Ref to message buffer
* @param[in] el Reference to data to be put in buffer. Size if already given in k_crt_send
* @return 0: operation did succed, -1: no room in ringbuffer
* @remark Interrupt will not enabled upon leaving, so ki_send is intended to be used from an ISR
* @remark only to be called before start of SNOT
*/
char ki_send(struct k_msg_t *pB, void *el);


/**
* Put data (one element of el_size)in the ringbuffer if there are room for it.
* DONE BY COPY !
* @param[in] pB Ref to message buffer
* @param [in] el Reference to data to be put in buffer. Size if already given in k_crt_send
* @return 0: operation did succed, -1: no room in ringbuffer
* @remark only to be called after start of SNOT
* @remark k_send does not block if no space in buffer. Instead -1 is returned
*/
char k_send(struct k_msg_t *pB, void *el);

/**
* Receive data (one element of el_size)in the ringbuffer if there are data
* DONE BY COPY !
* @param[in] pB Ref to message buffer
* @param [out] el Reference to where data shall be copied to at your site
* @param[in] timeout Max time you want to wait on data, -1: never, 0: forever, positive: nr of SNOT timer quants
* @param[out] lost_msg nr of lost messages since last receive. will clip at 10000. If lost_msg ptr is NULL then overrun counter
* is not reset to 0.
* @return 0: operation did succed, -1: no data in ringbuffer
* @remark only to be called after start of SNOT
*/
char k_receive(struct k_msg_t *pB, void *el, int timeout, int * lost_msg);

/**
* Receive data (one element of el_size)in the ringbuffer if there are data
* DONE BY COPY !
* No blocking if no data
* Interrupt will not be enabled after ki_receive
* @param[in] pB Ref to message buffer
* @param[out] el Reference to where data shall be copied to at your site
* @param[out] lost_msg nr of lost messages since last receive. will clip at 10000.  If lost_msg ptr is NULL then overrun counter
* is not reset to 0
* @return 0: operation did succed, -1: no data in ringbuffer
* @remark can be used from ISR
* @remark only to be called after start of SNOT
*/
char ki_receive(struct k_msg_t *pB, void *el, int * lost_msg);

/**
* start SNOT with tm tick speed (1= 1 msec, 5 = 5 msec)
* @param[in] tm Tick length in milli seconds
* @remark only to be called after init of SNOT
* @remark SNOT WILL NOT START IF YOU HAVE TRIED TO CREATE MORE TASKS/SEMS/MSG QS THAN YOU HAVE ALLOCATED SPACE FOR IN k_init !!!
*/
int k_start(int tm); // tm in milliseconds

/**
* Initialise SNOT. First function to be called.
* You have to give max number of tasks, semaphores and message queues you will use
* @param[in] nrTask ...
* @param[in] nrSem ...
* @param[in] nrMsg ...
 */
int k_init(int nrTask, int nrSem, int nrMsg);

/**
* Initialise blink on pin 13
* ON when dummy is running
*/
void k_bugblink13(char on);

/**
* returns number of unused space on a task stak
*/
int k_unused_stak(struct k_t *t);

/**
* returns amount of free memory in your system
*/
int freeRam(void);


#ifdef __cplusplus
}
#endif

#endif   // #ifndef SNOT

