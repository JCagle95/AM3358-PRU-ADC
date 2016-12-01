#include <stdint.h>

#define MCP4725_CMD_WRITEDAC            (0x40)  // Writes data to the DAC
#define MCP4725_CMD_WRITEDACEEPROM      (0x60)  // Writes data to the DAC and the EEPROM (persisting the assigned value after reset)
#define MCP4725_DEFAULT_ADDRESS         (0x62)	// The default I2C Chip address
#define MCP4725_EXTENDED_ADDRESS        (0x63)	// Only use if A0 is loaded

class MCP4725 {
public:

	// initializer
	MCP4725(int bus_interface);
	bool setVoltage(uint16_t Voltage, bool EEPROM);
private:
	// Store the I2C Linux Kernel handler
	int interface;
};
