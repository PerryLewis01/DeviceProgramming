from machine import Pin as pin
import time

led = pin(25, pin.OUT)

def blink_led(delay):
    
    led.on()
    time.sleep(delay)
    led.off()
    time.sleep(delay)
    
while True:
    blink_led(0.5)