// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// LED rates
static const int rate_1 = 500;  // ms
static const int rate_2 = 323;  // ms

// Pins
static const int led_pin = 2;  // Correction: définir explicitement GPIO 2

// Mutex pour synchroniser l'accès à la LED
SemaphoreHandle_t ledMutex;

// Our task: blink an LED at one rate
void toggleLED_1(void *parameter) {
  while(1) {
    // Prendre le mutex avant d'utiliser la LED
    if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      digitalWrite(led_pin, HIGH);
      vTaskDelay(pdMS_TO_TICKS(rate_1));
      digitalWrite(led_pin, LOW);
      vTaskDelay(pdMS_TO_TICKS(rate_1));
      // Libérer le mutex
      xSemaphoreGive(ledMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Our task: blink an LED at another rate
void toggleLED_2(void *parameter) {
  while(1) {
    // Prendre le mutex avant d'utiliser la LED
    if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      digitalWrite(led_pin, HIGH);
      vTaskDelay(pdMS_TO_TICKS(rate_2));
      digitalWrite(led_pin, LOW);
      vTaskDelay(pdMS_TO_TICKS(rate_2));
      // Libérer le mutex
      xSemaphoreGive(ledMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {

  // Configure pin
  pinMode(led_pin, OUTPUT);

  // Créer le mutex avant les tâches
  ledMutex = xSemaphoreCreateMutex();

  // Task to run forever
  xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
              toggleLED_1,  // Function to be called
              "Toggle 1",   // Name of task
              2048,         // Stack size (correction: augmenté à 2048)
              NULL,         // Parameter to pass to function
              1,            // Task priority (0 to configMAX_PRIORITIES - 1)
              NULL,         // Task handle
              app_cpu);     // Run on one core for demo purposes (ESP32 only)

  // Task to run forever
  xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
              toggleLED_2,  // Function to be called
              "Toggle 2",   // Name of task
              2048,         // Stack size (correction: augmenté à 2048)
              NULL,         // Parameter to pass to function
              1,            // Task priority (0 to configMAX_PRIORITIES - 1)
              NULL,         // Task handle
              app_cpu);     // Run on one core for demo purposes (ESP32 only)

  // If this was vanilla FreeRTOS, you'd want to call vTaskStartScheduler() in
  // main after setting up your tasks.
}

void loop() {
  // Do nothing
  // setup() and loop() run in their own task with priority 1 in core 1
  // on ESP32
}
