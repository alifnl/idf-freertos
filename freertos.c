#include <stdio.h>
// library for vtask delay
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// library for logging
#include "esp_log.h"
// library for error handling
#include "esp_err.h"

#include "freertos/semphr.h"
#define BUTTON_PIN 23

static const char *TAG = "FileSystem";
void check_button(void *pvParameters);
void Startnormaltask(void *pvParameters);
void Startlowtask(void *pvParameters);
void Starthightask(void *pvParameters);

SemaphoreHandle_t flag_semaphore = NULL;

void app_main(void)
{
  gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

  flag_semaphore = xSemaphoreCreateBinary();
  if (flag_semaphore != NULL)
  {
    ESP_LOGI(TAG, "flag semaphore created");
    xSemaphoreGive(flag_semaphore);
  }

  //5th parameter is priority, the higher the number the higher the priority
  xTaskCreatePinnedToCore(check_button, "check_button", 4096, NULL, 1, NULL, 1);       // without semaphore
  xTaskCreatePinnedToCore(Startnormaltask, "Startnormaltask", 4096, NULL, 2, NULL, 0); // without semaphore
  xTaskCreatePinnedToCore(Starthightask, "Starthightask", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(Startlowtask, "Startlowtask", 4096, NULL, 1, NULL, 0);
}

void check_button(void *pvParameters)
{
  while (1)
  {
    // TickType_t xLastWakeTime; //something like millis
    // xLastWakeTime = xTaskGetTickCount ();

    // execute something here
    static int a_z = 1;
    int a = gpio_get_level(BUTTON_PIN);
    if (a_z == 1 && a == 0)
    {
      ESP_LOGI(TAG, "button pushed");
    }
    a_z = a;
    // vTaskDelayUntil(&xLastWakeTime, 10) ;
    vTaskDelay(10); // 1 tick frequency = 100Hz
  }
}

void Startnormaltask(void *pvParameters)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for (;;)
  {
    ESP_LOGI(TAG, "Entered medium task");
    ESP_LOGI(TAG, "Leaving medium task");
    vTaskDelay(50);
  }
  /* USER CODE END 5 */
}

void Starthightask(void *pvParameters)
{
  for (;;)
  {

    ESP_LOGE(TAG, "Entered HPT and waiting for semaphore");
    if (xSemaphoreTake(flag_semaphore, portMAX_DELAY) == pdTRUE)
    {

      ESP_LOGE(TAG, "Semaphore acquired by HIGH Task");
      ESP_LOGE(TAG, "Leaving HPT and releasing semaphore");
      xSemaphoreGive(flag_semaphore);
      vTaskDelay(50);
    }
  }
}

void Startlowtask(void *pvParameters)
{
  for (;;)
  {
    ESP_LOGE(TAG, "Entered LPT and waiting for semaphore");
    // osSemaphoreWait(BinSemHandle, osWaitForever);

    if (xSemaphoreTake(flag_semaphore, portMAX_DELAY) == pdTRUE)
    {
      ESP_LOGE(TAG, "Semaphore acquired by LOW task");
      while (gpio_get_level(BUTTON_PIN) == 1){ // wait till the pin go low
        vTaskDelay(1); //need this to feed watchdog
      }
      ESP_LOGE(TAG, "Leaving LPT and releasing semaphore");
      xSemaphoreGive(flag_semaphore);
      // osSemaphoreRelease(BinSemHandle);
      vTaskDelay(50);
    }
  }
}