#include "i2c_software_slave_lib.h"

static inline void reset_values() {
    i2c_fifo.reset_fifo();
    i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_NULL;
    i2c_bit_counter = 0;
}


void i2c_software_slave_init(uint sda_pin, uint scl_pin, uint8_t slave_address, i2c_software_slave_event_handler event_handler)
{
    sda = sda_pin;
    scl = scl_pin;

    i2c_address            = slave_address;
    i2c_receive_condition  = ((i2c_address << 1) | 1); // Shift address up 1 bit and add 1 to end
    i2c_transmit_condition = ((i2c_address << 1) & ~1); // Shift address up 1 bit and add 0 to end

    _event_handler = event_handler;

    i2c_state = I2C_STATE_NULL;
    reset_values();

    // init i2c pins 
    gpio_init(sda);
    gpio_init(scl);

    gpio_set_dir(sda, GPIO_IN);
    gpio_set_dir(scl, GPIO_IN);

    gpio_set_slew_rate(sda, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(scl, GPIO_SLEW_RATE_FAST);

    // attach triggers to pins
    gpio_set_irq_callback(&i2c_software_slave_trigger_handler);
    
    gpio_set_irq_enabled(sda, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_set_irq_enabled(scl, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void i2c_software_slave_trigger_handler(uint gpio, uint32_t event)
{
    if (gpio == sda)
    {
        sda_trigger_handler(gpio, event);
    }
    else if (gpio == scl)
    {
        scl_trigger_handler(gpio, event);
    }
}

void sda_trigger_handler(uint gpio, uint32_t event)
{
    bool clock_level = gpio_get(scl);
    // Start condition is a falling edge while scl is high
    if (event == GPIO_IRQ_EDGE_FALL && clock_level)
    {
        i2c_state = I2C_STATE_START;
        reset_values();
        _event_handler(i2c_fifo.data, 0, I2C_SLAVE_START);
    }
    
    // Stop condition is a rising edge while scl is high
    else if (event == GPIO_IRQ_EDGE_RISE && clock_level)
    {
        i2c_state = I2C_STATE_NULL;
        reset_values();
        _event_handler(i2c_fifo.data, 0, I2C_SLAVE_STOP);
    }
}

void scl_trigger_handler(uint gpio, uint32_t event)
{
    // Whenever we a reading a pin it must be when scl is high
    if (event == GPIO_IRQ_EDGE_RISE)
    {
        switch(i2c_state)
        {   
        
        case I2C_STATE_START:
            
            // Read Bit
            i2c_fifo.shift_in(gpio_get(sda));
            
            // if i2c fifo now matches the transmit condition move to the transmit state and trigger an acknowledge
            if (i2c_fifo.data == i2c_transmit_condition)
            {
                i2c_state = I2C_STATE_TRANSMIT;
                i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_TRANSMIT;
                i2c_bit_counter = 0;
                i2c_fifo.reset_fifo();
            }
            // if the i2c fifo now matches the receive condition move to the receive state and trigger an acknowledge
            else if (i2c_fifo.data == i2c_receive_condition)
            {
                i2c_state = I2C_STATE_RECEIVE;
                i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_TRANSMIT;
                i2c_bit_counter = 0;
                i2c_fifo.reset_fifo();
            }
            break;
        

        case I2C_STATE_TRANSMIT:
            if (i2c_acknowledge_state == I2C_ACKNOWLEDGE_STATE_RECEIVE)
            {
                gpio_set_dir(sda, GPIO_IN);
                bool acknowledged = !gpio_get(sda);
                i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_NULL;
                gpio_set_dir(sda, GPIO_OUT);
                // if not acknowledged we must repeat the last byte
                if (!acknowledged && i2c_bit_counter >= 8)
                {
                    i2c_bit_counter -= 8; 
                }
            }
            break;
        
        case I2C_STATE_RECEIVE:
            if (i2c_acknowledge_state == I2C_ACKNOWLEDGE_STATE_NULL)
            {
                // Read data into fifo
                gpio_set_dir(sda, GPIO_IN);
                i2c_fifo.shift_in(gpio_get(sda));
                i2c_bit_counter++;
                if (i2c_bit_counter % 8 == 0)
                {
                    _event_handler(i2c_fifo.data, i2c_bit_counter / 8, I2C_SLAVE_RECEIVE);
                    i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_TRANSMIT;
                }
            }

        default:
            break;
        }
    }

    // SCL must be low to change SDA
    else if (event = GPIO_IRQ_EDGE_FALL)
    {
        switch (i2c_state)
        {
        case I2C_STATE_TRANSMIT:

            if (i2c_acknowledge_state == I2C_ACKNOWLEDGE_STATE_TRANSMIT)
            {
                gpio_set_dir(sda, GPIO_OUT);
                gpio_put(sda, 0);
                i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_NULL;
            }
            
            // State condition to transmit data from buffer
            else if (i2c_acknowledge_state == I2C_ACKNOWLEDGE_STATE_NULL)
            {
                // Every byte get the next one
                if (i2c_bit_counter % 8 == 0)
                {
                    _event_handler(i2c_fifo.data, i2c_bit_counter / 8, I2C_SLAVE_REQUEST);
                }

                // Write the next bit to the output
                gpio_put(sda, i2c_fifo.shift_in(0));
                i2c_bit_counter++;

                if (i2c_bit_counter % 8 == 0)
                {
                    i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_RECEIVE;
                }
            }
            
            break;
        
        case I2C_STATE_RECEIVE:
            if (i2c_acknowledge_state == I2C_ACKNOWLEDGE_STATE_TRANSMIT)
            {
                gpio_set_dir(sda, GPIO_OUT);
                gpio_put(sda, 0);
                i2c_acknowledge_state = I2C_ACKNOWLEDGE_STATE_NULL;
            }
            break;


        default:
            break;
        } // END switch i2c_state
    } // END if event
} // END scl trigger handling
