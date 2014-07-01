#include <arduous.h>
// #include <stdio.h>

#define LED_PIN 13
#define COOL_PIN 12

int socializing = 0;

void task1(void) {
    // ENABLE_INTERRUPTS();
    Serial.println("Thread 1 was started");
    while (1) {
        socializing++;
        delay(300);
        Serial.print("1: ");
        Serial.println(socializing);
    }
}

void task2(void) {
    Serial.println("Thread 2 was started");
    while (1) {
        socializing--;
        delay(250);
        Serial.print("2: ");
        Serial.println(socializing);
    }
}

void task3(void) {
    Serial.println("Thread 3 was started");
    while (1) {
        socializing++;
        delay(450);
        Serial.print("3: ");
        Serial.println(socializing);
    }
}

void task4(void) {
    Serial.println("Thread 4 was started");
    while (1) {
        socializing--;
        delay(550);
        Serial.print("4: ");
        Serial.println(socializing);
    }
}

void setup() {
    Serial.begin(9600);

    pinMode(LED_PIN, OUTPUT);
    pinMode(COOL_PIN, OUTPUT);

    Serial.println("Creating threads...");

    if (ardk_create_thread(task1))
        Serial.println("Thread 1 returned with error");

    if (ardk_create_thread(task2))
        Serial.println("Thread 2 returned with error");

    if (ardk_create_thread(task3))
        Serial.println("Thread 3 returned with error");

    if (ardk_create_thread(task4))
        Serial.println("Thread 4 returned with error");

    Serial.println("Starting kernel...");

    ardk_print_queue();

    if (ardk_start(2000))
        Serial.println("Kernel not started");
}

void loop() {/* Move along */}
