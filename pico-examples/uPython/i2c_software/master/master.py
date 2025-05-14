from machine import Pin
import time

class I2CMasterBitBang:
    def __init__(self, scl_pin, sda_pin, freq_hz=100000):
        """
        Initialise the I2C master bit-banging device

        scl_pin: GPIO pin number for SCL (clock)
        sda_pin: GPIO pin number for SDA (data)
        freq_hz: Frequency of I2C bus in Hz
        """

        # Setup Pins in the open-drain configuration
        self.scl = Pin(scl_pin, Pin.OPEN_DRAIN, Pin.PULL_UP)
        self.sda = Pin(sda_pin, Pin.OPEN_DRAIN, Pin.PULL_UP)

        self.scl.value(1)
        self.sda.value(1)

        # Calculate frequency delay
        # assuming 4 delays per cycle
        self.delay = 1 / (freq_hz * 4)

        time.sleep_us(10)

    def _delay_quarter_cycle(self):
        time.sleep(self.delay)
    
    def _start_condition(self):
        # Start condition SDA High -> Low while SCL High

        self.sda.value(1)
        self.scl.value(1)
        self._delay_quarter_cycle()

        self.sda.value(0)
        self._delay_quarter_cycle()

        self.scl.value(0)
        self._delay_quarter_cycle()

    def _stop_condition(self):
        # Stop condition SCL Low -> High while SCL Low
        self.sda.value(0)
        self.scl.value(0)
        self._delay_quarter_cycle()

        self.sda.value(1)
        self._delay_quarter_cycle()

        self.scl.value(1)
        self._delay_quarter_cycle()

    def _write_bit(self, bit): 
        # Set SDA to bit value while SCL is LOW
        self.scl.value(0)
        self.sda.value(bit)
        self._delay_quarter_cycle()
        
        # Toggle SCL HIGH - data is read during this period
        self.scl.value(1)
        self._delay_quarter_cycle()
        
        # Toggle SCL back to LOW
        self.scl.value(0)
        self._delay_quarter_cycle()

    def _read_bit(self) -> int:

        self.scl.value(0)
        self.sda.value(1) # release SDA (in open drain pull-up will keep it high unless eternally pulled)
        self._delay_quarter_cycle()

        self.scl.value(1)
        self._delay_quarter_cycle()

        bit = self.sda.value()

        self.scl.value(0)
        self._delay_quarter_cycle()

        return bit

    def _write_byte(self, byte) -> bool:
        
        # Write 8 bts MSB first
        for i in range(7, -1, -1):
            self._write_bit((byte >> i) & 0x01)
        
        ack = self._read_bit()

        return not ack

    def _read_byte(self, ack=True) -> int:

        byte : int = 0

        #Read 8 bits MSB first
        for i in range(8):
            bit = self._read_bit()
            byte = (byte << 1) | bit

        self._write_but(0 if ack else 1)

        return byte
    
    def write(self, addr, data, stop=True):

        # make data array like
        if isinstance(data, int):
            data = [data]

        self._start_condition()

        # Write address and write bit (0)
        if not self._write_bit((addr << 1) & 0xFE):
            self._stop_condition()
            return False # Slave failed to ack

        # Send data bytes
        for bute in data:
            if not self._write_byte(byte):
                self._stop_condition()
                return False # Slave failed to ack
            
        if stop:
            self._stop_condition()
        
        return True # Data sent and Acknowledged
    
    def read(self, addr, count=1):

        self._start_condition()

        # Write address and read bit (0)
        if not self._write_byte((addr << 1 | 0x01)):
            self._stop_condition()
            return None
        
        result = []
        for i in range(count):
            # Send ACk for all bytes except last
            ack = (i < count - 1)
            result.append(self._read_byte(ack))

        self._stop_condition()

        return result if count > 1 else result[0]
    
    def scan(self):
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
    
if __name__ == "__main__":
    i2c = I2CMasterBitBang(0, 1)

    devices = i2c.scan()
    print(f"Found {len(devices)} devices: {[hex(d) for d in devices]}")