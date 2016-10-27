.origin 0
.entrypoint START

.struct dataLoader
	.u32	RunFlag
	.u32	Delay
	.u32	MaxSize
	.u32	DataReady
	.u32	Data
.ends

// Define Base address for memory
#define	PRU_DATA1_BASE				0x00000000
#define	PRU_DATA0_BASE				0x00002000
#define	PRU_Shared_BASE				0x00010000

// Define Interrupt
#define PRU0_PRU1_INTERRUPT     	0x11
#define PRU1_PRU0_INTERRUPT     	0x12
#define PRU0_ARM_INTERRUPT      	0x13
#define PRU1_ARM_INTERRUPT      	0x14
#define ARM_PRU0_INTERRUPT      	0x15
#define ARM_PRU1_INTERRUPT      	0x16

#define PRU_RAM_ADDRESS				R15
#define BLOCK1_BASE_ADDRESS			R16
#define BLOCK2_BASE_ADDRESS			R17
#define BLOCK_SIZE					R18
#define DATA_ADDRESS				R19
#define DATA						R20
#define BLOCK_COUNT					R21
#define DELAY_REG					R22
#define RATE_REG					R23
#define OUTPUT_REG					R24

// Define SPI Pins
#define FS							R30.T2		// Chip Selection 		P8.43 - Output
#define SCLK						R30.T3		// Serial Clock 		P8.44 - Output
#define MISO						R31.T1		// Master In Slave Out 	P8.46 - Input
#define MOSI						R30.T0 		// Master Out Slave In 	P8.45 - Output
#define CONVST						R30.T4		// Conversion Start 	P8.41 - Output
#define EOC							R31.T5		// End of Conversion	P9.42 - Input

START:
	LDI		PRU_RAM_ADDRESS, PRU1_DATA_BASE
	ADD		BLOCK1_BASE_ADDRESS, PRU_RAM_ADDRESS, OFFSET(dataLoader.Data)
	MOV		BLOCK2_BASE_ADDRESS, PRU_SHARED_BASE
	MOV		DATA_ADDRESS, BLOCK1_BASE_ADDRESS
	LBBO	BLOCK_SIZE, PRU_RAM_ADDRESS, OFFSET(dataLoader.MaxSize), 4
	LDI		BLOCK_COUNT, 0

	SET		FS
	SET		CONVST
	CLR		MOSI
	SET		SCLK

SETUP:
	LDI		OUTPUT_REG, 0b1011111111110111
	JMP		MAIN_LOOP

SAMPLING:
	LDI		OUTPUT_REG, 0b1011
	CLR		CONVST 						// Signal conversion start
	WBS		EOC    						// Wait for EOC to pull high
	SET		CONVST						// Reset CONVST

MAIN_LOOP:
	CLR		FS
	AND		DATA, DATA, 0
	LDI		R0, 17

WRITE_DATA:
	SUB		R0, R0, 1					// Next Bit
	QBEQ	DATA_COMPLETE, R0, 0 		// Check to see if 16 bits are written/read already
	QBBS	SDO_HIGH, OUTPUT_REG, 0

	SDO_LOW:
		CLR		MOSI
		JMP		READ_DATA
	SDO_HIGH:
		SET		MOSI

READ_DATA:
	LSR		OUTPUT_REG, OUTPUT_REG, 1
	CLR		SCLK
	ADD		R0, R0, 0					// Time (SCLKF - SDOVALID)
	ADD		R0, R0, 0
	ADD		R0, R0, 0

	QBBC	LEFT_SHIFT, MISO			// Main data reading portion.1
	SET		DATA.T0						// Main data reading portion.2
	LEFT_SHIFT:							// Main data reading portion.3
		LSL		DATA, DATA, 1			// Main data reading portion.4

	LDI		DELAY_REG, 9				// Delay for 9 Cycle,  total 15 Cycles LOW

	DELAY_LOOP:
		SUB		DELAY_REG, DELAY_REG, 1
		QBLT	DELAY_LOOP, DELAY_REG, 0

	QBBS	WRITE_DATA, SCLK			// Going back to Write Data if SCLK is High
	SET		SCLK						// Rising Edge if SCLK is low
	LDI		DELAY_REG, 9				// Delay for 9 Cycle,  total 15 Cycles HIGH
	JMP		DELAY_LOOP					// Back to the Loop as SCLK High

DATA_COMPLETE:
	SET		FS							// De-select ADS8329
	SBBO	DATA, DATA_ADDRESS, 0, 2	// Store 2 bytes of data (16-bit)
	ADD 	DATA_ADDRESS, DATA_ADDRESS, 2			// Increment data pointer by 2 bytes

	CLR		MOSI						// Reset Initial Condition.1
	SET		SCLK						// Reset Initial Condition.2

	ADD		R1, BLOCK1_BASE_ADDRESS, BLOCK_SIZE		// Check to see if the blocks are filled or not.1
	QBEQ	TRIGGER, DATA_ADDRESS, R1				// Check to see if the blocks are filled or not.2
	ADD		R1, BLOCK2_BASE_ADDRESS, BLOCK_SIZE		// Check to see if the blocks are filled or not.3
	QBEQ	TRIGGER, DATA_ADDRESS, R1				// Check to see if the blocks are filled or not.4

CHECK:
	LBBO	R0, PRU_RAM_ADDRESS, OFFSET(dataLoader.RunFlag), 4
	QBEQ	EXIT, R0, 0
	LBBO	DELAY_REG, PRU_RAM_ADDRESS, OFFSET(dataLoader.Delay), 4

INTERSAMPLE_DELAY:						// Intersample Delay will determine the sampling rate in general
	SUB		DELAY_REG, DELAY_REG, 1
	QBLT	INTERSAMPLE_DELAY, DELAY_REG, 0
	JMP		SAMPLING

TRIGGER:
	ADD		BLOCK_COUNT, BLOCK_COUNT, 1
	SBBO	BLOCK_COUNT, PRU_RAM_ADDRESS, OFFSET(dataLoader.DataReady), 4
	QBBC	RESET_ADDRESS, BLOCK_COUNT, 0
	MOV		DATA_ADDRESS, BLOCK2_BASE_ADDRESS
	JMP		CHECK

RESET_ADDRESS:
	MOV		DATA_ADDRESS, BLOCK1_BASE_ADDRESS
	JMP		CHECK

EXIT:
	MOV		R31.b0, PRU1_ARM_INTERRUPT + 16
	HALT
