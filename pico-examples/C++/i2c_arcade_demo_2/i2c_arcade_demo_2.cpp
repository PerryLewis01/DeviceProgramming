#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// LED Pins
#define LED_PIN_0 8
#define LED_PIN_1 9
#define LED_PIN_2 10
#define LED_PIN_3 11
#define LED_PIN_4 12
#define LED_PIN_5 13
#define LED_PIN_6 14
#define LED_PIN_7 15

// Button pins
#define SDA_BUTTON_PIN 4
#define SCL_LED_PIN 5

// I2C address
#define I2C_ADDRESS 0x42


// In this demo the device acts as master and the student must act as slave responding correctly to any input
// The LED will show the last 8 bit of information sent over the bus since the last reset to help them
// the SCL LED will show the current state of the clock
// they have control over a button to modify SDA when its released

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
};


const int on = 1;
const int off = 0;


void set_bit(const uint location, const bool value, uint8_t& byte)
{
    byte = (byte & ~(0x01 << location)) | ((uint8_t)value << location);
}


class i2c_software
{
    public:
        int sda = 4;
        int scl = 5;
        uint64_t delay_us;

        fifo_8bit i2c_fifo;

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
            stop_condition();
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
                bool stop_at_end = (i == int(n_bytes) - 1);
                *(data + i) = read_byte(stop_at_end);
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
                bool bit = bool((address & (0x01 << (6-i))) >> (6-i));
                write_bit(bit);
                i2c_fifo.shift_in(bit);
            }
            
            // Read / Write bit
            write_bit(read);
            i2c_fifo.shift_in(read);

            // Acknowledge
            return read_acknowledge();
        }

        uint8_t read_byte(bool stop_at_end)
        {
            set_sda(off);
            gpio_set_dir(sda, GPIO_IN);
            delay();

            uint8_t output = 0;

            // Read byte
            for (uint i = 0; i < 8; i++)
            {
                bool bit = read_bit();
                set_bit(7 - i, bit, output);
                i2c_fifo.shift_in(bit);
            }

            gpio_set_dir(sda, GPIO_OUT);

            // Acknowledge
            printf("READING ACKNOWLEDGE\n");
            if (!stop_at_end) 
            {   
                printf("NOT STOPPING\n");
                write_bit(0);
            }
            else
            {
                printf("STOPPING\n");
                set_sda(off);
                delay();
                set_scl(on);
                delay();
                set_sda(on);
                delay();
                printf("STOPPED\n");
            }

            return output;
        }

        bool write_byte(uint8_t byte)
        {
            for (uint i = 0; i < 8; i++)
            {
                // MSB first
                bool bit = bool((byte & (0x01 << (7-i))) >> (7-i));
                write_bit(bit);
                i2c_fifo.shift_in(bit);
            }
            return read_acknowledge();
        }

        

};

int main()
{
    // Setup code
    stdio_init_all();
    sleep_ms(2000);
    printf("I2C Arcade Demo\n");
    
    // Init leds
    init_leds();

    // Init i2c with button
    i2c_software i2c(SDA_BUTTON_PIN, SCL_LED_PIN, 10000);

    static uint8_t number = 42;

    while (true)
    {
        // loop code
        i2c.write_bytes(I2C_ADDRESS, &number, 1);
        sleep_ms(500);  

        i2c.read_bytes(I2C_ADDRESS, &number, 1);
        sleep_ms(500);

        if (number == 0 || number == 255)
        {
            number = 42;
        }


    }
}