#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#define SDA_PIN 4
#define SCL_PIN 5


// LED pins
#define LED_PIN_0 17
#define LED_PIN_1 16
#define LED_PIN_2 15
#define LED_PIN_3 14
#define LED_PIN_4 21
#define LED_PIN_5 20
#define LED_PIN_6 19
#define LED_PIN_7 18

static uint bit_counter = 0;

// State Machine
static enum i2c_state_t {
    I2C_STATE_START = 1,
    I2C_STATE_TRANSMIT,
    I2C_STATE_RECEIVE,
    I2C_STATE_STOP,
} i2c_state;

void init_leds()
{
    gpio_init(LED_PIN_0);
    gpio_init(LED_PIN_1);
    gpio_init(LED_PIN_2);
    gpio_init(LED_PIN_3);
    gpio_init(LED_PIN_4);
    gpio_init(LED_PIN_5);
    gpio_init(LED_PIN_6);
    gpio_init(LED_PIN_7);

    gpio_set_dir(LED_PIN_0, GPIO_OUT);
    gpio_set_dir(LED_PIN_1, GPIO_OUT);
    gpio_set_dir(LED_PIN_2, GPIO_OUT);
    gpio_set_dir(LED_PIN_3, GPIO_OUT);
    gpio_set_dir(LED_PIN_4, GPIO_OUT);
    gpio_set_dir(LED_PIN_5, GPIO_OUT);
    gpio_set_dir(LED_PIN_6, GPIO_OUT);
    gpio_set_dir(LED_PIN_7, GPIO_OUT);

    gpio_put(LED_PIN_0, 0);
    gpio_put(LED_PIN_1, 0);
    gpio_put(LED_PIN_2, 0);
    gpio_put(LED_PIN_3, 0);
    gpio_put(LED_PIN_4, 0);
    gpio_put(LED_PIN_5, 0);
    gpio_put(LED_PIN_6, 0);
    gpio_put(LED_PIN_7, 0);
}

void set_leds(const uint8_t value)
{
    gpio_put(LED_PIN_0, (value & 0x01));
    gpio_put(LED_PIN_1, (value & 0x02));
    gpio_put(LED_PIN_2, (value & 0x04));
    gpio_put(LED_PIN_3, (value & 0x08));
    gpio_put(LED_PIN_4, (value & 0x10));
    gpio_put(LED_PIN_5, (value & 0x20));
    gpio_put(LED_PIN_6, (value & 0x40));
    gpio_put(LED_PIN_7, (value & 0x80));
}

// 8 bit fifo

struct fifo_8bit
{
    volatile uint8_t data = 0;

    // Move data into the fifo
    bool shift_in(bool bit)
    {   
        bool out = (data & 0x80);
        data = data << 1;
        data = (data & ~(0x01)) | ((uint8_t)bit);

        set_leds(data);
        return out;
    }

    // Reset fifo memory
    void reset_fifo()
    {
        data = 0;
    }
};

static fifo_8bit i2c_fifo;

void scl_button_handler(uint gpio, uint32_t event)
{
    if (gpio == SCL_PIN && event == GPIO_IRQ_EDGE_RISE)
    {
        bool bit = gpio_get(SDA_PIN);
        i2c_fifo.shift_in(bit);

        set_leds(i2c_fifo.data);
    }
}



int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // Setup code
    printf("I2C Button Master\n");
    
    // Init LEDs
    init_leds();

    // Add triggers to buttons
    gpio_init(SDA_PIN);
    gpio_init(SCL_PIN);
    gpio_set_dir(SDA_PIN, GPIO_IN);
    gpio_set_dir(SCL_PIN, GPIO_IN);

    gpio_set_irq_callback(&scl_button_handler);
    
    gpio_set_irq_enabled(SCL_PIN, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    while (true)
    {
        // loop code
    }

}