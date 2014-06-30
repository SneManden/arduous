#include <arduous.h>

#define LED_PIN 13

void task1(void) {
    while (1) {
        /* Gør noget her */
        digitalWrite(LED_PIN, HIGH);
    }
}

void task2(void) {
    while (1) {
        /* Gør noget her */
        digitalWrite(LED_PIN, LOW);
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);

    ardk_create_thread(task1);
    ardk_create_thread(task2);
    ardk_start();
}

void loop() {/* Move along */}
