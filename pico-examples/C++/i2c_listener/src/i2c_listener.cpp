#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#define CFG_TUD_CDC 1


#include "bsp/board.h"
#include "tusb.h"
#include "tusb_option.h"

// State machine for i2c
enum i2c_state_t {
    I2C_STATE_START = 0,
    I2C_STATE_TRANSMIT,
    I2C_STATE_RECEIVE,
    I2C_STATE_NULL,
};

// State machine for acknowledgement 
enum i2c_acknowledge_state_t {
    I2C_ACKNOWLEDGE_STATE_TRANSMIT = 0,
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

void send_32bit_serial(uint32_t data) {
    tud_cdc_write((uint8_t*)&data, 4);
    tud_cdc_write_flush();
}

class i2c_listener {
    public:
        int sda;
        int scl;

        volatile bool sda_level = false;
        volatile bool scl_level = false;

        volatile i2c_state_t i2c_state = I2C_STATE_NULL;
        volatile i2c_acknowledge_state_t i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_NULL;
        volatile i2c_state_t predicted_i2c_state = I2C_STATE_NULL;

        fifo_8bit i2c_fifo;

        i2c_listener(int sda_pin, int scl_pin) : sda(sda_pin), scl(scl_pin) 
        {
            sda_level = gpio_get(sda);
            scl_level = gpio_get(scl);
        }
        void trigger_handler(uint gpio, uint32_t events)
        {
            if (gpio == sda)
            {
                sda_trigger_handler(events);
            }
            else if (gpio == scl)
            {
                scl_trigger_handler(events);
            }
        }

        uint32_t message_queue;
        uint32_t message_queue_size = 0;

    private:
        uint32_t bit_counter = 0;

        uint32_t i2c_message(uint8_t data, i2c_state_t state, i2c_acknowledge_state_t acknowledge_state, bool acknowledged, uint8_t address) 
        { 
            // [START       ADDRESS         i2c_state   ack_state   acknowledged    DATA        ]
            // [1111 0000   0xxx xxxx       00xx        0xx         x               xxxx xxxx   ]
            return ((uint32_t)0b11110000 << 24) | ((uint32_t)address << 16) | ((uint32_t)state << 12) | ((uint32_t)acknowledge_state << 9) | ((uint32_t)acknowledged << 8) | ((uint32_t)data);
        }

        void sda_trigger_handler(uint32_t event) 
        {
            if (event == GPIO_IRQ_EDGE_FALL)
            {
                sda_level = false;
                if (scl_level)
                {
                    i2c_state = I2C_STATE_START;
                    bit_counter = 0;
                }
            }

            else if (event == GPIO_IRQ_EDGE_RISE)
            {
                sda_level = true;
                if (scl_level)
                {
                    i2c_state = I2C_STATE_NULL;
                    bit_counter = 0;
                }
            }
        }

        void scl_trigger_handler(uint32_t event) 
        {
            if (event == GPIO_IRQ_EDGE_RISE)
            {
                scl_level = true;

                // if doing something 
                if (i2c_state != I2C_STATE_NULL)
                {
                
                i2c_fifo.shift_in(sda_level);

                if (bit_counter == 6 && i2c_state == I2C_STATE_START)
                {
                    predicted_i2c_state = (sda_level) ? I2C_STATE_RECEIVE : I2C_STATE_TRANSMIT;
                }
                else if ((bit_counter + 1) % 9 == 8)
                {
                    i2c_acknowledge_state = (i2c_state == I2C_STATE_RECEIVE) ? I2C_ACKNOWLEDGE_STATE_TRANSMIT : I2C_ACKNOWLEDGE_STATE_RECEIVE;
                }
                else if ((bit_counter + 1) % 9 == 0)
                {
                    uint32_t msg = i2c_message(i2c_fifo.data, I2C_STATE_START, i2c_acknowledge_state, sda_level, i2c_fifo.data);
                    send_32bit_serial(msg);
                    i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_NULL;
                    i2c_state = predicted_i2c_state;
                }

                ++bit_counter;
                }
                
            }

            else if (event == GPIO_IRQ_EDGE_FALL)
            {
                scl_level = false;
            }
        }


};

static i2c_listener* global_listener;

void trigger_handler(uint gpio, uint32_t events) {
    global_listener->trigger_handler(gpio, events);
}


void init_interrupts(i2c_listener& listener)
{
    int sda = listener.sda;
    int scl = listener.scl;

    gpio_init(sda);
    gpio_init(scl);

    gpio_set_dir(sda, GPIO_IN);
    gpio_set_dir(scl, GPIO_IN);

    gpio_set_irq_callback(&trigger_handler);
    
    gpio_set_irq_enabled(sda, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_set_irq_enabled(scl, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
}


int main() {
    stdio_init_all();
    sleep_ms(2000);
    tusb_init();


    // Setup code
    global_listener = new i2c_listener(4, 5);
    init_interrupts(*global_listener);
    while (true)
    {
        // loop code
    }
}