#ifndef I2C_SOFTWARE_SLAVE_H
#define I2C_SOFTWARE_SLAVE_H

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define MAX_NUMBER_OF_SLAVES 16

// Event types
enum i2c_software_slave_event {
    I2C_SLAVE_START = 1,
    I2C_SLAVE_RECEIVE,
    I2C_SLAVE_REQUEST,
    I2C_SLAVE_STOP,
    I2C_SLAVE_NULL,
};

// State machine for i2c
enum i2c_state_t {
    I2C_STATE_START = 1,
    I2C_STATE_TRANSMIT,
    I2C_STATE_RECEIVE,
    I2C_STATE_NULL,
};

// State machine for acknowledgement 
enum i2c_acknowledge_state_t {
    I2C_ACKNOWLEDGE_STATE_TRANSMIT = 1,
    I2C_ACKNOWLEDGE_STATE_RECEIVE,
    I2C_ACKNOWLEDGE_STATE_NULL,
};

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
        return out;
    }

    // Reset fifo memory
    void reset_fifo()
    {
        data = 0;
    }
};

/// @brief a function type which specifies how to handle data in a out of the slave device
/// @param data data received to to send
/// @param byte_number the position of the data to be received or sent
/// @param event the type of event triggering the event handler
typedef void (*i2c_software_slave_event_handler)(volatile uint8_t &data, const uint byte_number, const i2c_software_slave_event event);


class i2c_software_slave
{
    public:
        i2c_software_slave(uint sda_pin, uint scl_pin, uint8_t slave_address, i2c_software_slave_event_handler event_handler)
        {
            sda = sda_pin;
            scl = scl_pin;

            i2c_address            = slave_address;
            i2c_transmit_condition  = ((i2c_address << 1) | 1); // Shift address up 1 bit and add 1 to end
            i2c_receive_condition = ((i2c_address << 1) & ~1); // Shift address up 1 bit and add 0 to end

            _event_handler = event_handler;

            i2c_state = I2C_STATE_NULL;
            reset_values();
        }


        void sda_trigger_handler(uint gpio, uint32_t event);
        void scl_trigger_handler(uint gpio, uint32_t event);

        inline void reset_values();

        uint get_sda_pin() { return sda; }
        uint get_scl_pin() { return scl; }

        uint8_t get_i2c_fifo() { return i2c_fifo.data; }
    
    private:
        uint sda;
        uint scl;
        uint8_t i2c_address;
        i2c_software_slave_event_handler _event_handler; 

        // Address + Read or write bit.
        uint8_t i2c_receive_condition;
        uint8_t i2c_transmit_condition;

        // i2c input fifo
        fifo_8bit i2c_fifo;

        // i2c state machine
        volatile  i2c_state_t i2c_state;

        // acknowledgement state machine
        volatile i2c_acknowledge_state_t i2c_acknowledge_state;

        // count the number of bits read / written
        volatile uint i2c_bit_counter;

};

static i2c_software_slave* i2c_software_slave_instances[MAX_NUMBER_OF_SLAVES];
static uint number_of_i2c_software_slave_instances = 0;

void i2c_software_slave_init(uint sda_pin, uint scl_pin, uint8_t slave_address, i2c_software_slave_event_handler event_handler);

// Trigger handler
void i2c_software_slave_trigger_handler(uint gpio, uint32_t event);

#endif