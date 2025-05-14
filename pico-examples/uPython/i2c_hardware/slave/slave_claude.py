# RP2040 I2C Slave implementation using PIO
# This implements a slave device that stores a received byte and returns it multiplied by 2

from machine import Pin
import rp2
import utime

# Define I2C parameters
SLAVE_ADDR = 0x41  # I2C slave address (you can change this)
SDA_PIN = 0        # GPIO 0
SCL_PIN = 1        # GPIO 1

# Global variable to store received byte
stored_byte = 0

# I2C slave PIO program
# This is a simplified I2C slave implementation focused on our specific use case
@rp2.asm_pio(
    set_init=(rp2.PIO.OUT_HIGH),
    autopull=False,
    autopush=False,
    sideset_init=(rp2.PIO.OUT_HIGH)
)
def i2c_slave_pio():
    # Wait for START condition (SDA going low while SCL is high)
    wait(1, gpio, SCL_PIN)         # Wait for SCL high
    wait(0, gpio, SDA_PIN)         # Wait for SDA going low (START condition)

    # Read 8 bits for address
    set(x, 7)                      # 8 bits (7 address bits + 1 R/W bit)
    label("read_address_loop")
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    wait(1, gpio, SCL_PIN)         # Wait for SCL high
    in_(pins, 1)                   # Read SDA into ISR
    jmp(x_dec, "read_address_loop")# Loop until all bits read
    
    # Check if the address matches and extract R/W bit
    mov(y, isr)                    # Save the address and R/W bit
    set(x, SLAVE_ADDR)             # Load our address
    jmp(x_not_y, "wait_for_start") # If address doesn't match, ignore
    
    # Address matches, send ACK
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    set(pindirs, 1)                # Set SDA as output
    set(pins, 0)                   # Pull SDA low for ACK
    wait(1, gpio, SCL_PIN)         # Wait for SCL high
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    set(pindirs, 0)                # Set SDA as input again
    
    # Check R/W bit
    jmp(pin, "read_from_master")   # If LSB=1, master wants to read from us
    
    # ---- Master writing to us ----
    # Read data byte
    set(x, 7)                      # 8 bits to read
    label("read_data_loop")
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    wait(1, gpio, SCL_PIN)         # Wait for SCL high
    in_(pins, 1)                   # Read SDA into ISR
    jmp(x_dec, "read_data_loop")   # Loop until all bits read
    
    # Send ACK
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    set(pindirs, 1)                # Set SDA as output
    set(pins, 0)                   # Pull SDA low for ACK
    wait(1, gpio, SCL_PIN)         # Wait for SCL high
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    set(pindirs, 0)                # Set SDA as input again
    
    # Store the received byte
    push()                         # Push received byte to CPU
    jmp("wait_for_stop")           # Done writing, wait for STOP
    
    # ---- Master reading from us ----
    label("read_from_master")
    pull()                         # Get byte to send from CPU
    
    # Send 8 bits
    set(x, 7)                      # 8 bits to send
    label("send_data_loop")
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    out(pindirs, 1)                # Set as output if bit is 0
    set(pindirs, 0) [7]            # Always set back to input (pull-up will make it high)
    wait(1, gpio, SCL_PIN)         # Wait for SCL high
    jmp(x_dec, "send_data_loop")   # Loop until all bits sent
    
    # Read ACK/NACK from master
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    set(pindirs, 0)                # Ensure SDA is input
    wait(1, gpio, SCL_PIN)         # Wait for SCL high
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    
    # Wait for STOP condition
    label("wait_for_stop")
    wait(0, gpio, SCL_PIN)         # Wait for SCL low
    wait(1, gpio, SCL_PIN)         # Wait for SCL high
    wait(1, gpio, SDA_PIN)         # Wait for SDA high (STOP condition)
    
    # Go back to waiting for START
    label("wait_for_start")
    jmp("wait_for_start")

def main():
    global stored_byte
    
    # Setup pins
    sda = Pin(SDA_PIN, Pin.IN, Pin.PULL_UP)
    scl = Pin(SCL_PIN, Pin.IN, Pin.PULL_UP)
    
    # Create a state machine with the I2C slave program
    sm = rp2.StateMachine(0, i2c_slave_pio, freq=100000, in_base=sda, out_base=sda, set_base=sda)
    
    # Start the state machine
    sm.active(1)
    
    # Create a timer to check for new data periodically
    last_byte = stored_byte
    
    print(f"I2C slave initialized at address 0x{SLAVE_ADDR:02X}")
    print(f"Listening on SDA=GPIO{SDA_PIN}, SCL=GPIO{SCL_PIN}")
    
    try:
        while True:
            # Check if there's data in the FIFO (master wrote to us)
            if sm.rx_fifo():
                # Read the byte from the FIFO
                received = sm.get()
                stored_byte = received
                print(f"Received: {stored_byte}")
                
                # Calculate the response (byte * 2)
                response = (stored_byte * 2) & 0xFF
                
                # Put the response in the TX FIFO for when master reads
                sm.put(response)
                print(f"Prepared response: {response}")
            
            # Sleep to avoid busy waiting
            utime.sleep_ms(10)
            
    except KeyboardInterrupt:
        print("Program terminated")
        sm.active(0)

if __name__ == "__main__":
    main()