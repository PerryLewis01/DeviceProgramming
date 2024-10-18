#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

/*
    I2C demo using bit banging to create i2c with software not using the hardware module
*/


#define I2C_SDA_PIN 	4
#define I2C_SCL_PIN 	5

const int on = 1;
const int off = 0;

const uint8_t I2C_ADDRESS = 0x42;


void set_bit(const uint location, const bool value, uint8_t& byte)
{
    byte = (byte & ~(0x01 << location)) | ((uint8_t)value << location);
}


class i2c_software
{
    public:
        int sda = I2C_SDA_PIN;
        int scl = I2C_SCL_PIN;
        uint64_t delay_us;

        i2c_software(int sda, int scl, int frequency_hz){
            sda = sda; 
            scl = scl;
            delay_us = MAX(1000000.0f / float(frequency_hz), 1); 

            gpio_init(sda);
            gpio_init(scl);

            gpio_set_dir(sda, GPIO_OUT);
            gpio_set_dir(scl, GPIO_OUT);

            gpio_set_slew_rate(sda, GPIO_SLEW_RATE_FAST);
            gpio_set_slew_rate(scl, GPIO_SLEW_RATE_FAST);

        };

        void write_bytes(uint8_t address, uint8_t* data, uint n_bytes)
        {
            // Try 3 times, to start communications 
            for (uint i = 0; i < 3; i++)
            {
                if (start_communication_with(address, false))
                    break;
            }

            for (uint i = 0; i < n_bytes; i++)
            {
                for (uint j = 0; j < 3; j++)
                {
                    if (write_byte(*(data + i)))
                        break;
                }
            }
        }

        void read_bytes(uint8_t address, uint8_t* data, uint n_bytes)
        {
            // Try 3 times, to start communications 
            for (uint i = 0; i < 3; i++)
            {
                if (start_communication_with(address, true))
                    break;
            }

            for (uint i = 0; i < n_bytes; i++)
            {
                *(data + i) = read_byte();
            }
        }


    private:

        void delay()
        {
            sleep_us(delay_us);
        }

        void set_sda(bool value)
        {
            gpio_put(sda, value);
        }

        bool get_sda()
        {
            return gpio_get(sda);
        }

        void set_scl(bool value)
        {
            gpio_put(scl, value);
        }

        void start_condition()
        {
            set_sda(on);
            set_scl(on);
            delay();
            set_sda(off);
            delay();
            set_scl(off);
            delay();
        }

        void stop_condition()
        {
            set_sda(off);
            set_scl(off);
            delay();
            set_scl(on);
            delay();
            set_sda(on);
        }

        void write_bit(bool bit)
        {
            set_sda(bit);
            delay();
            set_scl(on);
            delay();
            set_scl(off);
            delay();
        }

        bool read_bit()
        {
            delay();
            set_scl(on);
            bool bit = get_sda();
            delay();
            set_scl(off);
            delay();
            return bit;
        }

        bool read_acknowledge()
        {
            set_sda(off);
            gpio_set_dir(sda, 0);
            delay();
            
            set_scl(on);
            delay();

            // Low is an acknowledge
            bool acknowledged =  !get_sda();
            gpio_set_dir(sda, 1);

            set_scl(off);
            delay();

            return acknowledged;
        }

        // Start communications with a device, read set to true to read, false to write
        bool start_communication_with(uint8_t address, bool read)
        {
            start_condition();
            for (uint i = 0; i < 7; i++)
            {
                // MSB first
                write_bit(bool((address & (0x01 << (6-i))) >> (6-i)));
            }
            
            // Read / Write bit
            write_bit(read);

            // Acknowledge
            return read_acknowledge();
        }

        uint8_t read_byte()
        {
            set_sda(off);
            gpio_set_dir(sda, 0);
            delay();

            uint8_t output = 0;

            // Read byte
            for (uint i = 0; i < 8; i++)
            {
                set_bit(7 - i, read_bit(), output);
            }

            gpio_set_dir(sda, 1);

            // Acknowledge 
            write_bit(0);

            return output;
        }

        bool write_byte(uint8_t byte)
        {
            for (uint i = 0; i < 8; i++)
            {
                // MSB first
                write_bit(bool((byte & (0x01 << (7-i))) >> (7-i)));
            }
            return read_acknowledge();
        }

        

};


int main()
{
    
    stdio_init_all();
    sleep_ms(2000);
    printf("I2C Master\n");


    i2c_software i2c(I2C_SDA_PIN, I2C_SCL_PIN, 100000);

    static uint8_t number = 0;
    static uint8_t read_number = 0;

    while (true)
    {   
        printf("Writing %u\n", number);
        i2c.write_bytes(I2C_ADDRESS, &number, 1);
        printf("Wrote %u\n", number);

        sleep_ms(100);
        i2c.read_bytes(I2C_ADDRESS, &read_number, 1);
        printf("Read %u\n", read_number);

        number = read_number;

        sleep_ms(500);
    }
    
}
