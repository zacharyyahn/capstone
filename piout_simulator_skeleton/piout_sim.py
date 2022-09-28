import serial
import time
import sys

# Check for command line arg format
if (len(sys.argv) != 3):
    print("Usage: python piout_sim <filename> <COM#>")
    exit()

# Opens testfile to be simulated
file = open(sys.argv[1], 'r')

# Opens serial port (usually COM4) at baud rate of 9600
msp432 = serial.Serial(sys.argv[2], 9600)

# Loops through file character by character and writes
while 1:
    char = file.read(1)
    if not char:
        break

    msp432.write(char.encode())

    # Simulate image processing time on newline (30Hz)
    if (char == '\n'):
        time.sleep(1/30)





