#include "BMP180.h"
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

BMP180::BMP180(int bus_interface, bmp085_mode_t bmp085_mode){
	interface = bus_interface;
	if (ioctl(interface,I2C_SLAVE,(uint8_t)BMP085_ADDRESS) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		/* ERROR HANDLING; you can check errno to see what went wrong */
		return;
	}
	printf("I2C Initialization Success\n");

	uint8_t id;
	read8(BMP085_REGISTER_CHIPID, &id);
	if(id != 0x55){
		printf("BMP180 Chip ID mismatched, Error in chip\n");
		return;
	}
	BMP_Mode = bmp085_mode;
	readCoefficients();
	return;
}

bool BMP180::readS16(byte reg, int16_t *value){
  uint16_t i;
  if (!read16(reg, &i)){
	  return false;
  }
  *value = (int16_t)i;
  return true;
}

int32_t BMP180::computeB5(int32_t ut) {
	int32_t X1 = (ut - (int32_t)bmp180_coe.ac6) * ((int32_t)bmp180_coe.ac5) >> 15;
	int32_t X2 = ((int32_t)bmp180_coe.mc << 11) / (X1+(int32_t)bmp180_coe.md);
	return X1 + X2;
}

void BMP180::readCoefficients(void){
	readS16(BMP085_REGISTER_CAL_AC1, &bmp180_coe.ac1);
	readS16(BMP085_REGISTER_CAL_AC2, &bmp180_coe.ac2);
	readS16(BMP085_REGISTER_CAL_AC3, &bmp180_coe.ac3);
	read16(BMP085_REGISTER_CAL_AC4, &bmp180_coe.ac4);
	read16(BMP085_REGISTER_CAL_AC5, &bmp180_coe.ac5);
	read16(BMP085_REGISTER_CAL_AC6, &bmp180_coe.ac6);
	readS16(BMP085_REGISTER_CAL_B1, &bmp180_coe.b1);
	readS16(BMP085_REGISTER_CAL_B2, &bmp180_coe.b2);
	readS16(BMP085_REGISTER_CAL_MB, &bmp180_coe.mb);
	readS16(BMP085_REGISTER_CAL_MC, &bmp180_coe.mc);
	readS16(BMP085_REGISTER_CAL_MD, &bmp180_coe.md);
}

void BMP180::readRawTemperature(int32_t *temperature){
	uint16_t t;
	writeCommand(BMP085_REGISTER_CONTROL, BMP085_REGISTER_READTEMPCMD);
	usleep(10000);
	read16(BMP085_REGISTER_TEMPDATA, &t);
	*temperature = t;
}

void BMP180::readRawPressure(int32_t *pressure){
	uint8_t  p8;
	uint16_t p16;
	int32_t  p32;
	writeCommand(BMP085_REGISTER_CONTROL, BMP085_REGISTER_READPRESSURECMD + (BMP_Mode << 6));
	switch(BMP_Mode){
		case BMP085_MODE_ULTRALOWPOWER:
			usleep(5000);
			break;
		case BMP085_MODE_STANDARD:
			usleep(8000);
			break;
		case BMP085_MODE_HIGHRES:
			usleep(14000);
			break;
		case BMP085_MODE_ULTRAHIGHRES:
		default:
			usleep(26000);
			break;
	}
	read16(BMP085_REGISTER_PRESSUREDATA, &p16);
	p32 = (uint32_t)p16 << 8;
	read8(BMP085_REGISTER_PRESSUREDATA+2, &p8);
	p32 += p8;
	p32 >>= (8 - BMP_Mode);
	*pressure = p32;
}

void BMP180::getTemperature(float *temperature){
	if (ioctl(interface,I2C_SLAVE,(uint8_t)BMP085_ADDRESS) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		/* ERROR HANDLING; you can check errno to see what went wrong */
		return;
	}

	int32_t rawTemp;
	float Temp;
	readRawTemperature(&rawTemp);
	int32_t B5;
	B5 = computeB5(rawTemp);
	Temp = (float)((B5+8) >> 4);
	Temp /= 10.0;
	*temperature = Temp;

	//printf("rawTemp = %d",rawTemp);
	//printf("rawTemp = %d\t",Temp);
}

void BMP180::getPressure(float *pressure){
	if (ioctl(interface,I2C_SLAVE,(uint8_t)BMP085_ADDRESS) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		/* ERROR HANDLING; you can check errno to see what went wrong */
		return;
	}

	int32_t rawTemp = 0, rawPres = 0, compp = 0;
	int32_t  x1, x2, b5, b6, x3, b3, p;
	uint32_t b4, b7;

	readRawTemperature(&rawTemp);
	readRawPressure(&rawPres);
	b5 = computeB5(rawTemp);

	/* Pressure compensation */
	b6 = b5 - 4000;
	x1 = (bmp180_coe.b2 * ((b6 * b6) >> 12)) >> 11;
	x2 = (bmp180_coe.ac2 * b6) >> 11;
	x3 = x1 + x2;
	b3 = (((((int32_t) bmp180_coe.ac1) * 4 + x3) << BMP_Mode) + 2) >> 2;
	x1 = (bmp180_coe.ac3 * b6) >> 13;
	x2 = (bmp180_coe.b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (bmp180_coe.ac4 * (uint32_t) (x3 + 32768)) >> 15;
	b7 = ((uint32_t) (rawPres - b3) * (50000 >> BMP_Mode));

	if (b7 < 0x80000000){
		p = (b7 << 1) / b4;
	} else {
		p = (b7 / b4) << 1;
	}

	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	compp = p + ((x1 + x2 + 3791) >> 4);

	/* Assign compensated pressure value */
	*pressure = compp;
}

bool BMP180::writeCommand(byte reg, byte value){

	if (value == I2C_NO_VALUE){
		if (write(interface,(char*)&reg,1) != 1){
			return false;
		}
	} else {
		char buf[2] = {reg, value};
		if (write(interface,buf,2) != 2){
			return false;
		}
	}
	return true;
}

bool BMP180::read8(byte reg, uint8_t *value){
	char buf[1] = {reg};

	if (write(interface,(char*)&reg,1) != 1){
		return false;
	}

	if (read(interface,buf,1) != 1){
		return false;
	} else {
		*value = (uint8_t)buf[0];
		return true;
	}
}

bool BMP180::read16(byte reg, uint16_t *value){
	char buf[2] = {0};

	if (write(interface,(char*)&reg,1) != 1){
		return false;
	}

	if (read(interface,buf,2) != 2){
		return false;
	} else {
		*value = (uint16_t)((buf[0]<<8)+buf[1]);
		return true;
	}
}

bool BMP180::readBuf(byte reg, uint8_t *output, int len){
	char buf[len] = {0};
	if (write(interface,(char*)&reg,1) != 1){
		return false;
	}

	if (read(interface,buf,len) != len){
		return false;
	} else {
		memcpy(output,buf,len);
		return true;
	}
}
