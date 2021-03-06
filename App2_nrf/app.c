/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   main loop for the DecaRanging application
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
/* Includes */
#include "compiler.h"
#include "port.h"
#include "instance.h"
#include "deca_types.h"
#include "deca_spi.h"
#include <inttypes.h>
//#include "deca_usb.h"

#if VERSION == VER_1p0
	#define SOFTWARE_VER_STRING    "v1.0 diag & CIR" //16 bytes! --> static ranging with CIR
	//#define SOFTWARE_VER_STRING    "Ver. 1.1    " //16 bytes! --> many anchors configuration without CIR without diagnostics
#elif VERSION == VER_1p2
	#define SOFTWARE_VER_STRING    "Ver. 1.2a    " //16 bytes! --> many anchors configuration with diagnostics without CIR
#elif VERSION == VER_2p0
	#if ANTENNA_USED == ANTENNA_ORIG
		#define SOFTWARE_VER_STRING    "v. 2.2.0       " //16 bytes! --> many anchors configuration with diagnostics without CIR
	#elif ANTENNA_USED == ANTENNA_TAOGLAS
		#define SOFTWARE_VER_STRING    "v. 2.2.1       " //16 bytes! --> many anchors configuration with diagnostics without CIR
	#elif ANTENNA_USED == ANTENNA_CAMCO1
		#define SOFTWARE_VER_STRING    "v. 2.2.2       " //16 bytes! --> many anchors configuration with diagnostics without CIR
	#elif ANTENNA_USED == ANTENNA_CAMCO2
		#define SOFTWARE_VER_STRING    "v. 2.2.3       " //16 bytes! --> many anchors configuration with diagnostics without CIR
	#endif
#endif
#define LCD_BUFF_LEN (80)

#define MMIO16(addr)  (*(volatile uint16_t *)(addr))
#define MMIO32(addr)  (*(volatile uint32_t *)(addr))
#define U_ID_addr     0x1ffff7e8


extern void usb_run(void);
extern int usb_init(void);
extern void usb_printconfig(int, uint8*, int);
extern void send_usbmessage(uint8*, int);
extern instance_data_t instance_data[NUM_INST];
//extern double inst_idist[MAX_ANCHOR_LIST_SIZE];
extern double inst_idist[MAX_BUFF_SIZE];
extern double inst_idist_ImRpt[MAX_BUFF_SIZE];
extern int tx_buff_length;
extern uint8_t tx_buff[8000];
extern uint8_t send_tx_buff;
//extern uint16_t DW_VCP_DataTx   (uint8_t* Buf, uint32_t Len);

uint8 s1switch = 0;
uint8 sw1_role, sw1_chan, sw1_prf, sw1_prbl, sw1_addr;
uint16 instance_anchaddr = 0;
uint8 role, chan, prf, prbl, addr, phy_datarate, tagaddr, ancaddr, prb_code, opr_mode, opr_mode_seq, enableImRpt, enableLcdReport, enableCirRpt;
uint16 prb_len;
uint16 max_numTagToa, max_numTagTwr, max_numAnchTwr, trx_respTurnaround;
uint32 sync_period_toa;
uint8 instance_mode = ANCHOR;
uint8 dataseq[LCD_BUFF_LEN];
uint8 dataseq1[LCD_BUFF_LEN];
uint32_t pauseUsbReports;
uint32_t printLCDTWRReports  = 0;
uint8_t sendTWRRawReports = 1;
uint32 SysTickCnt; 									// system tick count
uint16 rangeTime, valid;
//int32 DistanceTA_mm[MAX_ANCHOR_LIST_SIZE]; 		// array of calculated distance between tag and anchors, in mm
int32 DistanceTA_mm[MAX_BUFF_SIZE]; 		// array of calculated distance between tag and anchors, in mm
uint16 n; 											// number of bytes to be sent via USB VCOM
uint8 usbVCOMout[400];
uint8 counter;
uint8 UpdateSettings = 0;
uint8 UART_rxbuff[20];
uint8 UART_txbuff[20];
uint8 idxUartRx;
uint8 idxUartTx;
uint16 numIterReportRanges;
uint16 numIterReportDiags;
int txdiff;
const uint32 lcdUpdatePeriod = 100000;
uint8 debug;

#if (ENABLE_CIR_DATA == 1)
int16 CIR_real[1016];
int16 CIR_imag[1016];
uint8 usbVCOM_CIR[4200];
#endif

uwbPhyConfig_t uwbPhyConfig;

//sfConfig_t sfConfig =
//{
//		(SLOT_PERIOD_6M81),   	// slot period ms
//		(NUM_SLOT_6M81),   		// number of slots
//		(SF_PERIOD), 			// superframe period
//		(POLL_SLEEP_DLY), 		// poll sleep (tag sleep time, usually = superframe period)
//		(POLL_TO_FINAL_DLY) 	// this is the Poll to Final delay NOTE: if using 6.81 so only 1 frame per ms allowed LDC
//};

sfConfig_t sfConfigTwr;

/* Read U_ID register */
void uid_read(u_id *id)
{
    id->off0 = MMIO16(U_ID_addr + 0x0);
    id->off2 = MMIO16(U_ID_addr + 0x2);
    id->off4 = MMIO32(U_ID_addr + 0x4);
    id->off8 = MMIO32(U_ID_addr + 0x8);
}

uint32 inittestapplication(uint8 s1switch)
{
    uint32 devID ;
//    instanceConfig_t uwbPhyConfig;
    int result;

    instance_data[0].opMode = opr_mode;
    instance_data[0].commandSeqNum = opr_mode_seq;
    instance_data[0].PeriodSync = sync_period_toa;

    SPI_ConfigFastRate(SPI_BaudRatePrescaler_32);  //max SPI before PLLs configured is ~4M

    //this is called here to wake up the device (i.e. if it was in sleep mode before the restart)
    devID = instancereaddeviceid() ;
    if(DWT_DEVICE_ID != devID) //if the read of device ID fails, the DW1000 could be asleep
    {
    	//reset the DW1000 by driving the RSTn line low
    	reset_DW1000();

        port_SPIx_clear_chip_select();  //CS low
        Sleep(1);   //200 us to wake up then waits 5ms for DW1000 XTAL to stabilise
        port_SPIx_set_chip_select();  //CS high
        Sleep(7);
        devID = instancereaddeviceid() ;

        //clear the sleep bit - so that after the hard reset below the DW does not go into sleep
        dwt_softreset();

        // SPI not working or Unsupported Device ID
        if(DWT_DEVICE_ID != devID)
            return(-1) ;
        //clear the sleep bit - so that after the hard reset below the DW does not go into sleep
        dwt_softreset();
    }

    //reset the DW1000 by driving the RSTn line low
    reset_DW1000();

    result = instance_init() ;
    if (0 > result) return(-1) ; // Some failure has occurred

    SPI_ConfigFastRate(SPI_BaudRatePrescaler_4); //increase SPI to max
    devID = instancereaddeviceid() ;

    if (DWT_DEVICE_ID != devID)   // Means it is NOT DW1000 device
    	// SPI not working or Unsupported Device ID
    	return(-1) ;

    if(role)
    {
    	instance_mode = ANCHOR;
    	instancesetaddresses(0x8000 | (uint16)addr);
    }
    else
    {
    	instance_mode = TAG;
    	instancesetaddresses((uint16)addr);
    }
    instancesetrole(instance_mode) ;     // Set this instance role

    // set UWB PHY data rate
    if(phy_datarate == 0)
    {
    	uwbPhyConfig.dataRate = DWT_BR_110K;
    	if(prbl == 0)
    	{
    		prb_len = 1024;
    		uwbPhyConfig.pacSize = DWT_PAC32;
    		uwbPhyConfig.preambleLen = DWT_PLEN_1024;
    		uwbPhyConfig.sfdTO = 1025+64-32;
    		uwbPhyConfig.nsSFD = 1;
    	}
    	else
    	{
    		prb_len = 2048;
    		uwbPhyConfig.pacSize = DWT_PAC64;
    		uwbPhyConfig.preambleLen = DWT_PLEN_2048;
    		uwbPhyConfig.sfdTO = 2049+64-64;
    		uwbPhyConfig.nsSFD = 1;
    	}
    }
    else if(phy_datarate == 1)
    {
    	uwbPhyConfig.dataRate = DWT_BR_850K;
    	if(prbl == 0)
    	{
    		prb_len = 256;
    		uwbPhyConfig.pacSize = DWT_PAC16;
    		uwbPhyConfig.preambleLen = DWT_PLEN_256;
    		uwbPhyConfig.sfdTO = 257+16-16;
    		uwbPhyConfig.nsSFD = 0;
    	}
    	else
    	{
    		prb_len = 512;
    		uwbPhyConfig.pacSize = DWT_PAC16;
    		uwbPhyConfig.preambleLen = DWT_PLEN_512;
    		uwbPhyConfig.sfdTO = 513+16-16;
    		uwbPhyConfig.nsSFD = 0;
    	}
    }
    else
    {
    	uwbPhyConfig.dataRate = DWT_BR_6M8;
    	if(prbl == 0)
    	{
    		prb_len = 64;
    		uwbPhyConfig.pacSize = DWT_PAC8;
    		uwbPhyConfig.preambleLen = DWT_PLEN_64;
    		uwbPhyConfig.sfdTO = 65+8-8;
    		uwbPhyConfig.nsSFD = 0;
    	}
    	else
    	{
    		prb_len = 128;
    		uwbPhyConfig.pacSize = DWT_PAC8;
    		uwbPhyConfig.preambleLen = DWT_PLEN_128;
    		uwbPhyConfig.sfdTO = 129+8-8;
    		uwbPhyConfig.nsSFD = 0;
    	}
    }

    if(UpdateSettings)
    {
    	uwbPhyConfig.dataRate = phy_datarate;
    	uwbPhyConfig.channelNumber = chan;
    	uwbPhyConfig.pulseRepFreq = prf+1;
    	uwbPhyConfig.preambleCode = prb_code;
    }
    else
    {
    	switch(sw1_chan)
    	{
    	case 0: {
    		chan = uwbPhyConfig.channelNumber = 2;
    		switch(sw1_prf)	{
    		case 0:
    			uwbPhyConfig.pulseRepFreq = DWT_PRF_16M;
    			prb_code = uwbPhyConfig.preambleCode = 4;
    			break;
    		case 1:
    			uwbPhyConfig.pulseRepFreq = DWT_PRF_64M;
    			prb_code = uwbPhyConfig.preambleCode = 10;
    			break;
    		}
    		break;
    	}
    	case 1: {
    		chan = uwbPhyConfig.channelNumber = 5;
    		switch(sw1_prf) {
    		case 0:
    			uwbPhyConfig.pulseRepFreq = DWT_PRF_16M;
    			prb_code = uwbPhyConfig.preambleCode = 3;
    			break;
    		case 1:
    			uwbPhyConfig.pulseRepFreq = DWT_PRF_64M;
    			prb_code = uwbPhyConfig.preambleCode = 12;
    			break;
    		}
    		break;
    	}
    	case 2: {
    		chan = uwbPhyConfig.channelNumber = 4;
    		switch(sw1_prf) {
    		case 0:
    			uwbPhyConfig.pulseRepFreq = DWT_PRF_16M;
    			prb_code = uwbPhyConfig.preambleCode = 8;
    			break;
    		case 1:
    			uwbPhyConfig.pulseRepFreq = DWT_PRF_64M;
    			prb_code = uwbPhyConfig.preambleCode = 17;
    			break;
    		}
    		break;
    	}
    	case 3: {
    		chan = uwbPhyConfig.channelNumber = 7;
    		switch(sw1_prf) {
    		case 0:
    			uwbPhyConfig.pulseRepFreq = DWT_PRF_16M;
    			prb_code = uwbPhyConfig.preambleCode = 7;
    			break;
    		case 1:
    			uwbPhyConfig.pulseRepFreq = DWT_PRF_64M;
    			prb_code = uwbPhyConfig.preambleCode = 18;
    			break;
    		}
    		break;
    	}
    	}
    }

//    switch(sw1_prbl)
//    {
//    case 0:
//    	prb_len = 64;
//    	uwbPhyConfig.preambleLen = DWT_PLEN_64;
//    	uwbPhyConfig.sfdTO = 65+8-8;
//    	break;
//    case 1:
//    	prb_len = 128;
//    	uwbPhyConfig.preambleLen = DWT_PLEN_128;
//    	uwbPhyConfig.sfdTO = 129+8-8;
//    	break;
//    default: break;
//    }
//
//    uwbPhyConfig.pacSize = DWT_PAC8;
//    uwbPhyConfig.nsSFD = 0;

//    sfConfigTwr.maxAnchor = MAX_ANCHOR_LIST_SIZE;
//    sfConfigTwr.maxTags = MAX_TAG_LIST_SIZE;
//    sfConfigTwr.numSlots = MAX_TAG_LIST_SIZE;
//    sfConfigTwr.slotPeriod = SLOT_PERIOD_6M81;
//    sfConfigTwr.sfPeriod = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;
//    sfConfigTwr.pollSleepDly = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;
//    sfConfigTwr.replyDly = POLL_TO_FINAL_DLY;
//    sfConfig.replyDlyToa = 1000;
//
//    if(phy_datarate == DWT_BR_110K) //DWT_BR_110K = 0
//    {
//    	sfConfigTwr.slotPeriod = SLOT_PERIOD_110K;
//    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_110K;
//    }
//    else if(phy_datarate == DWT_BR_850K) //DWT_BR_110K = 1
//    {
//    	sfConfigTwr.slotPeriod = SLOT_PERIOD_850K;
//    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_850K;
//    }
//    else	// DWT_BR_6M8 = 2
//    {
//    	sfConfigTwr.slotPeriod = SLOT_PERIOD_6M81;
//    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_6M81;
//    }
#if VERSION == VER_1p0
    max_numAnchTwr = 1;
    max_numTagTwr = 1;
    max_numTagTwr = 1;
#endif
    sfConfigTwr.maxAnchor = max_numAnchTwr;
    sfConfigTwr.maxTags = max_numTagTwr;
    sfConfigTwr.numSlots = max_numTagTwr;

    // enable CIR data?
    instance_data[0].enableCirData = enableCirRpt;

#if VERSION == VER_1p0
    if(phy_datarate == DWT_BR_110K) //DWT_BR_110K = 0
    {
    	sfConfigTwr.slotPeriod = SLOT_PERIOD_110K;
    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_110K;
    }
    else if(phy_datarate == DWT_BR_850K) //DWT_BR_110K = 1
    {
    	sfConfigTwr.slotPeriod = SLOT_PERIOD_850K;
    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_850K;
    }
    else	// DWT_BR_6M8 = 2
    {
    	sfConfigTwr.slotPeriod = SLOT_PERIOD_6M81;
    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_6M81;
    }
#else
    if(phy_datarate == DWT_BR_110K) //DWT_BR_110K = 0
    {
    	if(max_numAnchTwr<4)
    	{
    		sfConfigTwr.slotPeriod = (SLOT_PERIOD_110K >> 1);
    		sfConfigTwr.replyDly = (POLL_TO_FINAL_DLY_110K  >> 1);
    	}
    	else
    	{
    		sfConfigTwr.slotPeriod = (max_numAnchTwr*SLOT_PERIOD_110K) >> 3;
    		sfConfigTwr.replyDly = (uint32)(max_numAnchTwr*POLL_TO_FINAL_DLY_110K) >> 3;
    	}
    }
    else if(phy_datarate == DWT_BR_850K) //DWT_BR_110K = 1
    {
    	if(max_numAnchTwr<4)
    	{
    		sfConfigTwr.slotPeriod = (SLOT_PERIOD_850K >> 1);
    		sfConfigTwr.replyDly = (POLL_TO_FINAL_DLY_850K  >> 1);
    	}
    	else
    	{
    		sfConfigTwr.slotPeriod = (max_numAnchTwr*SLOT_PERIOD_850K) >> 3;
    		sfConfigTwr.replyDly = (uint32)(max_numAnchTwr*POLL_TO_FINAL_DLY_850K) >> 3;
    	}
    }
    else	// DWT_BR_6M8 = 2
    {
    	if(max_numAnchTwr<4)
    	{
    		sfConfigTwr.slotPeriod = (SLOT_PERIOD_6M81 >> 1);
    		sfConfigTwr.replyDly = (POLL_TO_FINAL_DLY_6M81  >> 1);
    	}
    	else
    	{
    		sfConfigTwr.slotPeriod = (max_numAnchTwr*SLOT_PERIOD_6M81) >> 3;
    		sfConfigTwr.replyDly = (uint32)(max_numAnchTwr*POLL_TO_FINAL_DLY_6M81) >> 3;
    	}
    }
#endif


    if(instance_data[0].enableCirData)
    {
    	sfConfigTwr.slotPeriod += SLOT_CIR_MARGIN;
    	if(sfConfigTwr.slotPeriod < SLOT_CIR_MIN)
    		sfConfigTwr.slotPeriod = SLOT_CIR_MIN;
    }

    // direct reporting after 1 TWR exchanges
    instance_data[0].enableReport = enableImRpt;
    instance_data[0].getReportReqs = 0;
//    instance_data[0].getReportResp = 1;

    // if enable report, increase slot period twice (second slot is used for reporting)
//    if (instance_data[0].enableReport)
//    	sfConfigTwr.slotPeriod += sfConfigTwr.slotPeriod + (sfConfigTwr.slotPeriod >> 1);

    sfConfigTwr.sfPeriod = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;
    sfConfigTwr.pollSleepDly = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;

    instance_data[0].trxResponseTurnaround = trx_respTurnaround;

    instance_data[0].finalTxTimeIdx = RRXT0+(5*max_numAnchTwr);
    instance_data[0].twrFinalMsgLen = 1+1+5+(5*max_numAnchTwr)+5+2;

    instance_config(&uwbPhyConfig, &sfConfigTwr) ;                  // Set operating channel etc

    // initialize init state
    if(instance_data[0].opMode == TWR)
    	instance_data[0].testAppState = TA_INIT ;
    else
    	instance_data[0].testAppState = TOA_INIT ;

    // initiate or update period sync
    instance_data[0].PeriodSync = sync_period_toa;

    if(instance_data[0].opMode == TOA && instance_data[0].PeriodSync*100 < (uint32)instance_data[0].maxMessageDuration)
    	instance_data[0].PeriodSync += (uint32)((float)instance_data[0].maxMessageDuration*0.01);

    if(instance_data[0].enableCirData)
    {
//    	instance_data[0].PeriodSync += SLOT_CIR_MARGIN;
    	if(instance_data[0].PeriodSync < SLOT_CIR_MIN)
    		instance_data[0].PeriodSync = SLOT_CIR_MIN;
    }

    // initiate or update max tags supported for TOA mode
    instance_data[0].maxSupportedTagTOA = max_numTagToa;

    // initiate or update max tags & anchors supported for TWR mode
    instance_data[0].maxAnchorsTwr = max_numAnchTwr;
    instance_data[0].maxTagsTwr = max_numTagTwr;

    // enable LCD update
    instance_data[0].updateLcdOn = enableLcdReport;

    // periodic force wake up
    instance_data[0].timeForceWakeUp = sfConfigTwr.sfPeriod;
    instance_data[0].periodForceWakeUp = 100*sfConfigTwr.sfPeriod;

    return devID;
}

/**
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/

// this function is called from UART RX interrupt
void process_uart_rxdata(void)
{
	uint8 status;
	status = USART3->SR;
	UART_rxbuff[idxUartRx] = USART3->DR;
	if(idxUartRx > 0 && UART_rxbuff[idxUartRx] == '~' && UART_rxbuff[idxUartRx-1] == 0xE8){
		idxUartRx = 1;
		UART_rxbuff[0] = 0xE8;
		UART_rxbuff[1] = '~';
	}
	idxUartRx++;
	if(idxUartRx >= 8)
		idxUartRx = 0;
	if(idxUartRx >= 7 && UART_rxbuff[0] == 0xE8 && UART_rxbuff[1] == '~' \
			&& UART_rxbuff[5] == '\r' && UART_rxbuff[6] == '\n') {
		idxUartRx = 0;
		instance_data[0].StatusIO1 = UART_rxbuff[2];
		instance_data[0].StatusIO2 = UART_rxbuff[3];
		instance_data[0].StatIOseqnum = UART_rxbuff[4];

//		instance_data[0].activateRanging = 1;
//		idxUartRx = 0;
	}
}

// this function is called from UWB RX in instance.c
void send_uart_txdata(uint8 len)
{
//	uint8 status;
	idxUartTx = 0;
	for(idxUartTx=0;idxUartTx<len;idxUartTx++) {
		USART_putc(UART_txbuff[idxUartTx]);
//		asm("NOP");
//		asm("NOP");
//		asm("NOP");
//		USART_SendData(USART3, (uint16)UART_txbuff[idx]);
//		USART3->DR = UART_txbuff[idx];
//		status = USART3->SR;
	}
}
void process_dwRSTn_irq(void)
{
    instance_notify_DW1000_inIDLE(1);
}

void process_deca_irq(void)
{
    do{

        instance_process_irq(0);

    }while(port_CheckEXT_IRQ() == 1); //while IRS line active (ARM can only do edge sensitive interrupts)

}

void initLCD(void)
{
    uint8 initseq[9] = { 0x39, 0x14, 0x55, 0x6D, 0x78, 0x38 /*0x3C*/, 0x0C, 0x01, 0x06 };
    uint8 command = 0x0;
    int j = 100000;

    writetoLCD( 9, 0,  initseq); //init seq
    while(j--);

    command = 0x2 ;  //return cursor home
    writetoLCD( 1, 0,  &command);
    command = 0x1 ;  //clear screen
    writetoLCD( 1, 0,  &command);
}

void setLCDline1(void)
{
	int role = instancegetrole();

	sprintf((char*)&dataseq[0], "Ch%d PRF%d %dsym", chan, (prf)? 64:16, prb_len);
	writetoLCD( 40, 1, dataseq); //send some data

//	tagaddr = sw1_addr;
//	ancaddr = sw1_addr;

	tagaddr = addr;
	ancaddr = addr;

	if(role == TAG)
	{
		if(instance_data[0].opMode == TWR)
			sprintf((char*)&dataseq1[0], "Tag:%d TWR v2.2.%d", tagaddr, instance_data[0].antennaType);
		else if(instance_data[0].opMode == TOA)
			sprintf((char*)&dataseq1[0], "Tag:%d ToA v2.2.%d", tagaddr, instance_data[0].antennaType);
		else
			sprintf((char*)&dataseq1[0], "Tag:%d    ", tagaddr);
		writetoLCD( 16, 1, dataseq1); //send some data

	}
	else if(role == ANCHOR)
	{
		if(instance_data[0].opMode == TWR)
			sprintf((char*)&dataseq1[0], "Anc:%d TWR v2.2.%d", ancaddr, instance_data[0].antennaType);
		else if(instance_data[0].opMode == TOA)
			sprintf((char*)&dataseq1[0], "Anc:%d ToA v2.2.%d", ancaddr, instance_data[0].antennaType);
		else
			sprintf((char*)&dataseq1[0], "Anc:%d    ", ancaddr);

		writetoLCD( 16, 1, dataseq1); //send some data
	}
}

#if ENABLE_CIR_DATA == 1
#pragma GCC optimize ("O3")
void sendCIRdata(void)
{

    				uint16 m, ii,jj,kk;

    				asm("NOP"); asm("NOP"); asm("NOP");

    				dwt_readaccdata(&instance_data[0].CIRdata[0], 2033, 0);
    				m = 0;
//    				usbVCOM_CIR[m++] = 'A';
//    				usbVCOM_CIR[m++] = 'n';
//    				usbVCOM_CIR[m++] = 'c';
//    				usbVCOM_CIR[m++] = ' ';
//    				usbVCOM_CIR[m++] = ancaddr;
    				for(jj=0;jj<4;jj++)
    				{
//       				m = 0;
	#if USB_MSG_BINARY == 0
    					m = 0;
    					if(jj == 0)
    						m += sprintf((char*)&usbVCOM_CIR[m], "CIR,");
	#endif

    					ii = 1;
    					for(kk=0;kk<127;kk++)
    					{
    						CIR_real[kk+127*jj] = (int16)((uint16)instance_data[0].CIRdata[ii+0+508*jj] |
    								(((uint16)instance_data[0].CIRdata[ii+1+508*jj]) << 8));

    						CIR_imag[kk+127*jj] = (int16)((uint16)instance_data[0].CIRdata[ii+2+508*jj] |
    								(((uint16)instance_data[0].CIRdata[ii+3+508*jj]) << 8));
    						ii += 4;
	#if USB_MSG_BINARY == 1
    						usbVCOM_CIR[m++] = CIR_real[kk+127*jj] >> 8;
    						usbVCOM_CIR[m++] = CIR_real[kk+127*jj];
    						usbVCOM_CIR[m++] = CIR_imag[kk+127*jj] >> 8;
    						usbVCOM_CIR[m++] = CIR_imag[kk+127*jj];
	#else
    						m += sprintf((char*)&usbVCOM_CIR[m], "%i,%i,", CIR_real[kk+127*jj],CIR_imag[kk+127*jj]);
	#endif
    					}

	#if USB_MSG_BINARY == 0
    					n = m;
    					send_usbmessage(&usbVCOM_CIR[0], n);
    					usb_run();
    					asm("NOP"); asm("NOP"); asm("NOP");
	#endif
    				}
//	#if USB_MSG_BINARY == 1
//       			n = m;
//       			send_usbmessage(&usbVCOM_CIR[0], n);
//       			usb_run();
//       			asm("NOP"); asm("NOP"); asm("NOP");
//	#endif

    				dwt_readaccdata(&instance_data[0].CIRdata[0], 2033, 2032);
//       			m = 0;
    				for(jj=0;jj<4;jj++)
    				{
//       				m = 0;
	#if USB_MSG_BINARY == 0
    					m = 0;
	#endif
    					ii = 1;
    					for(kk=0;kk<127;kk++)
    					{
    						CIR_real[508+kk+127*jj] = (int16)((uint16)instance_data[0].CIRdata[ii+0+508*jj] |
    								(((uint16)instance_data[0].CIRdata[ii+1+508*jj]) << 8));

    						CIR_imag[508+kk+127*jj] = (int16)((uint16)instance_data[0].CIRdata[ii+2+508*jj] |
    								(((uint16)instance_data[0].CIRdata[ii+3+508*jj]) << 8));
    						ii += 4;
	#if USB_MSG_BINARY == 1
    						usbVCOM_CIR[m++] = CIR_real[508+kk+127*jj] >> 8;
    						usbVCOM_CIR[m++] = CIR_real[508+kk+127*jj];
    						usbVCOM_CIR[m++] = CIR_imag[508+kk+127*jj] >> 8;
    						usbVCOM_CIR[m++] = CIR_imag[508+kk+127*jj];
	#else
    						m += sprintf((char*)&usbVCOM_CIR[m], "%i,%i,", CIR_real[508+kk+127*jj],CIR_imag[508+kk+127*jj]);
	#endif
    					}

    					if(jj == 3) {
	#if USB_MSG_BINARY == 0
    						m += sprintf((char*)&usbVCOM_CIR[m], "\r\n");
	#endif
    					}
	#if USB_MSG_BINARY == 0
    					n = m;
    					send_usbmessage(&usbVCOM_CIR[0], n);
    					usb_run();
    					asm("NOP"); asm("NOP"); asm("NOP");
	#endif
    				}
	#if USB_MSG_BINARY == 1
    				usbVCOM_CIR[m++] = 0x80;
    				usbVCOM_CIR[m++] = 0x80;
    				usbVCOM_CIR[m++] = 0x80;
    				usbVCOM_CIR[m++] = 0x80;
    				n = m;
    				send_usbmessage(&usbVCOM_CIR[0], n);
    				usb_run();
    				asm("NOP"); asm("NOP"); asm("NOP");
	#endif
}
#endif // ENABLE_CIR_DATA

/*
 * @fn      main()
 * @brief   main entry point
**/

#pragma GCC optimize ("O3")
int main(void)
{
    int i = 0;
    int rx = 0;
    int toggle = 0;
    uint8 command = 0x0;
    pauseUsbReports = 0;

    led_off(LED_ALL); //turn off all the LEDs

    peripherals_init();

    spi_peripheral_init();

    Sleep(1000); //wait for LCD to power on

    initLCD();

    memset(dataseq, 0, LCD_BUFF_LEN);
    memcpy(dataseq, (const uint8 *) "FlandersMake App", 16);
    writetoLCD( 40, 1, dataseq); //send some data
    memcpy(dataseq, (const uint8 *) SOFTWARE_VER_STRING, 16); // Also set at line #26 (Should make this from single value !!!)
    writetoLCD( 16, 1, dataseq); //send some data

    Sleep(1000);

    // enable the USB functionality
    usb_init();
    Sleep(1000);

    s1switch = is_button_low(0) << 6
        		| is_switch_on(TA_SW1_3) << 5
        		| is_switch_on(TA_SW1_4) << 4
        		| is_switch_on(TA_SW1_5) << 3
    		    | is_switch_on(TA_SW1_6) << 2
        		| is_switch_on(TA_SW1_7) << 1
        		| is_switch_on(TA_SW1_8) << 0;

    role = sw1_role = is_button_low(0);
//    sw1_chan = is_switch_on(TA_SW1_3) << 1 | is_switch_on(TA_SW1_4);
    sw1_chan = is_switch_on(TA_SW1_3);
    prbl = sw1_prbl = is_switch_on(TA_SW1_4);
    prf = sw1_prf = is_switch_on(TA_SW1_5);
//    sw1_prf = 1;
#if VERSION == VER_1p0
    phy_datarate = is_switch_on(TA_SW1_6) << 1 | is_switch_on(TA_SW1_7);
    if(phy_datarate > 2)
    	phy_datarate = 2;
    addr = sw1_addr = 0;
#else
    addr = sw1_addr = is_switch_on(TA_SW1_6) << 2 | is_switch_on(TA_SW1_7) << 1 | is_switch_on(TA_SW1_8);
//    sw1_addr = is_switch_on(TA_SW1_5) << 3 | is_switch_on(TA_SW1_6) << 2 | is_switch_on(TA_SW1_7) << 1 | is_switch_on(TA_SW1_8);
    // default data rate
    phy_datarate = DWT_BR_6M8;
#endif

    port_DisableEXT_IRQ(); //disable ScenSor IRQ until we configure the device

    //run DecaRangeRTLS application for TREK

    command = 0x2 ;  //return cursor home
    writetoLCD( 1, 0,  &command);
    memset(dataseq, ' ', LCD_BUFF_LEN);
    memcpy(dataseq, (const uint8 *) "FlandersMake App ", 16);
    writetoLCD( 16, 1, dataseq); //send some data

    led_off(LED_ALL);

//    instance_data[0].opMode = TWR;
//    instance_data[0].opMode = TOA;

#if VERSION == VER_1p0
    opr_mode = TWR;
#else
    opr_mode = TWR;
//    opr_mode = TOA;
#endif
    opr_mode_seq = 0;

    // default period sync
    instance_data[0].PeriodSync = sync_period_toa = TOA_DEFAULT_SYNC_PERIOD;

    // default max tags supported for TOA mode
    instance_data[0].maxSupportedTagTOA = max_numTagToa = MAX_TAG_LIST_SIZE_TOA;

    // initialize super frame config
    sfConfigTwr.maxAnchor = max_numAnchTwr = MAX_ANCHOR_LIST_SIZE;
    sfConfigTwr.maxTags = max_numTagTwr = MAX_TAG_LIST_SIZE;
    sfConfigTwr.numSlots = max_numTagTwr = MAX_TAG_LIST_SIZE;

    instance_data[0].trxResponseTurnaround = trx_respTurnaround = TRX_RESP_TURNAROUND_DEFAULT;

    instance_data[0].finalTxTimeIdx = RRXT0+(5*MAX_ANCHOR_LIST_SIZE);
    instance_data[0].twrFinalMsgLen = TAG_FINAL_MSG_LEN;

    instance_data[0].enableReport = enableImRpt = 0;
    instance_data[0].updateLcdOn = enableLcdReport = 0;
    instance_data[0].enableCirData = enableCirRpt = 0;

//    if(phy_datarate == DWT_BR_110K) //DWT_BR_110K = 0
//    {
//    	sfConfigTwr.slotPeriod = SLOT_PERIOD_110K;
//    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_110K;
//    }
//    else if(phy_datarate == DWT_BR_850K) //DWT_BR_110K = 1
//    {
//    	sfConfigTwr.slotPeriod = SLOT_PERIOD_850K;
//    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_850K;
//    }
//    else	// DWT_BR_6M8 = 2
//    {
//    	sfConfigTwr.slotPeriod = SLOT_PERIOD_6M81;
//    	sfConfigTwr.replyDly = POLL_TO_FINAL_DLY_6M81;
//    }
//    sfConfigTwr.sfPeriod = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;
//    sfConfigTwr.pollSleepDly = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;


    if(inittestapplication(0) == (uint32)-1)
    {
    	led_on(LED_ALL); //to display error....
    	dataseq[0] = 0x2 ;  //return cursor home
    	writetoLCD( 1, 0,  &dataseq[0]);
    	memset(dataseq, ' ', LCD_BUFF_LEN);
    	memcpy(dataseq, (const uint8 *) "ERROR   ", 12);
    	writetoLCD( 40, 1, dataseq); //send some data
    	memcpy(dataseq, (const uint8 *) "  INIT FAIL ", 12);
    	writetoLCD( 40, 1, dataseq); //send some data
    	return 0; //error
    }

    // Configure USB for output, (i.e. not USB to SPI)
    usb_printconfig(16, (uint8 *)SOFTWARE_VER_STRING, s1switch);

    //sleep for 5 seconds displaying last LCD message and flashing LEDs
    i=30;
    while(i--)
    {
    	if (i & 1) led_off(LED_ALL);
    	else    led_on(LED_ALL);

    	Sleep(200);
    }
    i = 0;
    led_off(LED_ALL);
    command = 0x2 ;  //return cursor home
    writetoLCD( 1, 0,  &command);

    memset(dataseq, ' ', LCD_BUFF_LEN);
    memset(dataseq1, ' ', LCD_BUFF_LEN);

    setLCDline1();

    command = 0x2 ;  //return cursor home
    writetoLCD( 1, 0,  &command);

    port_EnableEXT_IRQ(); //enable ScenSor IRQ before starting

    //memset(dataseq, ' ', LCD_BUFF_LEN);
    memset(dataseq1, ' ', LCD_BUFF_LEN);

    // clear diagnostics data
#if ENABLE_DIAGNOSTICS == 1
//    for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    for(uint8 ii=0;ii<MAX_BUFF_SIZE;ii++)
    {
    	instance_data[0].Adiags_data[ii].firstPath = 0;
    	instance_data[0].Adiags_data[ii].firstPathAmp1 = 0;
    	instance_data[0].Adiags_data[ii].firstPathAmp2 = 0;
    	instance_data[0].Adiags_data[ii].firstPathAmp3 = 0;
    	instance_data[0].Adiags_data[ii].maxGrowthCIR = 0;
    	instance_data[0].Adiags_data[ii].maxNoise = 0;
    	instance_data[0].Adiags_data[ii].rxPreamCount = 0;
    	instance_data[0].Adiags_data[ii].stdNoise = 0;
    }
//    for(uint8 ii=0;ii<MAX_TAG_LIST_SIZE;ii++)
    for(uint8 ii=0;ii<MAX_BUFF_SIZE;ii++)
    {
    	instance_data[0].Tdiags_data[ii].firstPath = 0;
    	instance_data[0].Tdiags_data[ii].firstPathAmp1 = 0;
    	instance_data[0].Tdiags_data[ii].firstPathAmp2 = 0;
    	instance_data[0].Tdiags_data[ii].firstPathAmp3 = 0;
    	instance_data[0].Tdiags_data[ii].maxGrowthCIR = 0;
    	instance_data[0].Tdiags_data[ii].maxNoise = 0;
    	instance_data[0].Tdiags_data[ii].rxPreamCount = 0;
    	instance_data[0].Tdiags_data[ii].stdNoise = 0;
    }
#endif

    instance_data[0].RangeBiasCorrection = CORRECT_RANGE_BIAS;
    instance_data[0].UseUpdatedBiasTable = 0;

    // copy default range bias table
    copyRangeBiasTable();
    // default correction factor for bias correction
    instance_data[0].BiasCorrFactor[0] = 763;	// Ch1
    instance_data[0].BiasCorrFactor[1] = 763;	// Ch2
    instance_data[0].BiasCorrFactor[2] = 763; 	// Ch3
    instance_data[0].BiasCorrFactor[3] = 1000;	// Ch4
    instance_data[0].BiasCorrFactor[4] = 662;	// Ch5
    instance_data[0].BiasCorrFactor[5] = 1000;	// Ch7

    instance_data[0].antennaType = 0; // default use TREK1000 antenna
//    for(uint8 ii=0;ii<6;ii++)
//    {
//    	instance_data[0].BiasStartPrf16[ii] = -23;
//    	instance_data[0].BiasStartPrf64[ii] = -17;
//    }
//    instance_data[0].BiasStartPrf16[3] = instance_data[0].BiasStartPrf16[5] = -28;
//    instance_data[0].BiasStartPrf64[3] = instance_data[0].BiasStartPrf64[5] = -30;

    counter = 0;
    // initialize operation mode, TWR or TOA
//    instance_data[0].opMode = TWR;
//    instance_data[0].opMode = TOA;
    // initialize gateway anchor address for TWR, or anchor sync for TOA
    instance_data[0].gatewayAnchorAddress = 0x8000;
//    // initialize mode
//    instance_data[0].mode =  ANCHOR;                                // assume listener,

    instance_data[0].instToSleep = FALSE;

    uid_read(&instance_data[0].mcuId);

//    instance_data[0].updateLcdOn = 0;

//    instance_data[0].enableReport = 1;
//    instance_data[0].enableReport = 0;
//    instance_data[0].getReportReqs = 0;
//    instance_data[0].getReportResp = 1;
    send_tx_buff = 0;
//    // enable USART
//    usartinit();

    // main loop
    while(1)
    {
    	n = 0;

    	if(UpdateSettings) {
    		dwt_forcetrxoff();	//this will clear all events
    		port_DisableEXT_IRQ(); //disable ScenSor IRQ until we configure the device
    		inittestapplication(0);
    		port_EnableEXT_IRQ(); //enable ScenSor IRQ before starting
//    		updateUWBsettings();
//    		// initialize init state
//    		if(instance_data[0].opMode == TWR)
//    			instance_data[0].testAppState = TA_INIT ;
//    		else
//    			instance_data[0].testAppState = TOA_INIT ;
    		led_off(LED_ALL);
    		command = 0x2 ;  //return cursor home
    		writetoLCD( 1, 0,  &command);
    		memset(dataseq, ' ', LCD_BUFF_LEN);
    		memset(dataseq1, ' ', LCD_BUFF_LEN);
    		setLCDline1();
    		command = 0x2 ;  //return cursor home
    		writetoLCD( 1, 0,  &command);
    		memset(dataseq1, ' ', LCD_BUFF_LEN);
    		UpdateSettings = 0;
    	}

    	if(send_tx_buff)
    	{
    		send_usbmessage(&tx_buff[0], tx_buff_length);
    		usb_run();
//    		pauseUsbReports = portGetTickCnt();

//    		DW_VCP_DataTx(tx_buff, tx_buff_length);
    		send_tx_buff = 0;
    		for(i=0;i<tx_buff_length;i++)
    			tx_buff[i] = 0;

    		pauseUsbReports = 0;

//    		asm("NOP"); asm("NOP"); asm("NOP"); asm("NOP"); asm("NOP");

    	}

//    	if(pauseUsbReports>0)
//    	{
//    		if(portGetTickCnt()-pauseUsbReports > 20)
//    			pauseUsbReports = 0;
//    	}

    	int monitor_local = instance_data[0].monitor ;
    	txdiff = (portGetTickCnt() - instance_data[0].timeofTx);

    	if(pauseUsbReports == 0 && instance_data[0].opMode == TWR) // TWR mode
    	{
    		instance_run_TWR();


    		instance_mode = instancegetrole();

    		//if delayed TX scheduled but did not happen after expected time then it has failed... (has to be < slot period)
    		//if anchor just go into RX and wait for next message from tags/anchors
    		//if tag handle as a timeout
    		if((monitor_local == 1) && ( txdiff > instance_data[0].slotPeriod))
    		{
    			//        	int an = 0;
    			//        	uint32 tdly ;
    			//        	uint32 reg1, reg2;

    			//        	reg1 = dwt_read32bitoffsetreg(0x0f, 0x1);
    			//        	reg2 = dwt_read32bitoffsetreg(0x019, 0x1);
    			//        	tdly = dwt_read32bitoffsetreg(0x0a, 0x1);
    			//        	an = sprintf((char*)&usbVCOMout[0], "T%08x %08x time %08x %08x", (unsigned int) reg2, (unsigned int) reg1,
    			//        			(unsigned int) dwt_read32bitoffsetreg(0x06, 0x1), (unsigned int) tdly);
    			//            send_usbmessage(&usbVCOMout[0], an);

    			instance_data[0].wait4ack = 0;

    			if(instance_mode == TAG)
    			{
    				inst_processrxtimeout(&instance_data[0]);
    			}
    			else //if(instance_mode == ANCHOR)
    			{
    				dwt_forcetrxoff();	//this will clear all events
    				//dwt_rxreset();
    				//enable the RX
    				instance_backtoanchor(&instance_data[0]);
    				dwt_rxenable(DWT_START_RX_IMMEDIATE);
    				instance_data[0].testAppState = TA_RXE_WAIT ;
    			}
    			instance_data[0].monitor = 0;
    		}

    		rx = (instance_data[0].newRange || instance_data[0].newRangeImRpt);
//    		rx = instancenewrange() ;

    		//if there is a new ranging report received or a new range has been calculated, then prepare data
    		//to output over USB - Virtual COM port, and update the LCD
    		if(rx != TOF_REPORT_NUL)
    		{
    			uint16 l = 0;
    			uint8 r= 0; // l: total number of ranges & r: number of slots counter
    			//uint16 aaddr, taddr;
    			//int correction ;

    			//send the new range information to LCD and/or USB
#if USB_MSG_BINARY == 0
    			uint16 aaddr;
    			aaddr = instancenewrangeancadd() & 0xff;
#endif
    			uint16 txa, rxa;
    			txa =  instancetxantdly();
    			rxa =  instancerxantdly();

    			uint16 taddr;
    			taddr = instancenewrangetagadd() & 0xff;
    			rangeTime = instancenewrangetim() & 0xffffffff;
    			if(instance_data[0].newRangeImRpt)
    				rangeTime = instance_data[0].newRangeTimeImRpt & 0xffffffff;
//#if LCD_UPDATE_ON == 1
    			if(instance_data[0].updateLcdOn && (printLCDTWRReports + lcdUpdatePeriod <= portGetTickCnt()))
    			{
    				//led_on(LED_PC9);
    				command = 0x2 ;  //return cursor home
    				writetoLCD( 1, 0,  &command);

    				memset(dataseq1, ' ', LCD_BUFF_LEN);
    				writetoLCD( 40, 1, dataseq); //send some data

    				//anchors will print a range to each tag in sequence with 1 second pause
    				//they will show the last rage to that tag
    				if(instance_mode == ANCHOR)
    				{
    					int b = 0;
    					double rangetotag = getTagDist(toggle) ;

    					while(((int) (rangetotag*1000)) == 0) //if 0 then go to next tag
    					{
//    						if(b > MAX_TAG_LIST_SIZE)
//    							break;
//
//    						toggle++;
//    						if(toggle >= MAX_TAG_LIST_SIZE)
//    							toggle = 0;

    						if(b > instance_data[0].maxTagsTwr)
    							break;

    						toggle++;
    						if(toggle >= instance_data[0].maxTagsTwr)
    							toggle = 0;

    						rangetotag = getTagDist(toggle) ;
    						b++;
    					}

    					sprintf((char*)&dataseq1[0], "A%d T%d: %3.2f m", ancaddr, toggle, rangetotag);
    					writetoLCD( 16, 1, dataseq1); //send some data

    					toggle++;

//    					if(toggle >= MAX_TAG_LIST_SIZE)
//    						toggle = 0;

    					if(toggle >= instance_data[0].maxTagsTwr)
    						toggle = 0;
    				}
    				else if(instance_mode == TAG)
    				{
    					sprintf((char*)&dataseq1[0], "T%d A%d: %3.2f m", tagaddr, toggle, instancegetidist(toggle));
    					//toggle = 1;
    					writetoLCD( 16, 1, dataseq1); //send some data

    					toggle++;

//    					if(toggle >= MAX_ANCHOR_LIST_SIZE)
//    						toggle = 0;

    					if(toggle >= instance_data[0].maxAnchorsTwr)
    						toggle = 0;
    				}
    				//led_off(LED_PC9);

    				//update the print time
    				printLCDTWRReports = portGetTickCnt();
    			}
//#endif
    			//led_on(LED_PC9);
//    			l = instancegetlcount() & 0xFFFF;
    			l = instance_data[0].longTermRangeCount;
    			if(instance_mode == TAG)
    			{
//    				r = instancegetrnum();
    				r = instance_data[0].rangeNum;
    			}
    			else
    			{
    				r = instancegetrnuma(taddr);
    			}

    			if(instance_data[0].newRange) {
    				valid = instancevalidranges();
//    				r = r-1;
//    				l = l-1;
    			}

    			if(instance_data[0].newRangeImRpt) {
    				valid = instance_data[0].rxResponseMaskReportImRpt;
//    				r = r+1;
//    				l = l+1;
    			}

//    			if(l > 254)
//    				debug++;

    			n = 0;
    			SysTickCnt = portGetTickCount();

    			numIterReportRanges = 8*(1 + ((instance_data[0].maxAnchorsTwr-1) >> 3));

//    			if(instance_data[0].getReportResp == 0)
//    				debug = 1;

//    			if(rx == TOF_REPORT_T2A && instance_data[0].getReportResp)
//    			if(rx == TOF_REPORT_T2A)
    			if(rx > 0)
    			{
//    				pauseUsbReports = 0;
    				//correction = instance_data.tagSleepCorrection2;
//    				if(instance_data[0].enableReport)
//    					instance_data[0].getReportResp = 0;

    				if(instance_mode == TAG) {

#if USB_MSG_BINARY == 1
    					uint8 jj = 0;
    					usbVCOMout[jj++] = 'T';
    					usbVCOMout[jj++] = 'a';
    					usbVCOMout[jj++] = 'g';
    					usbVCOMout[jj++] = ' ';
    					usbVCOMout[jj++] = taddr;
    					usbVCOMout[jj++] = (uint8)(rangeTime >> 8);
    					usbVCOMout[jj++] = (uint8)rangeTime;
    					usbVCOMout[jj++] = (uint8)(l >> 8);
    					usbVCOMout[jj++] = (uint8)l;
    					usbVCOMout[jj++] = (r >> 8);
    					usbVCOMout[jj++] = (uint8)r;
    					usbVCOMout[jj++] = (uint8)(valid >> 8);
    					usbVCOMout[jj++] = (uint8)valid;
//    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    					for(uint8 ii=0;ii<numIterReportRanges;ii++)
    					{
    						if(instance_data[0].newRange)
    							DistanceTA_mm[ii] = (int32)(1000*inst_idist[ii]);
    						else if(instance_data[0].newRangeImRpt)
    							DistanceTA_mm[ii] = (int32)(1000*inst_idist_ImRpt[ii]);

    						usbVCOMout[jj++] = DistanceTA_mm[ii] >> 24;
    						usbVCOMout[jj++] = DistanceTA_mm[ii] >> 16;
    						usbVCOMout[jj++] = DistanceTA_mm[ii] >> 8;
    						usbVCOMout[jj++] = DistanceTA_mm[ii];
    					}
    					numIterReportDiags = 8*(1 + ((instance_data[0].maxAnchorsTwr-1) >> 3));
	#if ENABLE_DIAGNOSTICS == 1
//    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    					for(uint8 ii=0;ii<numIterReportDiags;ii++)
    					{
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].firstPathAmp1 >> 8;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].firstPathAmp1;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].firstPathAmp2 >> 8;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].firstPathAmp2;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].firstPathAmp3 >> 8;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].firstPathAmp3;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].maxGrowthCIR >> 8;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].maxGrowthCIR;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].rxPreamCount >> 8;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].rxPreamCount;
//		#if VERSION == VER_1p0
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].maxNoise >> 8;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].maxNoise;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].stdNoise >> 8;
    						usbVCOMout[jj++] = instance_data[0].Adiags_data[ii].stdNoise;
    						uint32 FPindex1000 = (uint32)(1000*instance_data[0].Adiags_data[ii].firstPath);
    						for(uint8 kk = 0;kk<4;kk++)
    							usbVCOMout[jj++] = FPindex1000 >> (24-kk*8);
//		#endif
    					}
	#endif // ENABLE_DIAGNOSTICS

	#if VERSION == VER_1p0
    					usbVCOMout[jj++] = txa >> 8;
    					usbVCOMout[jj++] = txa;
    					usbVCOMout[jj++] = rxa >> 8;
    					usbVCOMout[jj++] = rxa;
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampTx1[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampRx1[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampTx2[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampRx2[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampTx3[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampRx3[0] >> (56-ii*8);
	#endif

//            		usbVCOMout[jj++] = instance_data[0].StatusIO1;
//            		usbVCOMout[jj++] = instance_data[0].StatusIO2;
//	#if ENABLE_CIR_DATA == 0
    					usbVCOMout[jj++] = '\r';
    					usbVCOMout[jj++] = '\n';
//	#endif
    					n = jj;
#else // USB_MSG_BINARY
	#if VERSION == VER_1p0
    					n = sprintf((char*)&usbVCOMout[0],
    							"T%d,%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,",taddr, SysTickCnt, rangeTime, valid, l, r, txa, rxa, chan, (sw1_prf)? 64:16, prb_len);
    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    						n += sprintf((char*)&usbVCOMout[n],	"R%d,%d,%d,", ii, instancegetidist_mm(ii), instancegetidistraw_mm(ii));
	#else // VERSION
    					n = sprintf((char*)&usbVCOMout[0],"T%d,%d,%d,",taddr,rangeTime,valid);
    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    						n += sprintf((char*)&usbVCOMout[n],	"%d,", instancegetidist_mm(ii));
	#endif // VERSION

	#if ENABLE_DIAGNOSTICS == 1
		#if VERSION == VER_1p0
    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    						n += sprintf((char*)&usbVCOMout[n], "dA%d,%g,%d,%d,%d,%d,%d,%d,%d,",
    								ii,
									instance_data[0].Adiags_data[ii].firstPath,
									instance_data[0].Adiags_data[ii].firstPathAmp1,
									instance_data[0].Adiags_data[ii].firstPathAmp2,
									instance_data[0].Adiags_data[ii].firstPathAmp3,
									instance_data[0].Adiags_data[ii].maxGrowthCIR,
									instance_data[0].Adiags_data[ii].maxNoise,
									instance_data[0].Adiags_data[ii].rxPreamCount,
									instance_data[0].Adiags_data[ii].stdNoise);
		#else // VERSION
    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    						n += sprintf((char*)&usbVCOMout[n], "%d,%d,%d,%d,%d,",
    								instance_data[0].Adiags_data[ii].firstPathAmp1,
									instance_data[0].Adiags_data[ii].firstPathAmp2,
									instance_data[0].Adiags_data[ii].firstPathAmp3,
									instance_data[0].Adiags_data[ii].maxGrowthCIR,
									instance_data[0].Adiags_data[ii].rxPreamCount);
		#endif // VERSION
	#endif // ENABLE_DIAGNOSTICS
	#if ENABLE_CIR_DATA == 0
    					n += sprintf((char*)&usbVCOMout[n], "\r\n");
	#endif // ENABLE_CIR_DATA
#endif // USB_MSG_BINARY
    				}

    				else // anchor mode
    				{

#if USB_MSG_BINARY == 1
    					uint8 jj = 0;
    					usbVCOMout[jj++] = 'T';
    					usbVCOMout[jj++] = 'a';
    					usbVCOMout[jj++] = 'g';
    					usbVCOMout[jj++] = ' ';
    					usbVCOMout[jj++] = taddr;
    					usbVCOMout[jj++] = rangeTime >> 8;
    					usbVCOMout[jj++] = rangeTime;
    					usbVCOMout[jj++] = l >> 8;
    					usbVCOMout[jj++] = l;
    					usbVCOMout[jj++] = r >> 8;
    					usbVCOMout[jj++] = r;
    					usbVCOMout[jj++] = valid >> 8;
    					usbVCOMout[jj++] = valid;
//    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    					for(uint8 ii=0;ii<numIterReportRanges;ii++)
    					{
    						DistanceTA_mm[ii] = (int32)(1000*inst_idist[ii]);

    						usbVCOMout[jj++] = DistanceTA_mm[ii] >> 24;
    						usbVCOMout[jj++] = DistanceTA_mm[ii] >> 16;
    						usbVCOMout[jj++] = DistanceTA_mm[ii] >> 8;
    						usbVCOMout[jj++] = DistanceTA_mm[ii];
    					}
    					usbVCOMout[jj++] = 'A';
    					usbVCOMout[jj++] = 'n';
    					usbVCOMout[jj++] = 'c';
    					usbVCOMout[jj++] = ' ';
    					usbVCOMout[jj++] = ancaddr;

    					numIterReportDiags = 8*(1 + ((instance_data[0].maxTagsTwr-1) >> 3));
	#if ENABLE_DIAGNOSTICS == 1
//    					for(uint8 ii=0;ii<MAX_TAG_LIST_SIZE;ii++)
//    					for(uint8 ii=0;ii<numIterReportDiags;ii++)
//    					{
    					uint8 ii = taddr % numIterReportDiags;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].firstPathAmp1 >> 8;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].firstPathAmp1;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].firstPathAmp2 >> 8;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].firstPathAmp2;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].firstPathAmp3 >> 8;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].firstPathAmp3;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].maxGrowthCIR >> 8;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].maxGrowthCIR;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].rxPreamCount >> 8;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].rxPreamCount;
//		#if VERSION == VER_1p0
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].maxNoise >> 8;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].maxNoise;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].stdNoise >> 8;
    						usbVCOMout[jj++] = instance_data[0].Tdiags_data[ii].stdNoise;
    						uint32 FPindex1000 = (uint32)(1000*instance_data[0].Tdiags_data[ii].firstPath);
    						for(uint8 kk = 0;kk<4;kk++)
    							usbVCOMout[jj++] = FPindex1000 >> (24-kk*8);
//		#endif

//    					}
	#endif // ENABLE_DIAGNOSTICS
//    						usbVCOMout[jj++] = txa >> 8;
//    						usbVCOMout[jj++] = txa;
//    						usbVCOMout[jj++] = rxa >> 8;
//    						usbVCOMout[jj++] = rxa;
    						for(uint8 ii=0;ii<8;ii++)
    							usbVCOMout[jj++] = instance_data[0].TimestampTx1[0] >> (56-ii*8);
    						for(uint8 ii=0;ii<8;ii++)
    							usbVCOMout[jj++] = instance_data[0].TimestampRx1[0] >> (56-ii*8);
    						for(uint8 ii=0;ii<8;ii++)
    							usbVCOMout[jj++] = instance_data[0].TimestampTx2[0] >> (56-ii*8);
    						for(uint8 ii=0;ii<8;ii++)
    							usbVCOMout[jj++] = instance_data[0].TimestampRx2[0] >> (56-ii*8);
    						for(uint8 ii=0;ii<8;ii++)
    							usbVCOMout[jj++] = instance_data[0].TimestampTx3[0] >> (56-ii*8);
    						for(uint8 ii=0;ii<8;ii++)
    							usbVCOMout[jj++] = instance_data[0].TimestampRx3[0] >> (56-ii*8);

	#if VERSION == VER_1p0
    					usbVCOMout[jj++] = txa >> 8;
    					usbVCOMout[jj++] = txa;
    					usbVCOMout[jj++] = rxa >> 8;
    					usbVCOMout[jj++] = rxa;
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampTx1[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampRx1[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampTx2[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampRx2[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampTx3[0] >> (56-ii*8);
    					for(uint8 ii=0;ii<8;ii++)
    						usbVCOMout[jj++] = instance_data[0].TimestampRx3[0] >> (56-ii*8);
	#endif

//            		usbVCOMout[jj++] = instance_data[0].StatusIO1;
//            		usbVCOMout[jj++] = instance_data[0].StatusIO2;
//	#if ENABLE_CIR_DATA == 0

    					usbVCOMout[jj++] = '\r';
    					usbVCOMout[jj++] = '\n';
//	#endif
    					n = jj;
#else // USB_MSG_BINARY
	#if VERSION == VER_1p0
    					n = sprintf((char*)&usbVCOMout[0],
    							"T%d,%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,", taddr, SysTickCnt, rangeTime, valid, l, r, txa, rxa, chan, (sw1_prf)? 64:16, prb_len);
    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    						n += sprintf((char*)&usbVCOMout[n],	"R%d,%d,%d,", ii, instancegetidist_mm(ii), instancegetidistraw_mm(ii));
	#else // VERSION
    					n = sprintf((char*)&usbVCOMout[0],"T%d,%d,%d,",taddr,rangeTime,valid);
    					for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    						n += sprintf((char*)&usbVCOMout[n],	"%d,", instancegetidist_mm(ii));
	#endif // VERSION

	#if ENABLE_DIAGNOSTICS == 1
		#if VERSION == VER_1p0
    					for(uint8 ii=0;ii<MAX_TAG_LIST_SIZE;ii++)
    						n += sprintf((char*)&usbVCOMout[n], "dT%d,%g,%d,%d,%d,%d,%d,%d,%d,",
    								taddr,
									instance_data[0].Tdiags_data[taddr].firstPath,
									instance_data[0].Tdiags_data[taddr].firstPathAmp1,
									instance_data[0].Tdiags_data[taddr].firstPathAmp2,
									instance_data[0].Tdiags_data[taddr].firstPathAmp3,
									instance_data[0].Tdiags_data[taddr].maxGrowthCIR,
									instance_data[0].Tdiags_data[taddr].maxNoise,
									instance_data[0].Tdiags_data[taddr].rxPreamCount,
									instance_data[0].Tdiags_data[taddr].stdNoise);
		#else // VERSION
    					for(uint8 ii=0;ii<MAX_TAG_LIST_SIZE;ii++)
    						n += sprintf((char*)&usbVCOMout[n], "%d,%d,%d,%d,%d,",
    								instance_data[0].Tdiags_data[ii].firstPathAmp1,
									instance_data[0].Tdiags_data[ii].firstPathAmp2,
									instance_data[0].Tdiags_data[ii].firstPathAmp3,
									instance_data[0].Tdiags_data[ii].maxGrowthCIR,
									instance_data[0].Tdiags_data[ii].rxPreamCount);
		#endif // VERSION
	#endif // ENABLE_DIAGNOSTICS
	#if ENABLE_CIR_DATA == 0
    					n += sprintf((char*)&usbVCOMout[n], "\r\n");
	#endif // ENABLE_CIR_DATA
#endif // USB_MSG_BINARY
    				}
    			}
//            else //anchor to anchor ranging
//            {
//            	n = sprintf((char*)&usbVCOMout[0], "ma %02x %08x %08x %08x %08x %04x %02x %08x a0:%d\r\n",
//														valid, instancegetidist_mm(0), instancegetidist_mm(1),
//														instancegetidist_mm(2), instancegetidist_mm(3),
//														l, instancegetrnumanc(0), rangeTime, aaddr);
//            }
    			//led_off(LED_PC9);
    			// clear ranges table
    			if(instance_data[0].newRange)
    			{
    				instance_data[0].newRange = 0;
    				instancecleardisttableall();
    			}

    			if(instance_data[0].newRangeImRpt)
    			{
    				instance_data[0].newRangeImRpt = 0;
    				instancecleardisttableall_ImRpt();
    			}

//    			instancecleardisttableall();
    			// clear diagnostics data
#if ENABLE_DIAGNOSTICS == 1
//    			for(uint8 ii=0;ii<MAX_ANCHOR_LIST_SIZE;ii++)
    			for(uint8 ii=0;ii<MAX_BUFF_SIZE;ii++)
    			{
    				instance_data[0].Adiags_data[ii].firstPath = 0;
    				instance_data[0].Adiags_data[ii].firstPathAmp1 = 0;
    				instance_data[0].Adiags_data[ii].firstPathAmp2 = 0;
    				instance_data[0].Adiags_data[ii].firstPathAmp3 = 0;
    				instance_data[0].Adiags_data[ii].maxGrowthCIR = 0;
    				instance_data[0].Adiags_data[ii].maxNoise = 0;
    				instance_data[0].Adiags_data[ii].rxPreamCount = 0;
    				instance_data[0].Adiags_data[ii].stdNoise = 0;
    			}
//    			for(uint8 ii=0;ii<MAX_TAG_LIST_SIZE;ii++)
    			for(uint8 ii=0;ii<MAX_BUFF_SIZE;ii++)
    			{
    				instance_data[0].Tdiags_data[ii].firstPath = 0;
    				instance_data[0].Tdiags_data[ii].firstPathAmp1 = 0;
    				instance_data[0].Tdiags_data[ii].firstPathAmp2 = 0;
    				instance_data[0].Tdiags_data[ii].firstPathAmp3 = 0;
    				instance_data[0].Tdiags_data[ii].maxGrowthCIR = 0;
    				instance_data[0].Tdiags_data[ii].maxNoise = 0;
    				instance_data[0].Tdiags_data[ii].rxPreamCount = 0;
    				instance_data[0].Tdiags_data[ii].stdNoise = 0;
    			}
#endif
    		} //if new range present

    		if(n > 0 && pauseUsbReports == 0)
    		{
    			send_usbmessage(&usbVCOMout[0], n);
    			usb_run();
#if ENABLE_CIR_DATA == 1
    			if(instance_data[0].enableCirData && instance_data[0].mode != TAG)
    				sendCIRdata();
#endif
    		}
    		n = 0;
    	}
    	else if(pauseUsbReports == 0 && instance_data[0].opMode == TOA) // TOA mode
    	{
    		instance_run_TOA();

//    		uint8 ll;

    		if(instance_data[0].updateLcdOn && (printLCDTWRReports + lcdUpdatePeriod <= portGetTickCnt()))
    		{
    			//led_on(LED_PC9);
    			command = 0x2 ;  //return cursor home
    			writetoLCD( 1, 0,  &command);

    			memset(dataseq1, ' ', LCD_BUFF_LEN);
    			writetoLCD( 40, 1, dataseq); //send some data

    			//anchors will print a range to each tag in sequence with 1 second pause
    			//they will show the last rage to that tag
    			if(instance_mode == ANCHOR || instance_mode == ANCHOR_SYNC)
    			{
    				for(uint8 ll=0;ll<MAX_BUFF_SIZE;ll++)
    				{
    					if(instance_data[0].NewBlinkData[ll] == 1)
    					{
    						int tagaddr = instance_data[0].TagIdBlinkArray[ll];
    						int diffTimestamp = (int)instance_data[0].diffRxTimestamp[ll];
    						sprintf((char*)&dataseq1[0], "A%d T%d: %d", ancaddr, tagaddr, diffTimestamp);
    						writetoLCD( 16, 1, dataseq1); //send some data
    					}
    				}
//    				int b = 0;
//    				int diffTimestamp = (int)instance_data[0].diffRxTimestamp[toggle] ;
//
//    				while(((int) (diffTimestamp)) == 0) //if 0 then go to next tag
//    				{
//    					if(b > instance_data[0].maxSupportedTagTOA)
//    						break;
//
//    					toggle++;
//
//    					if(toggle >= instance_data[0].maxSupportedTagTOA)
//    						toggle = 0;
//    					if(instance_data[0].NewBlinkData[toggle % MAX_BUFF_SIZE] == 1)
//    						diffTimestamp = (int)instance_data[0].diffRxTimestamp[toggle % MAX_BUFF_SIZE] ;
//    					b++;
//    				}

//    				sprintf((char*)&dataseq1[0], "ToA A%d T%d: %d", ancaddr, toggle, diffTimestamp);
//    				sprintf((char*)&dataseq1[0], "A%d T%d: %d", ancaddr, instance_data[0].TagIdBlinkArray[toggle % MAX_BUFF_SIZE], diffTimestamp);
//    				writetoLCD( 16, 1, dataseq1); //send some data

//    				toggle++;
//
//    				if(toggle >= instance_data[0].maxSupportedTagTOA)
//    					toggle = 0;
    			}
    			else if(instance_mode == TAG)
    			{
//    				sprintf((char*)&dataseq1[0], "ToA T%d A0: %d", tagaddr, (int)instance_data[0].diffTimestampTag[0]);
    				sprintf((char*)&dataseq1[0], "T%d: %d", tagaddr, (int)instance_data[0].diffTimestampTag[0]);
    				//toggle = 1;
    				writetoLCD( 16, 1, dataseq1); //send some data

//    				toggle++;
//
//    				if(toggle >= instance_data[0].maxAnchorsTwr)
//    					toggle = 0;
    			}
    			//led_off(LED_PC9);

    			//update the print time
    			printLCDTWRReports = portGetTickCnt();
    		}

    		for(uint8 ll=0;ll<MAX_BUFF_SIZE;ll++)
    		{
    			if(instance_data[0].NewBlinkData[ll] == 1) // send data via usb
    			{
    				rangeTime = instance_data[0].toaEvtTime[ll];
    				uint8 jj = 0;
    				usbVCOMout[jj++] = 'T';
    				usbVCOMout[jj++] = 'a';
    				usbVCOMout[jj++] = 'g';
    				usbVCOMout[jj++] = ' ';
    				usbVCOMout[jj++] = instance_data[0].TagIdBlinkArray[ll]; // Tag Id
    				usbVCOMout[jj++] = instance_data[0].instanceAddress16 & 0xFF; // Anchor Id (LSB)
    				usbVCOMout[jj++] = rangeTime >> 8;
    				usbVCOMout[jj++] = rangeTime;
    				usbVCOMout[jj++] = instance_data[0].SeqNumBlink[ll] >> 8;
    				usbVCOMout[jj++] = instance_data[0].SeqNumBlink[ll];
    				for(uint8 ii=0;ii<8;ii++)
    					usbVCOMout[jj++] = instance_data[0].syncRxTimestamp[ll] >> (56-ii*8);
    				for(uint8 ii=0;ii<8;ii++)
    					usbVCOMout[jj++] = instance_data[0].blinkRxTimestamp[ll] >> (56-ii*8);
    				for(uint8 ii=0;ii<8;ii++)
    					usbVCOMout[jj++] = instance_data[0].diffRxTimestamp[ll] >> (56-ii*8);
    				for(uint8 ii=0;ii<8;ii++)
    					usbVCOMout[jj++] = instance_data[0].diffTimestampTag[ll] >> (56-ii*8);
#if ENABLE_DIAGNOSTICS == 1
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].firstPathAmp1 >> 8;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].firstPathAmp1;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].firstPathAmp2 >> 8;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].firstPathAmp2;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].firstPathAmp3 >> 8;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].firstPathAmp3;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].maxGrowthCIR >> 8;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].maxGrowthCIR;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].rxPreamCount >> 8;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].rxPreamCount;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].maxNoise >> 8;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].maxNoise;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].stdNoise >> 8;
    				usbVCOMout[jj++] = instance_data[0].Tdiags_data[ll].stdNoise;
    				uint32 FPindex1000 = (uint32)(1000*instance_data[0].Tdiags_data[ll].firstPath);
    				for(uint8 kk = 0;kk<4;kk++)
    					usbVCOMout[jj++] = FPindex1000 >> (24-kk*8);
#endif
//#if ENABLE_CIR_DATA == 0
    				usbVCOMout[jj++] = '\r';
    				usbVCOMout[jj++] = '\n';
//#endif
    				n = jj;

    				// indicate that data has been copied to USB buffer
    				instance_data[0].NewBlinkData[ll] = 0;

    			// clear diagnostics data
//    			for(uint8 ii=0;ii<MAX_TAG_LIST_SIZE;ii++)
//    			{
    				instance_data[0].Tdiags_data[ll].firstPath = 0;
    				instance_data[0].Tdiags_data[ll].firstPathAmp1 = 0;
    				instance_data[0].Tdiags_data[ll].firstPathAmp2 = 0;
    				instance_data[0].Tdiags_data[ll].firstPathAmp3 = 0;
    				instance_data[0].Tdiags_data[ll].maxGrowthCIR = 0;
    				instance_data[0].Tdiags_data[ll].maxNoise = 0;
    				instance_data[0].Tdiags_data[ll].rxPreamCount = 0;
    				instance_data[0].Tdiags_data[ll].stdNoise = 0;
//    			}

    			}
    			if(n > 0 && pauseUsbReports == 0)
    			{
    				send_usbmessage(&usbVCOMout[0], n);
    				usb_run();
#if ENABLE_CIR_DATA == 1
//    				if(instance_data[0].enableCirData && instance_data[0].mode != TAG)
    				if(instance_data[0].enableCirData)
    					sendCIRdata();
#endif
    			}
    			n = 0;
    		}
    	}

//    		if(n > 0)
//    		{
//    			if(pauseTWRReports == 0)
//    			{
////       			if(counter >= 100 && instance_mode == ANCHOR)
////       			{
//    				send_usbmessage(&usbVCOMout[0], n);
//    				usb_run();
////       			}
////       			counter++;
//#if ENABLE_CIR_DATA == 1
//    				uint16 m, ii,jj,kk;
//
//    				asm("NOP"); asm("NOP"); asm("NOP");
//
//    				dwt_readaccdata(&instance_data[0].CIRdata[0], 2033, 0);
//    				m = 0;
//    				for(jj=0;jj<4;jj++)
//    				{
////       				m = 0;
//	#if USB_MSG_BINARY == 0
//    					m = 0;
//    					if(jj == 0)
//    						m += sprintf((char*)&usbVCOM_CIR[m], "CIR,");
//	#endif
//
//    					ii = 1;
//    					for(kk=0;kk<127;kk++)
//    					{
//    						CIR_real[kk+127*jj] = (int16)((uint16)instance_data[0].CIRdata[ii+0+508*jj] |
//    								(((uint16)instance_data[0].CIRdata[ii+1+508*jj]) << 8));
//
//    						CIR_imag[kk+127*jj] = (int16)((uint16)instance_data[0].CIRdata[ii+2+508*jj] |
//    								(((uint16)instance_data[0].CIRdata[ii+3+508*jj]) << 8));
//    						ii += 4;
//	#if USB_MSG_BINARY == 1
//    						usbVCOM_CIR[m++] = CIR_real[kk+127*jj] >> 8;
//    						usbVCOM_CIR[m++] = CIR_real[kk+127*jj];
//    						usbVCOM_CIR[m++] = CIR_imag[kk+127*jj] >> 8;
//    						usbVCOM_CIR[m++] = CIR_imag[kk+127*jj];
//	#else
//    						m += sprintf((char*)&usbVCOM_CIR[m], "%i,%i,", CIR_real[kk+127*jj],CIR_imag[kk+127*jj]);
//	#endif
//    					}
//
//	#if USB_MSG_BINARY == 0
//    					n = m;
//    					send_usbmessage(&usbVCOM_CIR[0], n);
//    					usb_run();
//    					asm("NOP"); asm("NOP"); asm("NOP");
//	#endif
//    				}
////	#if USB_MSG_BINARY == 1
////       			n = m;
////       			send_usbmessage(&usbVCOM_CIR[0], n);
////       			usb_run();
////       			asm("NOP"); asm("NOP"); asm("NOP");
////	#endif
//
//    				dwt_readaccdata(&instance_data[0].CIRdata[0], 2033, 2032);
////       			m = 0;
//    				for(jj=0;jj<4;jj++)
//    				{
////       				m = 0;
//	#if USB_MSG_BINARY == 0
//    					m = 0;
//	#endif
//    					ii = 1;
//    					for(kk=0;kk<127;kk++)
//    					{
//    						CIR_real[508+kk+127*jj] = (int16)((uint16)instance_data[0].CIRdata[ii+0+508*jj] |
//    								(((uint16)instance_data[0].CIRdata[ii+1+508*jj]) << 8));
//
//    						CIR_imag[508+kk+127*jj] = (int16)((uint16)instance_data[0].CIRdata[ii+2+508*jj] |
//    								(((uint16)instance_data[0].CIRdata[ii+3+508*jj]) << 8));
//    						ii += 4;
//	#if USB_MSG_BINARY == 1
//    						usbVCOM_CIR[m++] = CIR_real[508+kk+127*jj] >> 8;
//    						usbVCOM_CIR[m++] = CIR_real[508+kk+127*jj];
//    						usbVCOM_CIR[m++] = CIR_imag[508+kk+127*jj] >> 8;
//    						usbVCOM_CIR[m++] = CIR_imag[508+kk+127*jj];
//	#else
//    						m += sprintf((char*)&usbVCOM_CIR[m], "%i,%i,", CIR_real[508+kk+127*jj],CIR_imag[508+kk+127*jj]);
//	#endif
//    					}
//
//    					if(jj == 3) {
//	#if USB_MSG_BINARY == 0
//    						m += sprintf((char*)&usbVCOM_CIR[m], "\r\n");
//	#endif
//    					}
//	#if USB_MSG_BINARY == 0
//    					n = m;
//    					send_usbmessage(&usbVCOM_CIR[0], n);
//    					usb_run();
//    					asm("NOP"); asm("NOP"); asm("NOP");
//	#endif
//    				}
//	#if USB_MSG_BINARY == 1
//    				usbVCOM_CIR[m++] = 0x80;
//    				usbVCOM_CIR[m++] = 0x80;
//    				usbVCOM_CIR[m++] = 0x80;
//    				usbVCOM_CIR[m++] = 0x80;
//    				n = m;
//    				send_usbmessage(&usbVCOM_CIR[0], n);
//    				usb_run();
//    				asm("NOP"); asm("NOP"); asm("NOP");
//	#endif
//
//#endif // ENABLE_CIR_DATA
//    			}
//    			else
//    			{
//    				if(pauseTWRReports + 1000 <= portGetTickCnt())
//    				{
//    					pauseTWRReports = 0;
//    				}
//    			}
//    		}




        //led_on(LED_PC7);
//        usb_run();
        //led_off(LED_PC7);
    }


    return 0;
}



