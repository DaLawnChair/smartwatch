#include "driver/i2c_types.h"
#include "soc/clk_tree_defs.h"
#include <cstdio>
#include <cstdint>
#include <utility> // for std::move
#include <iostream> 
#include <chrono>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*#include "driver/i2c.h"*/
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
}

// ---------- Config ----------
static const char* TAG = "MAX30102";

static constexpr i2c_port_t I2C_PORT = I2C_NUM_0;
static constexpr gpio_num_t I2C_SDA  = GPIO_NUM_21;
static constexpr gpio_num_t I2C_SCL  = GPIO_NUM_22;
static constexpr uint32_t I2C_FREQ_HZ = 400000; // 400kHz I2C

static constexpr uint8_t MAX30102_ADDR = 0x57;  // typical 7-bit addr

// ---------- MAX30102 Registers ----------
static constexpr uint8_t REG_INTR_STATUS_1  = 0x00;
static constexpr uint8_t REG_INTR_STATUS_2  = 0x01;
static constexpr uint8_t REG_INTR_ENABLE_1  = 0x02;
static constexpr uint8_t REG_INTR_ENABLE_2  = 0x03;

static constexpr uint8_t REG_FIFO_WR_PTR    = 0x04;
static constexpr uint8_t REG_OVF_COUNTER    = 0x05;
static constexpr uint8_t REG_FIFO_RD_PTR    = 0x06;
static constexpr uint8_t REG_FIFO_DATA      = 0x07;

static constexpr uint8_t REG_FIFO_CONFIG    = 0x08;
static constexpr uint8_t REG_MODE_CONFIG    = 0x09;
static constexpr uint8_t REG_SPO2_CONFIG    = 0x0A;
static constexpr uint8_t REG_LED1_PA        = 0x0C; // LED1 = RED
static constexpr uint8_t REG_LED2_PA        = 0x0D; // LED2 = IR
static constexpr uint8_t REG_PILOT_PA       = 0x10;

static constexpr uint8_t REG_PART_ID        = 0xFF;

static constexpr uint16_t I2C_MASTER_TIMEOUT_MS = 1000; 

class I2C_Object {
public :
    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_handle_t bus_handle;
};

// ---------- I2C helpers ----------
// Changes in names from older versions for i2c write and reads: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/migration-guides/release-5.x/5.2/peripherals.html#major-changes-in-usage
static esp_err_t i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data}; // The packet must have an address followed by the data
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/* Initialize I2C master bus */
static void i2c_master_init_bus(i2c_master_bus_handle_t *bus_handle) {
    /*i2c_new_master_bus_config_t conf{};*/
    /*conf.mode = I2C_MODE_MASTER;*/
    /*conf.sda_io_num = I2C_SDA;*/
    /*conf.scl_io_num = I2C_SCL;*/
    /*conf.sda_pullup_en = GPIO_PULLUP_ENABLE;*/
    /*conf.scl_pullup_en = GPIO_PULLUP_ENABLE;*/
    /*conf.master.clk_speed = I2C_FREQ_HZ;*/
    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_NUM_0; // Or I2C_NUM_1
    bus_config.sda_io_num = GPIO_NUM_21;
    bus_config.scl_io_num = GPIO_NUM_22;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT; //I2C_FREQ_HZ; //I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7; // Optional
    /*bus_config.flags.enable_pullup_internally = true; // Optional*/ //[][]doesn't exist?
    /*bus_config.allow_pd = true; // can be powered down in light sleep mode*/ //[][]doens't exist?
    
    // aborts if initialization fails
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));
}

static void i2c_master_device_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle, uint8_t address){
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = address;
    dev_cfg.scl_speed_hz = I2C_FREQ_HZ;
    // aborts if initialization fails
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg, dev_handle));
}

static esp_err_t max30102_soft_reset(i2c_master_dev_handle_t dev_handle) {
    // enables the RESET bit to halt sampling, clear FIFO, and reset internal logic
    // RESET bit when done
   
    // MODE_CONFIG reset bit = 1<<6  
    esp_err_t err = i2c_write_reg(dev_handle, REG_MODE_CONFIG, 0x40);
    if (err != ESP_OK) return err;

    // repeatidly check for reset to clear. May take a few ms
    for (int i = 0; i < 50; i++) {
        uint8_t v = 0;
        err = i2c_read_reg(dev_handle, REG_MODE_CONFIG, &v, 1);
        if (err != ESP_OK) return err;
        if ((v & 0x40) == 0) return ESP_OK; // reset bit set back to 0 by device
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_ERR_TIMEOUT;
}
static esp_err_t max30102_init(i2c_master_bus_handle_t& bus_handle, i2c_master_dev_handle_t& dev_handle){
    i2c_master_init_bus(&bus_handle);
    i2c_master_device_init(&bus_handle, &dev_handle, MAX30102_ADDR);

    printf("DONE INITIALIZATION");

    // Probe PART ID
    uint8_t part = 0;
    esp_err_t err = i2c_read_reg(dev_handle, REG_PART_ID, &part, 1);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "PART_ID = 0x%02X (often 0x15 for MAX30102)", part);

    // reset
    err = max30102_soft_reset(dev_handle);
    if (err != ESP_OK) return err;

    // Disable internal interrupts (turn this off later to enable the INT pin)
    // internal interupts possible: FIFO almost full, New data ready, temperature ready, power ready, ambient light overflow
    // turned off because we want pooling of data
    err = i2c_write_reg(dev_handle, REG_INTR_ENABLE_1, 0x00); if (err != ESP_OK) return err;
    err = i2c_write_reg(dev_handle, REG_INTR_ENABLE_2, 0x00); if (err != ESP_OK) return err;

    // clear status flags from clear-on-read registers 
    uint8_t tmp[2];
    err = i2c_read_reg(dev_handle, REG_INTR_STATUS_1, tmp, 2);
    if (err != ESP_OK) return err;


    // Reset FIFO pointers (write, overflow, read). Extra redundancy
    err = i2c_write_reg(dev_handle, REG_FIFO_WR_PTR, 0x00); if (err != ESP_OK) return err;
    err = i2c_write_reg(dev_handle, REG_OVF_COUNTER, 0x00); if (err != ESP_OK) return err;
    err = i2c_write_reg(dev_handle, REG_FIFO_RD_PTR, 0x00); if (err != ESP_OK) return err;

    // FIFO_CONFIG: no averaging, rollover off, almost-full threshold 0
    // Alternatives
    // [7:5] Sample average
    // [4]   FIFO rollover enable*/
    // [3:0] Almost-full threshold
    err = i2c_write_reg(dev_handle, REG_FIFO_CONFIG, 0x00);
    if (err != ESP_OK) return err;

    // MODE_CONFIG: SpO2 mode (0x03) = RED + IR
    // 0x02: RED only
    err = i2c_write_reg(dev_handle, REG_MODE_CONFIG, 0x03);
    if (err != ESP_OK) return err;

    // SPO2_CONFIG:
    // ADC range 4096nA (01), sample rate 100Hz (011), pulse width 411us (18-bit) (11)
    // => 0b01_011_11 = 0x5F
    // can reduce pulse width to lower bits if needed, but may need to change logic later
    err = i2c_write_reg(dev_handle, REG_SPO2_CONFIG, 0x5F);
    if (err != ESP_OK) return err;

    // LED currents (adjust if needed)
    err = i2c_write_reg(dev_handle, REG_LED1_PA, 0x24); if (err != ESP_OK) return err; // RED
    err = i2c_write_reg(dev_handle, REG_LED2_PA, 0x24); if (err != ESP_OK) return err; // IR
    err = i2c_write_reg(dev_handle, REG_PILOT_PA, 0x10); if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Init done (SpO2 mode).");
    return ESP_OK;
}

// used for checking distance between write and read pointers, letting you know how many data enteries you can parse
// does so by reading from the read and write pointer registers and checks values exist, if they do,
// find the distance between the two pointers and sets difference in out_n
static esp_err_t max30102_fifo_unread(uint8_t* out_n, i2c_master_dev_handle_t dev_handle) {
    uint8_t wr = 0, rd = 0;
    esp_err_t err = i2c_read_reg(dev_handle, REG_FIFO_WR_PTR, &wr, 1);
    if (err != ESP_OK) return err;
    err = i2c_read_reg(dev_handle, REG_FIFO_RD_PTR, &rd, 1);
    if (err != ESP_OK) return err;

    // find out distance between write and read pointers
    *out_n = (wr - rd) & 0x1F; // pointers are 5-bit, 0x1F=31, so & 0x1F === mod 32
    return ESP_OK;
}

// reads a single data point 
static esp_err_t max30102_read_one(uint32_t* red, uint32_t* ir, i2c_master_dev_handle_t dev_handle) {
    uint8_t data[6] = {0};

    // REG_FIFO_DATA will hold the data 
    // automatically advances FIFO read pointer when this is read
    // if SPO2, 3 bytes for RED, 3 bytes for IR
    // [RED_MSB][RED][RED_LSB][IR_MSB][IR][IR_LSB]
    esp_err_t err = i2c_read_reg(dev_handle, REG_FIFO_DATA, data, sizeof(data));
    if (err != ESP_OK) return err;

    uint32_t red_raw = (uint32_t(data[0]) << 16) | (uint32_t(data[1]) << 8) | uint32_t(data[2]);
    uint32_t ir_raw  = (uint32_t(data[3]) << 16) | (uint32_t(data[4]) << 8) | uint32_t(data[5]);

    // 0x3FFFF = 18 bits. This is an 18bit mask. Max30102 uses 18bit ADC 
    red_raw &= 0x3FFFF;
    ir_raw  &= 0x3FFFF;

    *red = red_raw;
    *ir  = ir_raw;
    return ESP_OK;
}




// ---------- app_main ----------
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Booting...");

    I2C_Object i2c_handles = I2C_Object();

    esp_err_t err = max30102_init(i2c_handles.bus_handle, i2c_handles.dev_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MAX30102 init failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "Check wiring + VIN=3.3V + addr=0x57");
        return;
    }


    std::chrono::steady_clock::time_point last_beat = std::chrono::steady_clock::now();
 
    while (true) {
        uint8_t n = 0;
        err = max30102_fifo_unread(&n, i2c_handles.dev_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "FIFO unread failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        /*if (n == 0) {*/
        /*    std::printf("Waiting for new data\n");*/
        /*    vTaskDelay(pdMS_TO_TICKS(500));*/
        /*    continue;*/
        /*}*/

        // Read a few samples per loop to avoid excessive serial spam
        uint8_t to_read = (n > 5) ? 5 : n;
        std::chrono::steady_clock::time_point curr_beat = std::chrono::steady_clock::now();
 
        std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(curr_beat-last_beat);
        last_beat = curr_beat;
        auto beats_per_minute = 60 / ( delta.count() / 1000.0L); //[][]
        
        for (uint8_t i = 0; i < n; i++) {
            uint32_t red = 0, ir = 0;
            err = max30102_read_one(&red, &ir, i2c_handles.dev_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Read sample failed: %s", esp_err_to_name(err));
                break;
            }
            std::printf("RED=%lu  IR=%lu\n", (unsigned long)red, (unsigned long)ir);
            std::printf("BPM=%lu\n", (long) beats_per_minute);
        }

        std::printf("Done %i\n", n);

        /*vTaskDelay(pdMS_TO_TICKS(50));*/
    }


}

