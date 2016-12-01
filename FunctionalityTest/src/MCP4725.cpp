#include "MCP4725.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Store the I2C handler for use.
MCP4725::MCP4725(int bus_interface){
    interface = bus_interface;
}

// set voltage in 12-bit range
// bool EEPROM determine if the setted voltage is going to be the default output at startup next time (typically not used)
bool MCP4725::setVoltage(uint16_t Voltage, bool EEPROM){

    // ioctl will try to establish connection with MCP4725
    if (ioctl(interface,I2C_SLAVE, (uint8_t)MCP4725_DEFAULT_ADDRESS) < 0) {
		printf("MCP4725 :: Failed to acquire bus access and/or talk to slave.\n");
		return false;
	}

    // Three bytes to send
    char buf[3];
    if (EEPROM) {
        buf[0] = MCP4725_CMD_WRITEDACEEPROM;
    } else {
        buf[0] = MCP4725_CMD_WRITEDAC;
    }
    buf[1] = Voltage / 16;
    buf[2] = (Voltage % 16) << 4;

    // Write to I2C
    write(interface, buf, 3);
    return true;
}
