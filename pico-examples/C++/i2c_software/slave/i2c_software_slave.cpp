#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "i2c_software_slave_lib.h"

/*
    I2C demo using bit banging to create i2c with software slave not using the hardware module
*/


#define I2C_SDA_PIN 	4u
#define I2C_SCL_PIN 	5u

const uint8_t I2C_ADDRESS = 0x42;

static uint8_t received;

static void event_handler(volatile uint8_t &data, const uint byte_number, const i2c_software_slave_event event)
{
    switch (event)
    {
    case I2C_SLAVE_START:
        break;

    case I2C_SLAVE_RECEIVE:
        received = data;
        break;
    
    case I2C_SLAVE_REQUEST:
        received += 1;
        data = received;
        break;
    
    case I2C_SLAVE_STOP:
        break;
    
    default:
        break;
    }
}


int main()
{
    // Setup code
    stdio_init_all();
    sleep_ms(2000);
    printf("I2C Slave\n");

    // init i2c slave
    i2c_software_slave_init(I2C_SDA_PIN, I2C_SCL_PIN, I2C_ADDRESS, &event_handler);

    while (true)
    {   
        // loop code
    }
    
}
