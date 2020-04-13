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
#include "compiler.h"

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

#if 0 //ERWIN
  extern void usb_run(void);
  extern int usb_init(void);
  extern void usb_printconfig(int, uint8*, int);
  extern void send_usbmessage(uint8*, int);
#endif
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

/*DW1000 config function*/
static dwt_config_t example_init_config = {
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

#if 1 //for testing with the init-resp example
/* Preamble timeout, in multiple of PAC size. See NOTE 3 below. */
#define PRE_TIMEOUT 1000

/* Delay between frames, in UWB microseconds. See NOTE 1 below. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 100 

/*Should be accurately calculated during calibration*/
#define TX_ANT_DLY 16300
#define RX_ANT_DLY 16456	
uint32 inittestapplication(uint8 s1switch)
{
    uint32 devID = 0;
      /* Setup NRF52832 interrupt on GPIO 25 : connected to DW1000 IRQ*/
  vInterruptInit();
	
  /*Initialization UART*/
  boUART_Init ();
  printf("Singled Sided Two Way Ranging Initiator with Interrupt Example \r\n");
	
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
  dwt_configure(&example_init_config);

  /* Initialization of the DW1000 interrupt*/
  /* Callback are defined in ss_init_main.c */
  dwt_setcallbacks(instance_cbTxDone, instance_cbRxOk, instance_cbRxTo, instance_cbRxErr); //ERWIN dwt_setcallbacks(&tx_conf_cb, &rx_ok_cb, &rx_to_cb, &rx_err_cb);

  /* Enable wanted interrupts (TX confirmation, RX good frames, RX timeouts and RX errors). */
  dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | DWT_INT_RFTO | DWT_INT_RXPTO | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFSL | DWT_INT_SFDT, 1);

  /* Apply default antenna delay value. See NOTE 2 below. */
  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);

  /* Set preamble timeout for expected frames. See NOTE 3 below. */
  //dwt_setpreambledetecttimeout(0); // PRE_TIMEOUT
          
  /* Set expected response's delay and timeout. 
  * As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
  dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
  dwt_setrxtimeout(65000); // Maximum value timeout with DW1000 is 65ms  
    return devID;
}
#else
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
#endif

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
#if 0 //ERWIN
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

#define LED_TASK_DELAY        2000           /**< Task delay. Delays a LED0 task for 200 ms */
#define BUZZER_TASK_DELAY 1000
#define MAIN_TASK_DELAY 250

#ifdef USE_FREERTOS

TaskHandle_t  main_task_handle;   /**< Reference to main FreeRTOS task. */
TaskHandle_t  led_toggle_task_handle;   /**< Reference to LED0 toggling FreeRTOS task. */
TimerHandle_t buzzer_task_handle;  /**< Reference to Buzzer FreeRTOS timer. */


#endif

#ifdef USE_FREERTOS

/**@brief LED0 task entry function.
 *
 * @param[in] pvParameter   Pointer that will be used as the parameter for the task.
 */
#pragma GCC optimize ("O3")
static void led_toggle_task_function (void * pvParameter)
{
  UNUSED_PARAMETER(pvParameter);
  while (true)
  {
    //LEDS_INVERT(BSP_LED_0_MASK);
    /* Delay a task for a given number of ticks */
    vTaskDelay(LED_TASK_DELAY);
    /* Tasks must be implemented to never return... */
  }
}

/**@brief The function to call when the LED1 FreeRTOS timer expires.
 *
 * @param[in] pvParameter   Pointer that will be used as the parameter for the timer.
 */
//static void led_toggle_timer_callback (void * pvParameter)
//{
//  UNUSED_PARAMETER(pvParameter);
//  LEDS_INVERT(BSP_LED_1_MASK);
//}

/**@brief SS TWR Initiator task entry function.
*
* @param[in] pvParameter   Pointer that will be used as the parameter for the task.
*/
static void buzzer_task_function (void * pvParameter)
{
  UNUSED_PARAMETER(pvParameter);

  //dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

  //dwt_setleds(DWT_LEDS_ENABLE);

  while (true)
  {
    //Buzzer implementation here: PWM: duty cycle ? frequency ?
    /*
    if range to close
     toggle pin
    */
    //printf("test buzzer_task_function\n");
    /* Delay a task for a given number of ticks */
    //LEDS_INVERT(BSP_LED_1_MASK);
    vTaskDelay(BUZZER_TASK_DELAY);
    /* Tasks must be implemented to never return... */
  }
}
#else

#endif   // #ifdef USE_FREERTOS

/*
 * @fn      main()
 * @brief   main entry point
**/

#if 1 //test code for init-resp example

/* Frames used in the ranging process. See NOTE 1,2 below. */
static uint8 tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8 rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* Length of the common part of the message (up to and including the function code, see NOTE 1 below). */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4
/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;
/* Buffer to store received response message.
* Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 20
static uint8 rx_buffer[RX_BUF_LEN];
/*Transactions Counters */
static volatile int tx_count = 0 ; // Successful transmit counter
static volatile int rx_count = 0 ; // Successful receive counter
/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;
/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547
#endif
/*! ------------------------------------------------------------------------------------------------------------------
* @fn main()
*
* @brief Application entry point.
*
* @param  none
*
* @return none
*/
static void resp_msg_get_ts(uint8 *ts_field, uint32 *ts)
{
  int i;
  *ts = 0;
  for (i = 0; i < RESP_MSG_TS_LEN; i++)
  {
  *ts += ts_field[i] << (i * 8);
  }
}
void test_init_run(void)
{

  /* Loop forever initiating ranging exchanges. */


  /* Write frame data to DW1000 and prepare transmission. See NOTE 3 below. */
  tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
  dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
  dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1); /* Zero offset in TX buffer, ranging. */

  /* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay
  * set by dwt_setrxaftertxdelay() has elapsed. */
  dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

  /*Waiting for transmission success flag*/
  while (!(tx_ok_counter))
  {};

  if (tx_ok_counter)
  {
    tx_count++;
    printf("Transmission # : %d\r\n",tx_count);

    /*Reseting tx interrupt flag*/
    tx_ok_counter = 0 ;
  }

  /* Wait for reception, timeout or error interrupt flag*/
  while (!(rx_ok_counter || rx_timeout_counter|| rx_error_counter))
  {};

    if (rx_ok_counter > 0)
    {
      LEDS_INVERT(BSP_LED_0_MASK);
    }
    if (rx_error_counter > 0)
    {
      LEDS_INVERT(BSP_LED_1_MASK);
    }
    if (rx_timeout_counter > 0)
    {
      LEDS_INVERT(BSP_LED_2_MASK);
    }

  /* Increment frame sequence number after transmission of the poll message (modulo 256). */
  frame_seq_nb++;

  if (rx_ok_counter)
  {		
    uint32 frame_len;
#if 1
    /* A frame has been received, read it into the local buffer. */
    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
    if (frame_len <= RX_BUF_LEN)
    {
      dwt_readrxdata(rx_buffer, frame_len, 0);
    }

    /* Check that the frame is the expected response from the companion "SS TWR responder" example.
    * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
    rx_buffer[ALL_MSG_SN_IDX] = 0;
    if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
    {	
      rx_count++;
      printf("Reception # : %d\r\n",rx_count);
      float reception_rate = (float) rx_count / (float) tx_count * 100;
      printf("Reception rate # : %f\r\n",reception_rate);
      uint32 poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
      int32 rtd_init, rtd_resp;
      float clockOffsetRatio ;

      /* Retrieve poll transmission and response reception timestamps. See NOTE 4 below. */
      poll_tx_ts = dwt_readtxtimestamplo32();
      resp_rx_ts = dwt_readrxtimestamplo32();

      /* Read carrier integrator value and calculate clock offset ratio. See NOTE 6 below. */
      clockOffsetRatio = dwt_readcarrierintegrator() * (FREQ_OFFSET_MULTIPLIER * HERTZ_TO_PPM_MULTIPLIER_CHAN_5 / 1.0e6) ;

      /* Get timestamps embedded in response message. */
      resp_msg_get_ts(&rx_buffer[RESP_MSG_POLL_RX_TS_IDX], &poll_rx_ts);
      resp_msg_get_ts(&rx_buffer[RESP_MSG_RESP_TX_TS_IDX], &resp_tx_ts);

      /* Compute time of flight and distance, using clock offset ratio to correct for differing local and remote clock rates */
      rtd_init = resp_rx_ts - poll_tx_ts;
      rtd_resp = resp_tx_ts - poll_rx_ts;

      tof = ((rtd_init - rtd_resp * (1.0f - clockOffsetRatio)) / 2.0f) * DWT_TIME_UNITS; // Specifying 1.0f and 2.0f are floats to clear warning 
      distance = tof * SPEED_OF_LIGHT;
      printf("Distance : %f\r\n",distance);

      /*Reseting receive interrupt flag*/
      rx_ok_counter = 0; 
    }
#else
      printf("Reception\r\n");
      /*Reseting receive interrupt flag*/
      rx_ok_counter = 0;
#endif
   }

  if (rx_timeout_counter || rx_error_counter)
  {
    /* Reset RX to properly reinitialise LDE operation. */
    dwt_rxreset();

    /*Reseting interrupt flag*/
    rx_timeout_counter = 0 ;
    rx_error_counter = 0 ;
  }

    /* Execute a delay between ranging exchanges. */
    //     deca_sleep(RNG_DELAY_MS);
    //	return(1);
}

#pragma GCC optimize ("O3")
static void main_task_function(void * pvParameter)
{
    UNUSED_PARAMETER(pvParameter);

    


    int i = 0;
    int rx = 0;
    int toggle = 0;
    uint8 command = 0x0;
    pauseUsbReports = 0;


#if 0 //ERWIN
    led_off(LED_ALL); //turn off all the LEDs
#endif

#if 0 //ERWIN todo
    peripherals_init();

    spi_peripheral_init();
#endif

#if 0 //ERWIN
    Sleep(1000); //wait for LCD to power on

    initLCD();

    memset(dataseq, 0, LCD_BUFF_LEN);
    memcpy(dataseq, (const uint8 *) "FlandersMake App", 16);
    writetoLCD( 40, 1, dataseq); //send some data
    memcpy(dataseq, (const uint8 *) SOFTWARE_VER_STRING, 16); // Also set at line #26 (Should make this from single value !!!)
    writetoLCD( 16, 1, dataseq); //send some data

    Sleep(1000);
#endif

#if 0 //ERWIN todo
    // enable the USB functionality
    usb_init();
    Sleep(1000);
#endif

#if 0 //ERWIN
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
#else
    addr = sw1_addr = 0; //todo ERWIN

    // default data rate
    phy_datarate = DWT_BR_6M8;
#endif

    //ERWIN nrf_drv_gpiote_in_event_disable(DW1000_IRQ); //ERWIN todo port_DisableEXT_IRQ(); //disable ScenSor IRQ until we configure the device

    //run DecaRangeRTLS application for TREK

#if 0 //ERWIN
    command = 0x2 ;  //return cursor home
    writetoLCD( 1, 0,  &command);
    memset(dataseq, ' ', LCD_BUFF_LEN);
    memcpy(dataseq, (const uint8 *) "FlandersMake App ", 16);
    writetoLCD( 16, 1, dataseq); //send some data

    led_off(LED_ALL);

#endif

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
#if 0 //ERWIN
    	led_on(LED_ALL); //to display error....
    	dataseq[0] = 0x2 ;  //return cursor home
    	writetoLCD( 1, 0,  &dataseq[0]);
    	memset(dataseq, ' ', LCD_BUFF_LEN);
    	memcpy(dataseq, (const uint8 *) "ERROR   ", 12);
    	writetoLCD( 40, 1, dataseq); //send some data
    	memcpy(dataseq, (const uint8 *) "  INIT FAIL ", 12);
    	writetoLCD( 40, 1, dataseq); //send some data
#endif
    	//ERWIN todo return 0; //error
    }

#if 0
#if 0 //ERWIN
    // Configure USB for output, (i.e. not USB to SPI)
    usb_printconfig(16, (uint8 *)SOFTWARE_VER_STRING, s1switch);
#endif

#if 0 //ERWIN
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
#endif

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
//ERWIN todo    copyRangeBiasTable();
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
#if 0 //ERWIN
    send_tx_buff = 0;
#endif
//    // enable USART
//    usartinit();
#endif
    // main loop
    
    while(1)
    {
#if 1 //test code with init-resp example
        test_init_run();
#else
    	n = 0;
        printf("main_loop\n");

    	if(UpdateSettings) {
    		dwt_forcetrxoff();	//this will clear all events

    		nrf_drv_gpiote_in_event_disable(DW1000_IRQ); //ERWIN todo port_DisableEXT_IRQ(); //disable ScenSor IRQ until we configure the device

    		inittestapplication(0);

    		nrf_drv_gpiote_in_event_enable(DW1000_IRQ, true); //ERWIN todo port_EnableEXT_IRQ(); //enable ScenSor IRQ before starting

//    		updateUWBsettings();
//    		// initialize init state
//    		if(instance_data[0].opMode == TWR)
//    			instance_data[0].testAppState = TA_INIT ;
//    		else
//    			instance_data[0].testAppState = TOA_INIT ;
#if 0 //ERWIN
    		led_off(LED_ALL);
    		command = 0x2 ;  //return cursor home
    		writetoLCD( 1, 0,  &command);
    		memset(dataseq, ' ', LCD_BUFF_LEN);
    		memset(dataseq1, ' ', LCD_BUFF_LEN);
    		setLCDline1();
    		command = 0x2 ;  //return cursor home
    		writetoLCD( 1, 0,  &command);
    		memset(dataseq1, ' ', LCD_BUFF_LEN);
#endif
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
#endif
//    	if(pauseUsbReports>0)
//    	{
//    		if(portGetTickCnt()-pauseUsbReports > 20)
//    			pauseUsbReports = 0;
//    	}

    	int monitor_local = instance_data[0].monitor ;
    	txdiff = (portGetTickCnt() - instance_data[0].timeofTx);

    	if(pauseUsbReports == 0 && instance_data[0].opMode == TWR) // TWR mode
    	{
#if 0
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
#if 0 //ERWIN
    				//led_on(LED_PC9);
    				command = 0x2 ;  //return cursor home
    				writetoLCD( 1, 0,  &command);

    				memset(dataseq1, ' ', LCD_BUFF_LEN);
    				writetoLCD( 40, 1, dataseq); //send some data
#endif

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
#if 0 //ERWIN
    					sprintf((char*)&dataseq1[0], "A%d T%d: %3.2f m", ancaddr, toggle, rangetotag);
    					writetoLCD( 16, 1, dataseq1); //send some data
#endif

    					toggle++;

//    					if(toggle >= MAX_TAG_LIST_SIZE)
//    						toggle = 0;

    					if(toggle >= instance_data[0].maxTagsTwr)
    						toggle = 0;
    				}
    				else if(instance_mode == TAG)
    				{
#if 0 //ERWIN
    					sprintf((char*)&dataseq1[0], "T%d A%d: %3.2f m", tagaddr, toggle, instancegetidist(toggle));
    					//toggle = 1;
    					writetoLCD( 16, 1, dataseq1); //send some data
#endif

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
    			SysTickCnt = portGetTickCnt();

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
#if 0 //ERWIN
    			send_usbmessage(&usbVCOMout[0], n);
    			usb_run();
#endif
#if ENABLE_CIR_DATA == 1
    			if(instance_data[0].enableCirData && instance_data[0].mode != TAG)
    				sendCIRdata();
#endif
    		}
    		n = 0;
#endif
    	}
    	else if(pauseUsbReports == 0 && instance_data[0].opMode == TOA) // TOA mode
    	{
    		instance_run_TOA();

//    		uint8 ll;

#if 0 //ERWIN
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
#endif
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
#if 0 //ERWIN
    				send_usbmessage(&usbVCOMout[0], n);
    				usb_run();
#endif
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

      /* Delay a task for a given number of ticks */
      //LEDS_INVERT(BSP_LED_2_MASK);
      vTaskDelay(MAIN_TASK_DELAY);
      /* Tasks must be implemented to never return... */
    }


}

int main(void)
{
  /* Setup some LEDs for debug Green, Blue, Red on DWM1001-DEV */
  LEDS_CONFIGURE(BSP_LED_0_MASK | BSP_LED_1_MASK | BSP_LED_2_MASK);
  LEDS_OFF(BSP_LED_0_MASK | BSP_LED_1_MASK | BSP_LED_2_MASK );

  #ifdef USE_FREERTOS
    /* Create task for LED blinking*/
    UNUSED_VARIABLE(xTaskCreate(led_toggle_task_function, "LED", configMINIMAL_STACK_SIZE, NULL, 1, &led_toggle_task_handle));

    /* Create task for the buzzer */
    UNUSED_VARIABLE(xTaskCreate(buzzer_task_function, "BUZZER", configMINIMAL_STACK_SIZE, NULL, 2, &buzzer_task_handle));

    /* Create task for the main loop */
    UNUSED_VARIABLE(xTaskCreate(main_task_function, "MAIN LOOP", configMINIMAL_STACK_SIZE + 100, NULL, 3, &main_task_handle)); //Low priority numbers denote low priority tasks.
  #endif  

  #ifdef USE_FREERTOS		
    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();	

    while(1) 
    {
    };
  #else

    // No RTOS task here so just call the main loop here.
    // Loop forever responding to ranging requests.
    while (1)
    {
      //main_task_function();
    }

  #endif

}

//someone needs to define it

unsigned long portGetTickCnt(void)
{
  return xTaskGetTickCount();
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




