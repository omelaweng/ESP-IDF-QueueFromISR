#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/queue.h"
#include "driver/gpio.h"

#define LED_PIN 27
#define PUSH_BUTTON_PIN 33

TaskHandle_t myTaskHandle = NULL;
QueueHandle_t queue;

void Task(void *arg)
{
    // 1. เตรียมพื้นที่รับข้อมูลผ่าน queue
    char rxBuffer;
    bool led_status = false; // Track the LED status (on/off)

    // 2. วนลูปรอ
    while (1)
    {
        // 3. ถ้ามีการส่งข้อมูลผ่าน queue handle ที่รอรับ ให้เอาข้อมูลไปแสดงผล
        // queue consumer จะใช้ฟังก์ชัน xQueueReceive
        if (xQueueReceive(queue, &(rxBuffer), (TickType_t)5))
        {
            // Toggle the LED status on button press
            led_status = !led_status;
            gpio_set_level(LED_PIN, led_status);
            printf("Button pressed! LED %s\n", led_status ? "ON" : "OFF");
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Optional: add a delay to debounce
        }
    }
}

void IRAM_ATTR button_isr_handler(void *arg)
{
    // code จาก https://www.freertos.org/a00119.html
    char cIn;
    BaseType_t xHigherPriorityTaskWoken;

    /* We have not woken a task at the start of the ISR. */
    xHigherPriorityTaskWoken = pdFALSE;
    cIn = '1'; // Dummy value sent via the queue to indicate button press
    xQueueSendFromISR(queue, &cIn, &xHigherPriorityTaskWoken);
}

void app_main(void)
{
    // Configure the GPIO pins
    esp_rom_gpio_pad_select_gpio(PUSH_BUTTON_PIN);
    esp_rom_gpio_pad_select_gpio(LED_PIN);

    gpio_set_direction(PUSH_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    gpio_set_intr_type(PUSH_BUTTON_PIN, GPIO_INTR_POSEDGE);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(PUSH_BUTTON_PIN, button_isr_handler, NULL);

    // สร้าง queue ที่บรรจุตัวแปร char จำนวน 1 ตัว
    queue = xQueueCreate(1, sizeof(char));

    // ทดสอบว่าสร้าง queue สำเร็จ?
    if (queue == 0)
    {
        printf("Failed to create queue= %p\n", queue);
    }

    // Create the task pinned to core 1
    xTaskCreatePinnedToCore(Task, "My_Task", 4096, NULL, 10, &myTaskHandle, 1);
}