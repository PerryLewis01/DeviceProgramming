"""
I2C Bit-Banging Master Implementation for RP2040 in MicroPython

This class implements a software I2C master using GPIO pins (bit-banging).
It follows the I2C protocol specification with proper timing considerations.
"""
from machine import Pin
import time

class I2CBitBang:
    def __init__(self, scl_pin, sda_pin, freq=100000):
        """
        Initialize the I2C bit-banging master.
        
        Args:
            scl_pin: GPIO pin number for SCL (clock)
            sda_pin: GPIO pin number for SDA (data)
            freq: Desired I2C frequency in Hz (default 100kHz)
        """
        # Create pin objects with open-drain configuration
        self.scl = Pin(scl_pin, Pin.OPEN_DRAIN, Pin.PULL_UP)
        self.sda = Pin(sda_pin, Pin.OPEN_DRAIN, Pin.PULL_UP)
        
        # Set pins to HIGH by default (idle state)
        self.scl.value(1)
        self.sda.value(1)
        
        # Calculate delay time based on frequency
        # Using 4 transitions per bit for simple timing approximation
        self.delay = 1 / (freq * 4)
        
        # Short delay after initialization to ensure bus is stable
        time.sleep_us(10)
    
    def _delay_half_cycle(self):
        """Delay for half an I2C clock cycle."""
        time.sleep(self.delay)
    
    def _start_condition(self):
        """
        Generate I2C start condition:
        SDA goes from HIGH to LOW while SCL is HIGH
        """
        # Ensure both lines are HIGH (idle)
        self.sda.value(1)
        self.scl.value(1)
        self._delay_half_cycle()
        
        # Pull SDA LOW while SCL is HIGH
        self.sda.value(0)
        self._delay_half_cycle()
        
        # Pull SCL LOW (ready for first bit)
        self.scl.value(0)
        self._delay_half_cycle()
    
    def _stop_condition(self):
        """
        Generate I2C stop condition:
        SDA goes from LOW to HIGH while SCL is HIGH
        """
        # Ensure SDA is LOW and SCL is LOW
        self.sda.value(0)
        self.scl.value(0)
        self._delay_half_cycle()
        
        # Bring SCL HIGH
        self.scl.value(1)
        self._delay_half_cycle()
        
        # Bring SDA HIGH while SCL is HIGH (stop condition)
        self.sda.value(1)
        self._delay_half_cycle()
    
    def _write_bit(self, bit):
        """
        Write a single bit to the I2C bus.
        
        Args:
            bit: 0 or 1 to write to the bus
            
        Returns:
            None
        """
        # Set SDA to bit value while SCL is LOW
        self.scl.value(0)
        self.sda.value(bit)
        self._delay_half_cycle()
        
        # Toggle SCL HIGH - data is valid during this period
        self.scl.value(1)
        self._delay_half_cycle()
        
        # Toggle SCL back to LOW
        self.scl.value(0)
        self._delay_half_cycle()
    
    def _read_bit(self):
        """
        Read a single bit from the I2C bus.
        
        Returns:
            Bit value read (0 or 1)
        """
        # Release SDA to allow slave to control it
        self.scl.value(0)
        self.sda.value(1)  # Release SDA (pull-up will keep it HIGH if not pulled LOW)
        self._delay_half_cycle()
        
        # Toggle SCL HIGH - data is valid during this period
        self.scl.value(1)
        self._delay_half_cycle()
        
        # Read the bit value
        bit = self.sda.value()
        
        # Toggle SCL back to LOW
        self.scl.value(0)
        self._delay_half_cycle()
        
        return bit
    
    def _write_byte(self, byte):
        """
        Write a byte to the I2C bus and get acknowledgment.
        
        Args:
            byte: The byte to write (0-255)
            
        Returns:
            True if ACK received, False if NACK received
        """
        # Send each bit, MSB first
        for i in range(7, -1, -1):
            self._write_bit((byte >> i) & 0x01)
        
        # Read acknowledgment bit (ACK/NACK)
        # ACK = 0, NACK = 1
        ack = self._read_bit()
        
        return not ack  # Return True for ACK, False for NACK
    
    def _read_byte(self, ack=True):
        """
        Read a byte from the I2C bus.
        
        Args:
            ack: Whether to send ACK (True) or NACK (False) after reading
            
        Returns:
            The byte read from the bus (0-255)
        """
        byte = 0
        
        # Read 8 bits, MSB first
        for i in range(8):
            bit = self._read_bit()
            byte = (byte << 1) | bit
        
        # Send acknowledgment bit
        # ACK = 0, NACK = 1
        self._write_bit(0 if ack else 1)
        
        return byte
    
    def write(self, addr, data, stop=True):
        """
        Write data to a specific I2C device address.
        
        Args:
            addr: 7-bit I2C device address
            data: Single byte or list/bytes of data to write
            stop: Whether to generate a stop condition after writing
            
        Returns:
            True if successful (all bytes ACKed), False otherwise
        """
        # Convert single byte to list
        if isinstance(data, int):
            data = [data]
        
        # Start condition
        self._start_condition()
        
        # Send device address with write bit (0)
        if not self._write_byte((addr << 1) & 0xFE):
            self._stop_condition()
            return False  # No ACK received
        
        # Send data bytes
        for byte in data:
            if not self._write_byte(byte):
                self._stop_condition()
                return False  # No ACK received
        
        # Stop condition if requested
        if stop:
            self._stop_condition()
        
        return True  # All data sent and ACKed
    
    def read(self, addr, count=1):
        """
        Read data from a specific I2C device address.
        
        Args:
            addr: 7-bit I2C device address
            count: Number of bytes to read
            
        Returns:
            List of bytes read, or None if error
        """
        # Start condition
        self._start_condition()
        
        # Send device address with read bit (1)
        if not self._write_byte((addr << 1) | 0x01):
            self._stop_condition()
            return None  # No ACK received
        
        # Read requested number of bytes
        result = []
        for i in range(count):
            # Send ACK for all bytes except the last one
            ack = (i < count - 1)
            result.append(self._read_byte(ack))
        
        # Stop condition
        self._stop_condition()
        
        return result if count > 1 else result[0]
    
    def write_then_read(self, addr, write_data, read_count):
        """
        Write data to a device, then read from it - common I2C operation.
        
        Args:
            addr: 7-bit I2C device address
            write_data: Data to write (register address typically)
            read_count: Number of bytes to read
            
        Returns:
            Data read from device, or None if error
        """
        # Write phase without stop condition
        if not self.write(addr, write_data, stop=False):
            return None
            
        # Read phase (which generates stop condition)
        return self.read(addr, read_count)
    
    def scan(self):
        """
        Scan the I2C bus for connected devices.
        
        Returns:
            List of addresses that responded
        """
        found_devices = []
        
        for addr in range(0x08, 0x78):  # Standard addressable range
            # Start condition
            self._start_condition()
            
            # Try to address the device (write mode)
            if self._write_byte((addr << 1) & 0xFE):
                found_devices.append(addr)
            
            # Stop condition
            self._stop_condition()
        
        return found_devices


# Example usage:
if __name__ == "__main__":
    # Initialize I2C bit-banging on pins 0 (SCL) and 1 (SDA)
    i2c = I2CBitBang(0, 1)
    
    # Scan for devices
    devices = i2c.scan()
    print(f"Found {len(devices)} devices: {[hex(d) for d in devices]}")
    
    # Example: Write to a device at address 0x23
    # i2c.write(0x23, [0x10, 0x20])  # Write two bytes
    
    # Example: Read from a device at address 0x23
    # data = i2c.read(0x23, 2)  # Read two bytes
    
    # Example: Write then read (common pattern for register access)
    # Register 0x42, read 2 bytes
    # data = i2c.write_then_read(0x23, 0x42, 2)