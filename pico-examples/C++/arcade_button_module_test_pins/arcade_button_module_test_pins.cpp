#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"


#define SLAVE_MASTER_SWITCH_PIN 17
volatile bool slave_mode = false;

#define SDA_PIN 4
#define SCL_PIN 5

// LED pins
#define LED_PIN_0 6
#define LED_PIN_1 7
#define LED_PIN_2 8
#define LED_PIN_3 9
#define LED_PIN_4 10
#define LED_PIN_5 11
#define LED_PIN_6 12
#define LED_PIN_7 13

// STATE PINS
#define LED_PIN_I2C_NULL 18
#define LED_PIN_I2C_START 19
#define LED_PIN_I2C_RX 20
#define LED_PIN_I2C_TX 21

#define LED_PIN_ACK_NULL 14
#define LED_PIN_ACK_RX 15
#define LED_PIN_ACK_TX 16

// State Machines
static enum i2c_state_t {
    I2C_STATE_NULL = 0b1,
    I2C_STATE_START = 0b10,
    I2C_STATE_MOSI = 0b100,
    I2C_STATE_MISO = 0b1000,
} i2c_state;

static enum ack_state_t {
    ACK_STATE_NULL = 0b1,
    ACK_STATE_MOSI = 0b10,
    ACK_STATE_MISO = 0b100,
} ack_state;

void init_leds(void)
{
    gpio_init(LED_PIN_0);
    gpio_init(LED_PIN_1);
    gpio_init(LED_PIN_2);
    gpio_init(LED_PIN_3);
    gpio_init(LED_PIN_4);
    gpio_init(LED_PIN_5);
    gpio_init(LED_PIN_6);
    gpio_init(LED_PIN_7);

    gpio_init(LED_PIN_I2C_NULL);
    gpio_init(LED_PIN_I2C_RX);
    gpio_init(LED_PIN_I2C_TX);
    gpio_init(LED_PIN_I2C_START);

    gpio_init(LED_PIN_ACK_NULL);
    gpio_init(LED_PIN_ACK_RX);
    gpio_init(LED_PIN_ACK_TX);

    gpio_set_dir(LED_PIN_0, GPIO_OUT);
    gpio_set_dir(LED_PIN_1, GPIO_OUT);
    gpio_set_dir(LED_PIN_2, GPIO_OUT);
    gpio_set_dir(LED_PIN_3, GPIO_OUT);
    gpio_set_dir(LED_PIN_4, GPIO_OUT);
    gpio_set_dir(LED_PIN_5, GPIO_OUT);
    gpio_set_dir(LED_PIN_6, GPIO_OUT);
    gpio_set_dir(LED_PIN_7, GPIO_OUT);
    
    gpio_set_dir(LED_PIN_I2C_NULL,  GPIO_OUT);
    gpio_set_dir(LED_PIN_I2C_RX,    GPIO_OUT);
    gpio_set_dir(LED_PIN_I2C_TX,    GPIO_OUT);
    gpio_set_dir(LED_PIN_I2C_START, GPIO_OUT);

    gpio_set_dir(LED_PIN_ACK_NULL,  GPIO_OUT);
    gpio_set_dir(LED_PIN_ACK_RX,    GPIO_OUT);
    gpio_set_dir(LED_PIN_ACK_TX,    GPIO_OUT);

    gpio_put(LED_PIN_0, 0);
    gpio_put(LED_PIN_1, 0);
    gpio_put(LED_PIN_2, 0);
    gpio_put(LED_PIN_3, 0);
    gpio_put(LED_PIN_4, 0);
    gpio_put(LED_PIN_5, 0);
    gpio_put(LED_PIN_6, 0);
    gpio_put(LED_PIN_7, 0);

    gpio_put(LED_PIN_I2C_NULL,  0);
    gpio_put(LED_PIN_I2C_RX,    0);
    gpio_put(LED_PIN_I2C_TX,    0);
    gpio_put(LED_PIN_I2C_START, 0);

    gpio_put(LED_PIN_ACK_NULL,  0);
    gpio_put(LED_PIN_ACK_RX,    0);
    gpio_put(LED_PIN_ACK_TX,    0);
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

void set_state()
{
    // Uses global state variables
    gpio_put(LED_PIN_I2C_NULL   , (i2c_state & I2C_STATE_NULL));
    gpio_put(LED_PIN_I2C_START  , (i2c_state & I2C_STATE_START));
    gpio_put(LED_PIN_I2C_RX     , (i2c_state & I2C_STATE_MOSI));
    gpio_put(LED_PIN_I2C_TX     , (i2c_state & I2C_STATE_MISO));

    gpio_put(LED_PIN_ACK_NULL   , (ack_state & ACK_STATE_NULL));
    gpio_put(LED_PIN_ACK_RX     , (ack_state & ACK_STATE_MOSI));
    gpio_put(LED_PIN_ACK_TX     , (ack_state & ACK_STATE_MISO));
}

void shift_state(void)
{
    switch (i2c_state)
    {
    case I2C_STATE_NULL:
        i2c_state = I2C_STATE_START;
        break;
    case I2C_STATE_START:
        i2c_state = I2C_STATE_MOSI;
        break;
    case I2C_STATE_MOSI:
        i2c_state = I2C_STATE_MISO;
        break;
    case I2C_STATE_MISO:
        i2c_state = I2C_STATE_NULL;
        break;
    default:
        i2c_state = I2C_STATE_NULL;
        break;
    }

    switch (ack_state)
    {
    case ACK_STATE_NULL:
        ack_state = ACK_STATE_MOSI;
        break;
    case ACK_STATE_MOSI:
        ack_state = ACK_STATE_MISO;
        break;
    case ACK_STATE_MISO:
        ack_state = ACK_STATE_NULL;
        break;
    default:
        ack_state = ACK_STATE_NULL;
        break;
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);

    init_leds();

    static uint8_t led_toggle = 1;

    while(true)
    {
        set_leds(led_toggle);
        led_toggle = led_toggle << 1;
        led_toggle = (led_toggle == 0) ? 1 : led_toggle;

        // set_state();
        // shift_state();

        sleep_ms(200);

    }
}