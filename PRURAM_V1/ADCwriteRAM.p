.setcallreg r29.w0        //don't use r30 for call reg, that would clobber the output
.origin 0
.entrypoint START

.struct DataStorage
	.u32	RunFlag
	.u32	DataReady
	.u32	DataSize
.ends

// Define PRU RAM Access Variables
#define PRU_DATA0_BASE		0x00000000
#define PRU0_ARM_INTERRUPT	0x13
#define PRU_RAM_ADDRESS		R16
#define MAX_SIZE			R17
#define DATA_ADDRESS		R19
#define SECTOR_ONE_ADDRESS	R20
#define SECTOR_TWO_ADDRESS	R21
#define SECTOR_TWO_END		R23
#define SECTOR_COUNT		R22

// Define PRU PinOut
#define FS     r30.t1     //frame sync P9_29 - set to mode 5
#define SCLK   r30.t15    //serial clock P8_11 - set to mode 6
#define SDI    r30.t14    //serial data into ADC, out of PRU P8_12 - set to mode 6
#define SDO    r31.t3     //serial data out of ADC, into PRU p9_28 - set to mode 6
#define CONVST r30.t7     //starts conversion when low P9_25 - set to mode 5
#define EOC    r31.t0     //signals end of conversion P9_31 - set to mode 6

//register definitions:
#define SERIAL_REG r1     //use r1 for serial writing to ADC
#define DATA_REG  r2      //use r2 for reading data into
#define DELAY_REG r3      //use r2 for the delay loop

//misc constants:
#define SCLK_DELAY 20     //Serial clock delay counter

START:

	MOV		PRU_RAM_ADDRESS, PRU_DATA0_BASE
	LBBO 	MAX_SIZE, PRU_RAM_ADDRESS, OFFSET(DataStorage.DataSize), 4
	ADD 	SECTOR_ONE_ADDRESS, PRU_RAM_ADDRESS, OFFSET(DataStorage.DataSize) + 4
	ADD 	SECTOR_TWO_ADDRESS, SECTOR_ONE_ADDRESS, MAX_SIZE
	LDI		SECTOR_COUNT, 0
   	ADD		SECTOR_TWO_END, SECTOR_TWO_ADDRESS, MAX_SIZE
   	
	set CONVST
	clr FS
	ldi SERIAL_REG, 0b1100111111111111  //load value to write to CMR (must be prefixed by 1110)
	ldi r0, 16                          //load number of bits to write to CMR
	call CMR_WRITE
	
	ldi DELAY_REG, 100

START_DELAY:			//Wait so ADC can initialize
	sub DELAY_REG, DELAY_REG, 1
  	qbne START_DELAY, DELAY_REG, 0 

RESET_ADDRESS:
	MOV		DATA_ADDRESS, SECTOR_ONE_ADDRESS

INPUT_LOOP:
  	set FS
  	clr CONVST                          //start conversion

WAIT_FOR_EOC:
	//qbbc WAIT_FOR_EOC, EOC              //wait for EOC to go high
  	ldi DELAY_REG, 100
  	WAIT_FOR_EOC_DELAY:
    sub DELAY_REG, DELAY_REG, 1
    qbne WAIT_FOR_EOC_DELAY, DELAY_REG, 0

  	set CONVST                          //reset CONVST (conversion is over)

READ_DATA_IN:
  	ldi DELAY_REG, 5
  	ldi r0, 16                          //we will read/write 16 bits

  	READ_DATA_FS_DELAY:                 //delay a few cycles so we don't put FS low too soon
    sub DELAY_REG, DELAY_REG, 1
    qbne READ_DATA_FS_DELAY, DELAY_REG, 0

  	clr FS
  	ldi SERIAL_REG, 0b1101

	CONV_READ_LOOP:
		qbbs READ_SET, SDO              //set bottom bit of data register if SDO is 1
		jmp LSL_DATA_REG                //jump if not (no need to clear, lsl zero-extends)

	READ_SET:
		set DATA_REG.t0

	LSL_DATA_REG:
		lsl DATA_REG, DATA_REG, 1       //shift data reg left to prepare for next bit

	qbbs CONV_WRITE_SET, SERIAL_REG, 3    //write 1 if lowest bit in serial reg is set
		clr SDI                          //write 0 if not set
		jmp CONV_DELAY                   //serial clock

    CONV_WRITE_SET:
		set SDI

    CONV_DELAY:
 		clr SCLK                         //clock low
 		ldi DELAY_REG, SCLK_DELAY+2      //wait; +2 to keep duty cycle close to 50%

    CONV_DELAY_LOOP:
		sub DELAY_REG, DELAY_REG, 1
		qblt CONV_DELAY_LOOP, DELAY_REG, 0
		set SCLK                         //clock high
 		ldi DELAY_REG, SCLK_DELAY        //wait some more

    CONV_DELAY_LOOP_2:
 		sub DELAY_REG, DELAY_REG, 1
		qblt CONV_DELAY_LOOP_2, DELAY_REG, 0

    lsl SERIAL_REG, SERIAL_REG, 1       //get ready to write next bit
    sub r0, r0, 1
    qblt CONV_READ_LOOP, r0, 0          // read/write next bit if we're not done
    
    //this is the part where we would write the conversion result to memory
    
   	SBBO	DATA_REG, DATA_ADDRESS, 0, 2
   	ADD		DATA_ADDRESS, DATA_ADDRESS, 2
   	QBEQ	TRIGGER, DATA_ADDRESS, SECTOR_TWO_ADDRESS
	QBEQ	TRIGGER, DATA_ADDRESS, SECTOR_TWO_END
	
    //STORAGE END

CHECK:
	// Check Run flag
	LBBO 	r0, PRU_RAM_ADDRESS, OFFSET(DataStorage.RunFlag), 4
	QBEQ 	EXIT, r0, 0
    and DATA_REG, DATA_REG, 0x0         //clear data register - just for testing purposes right now; we will later do this after we write to memory
    jmp INPUT_LOOP
    
TRIGGER:
	ADD 	SECTOR_COUNT, SECTOR_COUNT, 1
	SBBO	SECTOR_COUNT, PRU_RAM_ADDRESS, OFFSET(DataStorage.DataReady), 4
	QBBC	RESET_ADDRESS, SECTOR_COUNT, 0
	JMP		CHECK
    
CMR_WRITE:
	
	CMR_WRITE_LOOP:
    	qbbs WRITE_SET, SERIAL_REG, 15    //write 1 if highest bit in serial reg is set
    	clr SDI                          //write 0 if not set
    	jmp CMR_WRITE_DELAY              //serial clock

  	WRITE_SET:
    	set SDI

  	CMR_WRITE_DELAY:
    	clr SCLK                         //clock low
    	ldi DELAY_REG, SCLK_DELAY+2      //wait; +2 to keep duty cycle close to 50%

	CMR_WRITE_DELAY_LOOP:
   		sub DELAY_REG, DELAY_REG, 1
    	qblt CMR_WRITE_DELAY_LOOP, DELAY_REG, 0
	
	set SCLK                         //clock high
	ldi DELAY_REG, SCLK_DELAY        //wait some more

	CMR_WRITE_DELAY_LOOP_2:
		sub DELAY_REG, DELAY_REG, 1
		qblt CMR_WRITE_DELAY_LOOP_2, DELAY_REG, 0

	lsl SERIAL_REG, SERIAL_REG, 1       //get ready to write next bit
	sub r0, r0, 1
	qblt CMR_WRITE_LOOP, r0, 0          //write next bit if we're not done
	ret

EXIT:
	HALT