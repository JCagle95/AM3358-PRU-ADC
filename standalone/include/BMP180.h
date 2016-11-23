#ifndef BMP180_H_
#define BMP180_H_

#include <stdint.h>

typedef unsigned char byte;

#define I2C_NO_VALUE					(0x00)
#define BMP085_ADDRESS                	(0x77)

enum {
	BMP085_REGISTER_CAL_AC1            = 0xAA,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC2            = 0xAC,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC3            = 0xAE,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC4            = 0xB0,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC5            = 0xB2,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_AC6            = 0xB4,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_B1             = 0xB6,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_B2             = 0xB8,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_MB             = 0xBA,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_MC             = 0xBC,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CAL_MD             = 0xBE,  // R   Calibration data (16 bits)
	BMP085_REGISTER_CHIPID             = 0xD0,
	BMP085_REGISTER_VERSION            = 0xD1,
	BMP085_REGISTER_SOFTRESET          = 0xE0,
	BMP085_REGISTER_CONTROL            = 0xF4,
	BMP085_REGISTER_TEMPDATA           = 0xF6,
	BMP085_REGISTER_PRESSUREDATA       = 0xF6,
	BMP085_REGISTER_READTEMPCMD        = 0x2E,
	BMP085_REGISTER_READPRESSURECMD    = 0x34
};

typedef enum {
	BMP085_MODE_ULTRALOWPOWER          = 0,
	BMP085_MODE_STANDARD               = 1,
	BMP085_MODE_HIGHRES                = 2,
	BMP085_MODE_ULTRAHIGHRES           = 3
} bmp085_mode_t;

typedef struct {
	int16_t  ac1;
	int16_t  ac2;
	int16_t  ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t  b1;
	int16_t  b2;
	int16_t  mb;
	int16_t  mc;
	int16_t  md;
}BMP180_calib_data;

class BMP180 {
public:
	BMP180(int bus_interface, bmp085_mode_t bmp085_mode);
	void getTemperature(float *temperature);
	void getPressure(float *pressure);
private:
	BMP180_calib_data bmp180_coe;
	bmp085_mode_t BMP_Mode;
	int interface;
	bool writeCommand(byte reg, byte value);
	bool read8(byte reg, uint8_t *value);
	bool read16(byte reg, uint16_t *value);
	bool readBuf(byte reg, uint8_t *output, int len);
	bool readS16(byte reg, int16_t *value);
	int32_t computeB5(int32_t ut);
	void readCoefficients();
	void readRawTemperature(int32_t *temperature);
	void readRawPressure(int32_t *pressure);
};

#endif /* BMP180_H_ */
