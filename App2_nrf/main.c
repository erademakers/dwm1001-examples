/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include "sdk_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "bsp.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_spi.h"
#include "nrf_uart.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf.h"
#include "app_error.h"
#include "app_util_platform.h"
#include "app_error.h"
#include <string.h>
#include "port_platform.h"
#include "deca_types.h"
#include "deca_param_types.h"
#include "deca_regs.h"
#include "deca_device_api.h"
#include "nrf_drv_gpiote.h"
#include "UART.h"
#include "instance.h"
	
//-----------------dw1000----------------------------

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
//extern instance_data_t instance_data[NUM_INST];
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

/*DWM1000 interrupt initialization and handler definition*/

/*!
* Interrupt handler calls the DW1000 ISR API. Call back corresponding to each event defined in ss_init_main
*/
void vInterruptHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  instance_process_irq(0); //ERWIN dwt_isr(); // DW1000 interrupt service routine 
}

/*!
* @brief Configure an IO pin as a positive edge triggered interrupt source.
*/
void vInterruptInit (void)
{
  ret_code_t err_code;

  if (nrf_drv_gpiote_is_init())
    printf("nrf_drv_gpiote_init already installed\n");
  else
    nrf_drv_gpiote_init();

  // input pin, +ve edge interrupt, no pull-up
  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
  in_config.pull = NRF_GPIO_PIN_NOPULL;

  // Link this pin interrupt source to its interrupt handler
  err_code = nrf_drv_gpiote_in_init(DW1000_IRQ, &in_config, vInterruptHandler);
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_event_enable(DW1000_IRQ, true);
}

uint32 inittestapplication(uint8 s1switch)
{
    uint32 devID ;
//    instanceConfig_t uwbPhyConfig;
    int result;

    //-------------dw1000  ini------------------------------------	

    /* Setup NRF52832 interrupt on GPIO 25 : connected to DW1000 IRQ*/
    vInterruptInit();
	
    /*Initialization UART*/
    boUART_Init ();

    instance_data[0].opMode = opr_mode;
    instance_data[0].commandSeqNum = opr_mode_seq;
    instance_data[0].PeriodSync = sync_period_toa;

    port_set_dw1000_slowrate(); //ERWIN SPI_ConfigFastRate(SPI_BaudRatePrescaler_32);  //max SPI before PLLs configured is ~4M

    //this is called here to wake up the device (i.e. if it was in sleep mode before the restart)
    devID = instancereaddeviceid() ;
    if(DWT_DEVICE_ID != devID) //if the read of device ID fails, the DW1000 could be asleep
    {
    	//reset the DW1000 by driving the RSTn line low
    	reset_DW1000();

#if 0 //ERWIN todo check if needed for nordic
        nrf_gpio_pin_clear(NRF_DRV_SPI_PIN_NOT_USED); //ERWIN port_SPIx_clear_chip_select();  //CS low
        Sleep(1);   //200 us to wake up then waits 5ms for DW1000 XTAL to stabilise
        nrf_gpio_pin_set(NRF_DRV_SPI_PIN_NOT_USED); //ERWIN port_SPIx_set_chip_select();  //CS high
#endif
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

    port_set_dw1000_fastrate(); //ERWIN todo SPI_ConfigFastRate(SPI_BaudRatePrescaler_4); //increase SPI to max
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
#if 0 //ERWIN
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
#endif
void process_dwRSTn_irq(void)
{
    instance_notify_DW1000_inIDLE(1);
}
#if 0
void process_deca_irq(void)
{
    do{

        instance_process_irq(0);

    }while(port_CheckEXT_IRQ() == 1); //while IRS line active (ARM can only do edge sensitive interrupts)

}
#endif

#if 0 //ERWIN
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
#endif

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

#if 0 //ERWIN
static dwt_config_t config = {
    5,                /* Channel number. */
    DWT_PRF_64M,      /* Pulse repetition frequency. */
    DWT_PLEN_128,     /* Preamble length. Used in TX only. */
    DWT_PAC8,         /* Preamble acquisition chunk size. Used in RX only. */
    10,               /* TX preamble code. Used in TX only. */
    10,               /* RX preamble code. Used in RX only. */
    0,                /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,       /* Data rate. */
    DWT_PHRMODE_STD,  /* PHY header mode. */
    (129 + 8 - 8)     /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};


/* Preamble timeout, in multiple of PAC size. See NOTE 3 below. */
#define PRE_TIMEOUT 1000

/* Delay between frames, in UWB microseconds. See NOTE 1 below. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 100 

/*Should be accurately calculated during calibration*/
#define TX_ANT_DLY 16300
#define RX_ANT_DLY 16456	

//--------------dw1000---end---------------

#endif

#define TASK_DELAY        200           /**< Task delay. Delays a LED0 task for 200 ms */
#define TIMER_PERIOD      2000          /**< Timer period. LED1 timer will expire after 1000 ms */

#ifdef USE_FREERTOS

TaskHandle_t  ss_initiator_task_handle;   /**< Reference to SS TWR Initiator FreeRTOS task. */
extern void ss_initiator_task_function (void * pvParameter);
TaskHandle_t  led_toggle_task_handle;   /**< Reference to LED0 toggling FreeRTOS task. */
TimerHandle_t led_toggle_timer_handle;  /**< Reference to LED1 toggling FreeRTOS timer. */
#endif

#ifdef USE_FREERTOS

/**@brief LED0 task entry function.
 *
 * @param[in] pvParameter   Pointer that will be used as the parameter for the task.
 */
static void led_toggle_task_function (void * pvParameter)
{
  UNUSED_PARAMETER(pvParameter);
  while (true)
  {
    LEDS_INVERT(BSP_LED_0_MASK);
    /* Delay a task for a given number of ticks */
    vTaskDelay(TASK_DELAY);
    /* Tasks must be implemented to never return... */
  }
}

/**@brief The function to call when the LED1 FreeRTOS timer expires.
 *
 * @param[in] pvParameter   Pointer that will be used as the parameter for the timer.
 */
static void led_toggle_timer_callback (void * pvParameter)
{
  UNUSED_PARAMETER(pvParameter);
  LEDS_INVERT(BSP_LED_1_MASK);
}
#else

  extern int ss_init_run(void);

#endif   // #ifdef USE_FREERTOS



int main(void)
{
#if 0 //ERWIN ORG example
  /* Setup some LEDs for debug Green and Blue on DWM1001-DEV */
  LEDS_CONFIGURE(BSP_LED_0_MASK | BSP_LED_1_MASK | BSP_LED_2_MASK);
  LEDS_ON(BSP_LED_0_MASK | BSP_LED_1_MASK | BSP_LED_2_MASK );

  #ifdef USE_FREERTOS
    /* Create task for LED0 blinking with priority set to 2 */
    UNUSED_VARIABLE(xTaskCreate(led_toggle_task_function, "LED0", configMINIMAL_STACK_SIZE + 200, NULL, 2, &led_toggle_task_handle));

    /* Start timer for LED1 blinking */
    led_toggle_timer_handle = xTimerCreate( "LED1", TIMER_PERIOD, pdTRUE, NULL, led_toggle_timer_callback);
    UNUSED_VARIABLE(xTimerStart(led_toggle_timer_handle, 0));

    /* Create task for SS TWR Initiator set to 2 */
    UNUSED_VARIABLE(xTaskCreate(ss_initiator_task_function, "SSTWR_INIT", configMINIMAL_STACK_SIZE + 200, NULL, 2, &ss_initiator_task_handle));
  #endif // #ifdef USE_FREERTOS
  
  //-------------dw1000  ini------------------------------------	

  /* Setup DW1000 IRQ pin */  
  nrf_gpio_cfg_input(DW1000_IRQ, NRF_GPIO_PIN_NOPULL); 		//irq
  
  /*Initialization UART*/
  boUART_Init ();
  printf("Singled Sided Two Way Ranging Initiator Example \r\n");
  
  /* Reset DW1000 */
  reset_DW1000(); 

  /* Set SPI clock to 2MHz */
  port_set_dw1000_slowrate();			
  
  /* Init the DW1000 */
  if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
  {
    //Init of DW1000 Failed
    while (1) {};
  }

  // Set SPI to 8MHz clock
  port_set_dw1000_fastrate();

  /* Configure DW1000. */
  dwt_configure(&config);

  /* Apply default antenna delay value. See NOTE 2 below. */
  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);

  /* Set preamble timeout for expected frames. See NOTE 3 below. */
  //dwt_setpreambledetecttimeout(0); // PRE_TIMEOUT
          
  /* Set expected response's delay and timeout. 
  * As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
  dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
  dwt_setrxtimeout(65000); // Maximum value timeout with DW1000 is 65ms  

  //-------------dw1000  ini------end---------------------------	
  // IF WE GET HERE THEN THE LEDS WILL BLINK

  #ifdef USE_FREERTOS		
    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();	

    while(1) 
    {};
  #else

    // No RTOS task here so just call the main loop here.
    // Loop forever responding to ranging requests.
    while (1)
    {
      ss_init_run();
    }

  #endif
#else



#endif
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The single-sided two-way ranging scheme implemented here has to be considered carefully as the accuracy of the distance measured is highly
 *    sensitive to the clock offset error between the devices and the length of the response delay between frames. To achieve the best possible
 *    accuracy, this response delay must be kept as low as possible. In order to do so, 6.8 Mbps data rate is used in this example and the response
 *    delay between frames is defined as low as possible. The user is referred to User Manual for more details about the single-sided two-way ranging
 *    process.  NB:SEE ALSO NOTE 11.
 * 2. The sum of the values is the TX to RX antenna delay, this should be experimentally determined by a calibration process. Here we use a hard coded
 *    value (expected to be a little low so a positive error will be seen on the resultant distance estimate. For a real production application, each
 *    device should have its own antenna delay properly calibrated to get good precision when performing range measurements.
 * 3. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive the complete response frame sent by the responder at the
 *    6.8M data rate used (around 200 s).
 * 4. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 5. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *     DW1000 API Guide for more details on the DW1000 driver functions.
 *
 ****************************************************************************************************************************************************/




