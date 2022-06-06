#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "ir_tools.h"

#define ESP_INR_FLAG_DEFAULT 0
#define CONFIG_SENSOR_TRAFFIC_A_PIN 16
#define CONFIG_SENSOR_TRAFFIC_B_PIN 25
#define CONFIG_GREEN_A_PIN 19
#define CONFIG_GREEN_B_PIN 27
#define CONFIG_YELLOW_A_PIN 18
#define CONFIG_YELLOW_B_PIN 13
#define CONFIG_RED_A_PIN 22
#define CONFIG_RED_B_PIN 23
#define TIME_BETWEEN_COLORS_MS 1000
#define TIME_BETWEEN_A_B_MS 15000

// TaskHandle_t myTaskHandle = NULL;
int traffic_a_counter = 0;
int traffic_b_counter = 0;
bool isGreenTrafficA = 0;
bool isGreenTrafficB = 0;
bool force_b = false;
bool force_a = false;
bool a_opening = 0;
bool b_opening = 0;

TaskHandle_t ISR = NULL;
TaskHandle_t ISR_B = NULL;

void sys_info();

void IRAM_ATTR sensor_isr_handler(void *arg) {
    xTaskResumeFromISR(ISR);
}

void IRAM_ATTR sensor_isr_b_handler(void *arg) {
    xTaskResumeFromISR(ISR_B);
}

void traffic_a_sensor_task(void *arg) {
    gpio_set_level(CONFIG_RED_A_PIN, 1);
    while(true) {
        printf("Waiting cars on traffic A!\n");
        vTaskSuspend(NULL);
        if (!force_a)
            ++traffic_a_counter;
        printf("Car passed A traffic!\n");
        printf("Current # of traffic A cars: %d\n", traffic_a_counter);
        if (!b_opening && (traffic_a_counter >= 5 || (force_a && isGreenTrafficB))) {
            a_opening = 1;
            b_opening = 0;
            printf("Traffic Change\n");
            
            printf("Green_B off\n");
            gpio_set_level(CONFIG_GREEN_B_PIN, 0);
            isGreenTrafficB = 0;

	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Yellow_B on\n");
            gpio_set_level(CONFIG_YELLOW_B_PIN, 1);
            
	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Yellow_B off\n");
            gpio_set_level(CONFIG_YELLOW_B_PIN, 0);

	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Red_B on\n");
            gpio_set_level(CONFIG_RED_B_PIN, 1);

	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Red_A off\n");
            gpio_set_level(CONFIG_RED_A_PIN, 0);

	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Yellow_A on\n");
            gpio_set_level(CONFIG_YELLOW_A_PIN, 1);
            
	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Yellow_A off\n");
            gpio_set_level(CONFIG_YELLOW_A_PIN, 0);

            
	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Green_A on\n");
            gpio_set_level(CONFIG_GREEN_A_PIN, 1);
            isGreenTrafficA = 1;

            traffic_a_counter = 0;
            a_opening = 0;
	        vTaskDelay(TIME_BETWEEN_A_B_MS / portTICK_PERIOD_MS);
            force_b = true;
            force_a = false;
            vTaskResume(ISR_B);
        }
    }
}

void traffic_b_sensor_task(void *arg) {
    while(true) {
        printf("Waiting cars on traffic B!\n");
        vTaskSuspend(NULL);
        if (!force_b)
            ++traffic_b_counter;
        printf("Car passed B traffic!\n");
        printf("Current # of traffic B cars: %d\n", traffic_b_counter);
        if (!a_opening && (traffic_b_counter >= 5 || (force_b && isGreenTrafficA))) {
            a_opening = 0;
            b_opening = 1;
            printf("Traffic Change\n");
            
            printf("Green_A off\n");
            gpio_set_level(CONFIG_GREEN_A_PIN, 0);
            isGreenTrafficA = 0;

	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Yellow_A on\n");
            gpio_set_level(CONFIG_YELLOW_A_PIN, 1);
            
	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Yellow_A off\n");
            gpio_set_level(CONFIG_YELLOW_A_PIN, 0);

	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Red_A on\n");
            gpio_set_level(CONFIG_RED_A_PIN, 1);

	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Red_B off\n");
            gpio_set_level(CONFIG_RED_B_PIN, 0);

	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Yellow_B on\n");
            gpio_set_level(CONFIG_YELLOW_B_PIN, 1);
            
	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Yellow_B off\n");
            gpio_set_level(CONFIG_YELLOW_B_PIN, 0);

            
	        vTaskDelay(TIME_BETWEEN_COLORS_MS / portTICK_PERIOD_MS);
            printf("Green_B on\n");
            gpio_set_level(CONFIG_GREEN_B_PIN, 1);
            isGreenTrafficB = 1;

            traffic_b_counter = 0;
            b_opening = 0;
            vTaskDelay(TIME_BETWEEN_A_B_MS / portTICK_PERIOD_MS);
            force_a = true;
            force_b = false;
            
            vTaskResume(ISR);
        }
    }
}

void traffic_handler(int id) {
    gpio_pad_select_gpio(CONFIG_SENSOR_TRAFFIC_A_PIN);
    gpio_pad_select_gpio(CONFIG_SENSOR_TRAFFIC_B_PIN);
    gpio_pad_select_gpio(CONFIG_GREEN_A_PIN);
    gpio_pad_select_gpio(CONFIG_GREEN_B_PIN);
    gpio_pad_select_gpio(CONFIG_YELLOW_A_PIN);
    gpio_pad_select_gpio(CONFIG_YELLOW_B_PIN);
    gpio_pad_select_gpio(CONFIG_RED_A_PIN);
    gpio_pad_select_gpio(CONFIG_RED_B_PIN);

    gpio_set_direction(CONFIG_SENSOR_TRAFFIC_A_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_SENSOR_TRAFFIC_B_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_GREEN_A_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_GREEN_B_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_YELLOW_A_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_YELLOW_B_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_RED_A_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_RED_B_PIN, GPIO_MODE_OUTPUT);

    gpio_set_intr_type(CONFIG_SENSOR_TRAFFIC_A_PIN, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(CONFIG_SENSOR_TRAFFIC_B_PIN, GPIO_INTR_NEGEDGE);

    gpio_install_isr_service(ESP_INR_FLAG_DEFAULT);

    gpio_isr_handler_add(CONFIG_SENSOR_TRAFFIC_A_PIN, sensor_isr_handler, NULL);
    gpio_isr_handler_add(CONFIG_SENSOR_TRAFFIC_B_PIN, sensor_isr_b_handler, NULL);

	printf("Handler Id#%d\n", id);
    xTaskCreate(traffic_a_sensor_task, "task1", 4096, NULL, 10, &ISR);
    xTaskCreate(traffic_b_sensor_task, "task2", 4096, NULL, 10, &ISR_B);
    // xTaskCreatePinnedToCore(task2, "task2", 4600, NULL, 10, &myTaskHandle2, 1);
}

void app_main(void)
{
    // sys_info();
    int i = 0;
 	traffic_handler(++i);
}

void sys_info() {
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
}
