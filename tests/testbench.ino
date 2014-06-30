#include <arduous.h>
// #include <stdio.h>

#define LED_PIN 13
#define COOL_PIN 12

int nextRunning;

void task1(void) {
    // Serial.println("1");
    Serial.println("Thread 1 was started");
    while (1) {
        /* Gør noget her */
        //digitalWrite(LED_PIN, HIGH);
    }
}

void task2(void) {
    // Serial.println("2");
    Serial.println("Thread 2 was started");
    while (1) {
        /* Gør noget her */
        //digitalWrite(LED_PIN, LOW);
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

    Serial.println("Starting kernel...");

    // ardk_print_queue();

    if (ardk_start(1000))
        Serial.println("Kernel not started");
}

void loop() {/* Move along */}
