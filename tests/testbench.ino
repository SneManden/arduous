#include <arduous.h>

#define LED_PIN 13

int nextRunning;

void task1(void) {
    // Serial.println("1");
    Serial.println("Task 1 was started");
    while (1) {
        /* Gør noget her */
        digitalWrite(LED_PIN, HIGH);
    }
}

void task2(void) {
    // Serial.println("2");
    Serial.println("Task 2 was started");
    while (1) {
        /* Gør noget her */
        digitalWrite(LED_PIN, LOW);
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);

    Serial.println("Creating threads...");

    if (ardk_create_thread(task1))
        Serial.println("Task 1 returned with error");
    else
        Serial.println("Task 1 created!");

    if (ardk_create_thread(task2))
        Serial.println("Task 2 returned with error");
    else
        Serial.println("Task 2 created!");

    Serial.println("Starting kernel...");

    ardk_start(1000);
}

void loop() {/* Move along */}
