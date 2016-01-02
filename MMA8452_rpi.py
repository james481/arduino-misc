#!/usr/bin/python

############################################################
# Raspberry Pi MMA8452Q Accelerometer I2C
#
# Log movement events above 1G in the Z axis to a log file
#
# Requires enabling I2C repeated start command support
# in kernel driver:
#
# echo -n 1 > /sys/module/i2c_bcm2708/parameters/combined
#
############################################################

from smbus import SMBus
import time
import logging
import RPi.GPIO as GPIO

# Log file
LOGFILE = './1g_events.log'

# Device I2C Address
ADDR = 0x1D

# Interrupt Pins
INTERRUPT_1 = 14
INTERRUPT_2 = 4

# Registers
STATUS = 0x00
OUT_X_MSB = 0x01
OUT_X_LSB = 0x02
OUT_Y_MSB = 0x03
OUT_Y_LSB = 0x04
OUT_Z_MSB = 0x05
OUT_Z_LSB = 0x06
SYSMOD = 0x0B
INT_SOURCE = 0x0C
WHO_AM_I = 0x0D
XYZ_DATA_CFG = 0x0E
HP_FILTER_CUTOFF = 0x0F
TRANSIENT_CFG = 0x1D
TRANSIENT_SRC = 0x1E
TRANSIENT_THS = 0x1F
TRANSIENT_COUNT = 0x20
CTRL_REG1 = 0x2A
CTRL_REG2 = 0x2B
CTRL_REG3 = 0x2C
CTRL_REG4 = 0x2D
CTRL_REG5 = 0x2E

# Setup Interrupt Pins
GPIO.setmode(GPIO.BCM)
GPIO.setup(INTERRUPT_1, GPIO.IN, pull_up_down=GPIO.PUD_UP);
GPIO.setup(INTERRUPT_2, GPIO.IN, pull_up_down=GPIO.PUD_UP);

# Create Bus / Setup Logging
bus = SMBus(1)
logging.basicConfig(
    filename=LOGFILE,
    level=logging.DEBUG,
    format='%(asctime)s : %(message)s'
)

# Check who am i = 0x2A
who_am_i = bus.read_byte_data(ADDR, WHO_AM_I)
if who_am_i != 0x2A:
    print "Device not active."
    exit(1)

# Set Standby mode for setting modes /
# 100 Hz Output Data Rate / 6.25 Hz Sleep Data Rate
bus.write_byte_data(ADDR, CTRL_REG1, 0x98)

# Set 8G Scale
cfg = bus.read_byte_data(ADDR, XYZ_DATA_CFG)
# Mask Scale bits
cfg &= 0xFC
cfg |= 0x02
bus.write_byte_data(ADDR, XYZ_DATA_CFG, cfg);

# Set Transient Event Detection on Z (0b100)
bus.write_byte_data(ADDR, TRANSIENT_CFG, 0x04)

# Set Transient Event Threshold to 1G - (1/0.063 = 15 = 0x0F)
bus.write_byte_data(ADDR, TRANSIENT_THS, 0x0F)

# Disable Transient Debounce
bus.write_byte_data(ADDR, TRANSIENT_COUNT, 0x0)

# Set Transient Interrupt Enabled, point to INTERRUPT_1
bus.write_byte_data(ADDR, CTRL_REG4, 0x20)
bus.write_byte_data(ADDR, CTRL_REG5, 0x20)

# Set Active (Ready) Mode
ready = bus.read_byte_data(ADDR, CTRL_REG1);
bus.write_byte_data(ADDR, CTRL_REG1, ready | 0x01)

# Wait for an Interrupt event
def interrupt1(channel):
    logging.warning('1g+ Z axis movement detected')
    if (bus.read_byte_data(ADDR, STATUS) & 0x08) >> 3:
        rawData = bus.read_i2c_block_data(ADDR, OUT_X_MSB, 6)
        z = ((rawData[4]<<8 | rawData[5])) >> 4
        cz = float(z / float(1<<11)) * 2.0
        logging.warning("z: %.4f cz: %.4f" % (z, cz))
    return

GPIO.add_event_detect(INTERRUPT_1, GPIO.FALLING, callback=interrupt1);

logging.info("Waiting for movement events.")

# Do Nothing / wait for interrupts forever
while True:
    time.sleep(1)
