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


/*
    --------------------------------------------
    I2C bit-bang class
    
    This class creates an i2c_software communication.

    The user can read or write bytes to the slave
    device as the master in the same way they would
    use the hardware i2c.
    --------------------------------------------
*/
class i2c_software
{
    public:
        int sda;
        int scl;
        uint64_t delay_us;

        // i2c_software constructor, creates an i2c_software object
        i2c_software(int sda, int scl, int frequency_hz){
            sda = sda; 
            scl = scl;
            
            /*
                --------------------------------------------
                1. Setting Constants

                Calculate the delay in microseconds required 
                to operate at the required frequency.

                Then initialise the gpio pins, setting their
                direction, and slew rate
                --------------------------------------------
            */
           
           // Insert your code here
            


        };


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
            /*
                --------------------------------------------
                2. Start and Stop conditions
                Write the set of instructions to perform an i2c
                start condition.

                reference the data-sheet or i2c standard to do this
                --------------------------------------------
            */

           // Insert your code here
        }

        void stop_condition()
        {
            /*
                --------------------------------------------
                2. Start and Stop conditions
                Write the set of instructions to perform an i2c
                stop condition.

                reference the data-sheet or i2c standard to do this
                --------------------------------------------
            */

           // Insert your code here
        }

        void write_bit(bool bit)
        {
            /*
                --------------------------------------------
                3. Read & Write bit
                Write the set of instructions to write ONE bit
                to the slave.

                reference the data-sheet or i2c standard to do this
                --------------------------------------------
            */

           // Insert your code here
        }

        bool read_bit()
        {
            /*
                --------------------------------------------
                3. Read & Write bit
                Write the set of instructions to read ONE bit
                to the slave.

                Changing the direction of the GPIO is slow, so don't
                change it here

                reference the data-sheet or i2c standard to do this
                --------------------------------------------
            */

           // Insert your code here
        }

        bool read_acknowledge()
        {
            /*
                --------------------------------------------
                3. Read & Write bit
                Define the set of instructions to read one acknowledge
                from the slave device to the master

                remember the direction of the GPIO will need to change
                and should be returned to normal afterwards.

                gpio_set_dir(sda, GPIO_IN);

                return true if a successful acknowledge is received

                reference the data-sheet or i2c standard to do this
                --------------------------------------------
            */

           // Insert your code here
        }

        // Start communications with a device, read set to true to read, false to write
        bool start_communication_with(uint8_t address, bool read)
        {
            /*
                --------------------------------------------
                4. Communications
                Define the set of instructions to start i2c communications 
                of either type read or type write.

                Return true if a successful acknowledge is received 

                reference the data-sheet or i2c standard to do this
                --------------------------------------------
            */

           // Insert your code here
        }

        bool write_byte(uint8_t byte)
        {
            /*
                --------------------------------------------
                4. Communications
                Define the set of instructions to write one byte (8 bits)
                from the input byte.

                This function will be called after a successful start condition
                and used to write a string of bytes to a slave device.

                Return true if a successful acknowledge is received

                reference the data-sheet or i2c standard to do this
                --------------------------------------------
            */

           // Insert your code here
        }

        uint8_t read_byte()
        {
            /*
                --------------------------------------------
                4. Communications
                Define the set of instructions to read one byte (8 bits)
                from the slave device into the returned uint8_t.

                This function will be called after a successful start condition
                and used to read a string of bytes to a slave device.

                Ensure you change the direction of the GPIO appropriately

                Ensure you acknowledge the slave device

                reference the data-sheet or i2c standard to do this
                --------------------------------------------
            */

           // Insert your code here
        }

    public:

        void write_bytes(uint8_t address, uint8_t* data, uint n_bytes)
        {
            /*
                --------------------------------------------
                5. Handling Complex communication
                Define how the device should write data to the slave

                Ensure an acknowledge is received from the device
                repeating up to three times before failure if the 
                slave does not respond.
                --------------------------------------------
            */

           // Insert your code here
        }

        void read_bytes(uint8_t address, uint8_t* data, uint n_bytes)
        {
            /*
                --------------------------------------------
                5. Handling Complex communication
                Define how the device should read data from the slave

                Ensure your device is correctly acknowledging the slave
                and receiving an acknowledge from the slave before starting
                the read. again trying up to three times before failure.
                --------------------------------------------
            */

           // Insert your code here
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
