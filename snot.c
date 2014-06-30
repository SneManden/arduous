/*****************************************************
 * snot.c  part of kernel SNOT                        *
 *                                                    *
 *  Created on: Apr 15, 2013                          *
 *      Author: jdn                                   *
 *                                                    *
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

#include "snot.h"

#if (SNOT_VRS != 1240)
#error "SNOT VERSION NOT UPDATED in snot.c /JDN"
#endif

#include <avr/interrupt.h>
#include <stdlib.h>

//----------------------------------------------------------------------------

struct k_t
*task_pool, *sem_pool, AQ, main_el,
    *pAQ, *pDmy, *pRun, *pSleepSem;

struct k_msg_t *send_pool;

int k_task, k_sem, k_msg;
char nr_task = 0,
    nr_sem = 0,
    nr_send = 0;    // counters for created KeRNeL items

volatile char k_running = 0,
    k_err_cnt = 0;
volatile unsigned int tcnt2;    // counters for timer system
volatile int fakecnt,
    fakecnt_preset;
volatile char bugblink=0;
int tmr_indx;

struct k_t *pEl;

/**
 * just for eating time
 * eatTime in msec's
 */

char k_eat_time(unsigned int eatTime)
{
    unsigned long l;
    // tested on uno for 5 msec and 500 msec
    // quants in milli seconds
  l = eatTime;
  l *=1323;
  while (l--) {
    asm("nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t nop");
  }
}

//---QOPS---------------------------------------------------------------------

void
enQ (struct k_t *Q, struct k_t *el)
{

  el->next = Q;
  el->pred = Q->pred;
  Q->pred->next = el;
  Q->pred = el;

}

//----------------------------------------------------------------------------

struct k_t *
deQ (struct k_t *el)
{

  el->pred->next = el->next;
  el->next->pred = el->pred;

  return (el);
}

//----------------------------------------------------------------------------

void
prio_enQ (struct k_t *Q, struct k_t *el)
{

  char prio = el->prio;

  Q = Q->next;      // bq first elm is Q head itself

  while (Q->prio <= prio) {
    Q = Q->next;
  }

  el->next = Q;
  el->pred = Q->pred;
  Q->pred->next = el;
  Q->pred = el;
}

//----------------------------------------------------------------------------

void
chg_Q_pos (struct k_t *el)
{

  struct k_t *q;

  q = el->next;
  while (q->prio < QHD_PRIO)    // lets find Q head
    q = q->next;
  prio_enQ (q, el);
}


//---HW timer IRS-------------------------------------------------------------
//------------timer section---------------------------------------------------
/*
 * The SNOT Timer is driven by timer2
 *
 *
 * Install the Interrupt Service Routine (ISR) for Timer2 overflow.
 * This is normally done by writing the address of the ISR in the
 * interrupt vector table but conveniently done by using ISR()
 *
 * Timer2 reload value, globally available
 */

ISR (TIMER2_OVF_vect, ISR_NAKED)
{
    // no local vars ! I think
  PUSHREGS ();

  TCNT2 = tcnt2;        // Reload the timer

  if (!k_running)
    goto exitt;

  fakecnt--;
  if (0 < fakecnt)      // how often shall we run KeRNeL timer code ?
    goto exitt;

  fakecnt = fakecnt_preset; // now it's time for doing RT stuff

  // looks maybe crazy to go through all semaphores and tasks
  // but you may have 3-4 tasks and 3-6 semaphores in your code
  //  so...
  // It's a good idea not to init snot with more items (tasks/Sem/msg descriptors than needed)
  pEl = sem_pool;       // check timers on semaphores - they may be cyclic
  for (tmr_indx = 0; tmr_indx < nr_sem; tmr_indx++) {
    if (0 < pEl->cnt2) {
      // timer on semaphore ?
      pEl->cnt2--;
      if (pEl->cnt2 <= 0) {
                // if cnt3 == 0 then it's a one shoot
        pEl->cnt2 = pEl->cnt3;  // preset again
        ki_signal (pEl);    //issue a signal to the semaphore
      }
    }
    pEl++;
  }

  // Chk timers on tasks - they may be one shoot waiting
  pEl = task_pool;
  for (tmr_indx = 0; tmr_indx < nr_task; tmr_indx++) {

    // timeout active on task ?
    if (0 < pEl->cnt2) {
      pEl->cnt2--;
      if (pEl->cnt2 <= 0) {
        // timeout my friend - and you are on a Q some where
        pEl->cnt2 = -1; // error timeout - a task can only one shoot !
        ((struct k_t *) (pEl->cnt3))->cnt1++;   // bq we are leaving this semQ
        prio_enQ (pAQ, deQ (pEl));  // to AQ
      }
    }
    pEl++;
  }

  prio_enQ (pAQ, deQ (pRun));   // round robbin

  K_CHG_STAK ();

 exitt:
  POPREGS ();
  RETI ();
}

void k_bugblink13(char blink)
{ // always reset by dummy
  if (blink) {
      DDRB=0xff;
      bugblink=1;
  }
  else {
      bugblink=0;
  }
}

//----------------------------------------------------------------------------
// http://arduinomega.blogspot.dk/2011/05/timer2-and-overflow-interrupt-lets-get.html
// Inspiration from  http://popdevelop.com/2010/04/mastering-timer-interrupts-on-the-arduino/
//TIMSK2 &= ~(1 << TOIE2);  // Disable the timer overflow interrupt while we're configuring
//TCCR2B &= ~(1 << WGM22);

int
k_start (int tm)
{

  if (k_err_cnt)
    return -1;      // will not start if errors during initialization

  DI ();

#if defined(__AVR_ATmega32U4__)
  // 32u4 have no intern/extern clock source register
#else
  ASSR &= ~(1 << AS2);  // Select clock source: internal I/O clock 32u4 does not have this facility
#endif

  if (0 < tm) {
    TIFR2 = 0x00;
    TCCR2B = 0x00;  //silencio from this timer
    TCCR2A &= ~((1 << WGM21) | (1 << WGM20));   //Configure timer2 in normal mode (pure counting, no PWM etc.)
    TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);  // Set prescaler to CPU clock divided by 1024 See p162 i atmega328
    TIMSK2 &= ~(1 << OCIE2A);   //Disable Compare Match A interrupt enable (only want overflow)
    TIMSK2 = 0x01;  //HACK ? ...
    TCCR2A = 0x00;  // normal

    /* for your and my own memory
     *  We need to calculate a proper value to load the timer counter.
     * The following loads the value 131 into the Timer 2 counter register
     * The math behind this is:
     * (CPU frequency) / (prescaler value) = 16000000/1024= 15625 Hz ~= 64us.
     * 100Hz = 10msec
     * 10000usec / 64us = 156.25
     * MAX(uint8) + 1 - 156 = 100;
     * JDN
     * 100 Hz ~ 100
     * tm in msec ->
     * cnt =  tm*1000/64
     * ex: 10 msec: 10000/64 =156
     *
     * some timer reg values:
     * 1msec: 240 5msec: 178  10msec: 100   15msec: 22
     */
    tcnt2 = 240;        // 1 msec as basic heart beat

    // lets set divider for timer ISR
    if (tm <= 0)
      fakecnt = fakecnt_preset = 10;    // 10 msec
    else
      fakecnt = fakecnt_preset = tm;

    TCNT2 = tcnt2;  // Finally load end enable the timer
    TIMSK2 |= (1 << TOIE2);
  }

  pRun = &main_el;      // just for ki_task_shift
  k_running = 1;

  DI ();
  ki_task_shift ();     // bye bye from here
  EI ();
  while (1);            // you will never come here
  return (0);           // ok you will never come here
}

//----------------------------------------------------------------------------
// Inspiration from "Multitasking on an AVR" by Richard Barry, March 2004
// avrfreaks.net

void __attribute__ ((naked, noinline)) ki_task_shift (void)
{

  PUSHREGS ();      // push task regs on stak so we are rdy to task shift

  if (pAQ->next == pRun)    // need to change task ?
    goto exitt;

  pRun->sp_lo = SPL;        // save stak ptr
  pRun->sp_hi = SPH;

  pRun = pAQ->next;     // find next to run == front task in active Q

  SPL = pRun->sp_lo;        // restablish stk ptr
  SPH = pRun->sp_hi;
 exitt:
  POPREGS ();           // restore regs

  RETI ();          // and do a reti NB this also enables interrupt !!!
}

// HW_DEP_ENDE

//----------------------------------------------------------------------------

struct k_t *
k_crt_task (void (*pTask) (void), char prio, char *pStk, int stkSize)
{

  struct k_t *pT;
  int i;
  char *s;

  if (k_running)
    return (NULL);

  if ((prio <= 0 ) || (DMY_PRIO < prio)) {
    pT = NULL;
    goto badexit;
  }

  if (k_task <= nr_task) {
    goto badexit;
  }

  pT = task_pool + nr_task; // lets take a task descriptor
  nr_task++;

  pT->cnt2 = 0;     // no time out running on you for the time being

  // HW_DEP_START
  // inspiration from http://dev.bertos.org/doxygen/frame_8h_source.html
  // and http://www.control.aau.dk/~jdn/kernels/krnl/
  // now we are going to precook stak
  pT->cnt1 = (int) (pStk);

  for (i = 0; i < stkSize; i++) // put hash code on stak to be used by k_unused_stak()
    pStk[i] = STAK_HASH;

  s = pStk + stkSize - 1;   // now we point on top of stak
  *(s--) = 0x00;        // 1 byte safety distance
  *(s--) = lo8 (pTask); //  so top now holds address of function
  *(s--) = hi8 (pTask); // which is code body for task

  // NB  NB 2560 use 3 byte for call/ret addresses the rest only 2
#if defined (__AVR_ATmega2560__)
  *(s--) = EIND;        // best guess : 3 byte addresses !!!
#endif

  *(s--) = 0x00;        // r1
  *(s--) = 0x00;        // r0
  *(s--) = 0x00;        // sreg

  //1280 and 2560 need to save rampz reg just in case
#if defined (__AVR_ATmega2560__) || defined (__AVR_ATmega1280__)
  *(s--) = RAMPZ;       // best guess
  *(s--) = EIND;        // best guess
#endif

  for (i = 0; i < 30; i++)  //r2-r31 = 30 regs
    *(s--) = 0x00;

  pT->sp_lo = lo8 (s);  // now we just need to save stakptr
  pT->sp_hi = hi8 (s);  // in thread descriptor
  //HW_DE_ENDE

  pT->prio = prio;      // maxv for holding org prio for inheritance
  pT->maxv = (int) prio;
  prio_enQ (pAQ, pT);       // and put task in active Q

  return (pT);      // shall be index to task descriptor

 badexit:
  k_err_cnt++;
  return (NULL);
}

//----------------------------------------------------------------------------

int
freeRam (void)
{

  extern int __heap_start, *__brkval;
  int v;

  return ((int) &v -
          (__brkval == 0 ? (int) &__heap_start : (int) __brkval));

}

//----------------------------------------------------------------------------

int
k_sleep (int time)
{
  return k_wait (pSleepSem, time);
}

//----------------------------------------------------------------------------

int
k_unused_stak (struct k_t *t)
{
  int i;
  volatile char *p;
  p = (char *) (t->cnt1);
  i = 0;

  while (*(p++) == STAK_HASH)   // cnt hash codes on stak == amount of unused stak pr dft :-)
    i++;
  return (i);
}

//----------------------------------------------------------------------------

int
k_stk_chk (struct k_t *t)
{

  int i = 0;
  char *pstk;

  pstk = (char *) (t->cnt1);

  DI ();
  while (*pstk == STAK_HASH) {
    pstk++;
    i++;
  }
  EI ();

  return (i);
}

//----------------------------------------------------------------------------

int
k_set_prio (char prio)
{

  if (!k_running)
    return (-1);

  DI ();
  if ((prio <= 0) || (DMY_PRIO <= prio)) {
    // not legal value my friend
    EI ();
    return (-2);
  }

  pRun->prio = prio;
  prio_enQ (pAQ, deQ (pRun));
  ki_task_shift ();
  EI ();

  return (0);
}

//----------------------------------------------------------------------------

struct k_t *
k_crt_sem (char init_val, int maxvalue)
{

  struct k_t *sem;

  if (k_running)
    return (NULL);

  if ((init_val < 0) || (maxvalue < 0)) {
    goto badexit;
  }

  if (k_sem <= nr_sem) {
    goto badexit;
  }

  sem = sem_pool + nr_sem;
  nr_sem++;

  sem->cnt2 = 0;        // no timer running
  sem->next = sem->pred = sem;
  sem->prio = QHD_PRIO;
  sem->cnt1 = init_val;
  sem->maxv = maxvalue;
  sem->clip = 0; 
  return (sem);

 badexit:
  k_err_cnt++;

  return (NULL);
}

//----------------------------------------------------------------------------

int
k_set_sem_timer (struct k_t *sem, int val)
{

    // there is no k_stop_sem_timer fct just call with val== 0

  if (val < 0)
    return (-1);        // bad value

  DI ();
  sem->cnt2 = sem->cnt3 = val;  // if 0 then timer is not running - so
  EI ();

  return (0);
}

//----------------------------------------------------------------------------

int
ki_signal (struct k_t *sem)
{

  if (sem->maxv <= sem->cnt1){
        if (10000 > sem->clip)
              sem->clip++;
    return (-1);
    }

  sem->cnt1++;      // Salute to Dijkstra

  if (sem->cnt1 <= 0) {
    sem->next->cnt2 = 0;    // return code == ok
    prio_enQ (pAQ, deQ (sem->next));
  }

  return (0);
}

//----------------------------------------------------------------------------

int
k_signal (struct k_t *sem)
{
    int res;

  DI ();

  res = ki_signal (sem);

  if (res == 0)
    ki_task_shift ();

  EI ();

  return (res);
}

//----------------------------------------------------------------------------

int
ki_nowait (struct k_t *sem)
{

  DI ();            // DI just to be sure
  if (0 < sem->cnt1) {
    //      lucky that we do not need to wait ?
    sem->cnt1--;        // Salute to Dijkstra
    return (0);
  } else
    return (-1);
}

//----------------------------------------------------------------------------

int
ki_wait (struct k_t *sem, int timeout)
{

  DI ();

  if (0 < sem->cnt1) {
    //      lucky that we do not need to wait ?
    sem->cnt1--;        // Salute to Dijkstra
    return (0);
  }

  if (timeout == -1) {
    // no luck, dont want to wait so bye bye
    return (-2);
  }

  // from here we want to wait
  pRun->cnt2 = timeout; // if 0 then wait forever

  if (timeout)
    pRun->cnt3 = (int) sem; // nasty keep ref to semaphore,
  //  so we can be removed if timeout occurs
  sem->cnt1--;      // Salute to Dijkstra

  enQ (sem, deQ (pRun));
  ki_task_shift ();     // call enables interrupt on return

  return ((char) (pRun->cnt2)); // 0: ok, -1: timeout
}

//----------------------------------------------------------------------------

int
k_wait (struct k_t *sem, int timeout)
{

  // copy of ki_wait just with EI()'s before leaving
  DI ();

  if (0 < sem->cnt1) {
    //      lucky that we do not need to wait ?
    sem->cnt1--;        // Salute to Dijkstra
    EI ();
    return (0);
  }

  if (timeout == -1) {
    // no luck, dont want to wait so bye bye
    EI ();
    return (-2);
  }

  // from here we want to wait
  pRun->cnt2 = timeout; // if 0 then wait forever

  if (timeout)
    pRun->cnt3 = (int) sem; // nasty keep ref to semaphore,
  //  so we can be removed if timeout occurs
  sem->cnt1--;      // Salute to Dijkstra

  enQ (sem, deQ (pRun));
  ki_task_shift ();     // call enables interrupt on return

  EI ();

  return (char) (pRun->cnt2);   // 0: ok, -1: timeout
}

//----------------------------------------------------------------------------
int
k_wait_lst (struct k_t *sem, int timeout, int *lost)
{
    DI();
    if (lost != NULL) {
        *lost = sem->clip;
        sem->clip = 0;
    }
  return k_wait(sem,timeout);
}


//----------------------------------------------------------------------------
#ifdef MUTEX

NOT TESTED !!!  /Jens
char
k_mutex_entry (struct k_t *sem, int timeout)
{

  // copy of ki_wait just with EI()'s before leaving
  DI ();

  if (0 < sem->cnt1) {
    //    lucky that we do not need to wait ?
    sem->cnt1--;        // Salute to Dijkstra
    sem->elm = pRun;    // I do own mutex now
    EI ();
    return (0);
  }

  if (timeout == -1) {
    // no luck, dont want to wait so bye bye
    EI ();
    return (-2);
  }

  // from here we want to wait
  pRun->cnt2 = timeout; // if 0 then wait forever

  if (timeout)
    pRun->cnt3 = (int) sem; // nasty keep ref to semaphore,
  //  so we can be removed if timeout occurs
  sem->cnt1--;      // Salute to Dijkstra

  prio_enQ (sem, deQ (pRun));   // NBNB priority based Q
  if (sem->next == pRun) {
    // I am in front so push priority for mutex owner
    sem->elm->prio = pRun->prio;
    chg_Q_pos (sem->elm);
  }
  ki_task_shift ();     // call enables interrupt on return

  // timeout ?
  if (pRun->cnt2) {
    // yes - if I was in front of Q then
    if (sem->next != sem) {
      // readjust mutex owners prioriy down
      if (sem->elm->prio < sem->next->prio) {
        sem->elm->prio = sem->next->prio;
        chg_Q_pos (sem->elm);   // move around in AQ
      }
    }
  } else {
    // now I am owner
    sem->elm = pRun;
  }

  EI ();

  return (char) (pRun->cnt2);   // 0: ok, -1: timeout
}

//----------------------------------------------------------------------------

char
k_mutex_leave (struct k_t *sem)
{

  volatile char res;

  DI ();

  pRun->prio = (char) (pRun->maxv); // back to org prio
  prio_enQ (pAQ, deQ (pRun));   // chg pos in AQ acc to prio

  res = ki_signal (sem);

  if (res == 0)
    ki_task_shift ();

  EI ();

  return (res);
}

//----------------------------------------------------------------------------
#endif

int
ki_semval (struct k_t *sem)
{

  int i;

  DI (); // just to be sure
  // bq we ant it to be atomic
  i = sem->cnt1;
  //   EI (); NOPE dont enable ISR my friend

  return (i);
}

//----------------------------------------------------------------------------

char
ki_send (struct k_msg_t *pB, void *el)
{

  int i;
  char *pSrc, *pDst;

  if (pB->nr_el <= pB->cnt) {
    // room for a putting new msg in Q ?
    if (pB->lost_msg < 10000)
      pB->lost_msg++;
    return (-1);        // nope
  }

  pB->cnt++;

  pSrc = (char *) el;

  pB->w++;
  if (pB->nr_el <= pB->w)   // simple wrap around
    pB->w = 0;

  pDst = pB->pBuf + (pB->w * pB->el_size);  // calculate where we shall put msg in ringbuf

  for (i = 0; i < pB->el_size; i++) {
    // copy to Q
    *(pDst++) = *(pSrc++);
  }

  ki_signal (pB->sem);  // indicate a new msg is in Q

  return (0);
}

//----------------------------------------------------------------------------

char
k_send (struct k_msg_t *pB, void *el)
{

  char res;

  DI ();

  res = ki_send (pB, el);

  if (res == 0)
    ki_task_shift ();

  EI ();

  return (res);
}

//----------------------------------------------------------------------------

char
ki_receive (struct k_msg_t *pB, void *el, int *lost_msg)
{

  int i;
  char *pSrc, *pDst;

  // can be called from ISR bq no blocking
  DI ();            // just to be sure

  if (ki_nowait (pB->sem) == 0) {

    pDst = (char *) el;
    pB->r++;
    pB->cnt--;      // got one

    if (pB->nr_el <= pB->r)
      pB->r = 0;

    pSrc = pB->pBuf + pB->r * pB->el_size;

    for (i = 0; i < pB->el_size; i++) {
      *(pDst++) = *(pSrc++);
    }
    if (lost_msg) {
      *lost_msg = pB->lost_msg;
      pB->lost_msg = 0;
    }
    return (0);     // yes
  }

  return (-1);      // nothing for you my friend
}

//----------------------------------------------------------------------------

char
k_receive (struct k_msg_t *pB, void *el, int timeout, int *lost_msg)
{

  int i;
  char *pSrc, *pDst;

  DI ();

  if (ki_wait (pB->sem, timeout) == 0) {
    // ki_wait bq then intr is not enabled when coming back
    pDst = (char *) el;
    pB->r++;
    pB->cnt--;      // got one

    if (pB->nr_el <= pB->r)
      pB->r = 0;

    pSrc = pB->pBuf + pB->r * pB->el_size;

    for (i = 0; i < pB->el_size; i++) {
      *(pDst++) = *(pSrc++);
    }
    if (lost_msg) {
      *lost_msg = pB->lost_msg;
      pB->lost_msg = 0;
    }

    EI ();
    return (0);     // yes
  }

  EI ();
  return (-1);      // nothing for you my friend
}

//----------------------------------------------------------------------------

struct k_msg_t *
k_crt_send_Q (int nr_el, int el_size, void *pBuf)
{

  struct k_msg_t *pMsg;

  if (k_running)
    return (NULL);

  if (k_msg <= nr_send)
    goto errexit;

  if (k_sem <= nr_sem)
    goto errexit;

  pMsg = send_pool + nr_send;
  nr_send++;

  pMsg->sem = k_crt_sem (0, nr_el);

  if (pMsg->sem == NULL)
    goto errexit;

  pMsg->pBuf = (char *) pBuf;
  pMsg->r = pMsg->w = -1;
  pMsg->el_size = el_size;
  pMsg->nr_el = nr_el;
  pMsg->lost_msg = 0;
  pMsg->cnt = 0;

  return (pMsg);

 errexit:
  k_err_cnt++;
  return (NULL);
}


//----------------------------------------------------------------------------

void
k_round_robbin (void)
{

  // reinsert running task in activeQ if round robbin is selected
  DI ();

  prio_enQ (pAQ, deQ (pRun));
  ki_task_shift ();

  EI ();
}

char dmy_stk[DMY_STK_SZ];

//----------------------------------------------------------------------------

void
dmy_task (void)
{

  while (1) {
    if (bugblink)
        PORTB = PORTB | 0b00100000; 
  }
}

//----------------------------------------------------------------------------

int
k_init (int nrTask, int nrSem, int nrMsg)
{

  if (k_running)  // are you a fool ???
    return (NULL);

  k_task = nrTask + 1;  // +1 due to dummy
  k_sem = nrSem + nrMsg + 1;    // due to that every msgQ has a builtin semaphore
  k_msg = nrMsg;

  task_pool = (struct k_t *) malloc (k_task * sizeof (struct k_t));
  sem_pool = (struct k_t *) malloc (k_sem * sizeof (struct k_t));
  send_pool = (struct k_msg_t *) malloc (k_msg * sizeof (struct k_msg_t));

    // we dont accept any errors
  if ((task_pool == NULL) || (sem_pool == NULL) || (send_pool == NULL)) {
    k_err_cnt++;
    goto leave;
  }

    // init AQ as empty double chained list
  pAQ = &AQ;
  pAQ->next = pAQ->pred = pAQ;
  pAQ->prio = QHD_PRIO;

  pDmy = k_crt_task (dmy_task, DMY_PRIO, dmy_stk, DMY_STK_SZ);

  pSleepSem = k_crt_sem (0, 2000);

 leave:
  return k_err_cnt;
}