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

MCP4725::MCP4725(int bus_interface){
    interface = bus_interface;
}

void MCP4725::setVoltage(uint16_t Voltage, bool EEPROM){
    if (ioctl(interface,I2C_SLAVE, (uint8_t)MCP4725_DEFAULT_ADDRESS) < 0) {
		printf("MCP4725 :: Failed to acquire bus access and/or talk to slave.\n");
		return;
	}

    char buf[3];

    if (EEPROM) {
        buf[0] = MCP4725_CMD_WRITEDACEEPROM;
    } else {
        buf[0] = MCP4725_CMD_WRITEDAC;
    }
    buf[1] = Voltage / 16;
    buf[2] = (Voltage % 16) << 4;

    write(interface, buf, 3);
}
