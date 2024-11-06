#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "i2c_software_slave_lib.h"

// I2C arcade demo, allows the user to act as master using two buttons and shows the output to 8 led's

// LED Pins
#define LED_PIN_0 18
#define LED_PIN_1 19
#define LED_PIN_2 20
#define LED_PIN_3 21
#define LED_PIN_4 14
#define LED_PIN_5 15
#define LED_PIN_6 16
#define LED_PIN_7 17

// init led pins
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

// Button pins
#define SDA_BUTTON_PIN 4
#define SCL_BUTTON_PIN 5

// I2C address
#define I2C_SLAVE_ADDRESS 0x42

static uint8_t data_received = 150;

void event_handler(volatile uint8_t &data, const uint byte_number, const i2c_software_slave_event event)
{
    switch (event)
    {
        case I2C_SLAVE_START:
            printf("I2C START\n");
            set_leds(data);
            break;
        case I2C_SLAVE_RECEIVE:
            printf("I2C RECEIVE %02x\n", data);
            set_leds(data);
            data_received = data;
            break;
        case I2C_SLAVE_REQUEST:
            printf("I2C REQUEST %02x\n", data_received*2);
            data = data_received * 2;
            set_leds(data);
            break;
        case I2C_SLAVE_STOP:
            printf("I2C STOP\n");
            break;
        default:
            break;
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // Setup code
    printf("I2C Arcade Demo\n");
    
    // Init leds
    init_leds();

    // Init i2c with buttons
    i2c_software_slave_init(SDA_BUTTON_PIN, SCL_BUTTON_PIN, I2C_SLAVE_ADDRESS, &event_handler);

    while (true)
    {
        // loop code
    }

}