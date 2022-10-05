=== Simulator Source Files ===

--- Purpose--- 

Simulate the serial output from the Raspberry pi to be 
received by the MSP.


---Usage---

MC Side

The main.c and UART.h/c files are files are part of a Code 
Compser Studio project (other files removed to reduce clutter). 
When run on the MSP, it configures the Serial Communication 
Interface to interrupt on receiving a character over UART. The
interrupt handler saves this character to a buffer, and separates
by line.

Laptop Side

Also included is a python script to be run in format:
python piout_sim.py <file.txt> <COM#>
The script will send the contents of the file character by character
over the specified COM port.

For any platform, run the python script list_serial_ports.py to 
print out the current serial ports. This is especially helpful 
on non-windows systems because instead of COM#, you will need to
provide the full device name like /dev/tty.usbmodemM43210051

---Verification---

The simulator was tested with the included testfile, and the contents
were successfully received by the MSP. This can be seen in the 
verification screenshot showing buffer contents after running the simulator.