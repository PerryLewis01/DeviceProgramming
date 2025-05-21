#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

const uint8_t i2c_address = 0x42;
const uint8_t i2c_mosi_condition = (i2c_address << 1) & ~1; // Master out | Slave in
const uint8_t i2c_miso_condition = (i2c_address << 1) |  1; // Master in  | Slave out

#define SLAVE_MASTER_SWITCH_PIN 17
volatile bool slave_mode = false;

#define SDA_PIN 5
#define SCL_PIN 4

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
#define LED_PIN_I2C_NULL 19
#define LED_PIN_I2C_START 20
#define LED_PIN_I2C_RX 21
#define LED_PIN_I2C_TX 18

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
    gpio_put(LED_PIN_I2C_RX     , (i2c_state & I2C_STATE_MISO));
    gpio_put(LED_PIN_I2C_TX     , (i2c_state & I2C_STATE_MOSI));

    gpio_put(LED_PIN_ACK_NULL   , (ack_state & ACK_STATE_NULL));
    gpio_put(LED_PIN_ACK_RX     , (ack_state & ACK_STATE_MISO));
    gpio_put(LED_PIN_ACK_TX     , (ack_state & ACK_STATE_MOSI));
}

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
} i2c_fifo;

uint32_t bit_counter = 0;

volatile bool scl_value = false;
volatile bool sda_value = false;

void scl_handler(uint32_t event)
{
    switch (event)
    {
    case GPIO_IRQ_EDGE_RISE:
        scl_value = true;
        
        switch (i2c_state)
        {
        case I2C_STATE_START:
            i2c_fifo.shift_in(sda_value);
            bit_counter++;

            if (bit_counter == 8)
            {
                if (slave_mode)
                {
                    if (i2c_fifo.data == i2c_mosi_condition)
                    {
                        i2c_state = I2C_STATE_MOSI;
                        ack_state = ACK_STATE_MISO;
                    }
                    else if (i2c_fifo.data == i2c_miso_condition)
                    {
                        i2c_state = I2C_STATE_MISO;
                        ack_state = ACK_STATE_MISO;
                    }
                    else
                    {
                        i2c_state = I2C_STATE_NULL;
                        ack_state = ACK_STATE_NULL;
                    }
                }
                else 
                {
                    if (i2c_fifo.data & 1) // last bit one master read
                        i2c_state = I2C_STATE_MISO;
                    else 
                        i2c_state = I2C_STATE_MOSI;

                    ack_state = ACK_STATE_MISO;
                }
                // no matter what reset the fifo and bit counter
                i2c_fifo.reset_fifo();
                // bit_counter = 0;
            }
            
            break;

        case I2C_STATE_MOSI:
            i2c_fifo.shift_in(sda_value);
            bit_counter++;

            if ((bit_counter % 9) == 8)
                ack_state = ACK_STATE_MISO;
            else if ((bit_counter % 9) == 0)
                ack_state = ACK_STATE_NULL;
        
            break;
        
        case I2C_STATE_MISO:
            i2c_fifo.shift_in(sda_value);
            bit_counter++;

            if ((bit_counter % 9) == 8)
                ack_state = ACK_STATE_MOSI;
            else if ((bit_counter % 9) == 0)
                ack_state = ACK_STATE_NULL;


            break;
        
        default:
            break;
        }

        break;

    case GPIO_IRQ_EDGE_FALL:
        scl_value = false;
        break;
    }
}

void sda_handler(uint32_t event)
{
    switch (event)
    {
    case GPIO_IRQ_EDGE_RISE:
        sda_value = true;
        if (scl_value)
        {
            i2c_state = I2C_STATE_NULL;
            ack_state = ACK_STATE_NULL;
            i2c_fifo.reset_fifo();
            bit_counter = 0;
        }
        break;
    
    case GPIO_IRQ_EDGE_FALL:
        sda_value = false;
        if (scl_value)
        {
            i2c_state = I2C_STATE_START;
            ack_state = ACK_STATE_NULL;
            i2c_fifo.reset_fifo();
            bit_counter = 0;
        }
        break;
    }
}

void slave_mode_handler(uint32_t event)
{
    switch (event)
    {
    case GPIO_IRQ_EDGE_FALL:
        slave_mode = false;
        i2c_fifo.reset_fifo();
        bit_counter = 0;
        break;
    
    case GPIO_IRQ_EDGE_RISE:
        slave_mode = true;
        i2c_fifo.reset_fifo();
        bit_counter = 0;
        break;
    }
}

void trigger_handler(uint gpio, uint32_t event)
{
    if (gpio == SCL_PIN)
        scl_handler(event);
    else if (gpio == SDA_PIN)
        sda_handler(event);
    else if (gpio == SLAVE_MASTER_SWITCH_PIN)
        slave_mode_handler(event);
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);
    
    // Init LEDs
    init_leds();

    // Add triggers to i2c pins
    gpio_init(SDA_PIN);
    gpio_init(SCL_PIN);
    gpio_set_dir(SDA_PIN, GPIO_IN);
    gpio_set_dir(SCL_PIN, GPIO_IN);

    sda_value = gpio_get(SDA_PIN);
    scl_value = gpio_get(SCL_PIN);

    gpio_set_irq_enabled(SCL_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(SDA_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);


    // Add trigger to slave master switch
    gpio_init(SLAVE_MASTER_SWITCH_PIN);
    gpio_set_dir(SLAVE_MASTER_SWITCH_PIN, GPIO_IN);

    slave_mode = gpio_get(SLAVE_MASTER_SWITCH_PIN);
    gpio_set_irq_enabled(SLAVE_MASTER_SWITCH_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Enable all interrupts. 
    gpio_set_irq_callback(&trigger_handler);
    irq_set_enabled(IO_IRQ_BANK0, true);




    while (true)
    {
        // update display regularly
        set_leds(i2c_fifo.data);
        set_state();
        sleep_ms(50);
    }

}