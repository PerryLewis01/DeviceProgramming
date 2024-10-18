
#include <stdio.h>
#include "pico/i2c_slave.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"


#define I2C i2c0
#define I2C_SDA_PIN 	4
#define I2C_SCL_PIN 	5

static uint I2C_SLAVE_ADDRESS = 0x42;

static uint8_t data_received = 150;

void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    // Depending on the type of event our handler must do different things
    switch (event)
    {
    case I2C_SLAVE_RECEIVE: // Master has written some data to this device
        data_received = i2c_read_byte_raw(i2c);
        break;
    case I2C_SLAVE_REQUEST:
        data_received++;
        i2c_write_byte_raw(i2c, data_received);
        break;

    // case I2C_SLAVE_FINISH: 
    //     // Code for stop
    //     break;
    default:
        break;
    }
}



int main()
{

    stdio_init_all();
    sleep_ms(2000);
    printf("I2C Slave\n");

    // Setup hardware i2c communication
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);


    i2c_init(I2C, 100000);
    // configure I2C0 for slave mode
    i2c_slave_init(I2C, I2C_SLAVE_ADDRESS, &i2c_slave_handler);


};