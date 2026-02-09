// Sourced from https://www.youtube.com/watch?v=Snp6iTu1R7E

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define I2C_TIMEOUT_TICKS pdMS_TO_TICKS(1000)

static const char *TAG = "I2C_SCAN";

/* Initialize I2C master bus */
static i2c_master_bus_handle_t i2c_master_init_bus(void)
{
    i2c_master_bus_config_t bus_config = {};
   
    bus_config.i2c_port = I2C_NUM_0;
    bus_config.sda_io_num = GPIO_NUM_21;
    bus_config.scl_io_num = GPIO_NUM_22;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    return bus_handle;
}

/* I2C scanning task */
static void check_address_task(void *arg)
{
    i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t) arg;

    while (1) {
        for (uint8_t addr = 0x03; addr < 0x78; addr++) {
            esp_err_t err = i2c_master_probe(
                bus_handle,
                addr,
                I2C_TIMEOUT_TICKS
            );

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Found I2C device at 0x%02X", addr);
            }
        }

        ESP_LOGI(TAG, "I2C scan complete");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void)
{
    i2c_master_bus_handle_t bus_handle = i2c_master_init_bus();

    xTaskCreatePinnedToCore(
        check_address_task,
        "i2c_scan",
        4096,
        (void *)bus_handle,   // ✅ pass handle, not &handle
        10,
        NULL,
        1
    );
}

