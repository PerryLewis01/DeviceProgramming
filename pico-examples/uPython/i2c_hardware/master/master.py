from machine import Pin, I2C
import time

# Setup I2C on the pico using built in I2C0
i2c = I2C(0, scl=Pin(17), sda=Pin(16), freq=100000)
slave_address = 0x42

def send_number(n, i2c):
    # Step 1: send number to slave
    i2c.writeto(slave_address, bytes([n]))

    # Step 2: wait some time for processing to be complete
    time.sleep(0.1)
    
    # Step 3: read the responce from slave (expect 1 byte)
    response = i2c.readfrom(slave_address, 1)
    
    return int.from_bytes(responce, 'little') # little endianess

while True:
    number = 5
    print(f"Sending number: {number}")
    
    result = send_number(number, i2c)
    print(f"Received result from slave: {result}")
    
    time.sleep(2)
    
