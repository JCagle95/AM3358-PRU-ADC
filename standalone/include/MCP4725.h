#include <stdint.h>

#define MCP4725_CMD_WRITEDAC            (0x40)  // Writes data to the DAC
#define MCP4725_CMD_WRITEDACEEPROM      (0x60)  // Writes data to the DAC and the EEPROM (persisting the assigned value after reset)
#define MCP4725_DEFAULT_ADDRESS         (0x62)
#define MCP4725_EXTENDED_ADDRESS        (0x63)

class MCP4725 {
public:
	MCP4725(int bus_interface);
	void setVoltage(uint16_t Voltage, bool EEPROM);
private:
	int interface;
};
