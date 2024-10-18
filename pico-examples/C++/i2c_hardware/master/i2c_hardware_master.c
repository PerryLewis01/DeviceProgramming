#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C				i2c0
#define I2C_SDA_PIN 	4
#define I2C_SCL_PIN 	5

static uint I2C_ADDRESS = 0x42;

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("I2C Master\n");

    // Setup hardware i2c communication
    i2c_init(I2C, 100000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);



    static uint8_t data = 0;
    static uint8_t data_received;

    // main loop
    while (true)
    {

        printf("Sending %u\n", data);
        i2c_write_blocking(I2C, I2C_ADDRESS, &data, 1, true);

        printf("Sent\n");
        sleep_ms(100);

        i2c_read_blocking(I2C, I2C_ADDRESS, &data_received, 1, false);
        printf("Received %u\n", data_received);

        data = data_received;

        if (data > 100)
            data = 0;

    }

    return 0;
}