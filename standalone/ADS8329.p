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
#define	PRU1_DATA_BASE				0x00000000
#define	PRU0_DATA_BASE				0x00002000
#define	PRU_SHARED_BASE				0x00010000

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
	LDI		PRU_RAM_ADDRESS, PRU1_DATA_BASE										// Store PRU1's base address in PRU_RAM_ADDRESS
	ADD		BLOCK1_BASE_ADDRESS, PRU_RAM_ADDRESS, OFFSET(dataLoader.Data)		// The first data block start at base address with certain offset due to status bytes in struct.
	MOV		BLOCK2_BASE_ADDRESS, PRU_SHARED_BASE								// The second data block has no offset because struct only present at the beginning.
	MOV		DATA_ADDRESS, BLOCK1_BASE_ADDRESS									// Start our recording at the initial pointer of first data block.
	LBBO	BLOCK_SIZE, PRU_RAM_ADDRESS, OFFSET(dataLoader.MaxSize), 4			// Determine the size of each data block (This is set in C-cdoe, in unit of [bytes])
	LDI		BLOCK_COUNT, 0														// Block Count is a simple counter that signal C-code when one data block is ready to be read.

	SET		FS			// Initialization.1
	SET		CONVST		// Initialization.2
	CLR		MOSI		// Initialization.3
	SET		SCLK		// Initialization.4

SETUP:
	LDI		OUTPUT_REG, 0b1011111111110111		// Load Configuration bits
	JMP		MAIN_LOOP							// Skip Sampling

SAMPLING:
	LDI		OUTPUT_REG, 0b1011			// Load Sampling bits
	//WBS		EOC    						// Wait for EOC to pull high
	SET		CONVST						// Reset CONVST

MAIN_LOOP:
	CLR		FS							// Frame Sync
	AND		DATA, DATA, 0				// Reset Data register
	LDI		R0, 17						// Load Cycle, because we decrement cycle register before check, we use 17 cycles.

WRITE_DATA:
	SUB		R0, R0, 1					// Next Bit
	QBEQ	DATA_COMPLETE, R0, 0 		// Check to see if 16 bits are written/read already

	QBBS	SDO_HIGH, OUTPUT_REG, 0		// Main Data writing code.1
	SDO_LOW:							// Main Data writing code.2
		CLR		MOSI					// Main Data writing code.3
		JMP		READ_DATA				// Main Data writing code.4
	SDO_HIGH:							// Main Data writing code.5
		SET		MOSI					// Main Data writing code.6

READ_DATA:
	LSR		OUTPUT_REG, OUTPUT_REG, 1	// Right shift Output bytes
	SET		SCLK						// SCLK Falling Edge, the ADS will read our output bit now
	ADD		R0, R0, 0					// Time (SCLKF - SDOVALID)
	ADD		R0, R0, 0					// The time is supposed to be minimumly 7.5ns
	ADD		R0, R0, 0					// Which is 1.5 clock cycle.

	QBBC	LEFT_SHIFT, MISO			// Main data reading portion.1
	SET		DATA.T0						// Main data reading portion.2
	LEFT_SHIFT:							// Main data reading portion.3
		LSL		DATA, DATA, 1			// Main data reading portion.4

	LDI		DELAY_REG, 9				// Delay for 9 Cycle,  total 15 Cycles LOW

	DELAY_LOOP:								// This delay loop is used for both SCLK high and SCLK low
		SUB		DELAY_REG, DELAY_REG, 1		// Delay 1 clock
		QBLT	DELAY_LOOP, DELAY_REG, 0	// Back to delay until Delay-register is 0

	QBBC	WRITE_DATA, SCLK			// Going back to Write Data if SCLK is High
	CLR		SCLK						// Rising Edge if SCLK is low
	LDI		DELAY_REG, 9				// Delay for 9 Cycle,  total 15 Cycles HIGH
	JMP		DELAY_LOOP					// Back to the Loop as SCLK High

DATA_COMPLETE:
	SET		FS										// De-select ADS8329
	LSR		DATA, DATA, 1							// Right Shift Once to obtain 16-bit correct data
	SBBO	DATA, DATA_ADDRESS, 0, 2				// Store 2 bytes of data (16-bit)
	ADD 	DATA_ADDRESS, DATA_ADDRESS, 2			// Increment data pointer by 2 bytes

	CLR		MOSI						// Reset Initial Condition.1
	SET		SCLK						// Reset Initial Condition.2
	CLR		CONVST 						// Signal conversion start

	ADD		R1, BLOCK1_BASE_ADDRESS, BLOCK_SIZE		// Check to see if the blocks are filled or not.1
	QBEQ	TRIGGER, DATA_ADDRESS, R1				// Check to see if the blocks are filled or not.2
	ADD		R1, BLOCK2_BASE_ADDRESS, BLOCK_SIZE		// Check to see if the blocks are filled or not.3
	QBEQ	TRIGGER, DATA_ADDRESS, R1				// Check to see if the blocks are filled or not.4

CHECK:
	LBBO	R0, PRU_RAM_ADDRESS, OFFSET(dataLoader.RunFlag), 4			// Run Flag is a input from C-code to tell us when to stop
	QBEQ	EXIT, R0, 0													// If RunFlag == False, we stop, jump to exit (Halt)
	LBBO	DELAY_REG, PRU_RAM_ADDRESS, OFFSET(dataLoader.Delay), 4		// Update the Delay register in case sampling rate is altered in C-code

INTERSAMPLE_DELAY:								// Intersample Delay will determine the sampling rate in general
	SUB		DELAY_REG, DELAY_REG, 1				// The delay register contain the delay that tells us how long should we wait till next sample
	QBLT	INTERSAMPLE_DELAY, DELAY_REG, 0		// Delay 1 clock
	JMP		SAMPLING							// After delay, go to sample section to request sample

TRIGGER:
	ADD		BLOCK_COUNT, BLOCK_COUNT, 1										// Increment block when filled up.
	SBBO	BLOCK_COUNT, PRU_RAM_ADDRESS, OFFSET(dataLoader.DataReady), 4	// Pass the current block count to C-code
	QBBC	RESET_ADDRESS, BLOCK_COUNT, 0									// If bit 0 of Block Count is 0, we reset the address pointer back to data block 1.
	MOV		DATA_ADDRESS, BLOCK2_BASE_ADDRESS								// Else we set the address pointer to data block 2.
	JMP		CHECK															// Check if C-code is still running

RESET_ADDRESS:
	MOV		DATA_ADDRESS, BLOCK1_BASE_ADDRESS		// Reset address pointer back to data block 1
	JMP		CHECK									// Check if C-code is still running

EXIT:												// Exit the program
	MOV		R31.b0, PRU1_ARM_INTERRUPT + 15			// Send interrupt to C-program
	HALT											// Halt the PRU1
