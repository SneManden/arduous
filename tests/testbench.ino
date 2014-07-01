#include <arduous.h>

int socializing = 0;


/**
 * THREADS
 */
/* Thread 1 */
void task1(void) {
    SERIAL.println("Thread 1 was started");
    while (1) {
        socializing++;
        delay(300);
        SERIAL.print("1: ");
        SERIAL.println(socializing);
    }
}
/* Thread 2 */
void task2(void) {
    SERIAL.println("Thread 2 was started");
    while (1) {
        socializing--;
        delay(250);
        SERIAL.print("2: ");
        SERIAL.println(socializing);
    }
}
/* Thread 3 */
void task3(void) {
    SERIAL.println("Thread 3 was started");
    while (1) {
        socializing++;
        delay(450);
        SERIAL.print("3: ");
        SERIAL.println(socializing);
    }
}
/* Thread 4 */
void task4(void) {
    SERIAL.println("Thread 4 was started");
    while (1) {
        socializing--;
        delay(550);
        SERIAL.print("4: ");
        SERIAL.println(socializing);
    }
}


void setup() {
    SERIAL.begin(9600);

    SERIAL.println("Creating threads...");

    if (ardk_create_thread(task1))
        SERIAL.println("Thread 1 returned with error");
    if (ardk_create_thread(task2))
        SERIAL.println("Thread 2 returned with error");
    if (ardk_create_thread(task3))
        SERIAL.println("Thread 3 returned with error");
    if (ardk_create_thread(task4))
        SERIAL.println("Thread 4 returned with error");

    ardk_print_queue();
    SERIAL.println("Starting kernel...");

    if (ardk_start(2000))
        SERIAL.println("An error occoured whilst starting the Kernel.");
}


void loop() {/* Move along */}