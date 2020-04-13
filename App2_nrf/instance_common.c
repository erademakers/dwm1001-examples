/*! ----------------------------------------------------------------------------
 *  @file    instance_common.c
 *  @brief   DecaWave application level common instance functions
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
#include "compiler.h"
#include "port_platform.h"
#include "deca_device_api.h"
//#include "deca_spi.h"

#include "instance.h"


// -------------------------------------------------------------------------------------------------------------------
//      Data Definitions
// -------------------------------------------------------------------------------------------------------------------

volatile int rx_error_counter = 0;
volatile int rx_ok_counter = 0;
volatile int rx_timeout_counter = 0;
volatile int tx_ok_counter = 0;
// -------------------------------------------------------------------------------------------------------------------
//double inst_tdist[MAX_TAG_LIST_SIZE] ;
//double inst_idist[MAX_ANCHOR_LIST_SIZE] ;
//double inst_idistraw[MAX_ANCHOR_LIST_SIZE] ;
double inst_tdist[MAX_BUFF_SIZE] ;
double inst_idist[MAX_BUFF_SIZE] ;
double inst_idist_ImRpt[MAX_BUFF_SIZE] ;
double inst_idistraw[MAX_BUFF_SIZE] ;
double inst_idistraw_ImRpt[MAX_BUFF_SIZE] ;
instance_data_t instance_data[NUM_INST] ;
double correction_factor;

//extern uint8 UART_txbuff[20];
//extern void send_uart_txdata(uint8 len);

//extern uint8 usbVCOMout[400];
//
//extern void usb_run(void);
//extern void send_usbmessage(uint8*, int);

//extern int instance_calcranges(uint32 *array, uint16 size, int reportRange, uint16* mask);

void ancprepareresponse(uint16 sourceAddress, uint8 srcAddr_index, uint8 fcode_index, uint8 *frame, uint32 uTimeStamp);
#if 0 //ERWIN
  void instance_txcallback(const dwt_callback_data_t *txd); //ERWIN
  void instance_rxcallback(const dwt_callback_data_t *rxd); //ERWIN
#endif
//void instance_cbTxDone(const dwt_cb_data_t *txd); //ERWIN
//void instance_cbRxOk(const dwt_cb_data_t *txd); //ERWIN
//void instance_cbRxTo(const dwt_cb_data_t *txd); //ERWIN
//void instance_cbRxErr(const dwt_cb_data_t *txd); //ERWIN
//void ancprepareresponse2(uint16 sourceAddress, uint8 srcAddr_index, uint8 fcode_index, uint8 *frame);

//int eventOutcount = 0;
//int eventIncount = 0;

// -------------------------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------
// convert microseconds to device time
uint64 convertmicrosectodevicetimeu (double microsecu)
{
    uint64 dt;
    long double dtime;

    dtime = (microsecu / (double) DWT_TIME_UNITS) / 1e6 ;

    dt =  (uint64) (dtime) ;

    return dt;
}

double convertdevicetimetosec(int32 dt)
{
    double f = 0;

    f =  dt * DWT_TIME_UNITS ;  // seconds #define TIME_UNITS          (1.0/499.2e6/128.0) = 15.65e-12

    return f ;
}

#pragma GCC optimize ("O3")
int reportTOF(int idx, uint32 tofx, uint8 isImRpt)
{
        double distance ;
        double distance_to_correct;
        double tof ;
        double range_bias;
//        double correction_factor;
        int32 tofi ;
        uint8 idx_corrfactor;

        // check for negative results and accept them making them proper negative integers
        tofi = (int32) tofx ; // make it signed
        if (tofi > 0x7FFFFFFF)  // close up TOF may be negative
        {
            tofi -= 0x80000000 ;  //
        }

        // convert to seconds (as floating point)
        tof = convertdevicetimetosec(tofi) ;          //this is divided by 4 to get single time of flight
        if(isImRpt)
        	inst_idistraw_ImRpt[idx] = distance = tof * SPEED_OF_LIGHT;
        else
        	inst_idistraw[idx] = distance = tof * SPEED_OF_LIGHT;

//#if (CORRECT_RANGE_BIAS == 1)
        if(instance_data[0].configData.chan < 7)
        	idx_corrfactor = instance_data[0].configData.chan-1;
        else
        	idx_corrfactor = 5;
        correction_factor = (double)(instance_data[0].BiasCorrFactor[idx_corrfactor])*0.001;
        if(instance_data[0].RangeBiasCorrection == 1)
        {
        //for the 6.81Mb data rate we assume gating gain of 6dB is used,
        //thus a different range bias needs to be applied
        //if(instance_data[instance].configData.dataRate == DWT_BR_6M8)
        distance_to_correct = distance;

        if(instance_data[0].smartPowerEn) //ERWIN if(instance_data[0].configData.smartPowerEn)
        {
//        	//1.31 for channel 2 and 1.51 for channel 5
//        	if(instance_data[0].configData.chan == 5)
//        	{
//        		distance_to_correct = distance/1.51;
//        	}
//        	else //channel 2
//        	{
//        		distance_to_correct = distance/1.31;
//			}
        	switch(instance_data[0].configData.chan)
        	{
        	case 1:
        	{
//        		distance_to_correct = 0.763*distance;
        		distance_to_correct = correction_factor*distance;
        		range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
//        		distance = distance - range_bias - 0.1;
        		distance = distance - range_bias;
        		break;
        	}
        	case 2:
        	{
//        		distance_to_correct = 0.763*distance;
        		distance_to_correct = correction_factor*distance;
        		range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
//        		distance = distance - range_bias - 0.1;
        		distance = distance - range_bias;
        		break;
        	}
        	case 3:
        	{
//        		distance_to_correct = 0.763*distance;
        		distance_to_correct = correction_factor*distance;
        		range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
//        		distance = distance - range_bias - 0.1;
        		distance = distance - range_bias;
        		break;
        	}
        	case 4:
        	{
//        		distance_to_correct = 0.763*distance;
        		distance_to_correct = correction_factor*distance;
        		range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
//        		distance = distance - range_bias - 0.1;
        		distance = distance - range_bias;
        		break;
        	}
//        	case 4:
//        	case 7:
//        	{
//        		distance_to_correct = distance;
//        		range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
////        		distance = distance - range_bias - 0.05;
//        		distance = distance - range_bias;
//        		break;
//        	}
        	case 5:
        	{
//        		distance_to_correct = 0.662*distance;
        		distance_to_correct = correction_factor*distance;
        		range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
        		distance = distance - range_bias;
        		break;
        	}
        	case 7:
        	{
//        		distance_to_correct = 0.662*distance;
        		distance_to_correct = correction_factor*distance;
        		range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
        		distance = distance - range_bias;
        		break;
        	}
//        	case 7:
//        	{
//        		distance_to_correct = distance;
//        		range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
//        		distance = distance - range_bias + 0.05;
//        		break;
//        	}
        	}
        }
        else
        {
        	distance_to_correct = distance;
        	distance = distance - dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
        }

//        range_bias = dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
//        distance = distance - range_bias;
//        distance = distance - dwt_getrangebias(instance_data[0].configData.chan, (float) distance_to_correct, instance_data[0].configData.prf);
        }
//#endif

//        if ((distance < 0) || (distance > 20000.000))    // discard any results less than <0 cm or >20 km
        if (distance > 20000.000)    // discard any results less than <0 cm or >20 km
            return 0;

        if(isImRpt)
        	inst_idist_ImRpt[idx] = distance;
        else
        {
        	inst_idist[idx] = distance;
        	instance_data[0].longTermRangeCount++ ;                          // for computing a long term average
        }

    return 1;
}// end of reportTOF

void setTagDist(int tidx, int aidx)
{
	inst_tdist[tidx] = inst_idist[aidx];
}

double getTagDist(int idx)
{
	return inst_tdist[idx];
}

void clearDistTable(int idx)
{
	inst_idistraw[idx] = 0;
	inst_idist[idx] = 0;
}

void clearDistTableImRpt(int idx)
{
	inst_idistraw_ImRpt[idx] = 0;
	inst_idist_ImRpt[idx] = 0;
}

void instancecleardisttableall(void)
{
	int i;

//	for(i=0; i<MAX_ANCHOR_LIST_SIZE; i++)
	for(i=0; i<MAX_BUFF_SIZE; i++)
	{
//		inst_idistraw[i] = 0xffff;
//		inst_idist[i] = 0xffff;
		inst_idistraw[i] = 0;
		inst_idist[i] = 0;
	}
}

void instancecleardisttableall_ImRpt(void)
{
	int i;

//	for(i=0; i<MAX_ANCHOR_LIST_SIZE; i++)
	for(i=0; i<MAX_BUFF_SIZE; i++)
	{
//		inst_idistraw[i] = 0xffff;
//		inst_idist[i] = 0xffff;
		inst_idistraw_ImRpt[i] = 0;
		inst_idist_ImRpt[i] = 0;
	}
}

// -------------------------------------------------------------------------------------------------------------------
//#if NUM_INST != 1
//#error These functions assume one instance only
//#else


// -------------------------------------------------------------------------------------------------------------------
// Set this instance role as the Tag, Anchor or Listener
void instancesetrole(int inst_mode)
{
    // assume instance 0, for this
    instance_data[0].mode =  inst_mode;                   // set the role
}

int instancegetrole(void)
{
    return instance_data[0].mode;
}

int instancenewrange(void)
{
	int x = instance_data[0].newRange;
    instance_data[0].newRange = TOF_REPORT_NUL;
    return x;
}

int instancenewrangeancadd(void)
{
    return instance_data[0].newRangeAncAddress;
}

int instancenewrangetagadd(void)
{
    return instance_data[0].newRangeTagAddress;
}

int instancenewrangetim(void)
{
    return instance_data[0].newRangeTime;
}

// -------------------------------------------------------------------------------------------------------------------
// function to clear counts/averages/range values
//
void instanceclearcounts(void)
{
    int instance = 0 ;
    int i= 0 ;

    instance_data[instance].rxTimeouts = 0 ;

    dwt_configeventcounters(1); //enable and clear - NOTE: the counters are not preserved when in DEEP SLEEP

    instance_data[instance].frameSN = 0;

    instance_data[instance].longTermRangeCount  = 0;


//    for(i=0; i<MAX_ANCHOR_LIST_SIZE; i++)
    for(i=0; i<MAX_BUFF_SIZE; i++)
	{
    	instance_data[instance].tofArray[i] = INVALID_TOF;
	}

//    for(i=0; i<MAX_TAG_LIST_SIZE; i++)
    for(i=0; i<MAX_BUFF_SIZE; i++)
	{
		instance_data[instance].tof[i] = INVALID_TOF;
	}

} // end instanceclearcounts()


// -------------------------------------------------------------------------------------------------------------------
// function to initialise instance structures
//
// Returns 0 on success and -1 on error
int instance_init(void)
{
    int instance = 0 ;
    int i;
    int result;

//    instance_data[instance].mode =  ANCHOR;                                // assume listener,
//    instance_data[instance].testAppState = TA_INIT ;
//    instance_data[instance].instToSleep = FALSE;


    // Reset the IC (might be needed if not getting here from POWER ON)
    // ARM code: Remove soft reset here as using hard reset in the inittestapplication() in the main.c file
    //dwt_softreset();

	//this initialises DW1000 and uses specified configurations from OTP/ROM
    //ERWIN result = dwt_initialise(DWT_LOADUCODE | DWT_LOADLDOTUNE | DWT_LOADTXCONFIG | DWT_LOADANTDLY| DWT_LOADXTALTRIM) ;
    result = dwt_initialise(DWT_LOADUCODE) ; //other defines don't exist  | DWT_LOADLDOTUNE | DWT_LOADTXCONFIG | DWT_LOADANTDLY| DWT_LOADXTALTRIM

    //this is platform dependent - only program if DW EVK/EVB
    dwt_setleds(3) ; //configure the GPIOs which control the leds on EVBs

    if (DWT_SUCCESS != result)
    {
        return (-1) ;   // device initialise has failed
    }


    instanceclearcounts() ;

    instance_data[instance].panID = 0xdeca ;

    instance_data[instance].wait4ack = 0;
    instance_data[instance].stopTimer = 0;
    instance_data[instance].instanceTimerEn = 0;

    instance_clearevents();

    //dwt_geteui(instance_data[instance].eui64);
    memset(instance_data[instance].eui64, 0, ADDR_BYTE_SIZE_L);

    instance_data[instance].tagSleepCorrection = 0;

    //ERWIN dwt_setautorxreenable(0); //disable auto RX re-enable
    dwt_setdblrxbuffmode(0); //disable double RX buffer

    // if using auto CRC check (DWT_INT_RFCG and DWT_INT_RFCE) are used instead of DWT_INT_RDFR flag
    // other errors which need to be checked (as they disable receiver) are
    //dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | (DWT_INT_SFDT | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);
    dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | (DWT_INT_ARFE | DWT_INT_RFSL | DWT_INT_SFDT | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO | DWT_INT_RXPTO), 1);

    
    dwt_setcallbacks(instance_cbTxDone, instance_cbRxOk, instance_cbRxTo, instance_cbRxErr); //ERWIN dwt_setcallbacks(instance_txcallback, instance_rxcallback);

    instance_data[instance].monitor = 0;

    instance_data[instance].lateTX = 0;
    instance_data[instance].lateRX = 0;

    instance_data[instance].responseTO = -1; //initialise
    for(i=0; i<128; i++)
    {
    	instance_data[instance].rxResps[i] = -10;
    }

    instance_data[instance].delayedReplyTime = 0;

    return 0 ;
}

// -------------------------------------------------------------------------------------------------------------------
//
// Return the Device ID register value, enables higher level validation of physical device presence
//

uint32 instancereaddeviceid(void)
{
    return dwt_readdevid() ;
}


// -------------------------------------------------------------------------------------------------------------------
//
// function to allow application configuration be passed into instance and affect underlying device operation
//
void instance_config(uwbPhyConfig_t *config, sfConfig_t *sfConfigTwr)
{
    int instance = 0 ;
    uint32 power = 0;
    uint8 otprev ;

    instance_data[instance].configData.chan = config->channelNumber ;
    instance_data[instance].configData.rxCode =  config->preambleCode ;
    instance_data[instance].configData.txCode = config->preambleCode ;
    instance_data[instance].configData.prf = config->pulseRepFreq ;
    instance_data[instance].configData.dataRate = config->dataRate ;
    instance_data[instance].configData.txPreambLength = config->preambleLen ;
    instance_data[instance].configData.rxPAC = config->pacSize ;
    instance_data[instance].configData.nsSFD = config->nsSFD ;
    instance_data[instance].configData.phrMode = DWT_PHRMODE_STD ;
    instance_data[instance].configData.sfdTO = config->sfdTO;


    //the DW1000 will automatically use gating gain for frames < 1ms duration (i.e. 6.81Mbps data rate)
    //smartPowerEn should be set based on the frame length, but we can also use dtaa rate.
    if(instance_data[instance].configData.dataRate == DWT_BR_6M8)
    {
        instance_data[instance].smartPowerEn = 1; //ERWIN instance_data[instance].configData.smartPowerEn = 1;
        dwt_setsmarttxpower(1); //ERWIN
    }
    else
    {
        instance_data[instance].smartPowerEn = 0; //ERWIN instance_data[instance].configData.smartPowerEn = 0;
        dwt_setsmarttxpower(0); //ERWIN
    }

    //configure the channel parameters
    dwt_configure(&instance_data[instance].configData /*ERWIN, DWT_LOADXTALTRIM*/);

    instance_data[instance].configTX.PGdly = txSpectrumConfig[config->channelNumber].PGdelay ;

#if 0 //ERWIN
    //firstly check if there are calibrated TX power value in the DW1000 OTP
    power = dwt_getotptxpower(config->pulseRepFreq, instance_data[instance].configData.chan);

    if((power == 0x0) || (power == 0xFFFFFFFF)) //if there are no calibrated values... need to use defaults
    {
        power = txSpectrumConfig[config->channelNumber].txPwr[config->pulseRepFreq- DWT_PRF_16M];
    }
#else
    power = 0; //ERWIN
#endif
    //Configure TX power
    instance_data[instance].configTX.power = power;

    //configure the tx spectrum parameters (power and PG delay)
    dwt_configuretxrf(&instance_data[instance].configTX);

    otprev = dwt_otprevision() ;  // this revision tells us how OTP is programmed.

//	if ((2 == otprev) || (3 == otprev))  // board is calibrated with TREK1000 with antenna delays set for each use case)
//	{
//		uint8 mode = (instance_data[instance].mode == ANCHOR ? 1 : 0);
////		uint8 chanindex = 0;
//
//		instance_data[instance].txAntennaDelay
//										= dwt_getTREKOTPantennadelay(mode,
//												instance_data[instance].configData.chan,
//												instance_data[instance].configData.dataRate) ;
//
//		// if nothing was actually programmed then set a reasonable value anyway
//		if ((instance_data[instance].txAntennaDelay == 0)
//				|| (instance_data[instance].txAntennaDelay == 0xffff))
//		{
////			if(instance_data[instance].configData.chan == 5)
////			{
////				chanindex = 1;
////			}
////			instance_data[instance].txAntennaDelay = rfDelaysTREK[chanindex];
//			instance_data[0].txAntennaDelay = rfDelays[config->pulseRepFreq - DWT_PRF_16M];
//		}
//
//	}
//	else // assume it is older EVK1000 programming.
//	{
//		//get the antenna delay that was read from the OTP calibration area
//		instance_data[instance].txAntennaDelay = dwt_readantennadelay(config->pulseRepFreq) >> 1;
//
//		// if nothing was actually programmed then set a reasonable value anyway
//		if ((instance_data[instance].txAntennaDelay == 0)
//				|| (instance_data[instance].txAntennaDelay == 0xffff))
//		{
//			instance_data[instance].txAntennaDelay = rfDelays[config->pulseRepFreq - DWT_PRF_16M];
//		}
//	}

    if(instance_data[0].antennaDelayChanged == 0)
    {
#if ANTENNA_USED ==	ANTENNA_TAOGLAS
    	instance_data[0].txAntennaDelay = rfDelaysAntTaoglas[config->channelNumber].antDelay[config->pulseRepFreq - DWT_PRF_16M];
    	memcpy(&instance_data[0].antDelaysTx,&rfDelaysAntTaoglas,sizeof(rfDelaysAntTaoglas));
    	memcpy(&instance_data[0].antDelaysRx,&rfDelaysAntTaoglas,sizeof(rfDelaysAntTaoglas));
#else
    	instance_data[0].txAntennaDelay = rfDelaysAntOrig[config->channelNumber].antDelay[config->pulseRepFreq - DWT_PRF_16M];
    	memcpy(&instance_data[0].antDelaysTx,&rfDelaysAntOrig,sizeof(rfDelaysAntOrig));
    	memcpy(&instance_data[0].antDelaysRx,&rfDelaysAntOrig,sizeof(rfDelaysAntOrig));
#endif
    	// -------------------------------------------------------------------------------------------------------------------
    	// set the antenna delay, we assume that the RX is the same as TX.
    	instance_data[instance].rxAntennaDelay = instance_data[instance].txAntennaDelay;
    }
    else
    {
    	instance_data[0].txAntennaDelay = instance_data[0].antDelaysTx[config->channelNumber].antDelay[config->pulseRepFreq - DWT_PRF_16M];
    	instance_data[0].rxAntennaDelay = instance_data[0].antDelaysRx[config->channelNumber].antDelay[config->pulseRepFreq - DWT_PRF_16M];
    }

	dwt_settxantennadelay(instance_data[instance].txAntennaDelay);
	dwt_setrxantennadelay(instance_data[instance].rxAntennaDelay);


    if(config->preambleLen == DWT_PLEN_64) //if preamble length is 64
    {
    	port_set_dw1000_slowrate(); //ERWIN SPI_ConfigFastRate(SPI_BaudRatePrescaler_32); //reduce SPI to < 3MHz

        dwt_loadopsettabfromotp(0);

        port_set_dw1000_fastrate(); //ERWIN SPI_ConfigFastRate(SPI_BaudRatePrescaler_4); //increase SPI to max
    }


////	instance_data[instance].sframePeriod = sfConfigTwr->sfPeriod;
//	instance_data[instance].slotPeriod = sfConfigTwr->slotPeriod;
//	instance_data[instance].tagSleepRnd = sfConfigTwr->slotPeriod;
////	instance_data[instance].numSlots = sfConfigTwr->numSlots;
//	instance_data[instance].numSlots = sfConfigTwr->maxTags;
//	instance_data[instance].sframePeriod = sfConfigTwr->numSlots*sfConfigTwr->slotPeriod;
//
//	sfConfigTwr->pollSleepDly = sfConfigTwr->numSlots*sfConfigTwr->slotPeriod;
//	instancesettagsleepdelay(sfConfigTwr->pollSleepDly); //set the Tag sleep time

//	instance_data[instance].maxSupportedAnchorTWR = sfConfigTwr->maxAnchor;
//	instance_data[instance].maxSupportedTagTWR = sfConfigTwr->maxTags;

	//last two slots are used for anchor to anchor ranging
	instance_data[instance].a0SlotTime = (instance_data[instance].numSlots-2) * instance_data[instance].slotPeriod;

	if(instance_data[instance].opMode == TOA)
	{
		instance_data[instance].maxMessageLength = 1+1+2+1+2+12+1;
			// FunctionCode(1), TagId(1), SequenceNum(2), PrevTagIdPosReport(1), PosReportSeqNum(2),
			// PosReportX(4), PosReportY(4), PosReportZ(4), PosReportValid(1)
		//set the default response delays
		instancesetreplydelay(1000, sfConfigTwr->maxAnchor);
	}
	else
	{
//		instance_data[instance].tagFinalMessageLength = 1+1+5+(5*sfConfigTwr->maxAnchor)+5+2;
		instance_data[instance].maxMessageLength = instance_data[instance].twrFinalMsgLen;
			// FunctionCode(1), Range Num (1), Poll_TxTime(5),
			// RespAnc0_RxTime(5),... RespAncN-1_RxTime(5),
			// Final_TxTime(5), Valid Response Mask (2),
		//set the default response delays
		instancesetreplydelay(sfConfigTwr->replyDly,sfConfigTwr->maxAnchor);
	}

}

// -------------------------------------------------------------------------------------------------------------------
// function to set the tag sleep time (in ms)
//
void instancesettagsleepdelay(int sleepdelay) //sleep in ms
{
    int instance = 0 ;
    instance_data[instance].tagSleepTime_ms = sleepdelay; //subtract the micro system delays (time it takes to switch states etc.)
}


int instancegetrnum(void) //get ranging number
{
	return instance_data[0].rangeNum;
}

int instancegetrnuma(int idx) //get ranging number
{
	return instance_data[0].rangeNumA[idx];
}

int instancegetrnumanc(int idx) //get ranging number
{
	return instance_data[0].rangeNumAAnc[idx];
}

int instancegetlcount(void) //get count of ranges used for calculation of lt avg
{
    int x = instance_data[0].longTermRangeCount;

    return (x);
}

double instancegetidist(int idx) //get instantaneous range
{
    double x ;

    idx &= (MAX_ANCHOR_LIST_SIZE - 1);

    x = inst_idist[idx];

    return (x);
}

double instancegetidistraw(int idx) //get instantaneous range (uncorrected)
{
    double x ;

    idx &= (MAX_ANCHOR_LIST_SIZE - 1);

    x = inst_idistraw[idx];

    return (x);
}

int instancegetidist_mm(int idx) //get instantaneous range
{
    int x ;

    idx &= (MAX_ANCHOR_LIST_SIZE - 1);

    x = (int)(inst_idist[idx]*1000);

    return (x);
}

int instancegetidistraw_mm(int idx) //get instantaneous range (uncorrected)
{
    int x ;

    idx &= (MAX_ANCHOR_LIST_SIZE - 1);

    x = (int)(inst_idistraw[idx]*1000);

    return (x);
}

void instance_backtoanchor(instance_data_t *inst)
{
	//stay in RX and behave as anchor
	inst->testAppState = TA_RXE_WAIT ;
	inst->mode = ANCHOR ;
	dwt_setrxtimeout(0);
	dwt_setpreambledetecttimeout(0);
	dwt_setrxaftertxdelay(0);
}


#pragma GCC optimize ("O3")
void inst_processrxtimeout(instance_data_t *inst)
{

    if(inst->opMode == TWR)
    {
    	//inst->responseTimeouts ++ ;
    	inst->rxTimeouts ++ ;
    	inst->done = INST_NOT_DONE_YET;
    	if(inst->mode == ANCHOR) //we did not receive the final - wait for next poll
    	{
    		//only enable receiver when not using double buffering
    		inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
    		dwt_setrxtimeout(0);

    	}
//    	else if(inst->mode == ANCHOR_SYNC) //send next sync broadcast
//    	{
//    		//    		dwt_forcetrxoff();	//this will clear all events
//    		inst->instToSleep = TRUE ;
//    		// initiate the re-transmission of the poll that was not responded to
//    		inst->testAppState = TA_TXE_WAIT ;
//    		inst->nextState = TWR_SYNC_BCAST ;
//    	}
    	else if(inst->mode == TAG)
    	{
    		//if tag times out - no response (check if we are to send a final)
    		//send the final only if it has received response from anchor 0
    		//        if((inst->previousState == TA_TXPOLL_WAIT_SEND) && ((inst->rxResponseMask & 0x0001) == 0))
    		if((inst->previousState == TA_TXPOLL_WAIT_SEND && inst->rxResponseMask == 0) || (inst->previousState == TA_TXFINAL_WAIT_SEND))
    		{
    			inst->instToSleep = TRUE ; //set sleep to TRUE so that tag will go to DEEP SLEEP before next ranging attempt
    			inst->testAppState = TA_TXE_WAIT ;
    			inst->nextState = TA_TXPOLL_WAIT_SEND ;
    		}
//    		else if (inst->previousState == TA_TXFINAL_WAIT_SEND) //got here from main (error sending final - handle as timeout)
//    		{
////    			if(inst->getReportReqs == 0)
////    			dwt_forcetrxoff();	//this will clear all events
//    			inst->instToSleep = TRUE ;
//    			// initiate the re-transmission of the poll that was not responded to
//    			inst->testAppState = TA_TXE_WAIT ;
//    			inst->nextState = TA_TXPOLL_WAIT_SEND ;
//    		}
    		else if((inst->previousState == TA_TXPOLL_WAIT_SEND && inst->rxResponseMask > 0)) //send the final
    		{
    			// initiate the re-transmission of the poll that was not responded to
    			inst->instToSleep = FALSE ;
    			inst->testAppState = TA_TXE_WAIT ;
    			inst->nextState = TA_TXFINAL_WAIT_SEND ;
    		}
    		else //send the final
    		{
    			// initiate the re-transmission of the poll that was not responded to
    			inst->testAppState = TA_TXE_WAIT ;
    			inst->nextState = TA_TXFINAL_WAIT_SEND ;
    		}

    	}
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
    	else //ANCHOR_RNG
    	{
    		//no Response form the other anchor
    		if(
    				((inst->previousState == TA_TXPOLL_WAIT_SEND)
    						&& ((A1_ANCHOR_ADDR == inst->instanceAddress16) && ((inst->rxResponseMaskAnc & 0x4) == 0)))
							||
							((inst->previousState == TA_TXPOLL_WAIT_SEND)
									&& ((GATEWAY_ANCHOR_ADDR == inst->instanceAddress16) && ((inst->rxResponseMaskAnc & 0x2) == 0)))
    		)
    		{
    			instance_backtoanchor(inst);
    		}
    		else if (inst->previousState == TA_TXFINAL_WAIT_SEND) //got here from main (error ending final - handle as timeout)
    		{
    			instance_backtoanchor(inst);
    		}
    		else //send the final
    		{
    			// initiate the re-transmission of the poll that was not responded to
    			inst->testAppState = TA_TXE_WAIT ;
    			inst->nextState = TA_TXFINAL_WAIT_SEND ;
    		}
    	}
#endif
    }
    else // TOA operation mode
    {
    	if(inst->mode == ANCHOR_SYNC) //we did not receive the blink - send next sync broadcast
    	{
//    		dwt_forcetrxoff();	//this will clear all events
    		inst->instToSleep = TRUE ;
    		// initiate the re-transmission of the poll that was not responded to
    		inst->testAppState = TA_TXE_WAIT ;
    		inst->nextState = TOA_SYNC_BCAST ;
    	}
    	else //(inst->mode == ANCHOR || inst->mode == TAG) //we did not receive the blink/sync - wait for next one
    	{
    		//only enable receiver when not using double buffering
    		inst->testAppState = inst->nextState = TA_RXE_WAIT ;              // wait for next frame
    		dwt_setrxtimeout(0);
    	}

    }
    //timeout - disable the radio (if using SW timeout the rx will not be off)
    //dwt_forcetrxoff() ;
}

//
// NB: This function is called from the (TX) interrupt handler
//
//ERWIN
#if 1 //for testing with init-resp example
void instance_cbTxDone(const dwt_cb_data_t *rxd)
{
  tx_ok_counter++;
}
#else
#pragma GCC optimize ("O3")
void instance_cbTxDone(const dwt_cb_data_t *txd)
{
        int instance = 0;
	uint8 txTimeStamp[5] = {0, 0, 0, 0, 0};
	uint8 txevent = DWT_SIG_TX_DONE; //ERWIN txd->event;
	event_data_t dw_event;

	dw_event.uTimeStamp = portGetTickCnt(); //ERWIN
	instance_data[instance].timeForceWakeUp = dw_event.uTimeStamp;

	if(txevent == DWT_SIG_TX_DONE)
	{
                tx_ok_counter++;
		//uint64 txtimestamp = 0;

		//NOTE - we can only get TX good (done) while here
		//dwt_readtxtimestamp((uint8*) &instance_data[instance].txu.txTimeStamp);

		dwt_readtxtimestamp(txTimeStamp) ;
		dw_event.timeStamp32l = (uint32)txTimeStamp[0] + ((uint32)txTimeStamp[1] << 8) + ((uint32)txTimeStamp[2] << 16) + ((uint32)txTimeStamp[3] << 24);
		dw_event.timeStamp = txTimeStamp[4];
	        dw_event.timeStamp <<= 32;
		dw_event.timeStamp += dw_event.timeStamp32l;
		dw_event.timeStamp32h = ((uint32)txTimeStamp[4] << 24) + (dw_event.timeStamp32l >> 8);

		instance_data[instance].stopTimer = 0;

		dw_event.rxLength = instance_data[instance].psduLength;
		dw_event.type =  0;
		dw_event.type_pend =  0;
		dw_event.type_save = DWT_SIG_TX_DONE;

		memcpy((uint8 *)&dw_event.msgu.frame[0], (uint8 *)&instance_data[instance].msg_f, instance_data[instance].psduLength);

		instance_putevent(dw_event, DWT_SIG_TX_DONE);

		instance_data[instance].txMsgCount++;

		instance_data[instance].txu.txTimeStamp = dw_event.timeStamp;

//#if VERSION == VER_1p0
//		if(instance_data[instance].previousState == TA_TXFINAL_WAIT_SEND)
//			if(instance_data[instance].tempTimestampTx3 != dw_event.timeStamp)
//				instance_data[instance].tempTimestampTx3 = dw_event.timeStamp;
//#endif

//		switch(instance_data[instance].mode)
//		{
//		case TAG:
//			{
//				if(instance_data[instance].previousState == TA_TXPOLL_WAIT_SEND)
//					instance_data[instance].tempTimestampTx1 = dw_event.timeStamp;
////					memcpy(&instance_data[instance].TimestampTx1,&txTimeStamp,5);
//				else if(instance_data[instance].previousState == TA_TXFINAL_WAIT_SEND)
//					instance_data[instance].tempTimestampTx3 = dw_event.timeStamp;
////					memcpy(&instance_data[instance].TimestampTx3,&txTimeStamp,5);
//			}
//			break;
//		case ANCHOR:
//			{
////				if(instance_data[instance].msg_f.messageData[FCODE] == RTLS_DEMO_MSG_ANCH_RESP)
////					memcpy(&instance_data[instance].TimestampTx2,&txTimeStamp,5);
//				instance_data[instance].TimestampTx2 = dw_event.timeStamp;
//				instance_data[instance].txu.anchorRespTxTime = dw_event.timeStamp;
//			}
//			break;
//		default:
//			break;
//		}
	}
	else if(txevent == DWT_SIG_TX_AA_DONE)
	{
		//auto ACK confirmation
		dw_event.rxLength = 0;
		dw_event.type =  0;
		dw_event.type_save = DWT_SIG_TX_AA_DONE;

		instance_putevent(dw_event, DWT_SIG_TX_AA_DONE);

		//printf("TX AA time %f ecount %d\n",convertdevicetimetosecu(instance_data[instance].txu.txTimeStamp), instance_data[instance].dweventCnt);
	}

	instance_data[instance].monitor = 0;
}
#endif
#if 0 //ERWIN
#pragma GCC optimize ("O3")
void instance_txcallback(const dwt_callback_data_t *txd)
{
	int instance = 0;
	uint8 txTimeStamp[5] = {0, 0, 0, 0, 0};
	uint8 txevent = 0; //ERWIN txd->event;
	event_data_t dw_event;

	dw_event.uTimeStamp = portGetTickCnt(); //ERWIN
	instance_data[instance].timeForceWakeUp = dw_event.uTimeStamp;

	if(txevent == DWT_SIG_TX_DONE)
	{
		//uint64 txtimestamp = 0;

		//NOTE - we can only get TX good (done) while here
		//dwt_readtxtimestamp((uint8*) &instance_data[instance].txu.txTimeStamp);

		dwt_readtxtimestamp(txTimeStamp) ;
		dw_event.timeStamp32l = (uint32)txTimeStamp[0] + ((uint32)txTimeStamp[1] << 8) + ((uint32)txTimeStamp[2] << 16) + ((uint32)txTimeStamp[3] << 24);
		dw_event.timeStamp = txTimeStamp[4];
	    dw_event.timeStamp <<= 32;
		dw_event.timeStamp += dw_event.timeStamp32l;
		dw_event.timeStamp32h = ((uint32)txTimeStamp[4] << 24) + (dw_event.timeStamp32l >> 8);

		instance_data[instance].stopTimer = 0;

		dw_event.rxLength = instance_data[instance].psduLength;
		dw_event.type =  0;
		dw_event.type_pend =  0;
		dw_event.type_save = DWT_SIG_TX_DONE;

		memcpy((uint8 *)&dw_event.msgu.frame[0], (uint8 *)&instance_data[instance].msg_f, instance_data[instance].psduLength);

		instance_putevent(dw_event, DWT_SIG_TX_DONE);

		instance_data[instance].txMsgCount++;

		instance_data[instance].txu.txTimeStamp = dw_event.timeStamp;

//#if VERSION == VER_1p0
//		if(instance_data[instance].previousState == TA_TXFINAL_WAIT_SEND)
//			if(instance_data[instance].tempTimestampTx3 != dw_event.timeStamp)
//				instance_data[instance].tempTimestampTx3 = dw_event.timeStamp;
//#endif

//		switch(instance_data[instance].mode)
//		{
//		case TAG:
//			{
//				if(instance_data[instance].previousState == TA_TXPOLL_WAIT_SEND)
//					instance_data[instance].tempTimestampTx1 = dw_event.timeStamp;
////					memcpy(&instance_data[instance].TimestampTx1,&txTimeStamp,5);
//				else if(instance_data[instance].previousState == TA_TXFINAL_WAIT_SEND)
//					instance_data[instance].tempTimestampTx3 = dw_event.timeStamp;
////					memcpy(&instance_data[instance].TimestampTx3,&txTimeStamp,5);
//			}
//			break;
//		case ANCHOR:
//			{
////				if(instance_data[instance].msg_f.messageData[FCODE] == RTLS_DEMO_MSG_ANCH_RESP)
////					memcpy(&instance_data[instance].TimestampTx2,&txTimeStamp,5);
//				instance_data[instance].TimestampTx2 = dw_event.timeStamp;
//				instance_data[instance].txu.anchorRespTxTime = dw_event.timeStamp;
//			}
//			break;
//		default:
//			break;
//		}
	}
	else if(txevent == DWT_SIG_TX_AA_DONE)
	{
		//auto ACK confirmation
		dw_event.rxLength = 0;
		dw_event.type =  0;
		dw_event.type_save = DWT_SIG_TX_AA_DONE;

		instance_putevent(dw_event, DWT_SIG_TX_AA_DONE);

		//printf("TX AA time %f ecount %d\n",convertdevicetimetosecu(instance_data[instance].txu.txTimeStamp), instance_data[instance].dweventCnt);
	}

	instance_data[instance].monitor = 0;
}
#endif
/**
 * @brief function to re-enable the receiver and also adjust the timeout before sending the final message
 * if it is time so send the final message, the callback will notify the application, else the receiver is
 * automatically re-enabled
 *
 * this function is only used for tag when ranging to other anchors
 */
uint8 tagrxreenable(uint16 sourceAddress)
{
	uint8 type_pend = DWT_SIG_DW_IDLE;
	int instance = 0;

	if(sourceAddress == GATEWAY_ANCHOR_ADDR + instance_data[0].maxAnchorsTwr-1)
		//if we got Response from last anchor (e.g. anchor MAX_ANCHORS_LIST-1) - this is the last expected response - send the final
		type_pend = DWT_SIG_DW_IDLE; //report timeout - send the final
	else
	{
		if(instance_data[instance].responseTO == 1) //can get here as result of error frame so need to check
		{
			//			dwt_setrxtimeout((uint16)instance_data[0].fwtoTime_sy * instance_data[0].responseTO * 2); //reconfigure the timeout
			//			dwt_setrxtimeout((uint16)instance_data[0].fwtoTime_sy * instance_data[0].responseTO); //reconfigure the timeout
			dwt_setrxtimeout((uint16)instance_data[0].fwtoTime_sy);
			dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
			type_pend = DWT_SIG_RX_PENDING ;
		}
		else if(instance_data[instance].responseTO > 1) //can get here as result of error frame so need to check
		{
//			dwt_setrxtimeout((uint16)instance_data[0].fwtoTime_sy * instance_data[0].responseTO * 2); //reconfigure the timeout
//			dwt_setrxtimeout((uint16)instance_data[0].fwtoTime_sy * instance_data[0].responseTO); //reconfigure the timeout
			dwt_setrxtimeout((uint16)instance_data[0].fwtoTime_sy * 2);
			dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
			type_pend = DWT_SIG_RX_PENDING ;
		}
		else //last response was not received (got error/frame was corrupt)
		{
			type_pend = DWT_SIG_DW_IDLE; //report timeout - send the final
		}
	}

	return type_pend;
}

#if(ANCTOANCTWR == 1) //allow anchor to anchor ranging
/**
 * @brief function to re-enable the receiver and also adjust the timeout before sending the final message
 * if it is time so send the final message, the callback will notify the application, else the receiver is
 * automatically re-enabled
 *
 * this function is only used for anchors (having a role of ANCHOR_RNG) when ranging to other anchors
 */
uint8 ancsendfinalorrxreenable(uint16 sourceAddress)
{
	uint8 type_pend = DWT_SIG_DW_IDLE;
	uint8 anc = sourceAddress & 0xFF;
	int instance = 0;

	if(instance_data[instance].instanceAddress16 == GATEWAY_ANCHOR_ADDR)
	{
		switch(anc)
		{
			//if we got Response from anchor 1 - go back to wait for next anchor's response
			case 1:
				dwt_setrxtimeout((uint16)instance_data[instance].fwtoTime_sy); //reconfigure the timeout
				dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
				type_pend = DWT_SIG_RX_PENDING ;
				break;

			//if we got Response from anchor 2 - this is the last expected response - send the final
			case 2:
			default:
				type_pend = DWT_SIG_DW_IDLE;
				break;
		}
	}

	if(instance_data[instance].instanceAddress16 == A1_ANCHOR_ADDR)
	{
		switch(anc)
		{
			//if we got Response from anchor 2 - this is the last expected response - send the final
			case 2:
			default:
				type_pend = DWT_SIG_DW_IDLE;
				break;
		}
	}
	return type_pend;
}
#endif

/**
 * @brief this function either enables the receiver (delayed)
 *
 **/
void ancenablerx(void)
{
	int instance = 0;
	//subtract preamble length
	dwt_setdelayedtrxtime(instance_data[instance].delayedReplyTime - instance_data[instance].fixedReplyDelayAncP) ;
	if(dwt_rxenable(DWT_START_RX_DELAYED)) //delayed rx
	{
		//if the delayed RX failed - time has passed - do immediate enable
		//led_on(LED_PC9);
		dwt_setrxtimeout((uint16)instance_data[instance].fwtoTimeAnc_sy*2); //reconfigure the timeout before enable
		//longer timeout as we cannot do delayed receive... so receiver needs to stay on for longer
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
		dwt_setrxtimeout((uint16)instance_data[instance].fwtoTimeAnc_sy); //restore the timeout for next RX enable
		instance_data[instance].lateRX++;
		//led_off(LED_PC9);
	}

}

/**
 * @brief this function either re-enables the receiver (delayed or immediate) or transmits the response frame
 *
 * @param the sourceAddress is the address of the sender of the current received frame
 * @param ancToAncTWR == 1 means that the anchor is ranging to another anchor, if == 0 then ranging to a tag
 *
 */
#pragma GCC optimize ("O0")
uint8 anctxorrxreenable(uint16 sourceAddress, int ancToAncTWR)
{
	uint8 type_pend = DWT_SIG_DW_IDLE;

	int instance = 0;

	if(instance_data[instance].responseTO == 0) //go back to RX without TO - ranging has finished. (wait for Final but no TO)
	{
		dwt_setrxtimeout(0); //reconfigure the timeout
		dwt_setpreambledetecttimeout(0);
	}
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
	int sendResp = 0;
	if((ancToAncTWR & 1) == 1)
	{
		if(instance_data[instance].responseTO == 1) //if one response left to receive (send a response now)
		{
			sendResp = 1;
		}
		//if A0 or A3 go back to RX as they do not send any responses when Anchor to Anchor ranging
		if((instance_data[instance].gatewayAnchor)
			|| (instance_data[instance].shortAdd_idx == 3)) //if this is anchor ID 3 do not reply to anchor poll
		{
			dwt_setrxtimeout(0);
			dwt_rxenable(DWT_START_RX_IMMEDIATE);
			return DWT_SIG_RX_PENDING ;
		}
	}
#endif

	//configure delayed reply time (this is incremented for each received frame) it is timed from Poll rx time
	instance_data[instance].delayedReplyTime += (instance_data[instance].fixedReplyDelayAnc >> 8);

	//this checks if to send a frame
//	if((((ancToAncTWR & 1) == 0) && ((instance_data[instance].responseTO + instance_data[instance].shortAdd_idx) == NUM_EXPECTED_RESPONSES)) //it's our turn to tx
//		|| (sendResp == 1))
//	if((instance_data[instance].responseTO + instance_data[instance].shortAdd_idx) == NUM_EXPECTED_RESPONSES)
	if((instance_data[instance].responseTO + instance_data[instance].shortAdd_idx) == (instance_data[instance].maxAnchorsTwr-1))
	{
		//led_on(LED_PC9);
		//response is expected
		instance_data[instance].wait4ack = DWT_RESPONSE_EXPECTED; //re has/will be re-enabled

		dwt_setdelayedtrxtime(instance_data[instance].delayedReplyTime) ;
		if(dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED))
		{
			//if TX has failed - we need to re-enable RX for the next response or final reception...
			dwt_setrxaftertxdelay(0);
			instance_data[instance].wait4ack = 0; //clear the flag as the TX has failed the TRX is off
			instance_data[instance].lateTX++;
			instance_data[instance].delayedReplyTime += 2*(instance_data[instance].fixedReplyDelayAnc >> 8); //to take into account W4R
			ancenablerx();
			type_pend = DWT_SIG_RX_PENDING ;
		}
		else
		{
			instance_data[instance].delayedReplyTime += (instance_data[instance].fixedReplyDelayAnc >> 8); //to take into account W4R
			type_pend = DWT_SIG_TX_PENDING ; // exit this interrupt and notify the application/instance that TX is in progress.
			instance_data[instance].timeofTx = portGetTickCnt();
			instance_data[instance].monitor = 1;
		}
		//led_off(LED_PC9);
	}
	else //stay in receive
	{
		if(sourceAddress == 0) //we got here after RX error, as we don't need to TX, we just enable RX
		{
			dwt_setrxtimeout(0);
			dwt_rxenable(DWT_START_RX_IMMEDIATE);
		}
		else
		{
			//led_on(LED_PC9);
			ancenablerx();
			//led_off(LED_PC9);
		}

		type_pend = DWT_SIG_RX_PENDING ;
	}
	//if time to send a response

	return type_pend;
}

/**
 * @brief this function handles frame error event, it will either signal TO or re-enable the receiver
 */
void handle_error_unknownframe(event_data_t dw_event)
{
	int instance = 0;
	//re-enable the receiver (after error frames as we are not using auto re-enable
	//for ranging application rx error frame is same as TO - as we are not going to get the expected frame
	if(instance_data[instance].mode == ANCHOR)
	{
		//if we are participating in the ranging (i.e. Poll was received)
		//and we get an rx error (in one of the responses)
		//need to consider this as a timeout as we could be sending our response next and
		//the applications needs to know to change the state
		//
		if(instance_data[instance].responseTO > 0)
		{
			instance_data[instance].responseTO--;

			//send a response or re-enable rx
			dw_event.type_pend = anctxorrxreenable(0, 0);
			dw_event.type = 0;
			dw_event.type_save = 0x40 | DWT_SIG_RX_TIMEOUT;
			dw_event.rxLength = 0;

			instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);
		}
		else
		{
			dwt_setrxtimeout(0); //reconfigure the timeout
			dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
		}
	}
	else if(instance_data[instance].mode == LISTENER)
	{
		dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
	}
	else
	{
//		instance_data[instance].responseTO--; //got something (need to reduce timeout (for remaining responses))

		if(instance_data[instance].opMode == TWR && instance_data[instance].enableReport == 0)
		{
			instance_data[instance].responseTO--; //got something (need to reduce timeout (for remaining responses))

			dw_event.type_pend = tagrxreenable(0); //check if receiver will be re-enabled or it's time to send the final
			dw_event.type = 0;
			dw_event.type_save = 0x40 | DWT_SIG_RX_TIMEOUT;
			dw_event.rxLength = 0;

			instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);
		}
		else
		{
//			dw_event.rxLength = 0;
//			dw_event.type = 0;
//			dw_event.type_save = 0x80 | DWT_SIG_RX_TIMEOUT;
//			//printf("PC timeout DWT_SIG_RX_TIMEOUT\n");
//			instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);

//			dwt_forcetrxoff();	//this will clear all events
			dwt_setrxtimeout(0); //reconfigure the timeout
//			dwt_setpreambledetecttimeout(0);
//			dwt_setrxaftertxdelay(0);
			dwt_rxenable(DWT_START_RX_IMMEDIATE) ;

//			return;
		}
	}
}

#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
/**
 * @brief this function prepares and writes the anchor to anchor response frame into the TX buffer
 * it is called after anchor receives a Poll from an anchor
 */
void ancprepareresponse2(uint16 sourceAddress, uint8 srcAddr_index, uint8 fcode_index, uint8 *frame)
{
	uint16 frameLength = 0;
	uint8 tof_idx = (sourceAddress) & 0xFF ;
	int instance = 0;

	instance_data[instance].psduLength = frameLength = ANCH_RESPONSE_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;
	//set the destination address (copy source as this is a reply)
	memcpy(&instance_data[instance].msg_f.destAddr[0], &frame[srcAddr_index], ADDR_BYTE_SIZE_S); //remember who to send the reply to (set destination address)
	instance_data[instance].msg_f.sourceAddr[0] = instance_data[instance].eui64[0];
	instance_data[instance].msg_f.sourceAddr[1] = instance_data[instance].eui64[1];
	// Write calculated TOF into response message (get the previous ToF+range number from that anchor)
	memcpy(&(instance_data[instance].msg_f.messageData[TOFR]), &instance_data[instance].tofAnc[tof_idx], 4);
	instance_data[instance].msg_f.messageData[TOFRN] = instance_data[instance].rangeNumAAnc[tof_idx]; //get the previous range number

	instance_data[instance].rangeNumAAnc[tof_idx] = 0; //clear the entry
	instance_data[instance].rangeNumAnc = frame[POLL_RNUM + fcode_index] ;
	instance_data[instance].msg_f.seqNum = instance_data[instance].frameSN++;

	//set the delayed rx on time (the final message will be sent after this delay)
	dwt_setrxaftertxdelay(instance_data[instance].ancRespRxDelay);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)

	instance_data[instance].tagSleepCorrection = 0;
	instance_data[instance].msg_f.messageData[RES_TAG_SLP0] = 0 ;
	instance_data[instance].msg_f.messageData[RES_TAG_SLP1] = 0 ;

	instance_data[instance].msg_f.messageData[FCODE] = RTLS_DEMO_MSG_ANCH_RESP2; //message function code (specifies if message is a poll, response or other...)

	//write the TX data
	dwt_writetxfctrl(frameLength, 0);
	dwt_writetxdata(frameLength, (uint8 *)  &instance_data[instance].msg_f, 0) ;	// write the frame data

}
#endif

/**
 * @brief this function prepares and writes the anchor to tag response frame into the TX buffer
 * it is called after anchor receives a Poll from a tag
 */
void ancprepareresponse(uint16 sourceAddress, uint8 srcAddr_index, uint8 fcode_index, uint8 *frame, uint32 uTimeStamp)
{
	uint16 frameLength = 0;
	uint8 tof_idx = (sourceAddress) & 0xFF ;
	uint8 tof_idx_ImRpt;
	int instance = 0;


	if(tof_idx == 0)
		tof_idx_ImRpt = instance_data[instance].maxTagsTwr - 1;
	else
		tof_idx_ImRpt = tof_idx-1;

	instance_data[instance].psduLength = frameLength = ANCH_RESPONSE_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;
//	memcpy(&instance_data[instance].msg_f.destAddr[0], &frame[srcAddr_index], ADDR_BYTE_SIZE_S); //remember who to send the reply to (set destination address)
	memcpy(&instance_data[instance].msg_f.destAddr[0], &frame[srcAddr_index], ADDR_BYTE_SIZE_S); //remember who to send the reply to (set destination address)
	instance_data[instance].msg_f.sourceAddr[0] = instance_data[instance].eui64[0];
	instance_data[instance].msg_f.sourceAddr[1] = instance_data[instance].eui64[1];

	// Write calculated TOF into response message (get the previous ToF+range number from that tag)
	memcpy(&(instance_data[instance].msg_f.messageData[TOFR]), &instance_data[instance].tof[tof_idx], 4);
	instance_data[instance].msg_f.messageData[TOFRN] = instance_data[instance].rangeNumA[tof_idx]; //get the previous range number

	memcpy(&(instance_data[instance].msg_f.messageData[TOFRPT]), &instance_data[instance].tof[tof_idx_ImRpt], 4); // report TOF
	instance_data[instance].msg_f.messageData[TOFRPT_N] = instance_data[instance].rangeNumA[tof_idx_ImRpt]; //get the range number of tag to be reported
	instance_data[instance].msg_f.messageData[TOFRPT_ID] = tof_idx_ImRpt; //tag ID for report

#if VERSION == VER_1p0
	memcpy(&instance_data[instance].msg_f.messageData[RESP_TIMESTAMP1],(uint8*)&instance_data[instance].TimestampRx1[0],5);
	memcpy(&instance_data[instance].msg_f.messageData[RESP_TIMESTAMP2],(uint8*)&instance_data[instance].TimestampTx2[0],5);
	memcpy(&instance_data[instance].msg_f.messageData[RESP_TIMESTAMP3],(uint8*)&instance_data[instance].TimestampRx3[0],5);
#else
//	instance_data[instance].msg_f.messageData[RESP_CMDSEQNUM] = instance_data[instance].commandSeqNum;
//	instance_data[instance].msg_f.messageData[RESP_CMDOPMODE] = instance_data[instance].opMode;
#endif
//	instance_data[instance].rangeNumA[tof_idx] = 0; //clear after copy above...
//	instance_data[instance].rangeNumA[tof_idx_ImRpt] = 0; //clear after copy above...
	instance_data[instance].rangeNum = frame[POLL_RNUM+fcode_index] ;
	instance_data[instance].msg_f.seqNum = instance_data[instance].frameSN++;

	//we have our range - update the own mask entry...
	if(instance_data[instance].tof[tof_idx] != INVALID_TOF) //check the last ToF entry is valid and copy into the current array
	{
		instance_data[instance].rxResponseMask = (0x0001 << instance_data[instance].shortAdd_idx);
		instance_data[instance].tofArray[instance_data[instance].shortAdd_idx] = instance_data[instance].tof[tof_idx];
	}
	else	//reset response mask
	{
		instance_data[instance].tofArray[instance_data[instance].shortAdd_idx] = INVALID_TOF ;
		instance_data[instance].rxResponseMask = 0;	//reset the mask of received responses when rx poll
	}

//	//we have our range - update the own mask entry (for the immediate reporting)...
//	if(instance_data[instance].tof[tof_idx_ImRpt] != INVALID_TOF) //check the last ToF entry is valid and copy into the current array
//	{
//		instance_data[instance].rxResponseMaskImRpt = (0x0001 << instance_data[instance].shortAdd_idx);
//		instance_data[instance].tofArrayImRpt[instance_data[instance].shortAdd_idx] = instance_data[instance].tof[tof_idx_ImRpt];
//	}
//	else	//reset response mask
//	{
//		instance_data[instance].tofArrayImRpt[instance_data[instance].shortAdd_idx] = INVALID_TOF ;
//		instance_data[instance].rxResponseMaskImRpt = 0;	//reset the mask of received responses when rx poll
//	}

	//set the delayed rx on time (the final message will be sent after this delay)
	dwt_setrxaftertxdelay(instance_data[instance].ancRespRxDelay);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)

	// transfer time
//	instance_data[instance].SyncedTime = portGetTickCnt() + instance_data[instance].OffsetTime; //ERWIN
//#if VERSION == VER_1p0
//	memcpy(&instance_data[instance].msg_f.messageData[RESP_TIMESTAMP3+5],(uint8*)&instance_data[instance].SyncedTime,4);
//#else
//	memcpy(&instance_data[instance].msg_f.messageData[TOFRN+1],(uint8*)&instance_data[instance].SyncedTime,4);
//#endif
	//if this is gateway anchor - calculate the slot period correction
	if(instance_data[instance].gatewayAnchor)
	{
		int error = 0;
		int currentSlotTime = 0;
		int expectedSlotTime = 0;

		//find the time in the current superframe
		currentSlotTime = uTimeStamp % instance_data[instance].sframePeriod;

		//this is the slot time the poll should be received in (Mask 0x07 for the 8 MAX tags we support in TREK)
		expectedSlotTime = (sourceAddress&0xFF) * instance_data[instance].slotPeriod; //

		//error = expectedSlotTime - currentSlotTime
		error = expectedSlotTime - currentSlotTime;

//		if(instance_data[instance].enableReport)
//		{
//			if(instance_data[instance].getReportReqs)
//			{
//				error = 0;
//				instance_data[instance].getReportResp = 1;
//			}
//			else
//				instance_data[instance].getReportResp = 0;
//		}
//		else
//			instance_data[instance].getReportResp = 1;

		if(error < (-(instance_data[instance].sframePeriod>>1))) //if error is more negative than 0.5 period, add whole period to give up to 1.5 period sleep
		{
			instance_data[instance].tagSleepCorrection = (instance_data[instance].sframePeriod + error);
		}
		else //the minimum Sleep time will be 0.5 period
		{
			instance_data[instance].tagSleepCorrection = error;
		}
		instance_data[instance].msg_f.messageData[RES_TAG_SLP0] = instance_data[instance].tagSleepCorrection & 0xFF ;
		instance_data[instance].msg_f.messageData[RES_TAG_SLP1] = (instance_data[instance].tagSleepCorrection >> 8) & 0xFF;
	}
	else
	{
		instance_data[instance].tagSleepCorrection = 0;
		instance_data[instance].msg_f.messageData[RES_TAG_SLP0] = 0 ;
		instance_data[instance].msg_f.messageData[RES_TAG_SLP1] = 0 ;
	}
//	if (isReport)
//		instance_data[instance].msg_f.messageData[FCODE] = RTLS_DEMO_MSG_REPORT_BCAST; //message function code (specifies if message is a poll, response or other...)
//	else
		instance_data[instance].msg_f.messageData[FCODE] = RTLS_DEMO_MSG_ANCH_RESP; //message function code
//		instance_data[instance].msg_f.messageData[RES_TAG_SLP1+1] = instance_data[instance].getReportResp;

	//write the TX data
	dwt_writetxfctrl(frameLength, 0, 0/*ERWIN*/);
	dwt_writetxdata(frameLength, (uint8 *)  &instance_data[instance].msg_f, 0) ;	// write the frame data

}

/**
 * @brief this is the receive event callback handler, the received event is processed and the instance either
 * responds by sending a response frame or re-enables the receiver to await the next frame
 * once the immediate action is taken care of the event is queued up for application to process
 */
//ERWIN
#if 1 //for testing with init-resp example
void instance_cbRxOk(const dwt_cb_data_t *rxd)
{
  rx_ok_counter++;
}
#else
#pragma GCC optimize ("O3")
void instance_cbRxOk(const dwt_cb_data_t *rxd)
{
	int instance = 0;
	uint8 rxTimeStamp[5]  = {0, 0, 0, 0, 0};

        uint8 rxd_event = 0;
	uint8 fcode_index  = 0;
	uint8 srcAddr_index = 0;
	uint8 dstAddr_index = 0;
	event_data_t dw_event;

        rx_ok_counter++;

//	uint8 aaa = 0;

	//microcontroller time at which we received the frame
        dw_event.uTimeStamp = portGetTickCnt(); //ERWIN
        instance_data[instance].timeForceWakeUp = dw_event.uTimeStamp;

        //if we got a frame with a good CRC - RX OK

        dw_event.rxLength = rxd->datalength;

        //need to process the frame control bytes to figure out what type of frame we have received
        if(((rxd->fctrl[0] == 0x41) || (rxd->fctrl[0] == 0x61))
                        &&
                        ((rxd->fctrl[1] & 0xCC) == 0x88)) //short address
        {

                fcode_index = FRAME_CRTL_AND_ADDRESS_S; //function code is in first byte after source address
                srcAddr_index = FRAME_CTRLP + ADDR_BYTE_SIZE_S;
                dstAddr_index = srcAddr_index - ADDR_BYTE_SIZE_S;
                rxd_event = DWT_SIG_RX_OKAY;
        }
        else
        {
                rxd_event = SIG_RX_UNKNOWN; //not supported - all TREK1000 frames are short addressed
        }

        //read RX timestamp
        dwt_readrxtimestamp(rxTimeStamp) ;
        dwt_readrxdata((uint8 *)&dw_event.msgu.frame[0], rxd->datalength, 0);  // Read Data Frame
    	dw_event.timeStamp32l =  (uint32)rxTimeStamp[0] + ((uint32)rxTimeStamp[1] << 8) + ((uint32)rxTimeStamp[2] << 16) + ((uint32)rxTimeStamp[3] << 24);
    	dw_event.timeStamp = rxTimeStamp[4];
    	dw_event.timeStamp <<= 32;
    	dw_event.timeStamp += dw_event.timeStamp32l;
    	dw_event.timeStamp32h = ((uint32)rxTimeStamp[4] << 24) + (dw_event.timeStamp32l >> 8);

        dw_event.type = 0; //type will be added as part of adding to event queue
		dw_event.type_save = rxd_event;
		dw_event.type_pend = DWT_SIG_DW_IDLE;

		uint16 sourceAddress = (((uint16)dw_event.msgu.frame[srcAddr_index+1]) << 8) + dw_event.msgu.frame[srcAddr_index];
		uint16 destinAddress = (((uint16)dw_event.msgu.frame[dstAddr_index+1]) << 8) + dw_event.msgu.frame[dstAddr_index];

		//if Listener then just report the received frame to the instance (application)
		if(rxd_event == DWT_SIG_RX_OKAY) //Process good/known frame types
		{
#if (READ_DIAGNOSTICS) == 1
			dwt_rxdiag_t diagn;
			dwt_readdiagnostics(&diagn);

			if((sourceAddress & 0xFF00) == 0x8000)
				memcpy(&(instance_data[instance].Adiags_data[(uint8)sourceAddress & 0xFF]), &diagn, sizeof(diagn));
			else
				memcpy(&(instance_data[instance].Tdiags_data[(uint8)sourceAddress & 0xFF]), &diagn, sizeof(diagn));
#endif
			if(instance_data[instance].mode != LISTENER)
			{
				if(instance_data[instance].mode == TAG) //if tag got a good frame - this is probably a response, but could also be some other non-ranging frame
					//(although due to frame filtering this is limited as non-addressed frames are filtered out)
				{
					instance_data[instance].responseTO--; //got 1 more response or other RX frame - need to reduce timeout (for next response)
				}

				//check if this is a TWR message (and also which one)
				switch(dw_event.msgu.frame[fcode_index])
				{
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
					//poll message from an anchor
					case RTLS_DEMO_MSG_ANCH_POLL:
					{
						//the anchor to anchor ranging poll frames are ignored by A0 and A3
						if(instance_data[instance].gatewayAnchor || (instance_data[instance].instanceAddress16 > A2_ANCHOR_ADDR))
						{
							//ignore poll from anchor 1 if gateway or anchor 3
							//anchors 2 and 3 will never send polls
							dw_event.type_pend = DWT_SIG_DW_IDLE ;
							break;
						}

						if(instance_data[instance].mode == TAG)  //tag should ignore any other Polls from anchors
						{
							instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
							handle_error_unknownframe(dw_event);
							instance_data[instance].stopTimer = 1;
							instance_data[instance].rxMsgCount++;
							return;
						}

						//update the response index and number of responses received tables
						instance_data[instance].rxRespsIdx = (uint8) ((dw_event.msgu.frame[POLL_RNUM+fcode_index] & 0xFF)
																+ (((sourceAddress & 0xFF) + 8) << 4));
						instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = 0;
						//debug LED on
						led_on(LED_PC9);

						//prepare the response and write it to the tx buffer
						ancprepareresponse2(sourceAddress, srcAddr_index, fcode_index, &dw_event.msgu.frame[0]);

						//A2 does not need timeout if ranging to A1
						if(sourceAddress != A1_ANCHOR_ADDR)
						{
							dwt_setrxtimeout((uint16)instance_data[instance].fwtoTimeAnc_sy); //reconfigure the timeout for response
						}

						//set the bast reply time (the actual will be Poll rx time + instance_data[0].fixedReplyDelayAnc)
						instance_data[instance].delayedReplyTime = dw_event.timeStamp32h ;
						instance_data[instance].responseTO = (instance_data[instance].instanceAddress16 - sourceAddress) & 0xFF; //set number of expected responses

						dw_event.type_pend = anctxorrxreenable(instance_data[instance].instanceAddress16, 2+1);

						instance_data[instance].tofAnc[sourceAddress & 0xFF] = INVALID_TOF; //clear ToF ..
						//debug LED off
						led_off(LED_PC9);
						break;
					}
#endif

					case RTLS_DEMO_MSG_TAG_POLL:
					{
						if(instance_data[instance].mode == TAG) //tag should ignore any other Polls from tags
						{
							instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
							handle_error_unknownframe(dw_event);
//							instance_data[instance].stopTimer = 1;
							instance_data[instance].rxMsgCount++;

//							dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
							return;
						}
						else
						{
							instance_data[instance].instanceWakeTime = portGetTickCnt(); //ERWIN
							instance_data[instance].timeForceWakeUp = instance_data[instance].instanceWakeTime;
							instance_data[instance].gatewayAnchorAddress = dw_event.msgu.frame[WHOISGATEWAY+fcode_index] | 0x8000;
//							instance_data[instance].getReportReqs = dw_event.msgu.frame[WHOISGATEWAY+fcode_index+1];

							if(instance_data[instance].instanceAddress16 == instance_data[instance].gatewayAnchorAddress)
							{
								instance_data[instance].gatewayAnchor = TRUE;

							}
							else
								instance_data[instance].gatewayAnchor = FALSE;
//#if VERSION == VER_1p0
//							memcpy(&instance_data[instance].TimestampTx1,&dw_event.msgu.frame[POLL_TIMESTAMP1+fcode_index],5);
//							memcpy(&instance_data[instance].TimestampRx2,&dw_event.msgu.frame[POLL_TIMESTAMP2+fcode_index],5);
//							memcpy(&instance_data[instance].TimestampTx3,&dw_event.msgu.frame[POLL_TIMESTAMP3+fcode_index],5);
//#endif
//							memcpy(&instance_data[instance].tempTimestampRx1,&rxTimeStamp,5);
						}

						instance_data[instance].rxRespsIdx = (int8) ((dw_event.msgu.frame[POLL_RNUM+fcode_index] & 0xf)
																+ ((sourceAddress & 0xFF) << 4));
						instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = 0;
						//debug LED on
						//ERWIN led_on(LED_PC9);
						//prepare the response and write it to the tx buffer
						ancprepareresponse(sourceAddress, srcAddr_index, fcode_index, &dw_event.msgu.frame[0], dw_event.uTimeStamp);

						dwt_setrxtimeout((uint16)instance_data[instance].fwtoTimeAnc_sy); //reconfigure the timeout for response

						instance_data[instance].delayedReplyTime = dw_event.timeStamp32h /*+ (instance_data[0].fixedReplyDelayAnc >> 8)*/ ;
//						instance_data[instance].responseTO = NUM_EXPECTED_RESPONSES; //set number of expected responses to 3 (from other anchors)
//						instance_data[instance].responseTO = (instance_data[instance].maxAnchorsTwr-1);
						instance_data[instance].responseTO = (instance_data[instance].maxAnchorsTwr);

						dw_event.type_pend = anctxorrxreenable(instance_data[instance].instanceAddress16, 2+0);

						instance_data[instance].tof[sourceAddress & 0xFF] = INVALID_TOF; //clear ToF ..
						//debug LED off
						//ERWIN led_off(LED_PC9);
					}
					break;

					//we got a response from a "responder" (anchor)
					case RTLS_DEMO_MSG_ANCH_RESP:
					case RTLS_DEMO_MSG_ANCH_RESP2:
					{
						//we are a tag
					    if(instance_data[instance].mode == TAG)
					    {
					    	instance_data[instance].timeForceWakeUp = portGetTickCnt(); //ERWIN
					    	if(destinAddress == instance_data[instance].instanceAddress16)
					    	{
					    		uint8 indexForFinalMsg;

					    		//							if(sourceAddress > 0x8000)
					    		//								aaa++;

					    		instance_data[instance].rxResps[instance_data[instance].rangeNum]++;
					    		dw_event.type_pend = tagrxreenable(sourceAddress); //responseTO decremented above...
					    		indexForFinalMsg = RRXT0 + 5*(sourceAddress & 0xFF);

					    		instance_data[instance].rxResponseMask |= (0x0001 << (sourceAddress & 0xFF)); //add anchor ID to the mask
					    		//							instance_data[instance].getReportResp |= dw_event.msgu.frame[RES_TAG_SLP1+fcode_index+1];
					    		// Write Response RX time field of Final message
					    		memcpy(&(instance_data[instance].msg_f.messageData[indexForFinalMsg]), rxTimeStamp, 5);
#if VERSION == VER_1p0
					    		//							memcpy(&instance_data[instance].TimestampRx1,&dw_event.msgu.frame[RESP_TIMESTAMP1+fcode_index],5);
					    		//							memcpy(&instance_data[instance].TimestampTx2,&dw_event.msgu.frame[RESP_TIMESTAMP2+fcode_index],5);
					    		//							memcpy(&instance_data[instance].TimestampRx3,&dw_event.msgu.frame[RESP_TIMESTAMP3+fcode_index],5);
					    		//
					    		//							instance_data[instance].TimestampTx1 = instance_data[instance].tempTimestampTx1;
					    		//							instance_data[instance].TimestampRx2 = instance_data[instance].tempTimestampRx2;
					    		//							instance_data[instance].TimestampTx3 = instance_data[instance].tempTimestampTx3;

					    		instance_data[instance].tempTimestampRx2 = dw_event.timeStamp;
#endif
					    		//							instance_data[instance].msg_f.messageData[VRESP1] = instance_data[instance].rxResponseMask;
					    		//							instance_data[instance].msg_f.messageData[VRESP2] = instance_data[instance].rxResponseMask >> 8;
					    		instance_data[instance].msg_f.messageData[instance_data[instance].finalTxTimeIdx+5] = instance_data[instance].rxResponseMask;
					    		instance_data[instance].msg_f.messageData[instance_data[instance].finalTxTimeIdx+6] = instance_data[instance].rxResponseMask >> 8;
					    	}
					    	else
					    		//					    	if(dw_event.msgu.frame[TOFRPT_ID+fcode_index] == (uint8)(instance_data[instance].instanceAddress16 & 0x00ff))
					    		if(dw_event.msgu.frame[TOFRPT_ID+fcode_index] == (uint8)(instance_data[instance].instanceAddress16 & 0x00ff) && \
					    				(dw_event.msgu.frame[TOFRPT_N+fcode_index] == instance_data[instance].rangeNum-1))
					    		{
					    			//					    		dwt_forcetrxoff();	//this will clear all events

					    			instance_data[instance].rxRespsImRpt[instance_data[instance].rangeNum]++;

					    			//copy the ToF and put into array (array holds last 4 ToFs)
					    			memcpy(&instance_data[instance].tofArrayImRpt[(sourceAddress & 0xFF)], &(dw_event.msgu.frame[TOFRPT+fcode_index]), 4);

					    			//check if the ToF is valid, this makes sure we only report valid ToFs
					    			//e.g. consider the case of reception of response from anchor a1 (we are anchor a2)
					    			//if a1 got a Poll with previous Range number but got no Final, then the response will have
					    			//the correct range number but the range will be INVALID_TOF
					    			if(instance_data[instance].tofArrayImRpt[(sourceAddress & 0xFF)] != INVALID_TOF)
					    				instance_data[instance].rxResponseMaskImRpt |= (0x0001 << (sourceAddress & 0xFF));

					    			//					    		led_on(LED_PC9);
					    			//					    		//send a response or re-enable rx
					    			dwt_setrxtimeout(0); //reconfigure the timeout
//					    			dwt_setpreambledetecttimeout(0);
					    			dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
					    			//					    		dw_event.type_pend = DWT_SIG_RX_PENDING ;
					    			//					    		instance_data[instance].testAppState = TA_RXE_WAIT;
					    			//					    		led_off(LED_PC9);

					    			//					    		instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
					    			//					    		handle_error_unknownframe(dw_event);
					    			//					    		instance_data[instance].stopTimer = 1;
					    			//					    		instance_data[instance].rxMsgCount++;
					    			return;

					    			//					    		dw_event.type_pend = tagrxreenable(sourceAddress); //responseTO decremented above...
					    		}
					    		else
					    		{
					    			instance_data[instance].tofArrayImRpt[(sourceAddress & 0xFF)] = INVALID_TOF;

					    			instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
					    			handle_error_unknownframe(dw_event);
//					    			instance_data[instance].stopTimer = 1;
					    			instance_data[instance].rxMsgCount++;

					    			//								dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
					    			return;
					    		}
					    }
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
						else if (instance_data[instance].mode == ANCHOR_RNG) //A0 and A1 only when ranging to other anchors
						{
							uint8 index ;
							instance_data[instance].rxResps[instance_data[instance].rangeNumAnc]++;
							dw_event.type_pend = ancsendfinalorrxreenable(sourceAddress);
							index = RRXT0 + 5*(sourceAddress & 0xFF);

							instance_data[instance].rxResponseMaskAnc |= (0x0001 << (sourceAddress & 0xFF)); //add anchor ID to the mask
							// Write Response RX time field of Final message
							memcpy(&(instance_data[instance].msg_f.messageData[index]), rxTimeStamp, 5);
						}
#endif
						else //normal anchor mode
						{
							instance_data[instance].instanceWakeTime = portGetTickCnt(); //ERWIN
							instance_data[instance].timeForceWakeUp = instance_data[instance].instanceWakeTime;

							//got a response... (check if we got a Poll with the same range number as in this response)
							if(RTLS_DEMO_MSG_ANCH_RESP == dw_event.msgu.frame[fcode_index])
							{
								if((instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] >= 0) //we got the poll else ignore this response
									&& (instance_data[instance].responseTO > 0)	) //if responseTO == 0 we have already received all of the responses - meaning should not be here => error
								{
									instance_data[instance].rxResps[instance_data[instance].rxRespsIdx]++; //increment the number of responses received
									instance_data[instance].responseTO--;

									//send a response or re-enable rx
									dw_event.type_pend = anctxorrxreenable(sourceAddress, 4+0);
								}
								else //like a timeout (error) ...
								{
									//ERWIN led_on(LED_PC9);
									//send a response or re-enable rx
									dwt_setrxtimeout(0); //reconfigure the timeout
									dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
									dw_event.type_pend = DWT_SIG_RX_PENDING ;
									//ERWIN led_off(LED_PC9);
								}
							}
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
							else //in anchor mode and got RTLS_DEMO_MSG_ANCH_RESP2
							{
								if((instance_data[instance].gatewayAnchor) &&
									(instance_data[instance].rxResps[instance_data[instance].rangeNumAnc]) == 2)
								{//got two responses A1 and A2 this is third (A2's to A1)
									instance_data[instance].rxResps[instance_data[instance].rangeNumAnc]++;
									instance_data[instance].rxResponseMaskAnc |= 0x8 ;

									dw_event.type_pend = anctxorrxreenable(sourceAddress, 4+1); //re-enable the RX
								}
								//A2 got A1's response to A0 - send A2 response (but only if we got the Poll from A0)
								else if((instance_data[instance].instanceAddress16 == A2_ANCHOR_ADDR) &&
										(instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] >= 0) )
								{
									instance_data[instance].rxResps[instance_data[instance].rxRespsIdx]++;
									instance_data[instance].responseTO--;

									dwt_setrxtimeout(0);
									dwt_setrxaftertxdelay(0); //clear rx on delay as Final will come sooner than if we were waiting for next Response
									dw_event.type_pend = anctxorrxreenable(sourceAddress, 1);
								}
								else // if other anchor A1, A2, A3 .. ignore these responses when in ANCHOR mode
								{
									dwt_setrxtimeout(0); //reconfigure the timeout
									dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
									dw_event.type_pend = DWT_SIG_RX_PENDING ;
								}
							}
#endif
						}

					}
					break;

					case RTLS_DEMO_MSG_TAG_FINAL:
					case RTLS_DEMO_MSG_ANCH_FINAL:
						if(instance_data[instance].mode == TAG) //tag should ignore any other Final from anchors
						{
							instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
							handle_error_unknownframe(dw_event);
//							instance_data[instance].stopTimer = 1;
							instance_data[instance].rxMsgCount++;

//							dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
							return;
						}
						else
						{
							instance_data[instance].instanceWakeTime = portGetTickCnt(); //ERWIN
							instance_data[instance].timeForceWakeUp = instance_data[instance].instanceWakeTime;
						}
//						else
//						{
//							int64 Rb, Da, Ra, Db ;
//							uint64 tagFinalTxTime  = 0;
//							uint64 tagFinalRxTime  = 0;
//							uint64 tagPollTxTime  = 0;
//							uint64 anchorRespRxTime  = 0;
//							uint64 tof = INVALID_TOF;
//
//							double RaRbxDaDb = 0;
//							double RbyDb = 0;
//							double RayDa = 0;
//
//							//								instance_data[instance].TimestampTx1 = 0;
//							//								instance_data[instance].TimestampRx2 = 0;
//							//								instance_data[instance].TimestampTx3 = 0;
//							//
//							//								instance_data[instance].TimestampRx1 = 0;
//							//								instance_data[instance].TimestampTx2 = 0;
//							//								instance_data[instance].TimestampRx3 = 0;
//
//							//								uint8 validResp = messageData[VRESP];
//							//								uint16 validResp = messageData[VRESP1] | (uint16)(messageData[VRESP2] << 8);
//							uint16 validResp = dw_event.msgu.frame[fcode_index+instance_data[instance].finalTxTimeIdx+5] | \
//									(uint16)(dw_event.msgu.frame[fcode_index+instance_data[instance].finalTxTimeIdx+6] << 8);
//							uint8 indexFromFinalMsg = RRXT0 + 5*(instance_data[instance].shortAdd_idx);
//
//							//								// get command
//							//								if(instance_data[instance].commandSeqNum < messageData[FINAL_CMDSEQNUM] || \
//							//										(instance_data[instance].commandSeqNum > 250 && messageData[FINAL_CMDSEQNUM] < 5))
//							//								{
//							//									instance_data[instance].commandSeqNum = messageData[FINAL_CMDSEQNUM];
//							//									instance_data[instance].opMode = messageData[FINAL_CMDOPMODE];
//							//									UpdateSettings = 1;
//							//								}
//
//							// get the status IO message
//							//								instance_data[instance].StatusIO1 = messageData[STATUSIO1FINAL];
//							//								instance_data[instance].StatusIO2 = messageData[STATUSIO2FINAL];
//							//								instance_data[instance].StatIOseqnum = messageData[STATIOSEQFINAL];
//
//							//								if(ANCHOR_SYNC == instance_data[instance].mode)
//							//								{
//							//									instance_data[instance].testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
//							//									instance_data[instance].nextState = TWR_SYNC_BCAST ;
//							//									instance_data[instance].instToSleep = TRUE;
//							//									//										instance_data[instance].instToSleep = FALSE;
//							//								}
//							//								else
//							if((RTLS_DEMO_MSG_TAG_FINAL == dw_event.msgu.frame[fcode_index]) &&
//									(instance_data[instance].rangeNumA[sourceAddress & 0xFF] != dw_event.msgu.frame[POLL_RNUM+fcode_index])) //Final's range number needs to match Poll's or else discard this message
//							{
//								instance_data[instance].testAppState = TA_RXE_WAIT ;              // wait for next frame
//								break;
//							}
//
//							/*if((RTLS_DEMO_MSG_ANCH_FINAL == fcode) &&
//																(((instance_data[instance].rangeNumAAnc[tof_idx] != messageData[POLL_RNUM]) //Final's range number needs to match Poll's or else discard this message
//																|| instance_data[instance].gatewayAnchor) //gateway can ignore the Final (from A1 to A2 exchange)
//																|| (A3_ANCHOR_ADDR == instance_data[instance].instanceAddress16))) //A3 does not care about Final from A1 or A0
//														{
//						                                    instance_data[instance].testAppState = TA_RXE_WAIT ;              // wait for next frame
//						                                    break;
//														}*/
//
//							/*if (A1_ANCHOR_ADDR == instance_data[instance].instanceAddress16) //this is A1
//						                                {
//						                                	if(GATEWAY_ANCHOR_ADDR == (srcAddr[0] | ((uint32)(srcAddr[1] << 8)))) //final is from A0
//						                                	{
//						                                		//ENABLE TIMER ONLY IF FINAL RECEIVED
//																instance_data[instance].instanceTimerEn = 1;
//						                                	}
//						                                }*/
//							//output data over USB...
//							instance_data[instance].newRangeAncAddress = instance_data[instance].instanceAddress16;
//
//							//if we got the final, maybe the tag did not get our response, so
//							//we can use other anchors responses/ToF if there are any.. and output..
//							//but we cannot calculate new range
//							if(((validResp & (0x0001<<(instance_data[instance].shortAdd_idx))) != 0))
//							{
//								// time of arrival of Final message
//								tagFinalRxTime = dw_event.timeStamp ; //Final's Rx time
//
//								//printf("FinalRx Timestamp: %4.15e\n", convertdevicetimetosecu(dw_event.timeStamp));
//								instance_data[instance].delayedReplyTime = 0 ;
//
//								// times measured at Tag extracted from the message buffer
//								// extract 40bit times
//								memcpy(&tagPollTxTime, &(dw_event.msgu.frame[PTXT+fcode_index]), 5);
//								memcpy(&anchorRespRxTime, &(dw_event.msgu.frame[indexFromFinalMsg+fcode_index]), 5);
//								//									memcpy(&tagFinalTxTime, &(dw_event.msgu.frame[FTXT+fcode_index]), 5);
//								memcpy(&tagFinalTxTime, &(dw_event.msgu.frame[instance_data[instance].finalTxTimeIdx+fcode_index]), 5);
//#if VERSION == VER_1p0
//								for(uint8 ii=0;ii<9;ii++)
//								{
//									instance_data[instance].TimestampTx1[9-ii] = instance_data[instance].TimestampTx1[8-ii];
//									instance_data[instance].TimestampTx2[9-ii] = instance_data[instance].TimestampTx2[8-ii];
//									instance_data[instance].TimestampTx3[9-ii] = instance_data[instance].TimestampTx3[8-ii];
//									instance_data[instance].TimestampRx1[9-ii] = instance_data[instance].TimestampRx1[8-ii];
//									instance_data[instance].TimestampRx2[9-ii] = instance_data[instance].TimestampRx2[8-ii];
//									instance_data[instance].TimestampRx3[9-ii] = instance_data[instance].TimestampRx3[8-ii];
//								}
//
//								instance_data[instance].TimestampTx1[0] = tagPollTxTime;
//								instance_data[instance].TimestampRx2[0] = anchorRespRxTime;
//								instance_data[instance].TimestampTx3[0] = tagFinalTxTime;
//
//								instance_data[instance].TimestampRx1[0] = instance_data[instance].tagPollRxTime & MASK_40BIT;
//								instance_data[instance].TimestampTx2[0] = instance_data[instance].txu.anchorRespTxTime & MASK_40BIT;
//								instance_data[instance].TimestampRx3[0] = tagFinalRxTime & MASK_40BIT;
//#endif
////									memcpy(&instance_data[instance].TimestampRx1[0],(uint8 *)&(instance_data[instance].tagPollRxTime),5);
//								//									memcpy(&instance_data[instance].TimestampTx2[0],(uint8 *)&(instance_data[instance].txu.anchorRespTxTime),5);
//								//									memcpy(&instance_data[instance].TimestampRx3[0],(uint8 *)&(tagFinalRxTime),5);
//
//								// poll response round trip delay time is calculated as
//								// (anchorRespRxTime - tagPollTxTime) - (anchorRespTxTime - tagPollRxTime)
//								Ra = (int64)((anchorRespRxTime - tagPollTxTime) & MASK_40BIT);
//								Db = (int64)((instance_data[instance].txu.anchorRespTxTime - instance_data[instance].tagPollRxTime) & MASK_40BIT);
//
//								// response final round trip delay time is calculated as
//								// (tagFinalRxTime - anchorRespTxTime) - (tagFinalTxTime - anchorRespRxTime)
//								Rb = (int64)((tagFinalRxTime - instance_data[instance].txu.anchorRespTxTime) & MASK_40BIT);
//								Da = (int64)((tagFinalTxTime - anchorRespRxTime) & MASK_40BIT);
//
//								RaRbxDaDb = (((double)Ra))*(((double)Rb))
//																	- (((double)Da))*(((double)Db));
//
//								RbyDb = ((double)Rb + (double)Db);
//
//								RayDa = ((double)Ra + (double)Da);
//
//								tof = (int32) ( RaRbxDaDb/(RbyDb + RayDa) );
//							}
//
//							//tag to anchor ranging
//							if(RTLS_DEMO_MSG_TAG_FINAL == dw_event.msgu.frame[fcode_index])
//							{
//								instance_data[instance].newRangeTagAddress = sourceAddress;
//								//time-of-flight
//								instance_data[instance].tof[instance_data[instance].newRangeTagAddress & 0xFF] = tof;
//								//calculate all tag - anchor ranges... and report
//								//									instance_data[instance].newRange = instance_calcranges(&instance_data[instance].tofArray[0], MAX_ANCHOR_LIST_SIZE, TOF_REPORT_T2A, &instance_data[instance].rxResponseMask);
//								instance_data[instance].newRange = instance_calcranges(&instance_data[instance].tofArray[0], instance_data[instance].maxAnchorsTwr, TOF_REPORT_T2A, &instance_data[instance].rxResponseMask);
//								instance_data[instance].rxResponseMaskReport = instance_data[instance].rxResponseMask; //copy the valid mask to report
//								instance_data[instance].rxResponseMask = 0;
//								//we have our range - update the own mask entry...
//								if(tof != INVALID_TOF) //check the last ToF entry is valid and copy into the current array
//								{
//									setTagDist(sourceAddress & 0xFF, instance_data[instance].shortAdd_idx); //copy distance from this anchor to the tag into array
//
//									instance_data[instance].rxResponseMask = (0x0001 << instance_data[instance].shortAdd_idx);
//									instance_data[instance].tofArray[instance_data[instance].shortAdd_idx] = tof;
//								}
//								instance_data[instance].newRangeTime = dw_event.uTimeStamp ;
//							}
////#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
////							else //anchor to anchor ranging
////							{
////								instance_data[instance].newRangeTagAddress = sourceAddress;
////								//time-of-flight
////								instance_data[instance].tofAnc[tof_idx] = tof;
////							}
////#endif
//							//reset the response count
//							if(instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] >= 0)
//							{
//								instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = -1 * instance_data[instance].rxResps[instance_data[instance].rxRespsIdx];
//								if(instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] == 0) //as A0 will have this as 0 when ranging to A1
//									instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = -1 ;
//							}
//
//							if (instance_data[instance].mode == ANCHOR)
//							{
//								if (instance_data[instance].enableReport)
//								{
//									instance_data[instance].rxRespsIdx = (int8) ((dw_event.msgu.frame[POLL_RNUM+fcode_index] & 0xf)
//											+ ((sourceAddress & 0xFF) << 4));
//									instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = 0;
//									//debug LED on
//									led_on(LED_PC9);
//									//prepare the response and write it to the tx buffer
//									ancprepareresponse(sourceAddress, srcAddr_index, fcode_index, &dw_event.msgu.frame[0], dw_event.uTimeStamp);
//
//									dwt_setrxtimeout((uint16)instance_data[instance].fwtoTimeAnc_sy); //reconfigure the timeout for response
//
//									instance_data[instance].delayedReplyTime = dw_event.timeStamp32h /*+ (instance_data[0].fixedReplyDelayAnc >> 8)*/ ;
//									//						instance_data[instance].responseTO = NUM_EXPECTED_RESPONSES; //set number of expected responses to 3 (from other anchors)
//									instance_data[instance].responseTO = (instance_data[instance].maxAnchorsTwr-1);
//
//									dw_event.type_pend = anctxorrxreenable(instance_data[instance].instanceAddress16, 2+0);
//
////									instance_data[instance].tof[sourceAddress & 0xFF] = INVALID_TOF; //clear ToF ..
//									//debug LED off
//									led_off(LED_PC9);
////									instance_data[instance].testAppState = TWR_REPORT_BCAST;
//									// update reply delay
////									instance_data[instance].delayedReplyTime = dw_event.timeStamp32h;
////									instance_data[instance].delayedReplyTime += 2*(instance_data[instance].fixedReplyDelayAnc >> 8);
////									uint32 temp = instance_data[instance].shortAdd_idx*(instance_data[instance].fixedReplyDelayAnc >> 8);
////									instance_data[instance].delayedReplyTime += temp;
//								}
////								else
//									instance_data[instance].testAppState = TA_RXE_WAIT ;              // wait for next frame
//							}
//							//					            if(instance_data[instance].antennaDelayChanged == 1)
//														//					            	instancesetantennadelays(); //this will update the antenna delay if it has changed
//							instancesettxpower(); // configure TX power if it has changed
//
//							//					            usbVCOMout[0] = '$';
//							//					            usbVCOMout[1] = messageData[STATUSIO1FINAL];
//							//					            usbVCOMout[2] = messageData[STATUSIO2FINAL];
//							//					            usbVCOMout[3] = messageData[STATIOSEQFINAL];
//							//					            usbVCOMout[4] = '\r';
//							//					            usbVCOMout[5] = '\n';
//							//					            send_usbmessage(&usbVCOMout[0], 6);
//							//					            usb_run();
//
//							//#if VERSION == VER_1p0
//							//					            memcpy(&instance_data[instance].TimestampRx1,&instance_data[instance].tempTimestampRx1,5);
//							//					            memcpy(&instance_data[instance].TimestampTx2,&instance_data[instance].tempTimestampTx2,5);
//							//					            memcpy(&instance_data[instance].TimestampRx3,&instance_data[instance].tempTimestampRx3,5);
//							//#endif
//
//						}
//
////						else if(instance_data[instance].gatewayAnchor && instance_data[instance].enableReport)
////							memcpy(&instance_data[instance].TimestampRx3,&rxTimeStamp,5);
						break;

//					case RTLS_DEMO_MSG_REPORT_BCAST:
//						//we are a tag
//						if(instance_data[instance].mode == TAG)
//						{
////							uint8 indexForFinalMsg;
//
//							//							if(sourceAddress > 0x8000)
//							//								aaa++;
//
//							instance_data[instance].rxResps[instance_data[instance].rangeNum]++;
//							dw_event.type_pend = tagrxreenable(sourceAddress); //responseTO decremented above...
////							indexForFinalMsg = RRXT0 + 5*(sourceAddress & 0xFF);
//
//							instance_data[instance].rxResponseMask |= (0x0001 << (sourceAddress & 0xFF)); //add anchor ID to the mask
//
//#if VERSION == VER_1p0
//							//							memcpy(&instance_data[instance].TimestampRx1,&dw_event.msgu.frame[RESP_TIMESTAMP1+fcode_index],5);
//							//							memcpy(&instance_data[instance].TimestampTx2,&dw_event.msgu.frame[RESP_TIMESTAMP2+fcode_index],5);
//							//							memcpy(&instance_data[instance].TimestampRx3,&dw_event.msgu.frame[RESP_TIMESTAMP3+fcode_index],5);
//							//
//							//							instance_data[instance].TimestampTx1 = instance_data[instance].tempTimestampTx1;
//							//							instance_data[instance].TimestampRx2 = instance_data[instance].tempTimestampRx2;
//							//							instance_data[instance].TimestampTx3 = instance_data[instance].tempTimestampTx3;
//
//							instance_data[instance].tempTimestampRx2 = dw_event.timeStamp;
//#endif
//							//							instance_data[instance].msg_f.messageData[VRESP1] = instance_data[instance].rxResponseMask;
//							//							instance_data[instance].msg_f.messageData[VRESP2] = instance_data[instance].rxResponseMask >> 8;
////							instance_data[instance].msg_f.messageData[instance_data[instance].finalTxTimeIdx+5] = instance_data[instance].rxResponseMask;
////							instance_data[instance].msg_f.messageData[instance_data[instance].finalTxTimeIdx+6] = instance_data[instance].rxResponseMask >> 8;
//						}
//						else //normal anchor mode
//						{
//							if((instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] >= 0) //we got the poll else ignore this response
//									&& (instance_data[instance].responseTO > 0)	) //if responseTO == 0 we have already received all of the responses - meaning should not be here => error
//							{
//								instance_data[instance].rxResps[instance_data[instance].rxRespsIdx]++; //increment the number of responses received
//								instance_data[instance].responseTO--;
//
//								//send a response or re-enable rx
//								dw_event.type_pend = anctxorrxreenable(sourceAddress, 4+0);
//							}
//							else //like a timeout (error) ...
//							{
//								led_on(LED_PC9);
//								//send a response or re-enable rx
//								dwt_setrxtimeout(0); //reconfigure the timeout
//								dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
//								dw_event.type_pend = DWT_SIG_RX_PENDING ;
//								led_off(LED_PC9);
//							}
//						}
//						break;

//					case RTLS_DEMO_MSG_IOSTATUS:
//						// get the status IO message
//						instance_data[instance].StatusIO1 = dw_event.msgu.frame[10];
//						instance_data[instance].StatusIO2 = dw_event.msgu.frame[11];
//						instance_data[instance].StatIOseqnum = dw_event.msgu.frame[12];
//
//						usbVCOMout[0] = '$';
//						usbVCOMout[1] = instance_data[instance].StatusIO1;
//						usbVCOMout[2] = instance_data[instance].StatusIO2;
//						usbVCOMout[3] = instance_data[instance].StatIOseqnum;
//						usbVCOMout[4] = '\r';
//						usbVCOMout[5] = '\n';
//						send_usbmessage(&usbVCOMout[0], 6);
//						usb_run();
//						break;

					case TOA_DEMO_MSG_SYNC_BCAST:
                                                {
						//ERWIN led_on(LED_PC9);
						uint8 idDestTagBlink = dw_event.msgu.frame[fcode_index+SYNC_BLINK_TAGID] % MAX_BUFF_SIZE;
						uint8 idPosTagReport = dw_event.msgu.frame[fcode_index+SYNC_REP_TAGID] % MAX_BUFF_SIZE;

						// save Tag ID BLink
						instance_data[instance].TagIdBlink = dw_event.msgu.frame[fcode_index+SYNC_BLINK_TAGID];
						instance_data[instance].TagIdBlinkArray[idDestTagBlink] = instance_data[instance].TagIdBlink;
						// save Tag ID Report
						instance_data[instance].TagIdReport = dw_event.msgu.frame[fcode_index+SYNC_REP_TAGID];
						instance_data[instance].TagIdReportArray[idPosTagReport] = instance_data[instance].TagIdReport;
						// save SYNC RX timestamp
						instance_data[instance].syncRxTimestamp[idDestTagBlink] = dw_event.timeStamp;
						//when receives a SYNC, we need to remember the new sequence number
						memcpy(&instance_data[instance].SeqNumBlink[idDestTagBlink],&dw_event.msgu.frame[fcode_index+SYNC_BLINK_SEQNUM],2);
						// copy sequence number of the position report
						memcpy(&instance_data[instance].SeqNumReport[idPosTagReport],&dw_event.msgu.frame[fcode_index+SYNC_REP_SEQNUM],2);
						// get the position messages
						memcpy(&instance_data[instance].ReportPosX[idPosTagReport],&dw_event.msgu.frame[fcode_index+SYNC_REP_POS_X],sizeof(float));
						memcpy(&instance_data[instance].ReportPosY[idPosTagReport],&dw_event.msgu.frame[fcode_index+SYNC_REP_POS_Y],sizeof(float));
						memcpy(&instance_data[instance].ReportPosZ[idPosTagReport],&dw_event.msgu.frame[fcode_index+SYNC_REP_POS_Z],sizeof(float));
						// get status report whether or not it is valid
						instance_data[instance].ReportValid[idPosTagReport] = dw_event.msgu.frame[fcode_index+SYNC_REP_VALID];
						// reset blink data info
						instance_data[instance].NewBlinkData[idDestTagBlink] = 0;

//						uint8 kk = 0;
//						UART_txbuff[kk++] = 0;
//						UART_txbuff[kk++] = 0xE8;
//						UART_txbuff[kk++] = '~';
//						UART_txbuff[kk++] = instance_data[instance].TagIdBlink;
//						UART_txbuff[kk++] = (instance_data[instance].SeqNumBlink[idDestTagBlink] >> 8);
//						UART_txbuff[kk++] = instance_data[instance].SeqNumBlink[idDestTagBlink];
//						UART_txbuff[kk++] = '\r';
//						UART_txbuff[kk++] = '\n';
//
//						send_uart_txdata(kk);


						// read diagnostics data
#if (READ_DIAGNOSTICS) == 1
//						dwt_rxdiag_t diagn;
						dwt_readdiagnostics(&instance_data[instance].Adiags_data[sourceAddress & 0xFF]);

//						if(instance_data[instance].mode == TAG)
//						{
//							memcpy(&(instance_data[instance].Adiags_data[sourceAddress & 0xFF]), &diagn, sizeof(diagn));
//						}
//						else // srcAddr[1] == 0 means data coming from tag
//						{
//							uint8 idx;
//							idx = (sourceAddress & 0xFF) % MAX_BUFF_SIZE_TOA;
//							memcpy(&(instance_data[instance].Tdiags_data[idx]), &diagn, sizeof(diagn));
//						}
#endif

						//send response or re-enable rx
						if(instance_data[instance].mode == TAG && \
								instance_data[instance].TagIdBlink == (instance_data[instance].instanceAddress16 & 0xFF))
						{
							// set delayed reply (BLINK)
//							instance_data[instance].delayedReplyTime = \
//									dw_event.timeStamp32h + (instance_data[instance].fixedReplyDelayAnc >> 8);
							instance_data[instance].tagCalculatedFinalTxTime = ((dw_event.timeStamp + instance_data[instance].fixedReplyDelayAnc) & MASK_TXDTS);
							instance_data[instance].delayedReplyTime = instance_data[instance].tagCalculatedFinalTxTime >> 8; // high 32 bits
							// Calculate Time Final message will be sent and write this field of Final message
							// Sending time will be delayedReplyTime, snapped to ~125MHz or ~250MHz boundary by
							// zeroing its low 9 bits, and then having the TX antenna delay added
							// getting antenna delay from the device and add it to the Calculated TX Time
							instance_data[instance].tagCalculatedFinalTxTime += instance_data[instance].txAntennaDelay;
							instance_data[instance].tagCalculatedFinalTxTime &= MASK_40BIT;
							// diffRxTimestamp computed in tag
							instance_data[instance].diffTimestampTag[idDestTagBlink] = instance_data[instance].tagCalculatedFinalTxTime - dw_event.timeStamp;
							// prepare message
							instance_data[instance].msg_f.messageData[FCODE] = TOA_DEMO_MSG_TAG_BLINK;
							//Tag blink sequence number
							memcpy(&instance_data[instance].msg_f.messageData[TAG_BLINK_SEQNUM], \
									&instance_data[instance].SeqNumBlink[idDestTagBlink], 2);
							//Tag blink diff timestamps (computed reply delay)
							memcpy(&instance_data[instance].msg_f.messageData[TAG_BLINK_DIFFTS], \
									&instance_data[instance].diffTimestampTag[idDestTagBlink], 5);
//							instance_data[instance].msg_f.messageData[BLINK_CMDSEQNUM] = instance_data[instance].commandSeqNum;
//							instance_data[instance].msg_f.messageData[BLINK_CMDOPMODE] = instance_data[instance].opMode;
							instance_data[instance].msg_f.seqNum = instance_data[instance].frameSN++; //copy sequence number and then increment
							instance_data[instance].msg_f.sourceAddr[0] = instance_data[instance].eui64[0]; //copy the address
							instance_data[instance].msg_f.sourceAddr[1] = instance_data[instance].eui64[1]; //copy the address
							instance_data[instance].msg_f.destAddr[0] = 0xff;  //set the destination address (broadcast == 0xffff)
							instance_data[instance].msg_f.destAddr[1] = 0xff;  //set the destination address (broadcast == 0xffff)
							instance_data[instance].psduLength = (TAG_BLINK_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
//							dwt_setrxaftertxdelay(instance_data[instance].ancRespRxDelay);
							// write the frame data to DW TX buffer
							dwt_writetxdata(instance_data[instance].psduLength, (uint8 *)  &instance_data[instance].msg_f, 0) ;
							dwt_writetxfctrl(instance_data[instance].psduLength, 0, 0/*ERWIN*/); //write frame control
							// set this to automatically enable receiver after transmission
							instance_data[instance].wait4ack = DWT_RESPONSE_EXPECTED; //response is expected -> automatically enable the receiver
							// send delayed TX BLINK
							dwt_setdelayedtrxtime(instance_data[instance].delayedReplyTime) ;
							if(dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED))
							{
								//if TX has failed - we need to re-enable RX for the next response or final reception...
								dwt_setrxaftertxdelay(0);
								instance_data[instance].wait4ack = 0; //clear the flag as the TX has failed the TRX is off
								instance_data[instance].lateTX++;
								instance_data[instance].delayedReplyTime += 2*(instance_data[instance].fixedReplyDelayAnc >> 8); //to take into account W4R
								ancenablerx();
								dw_event.type_pend = DWT_SIG_RX_PENDING ;
							}
							else
							{
								instance_data[instance].delayedReplyTime += (instance_data[instance].fixedReplyDelayAnc >> 8); //to take into account W4R
								dw_event.type_pend = DWT_SIG_TX_PENDING ; // exit this interrupt and notify the application/instance that TX is in progress.
								instance_data[instance].timeofTx = portGetTickCnt();
								instance_data[instance].monitor = 1;
							}

							instance_data[instance].testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
							instance_data[instance].previousState = TOA_TAG_BLINK;

							instance_data[instance].toaEvtTime[idDestTagBlink] = portGetTickCnt() & 0xffffffff;
						}
						else
						{
							dwt_setrxtimeout(0); //reconfigure the timeout
							dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
							dw_event.type_pend = DWT_SIG_RX_PENDING ;
						}
						//ERWIN led_off(LED_PC9);

//						uint8 kk = 0;
//						kk = 0;
//						UART_txbuff[kk++] = 0;
//						//				UART_txbuff[kk++] = 0xE8;
//						//				UART_txbuff[kk++] = '~';
//						//				UART_txbuff[kk++] = instance_data[instance].TagIdBlinkArray[instance_data[instance].idxBlink];
//						//				UART_txbuff[kk++] = (instance_data[instance].SeqNumBlink[instance_data[instance].idxBlink] >> 8);
//						//				UART_txbuff[kk++] = instance_data[instance].SeqNumBlink[instance_data[instance].idxBlink];
//						UART_txbuff[kk++] = '\r';
//						UART_txbuff[kk++] = '\n';
//
//						send_uart_txdata(kk);
                                                }
						break;

					case TOA_DEMO_MSG_TAG_BLINK:
						//ERWIN led_on(LED_PC9);

						instance_data[instance].TagIdBlink = (sourceAddress & 0xFF);
						uint8 idSrcTagBlink = instance_data[instance].TagIdBlink % MAX_BUFF_SIZE;

						instance_data[instance].blinkRxTimestamp[idSrcTagBlink] = dw_event.timeStamp ; //save BLINK Rx time

						//when receives BLINK, save the data
						instance_data[instance].TagIdBlinkArray[idSrcTagBlink] = instance_data[instance].TagIdBlink;
						memcpy(&instance_data[instance].SeqNumBlink[idSrcTagBlink],&dw_event.msgu.frame[fcode_index+TAG_BLINK_SEQNUM],2);
						memcpy(&instance_data[instance].diffTimestampTag[idSrcTagBlink],&dw_event.msgu.frame[fcode_index+TAG_BLINK_DIFFTS],5);

						//check from the sequence number
//						uint16 seqNumBlink = (uint16)messageData[TAG_BLINK_SEQNUM] | (uint16)(messageData[TAG_BLINK_SEQNUM+1] << 8);
						//                            	if(instance_data[instance].SeqNumBlink[(srcAddr[0] & 0xFF)] == seqNumBlink) {
//						if(instance_data[instance].blinkRxTimestamp[idSrcTagBlink] >= instance_data[instance].syncRxTimestamp[idSrcTagBlink])
							instance_data[instance].diffRxTimestamp[idSrcTagBlink] = \
							instance_data[instance].blinkRxTimestamp[idSrcTagBlink] - instance_data[instance].syncRxTimestamp[idSrcTagBlink];
//						else
//							instance_data[instance].diffRxTimestamp[idSrcTagBlink] = \
//							instance_data[instance].blinkRxTimestamp[idSrcTagBlink] \
//							+ 0x000000FFFFFFFFFF - instance_data[instance].syncRxTimestamp[idSrcTagBlink];
						//                            	}
//						instance_data[instance].diffRxTimestamp[idSrcTagBlink] -= instance_data[instance].diffTimestampTag[idSrcTagBlink];

						instance_data[instance].toaEvtTime[idSrcTagBlink] = portGetTickCnt() & 0xffffffff;
						// set blink data info
						instance_data[instance].NewBlinkData[idSrcTagBlink] = 1;
						//send a response or re-enable rx
//						if(instance_data[instance].mode == ANCHOR_SYNC)
//							instance_data[instance].stopTimer = 1;
//						dwt_setrxtimeout(0); //reconfigure the timeout
						dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
						dw_event.type_pend = DWT_SIG_RX_PENDING ;

						// read diagnostics data
#if (READ_DIAGNOSTICS) == 1
						if(instance_data[instance].mode != TAG)
							dwt_readdiagnostics(&instance_data[instance].Tdiags_data[idSrcTagBlink]);

//						if(instance_data[instance].mode != TAG)
//							memcpy(&(instance_data[instance].Tdiags_data[idSrcTagBlink]), &diagn, sizeof(diagn));
#endif
						//ERWIN led_off(LED_PC9);
						break;

					//if anchor fall into case below and process the frame
					default:  //process rx frame
					{
						dw_event.type_pend = DWT_SIG_DW_IDLE;
					}
					break;

				}
			}//end of if not Listener mode
	    	instance_data[instance].stopTimer = 1;

            instance_putevent(dw_event, rxd_event);

			instance_data[instance].rxMsgCount++;
		}
//		else if(rxd_event == DWT_SIG_RX_SYNCLOSS)
//		{
//			dwt_softreset();
//			return;
//		}
		else //if (rxd_event == SIG_RX_UNKNOWN) //need to re-enable the rx (got unknown frame type)
		{
			dwt_softreset();
			handle_error_unknownframe(dw_event);
			return;
		}
}
#endif
//ERWIN
#if 1 //for testing with init-resp example
void instance_cbRxTo(const dwt_cb_data_t *rxd)
{
  rx_timeout_counter++;
}
#else
#pragma GCC optimize ("O3")
void instance_cbRxTo(const dwt_cb_data_t *rxd)
{
	int instance = 0;
	uint8 rxTimeStamp[5]  = {0, 0, 0, 0, 0};

        uint8 rxd_event = 0;
	uint8 fcode_index  = 0;
	uint8 srcAddr_index = 0;
	uint8 dstAddr_index = 0;
	event_data_t dw_event;

        rx_timeout_counter++;

//	uint8 aaa = 0;

	//microcontroller time at which we received the frame
        dw_event.uTimeStamp = portGetTickCnt(); //ERWIN
        instance_data[instance].timeForceWakeUp = dw_event.uTimeStamp;

        //if tag and got TO, then did not get any or some responses - check if need to send final.

          dw_event.type_pend = DWT_SIG_DW_IDLE;

          if(instance_data[instance].mode == ANCHOR)
          {
                  //check if anchor has received all of the responses from other anchors (it could have received only 1 or 2)
                  //it's timed out (re-enable rx or tx response)
                  if(instance_data[instance].responseTO > 0)
                  {
                          instance_data[instance].responseTO--;
                          //send a response or re-enable rx
                          dw_event.type_pend = anctxorrxreenable(instance_data[instance].instanceAddress16, 6+0);
                  }
          }
          dw_event.type = 0;
          dw_event.type_save = DWT_SIG_RX_TIMEOUT;
          dw_event.rxLength = 0;
          dw_event.timeStamp = 0;
          dw_event.timeStamp32l = 0;
          dw_event.timeStamp32h = 0;

          instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);
          //printf("RX timeout while in %d\n", instance_data[instance].testAppState);

}
#endif
//ERWIN
#if 1 //for testing with init-resp example
void instance_cbRxErr(const dwt_cb_data_t *rxd)
{
  rx_error_counter++;
}
#else
#pragma GCC optimize ("O3")
void instance_cbRxErr(const dwt_cb_data_t *rxd)
{
	int instance = 0;
	uint8 rxTimeStamp[5]  = {0, 0, 0, 0, 0};

        uint8 rxd_event = 0;
	uint8 fcode_index  = 0;
	uint8 srcAddr_index = 0;
	uint8 dstAddr_index = 0;
	event_data_t dw_event;

        rx_error_counter++;

//	uint8 aaa = 0;

	//microcontroller time at which we received the frame
        dw_event.uTimeStamp = portGetTickCnt(); //ERWIN
        instance_data[instance].timeForceWakeUp = dw_event.uTimeStamp;

        //led_on(LED_PC9);
        handle_error_unknownframe(dw_event);
        //ERWIN return;
        //led_off(LED_PC9);
}
#endif
#if 0 //ERWIN
#pragma GCC optimize ("O3")
void instance_rxcallback(const dwt_cb_data_t *rxd)
{
	int instance = 0;
	uint8 rxTimeStamp[5]  = {0, 0, 0, 0, 0};

    uint8 rxd_event = 0;
	uint8 fcode_index  = 0;
	uint8 srcAddr_index = 0;
	uint8 dstAddr_index = 0;
	event_data_t dw_event;

//	uint8 aaa = 0;

	//microcontroller time at which we received the frame
    dw_event.uTimeStamp = portGetTickCnt(); //ERWIN
    instance_data[instance].timeForceWakeUp = dw_event.uTimeStamp;

    //if we got a frame with a good CRC - RX OK
    if(rxd->event == DWT_SIG_RX_OKAY)
	{
 		dw_event.rxLength = rxd->datalength;

		//need to process the frame control bytes to figure out what type of frame we have received
		if(((rxd->fctrl[0] == 0x41) || (rxd->fctrl[0] == 0x61))
				&&
				((rxd->fctrl[1] & 0xCC) == 0x88)) //short address
		{

			fcode_index = FRAME_CRTL_AND_ADDRESS_S; //function code is in first byte after source address
			srcAddr_index = FRAME_CTRLP + ADDR_BYTE_SIZE_S;
			dstAddr_index = srcAddr_index - ADDR_BYTE_SIZE_S;
			rxd_event = DWT_SIG_RX_OKAY;
		}
		else
		{
			rxd_event = SIG_RX_UNKNOWN; //not supported - all TREK1000 frames are short addressed
		}

        //read RX timestamp
        dwt_readrxtimestamp(rxTimeStamp) ;
        dwt_readrxdata((uint8 *)&dw_event.msgu.frame[0], rxd->datalength, 0);  // Read Data Frame
    	dw_event.timeStamp32l =  (uint32)rxTimeStamp[0] + ((uint32)rxTimeStamp[1] << 8) + ((uint32)rxTimeStamp[2] << 16) + ((uint32)rxTimeStamp[3] << 24);
    	dw_event.timeStamp = rxTimeStamp[4];
    	dw_event.timeStamp <<= 32;
    	dw_event.timeStamp += dw_event.timeStamp32l;
    	dw_event.timeStamp32h = ((uint32)rxTimeStamp[4] << 24) + (dw_event.timeStamp32l >> 8);

        dw_event.type = 0; //type will be added as part of adding to event queue
		dw_event.type_save = rxd_event;
		dw_event.type_pend = DWT_SIG_DW_IDLE;

		uint16 sourceAddress = (((uint16)dw_event.msgu.frame[srcAddr_index+1]) << 8) + dw_event.msgu.frame[srcAddr_index];
		uint16 destinAddress = (((uint16)dw_event.msgu.frame[dstAddr_index+1]) << 8) + dw_event.msgu.frame[dstAddr_index];

		//if Listener then just report the received frame to the instance (application)
		if(rxd_event == DWT_SIG_RX_OKAY) //Process good/known frame types
		{
#if (READ_DIAGNOSTICS) == 1
			dwt_rxdiag_t diagn;
			dwt_readdignostics(&diagn);

			if((sourceAddress & 0xFF00) == 0x8000)
				memcpy(&(instance_data[instance].Adiags_data[(uint8)sourceAddress & 0xFF]), &diagn, sizeof(diagn));
			else
				memcpy(&(instance_data[instance].Tdiags_data[(uint8)sourceAddress & 0xFF]), &diagn, sizeof(diagn));
#endif
			if(instance_data[instance].mode != LISTENER)
			{
				if(instance_data[instance].mode == TAG) //if tag got a good frame - this is probably a response, but could also be some other non-ranging frame
					//(although due to frame filtering this is limited as non-addressed frames are filtered out)
				{
					instance_data[instance].responseTO--; //got 1 more response or other RX frame - need to reduce timeout (for next response)
				}

				//check if this is a TWR message (and also which one)
				switch(dw_event.msgu.frame[fcode_index])
				{
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
					//poll message from an anchor
					case RTLS_DEMO_MSG_ANCH_POLL:
					{
						//the anchor to anchor ranging poll frames are ignored by A0 and A3
						if(instance_data[instance].gatewayAnchor || (instance_data[instance].instanceAddress16 > A2_ANCHOR_ADDR))
						{
							//ignore poll from anchor 1 if gateway or anchor 3
							//anchors 2 and 3 will never send polls
							dw_event.type_pend = DWT_SIG_DW_IDLE ;
							break;
						}

						if(instance_data[instance].mode == TAG)  //tag should ignore any other Polls from anchors
						{
							instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
							handle_error_unknownframe(dw_event);
							instance_data[instance].stopTimer = 1;
							instance_data[instance].rxMsgCount++;
							return;
						}

						//update the response index and number of responses received tables
						instance_data[instance].rxRespsIdx = (uint8) ((dw_event.msgu.frame[POLL_RNUM+fcode_index] & 0xFF)
																+ (((sourceAddress & 0xFF) + 8) << 4));
						instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = 0;
						//debug LED on
						led_on(LED_PC9);

						//prepare the response and write it to the tx buffer
						ancprepareresponse2(sourceAddress, srcAddr_index, fcode_index, &dw_event.msgu.frame[0]);

						//A2 does not need timeout if ranging to A1
						if(sourceAddress != A1_ANCHOR_ADDR)
						{
							dwt_setrxtimeout((uint16)instance_data[instance].fwtoTimeAnc_sy); //reconfigure the timeout for response
						}

						//set the bast reply time (the actual will be Poll rx time + instance_data[0].fixedReplyDelayAnc)
						instance_data[instance].delayedReplyTime = dw_event.timeStamp32h ;
						instance_data[instance].responseTO = (instance_data[instance].instanceAddress16 - sourceAddress) & 0xFF; //set number of expected responses

						dw_event.type_pend = anctxorrxreenable(instance_data[instance].instanceAddress16, 2+1);

						instance_data[instance].tofAnc[sourceAddress & 0xFF] = INVALID_TOF; //clear ToF ..
						//debug LED off
						led_off(LED_PC9);
						break;
					}
#endif

					case RTLS_DEMO_MSG_TAG_POLL:
					{
						if(instance_data[instance].mode == TAG) //tag should ignore any other Polls from tags
						{
							instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
							handle_error_unknownframe(dw_event);
//							instance_data[instance].stopTimer = 1;
							instance_data[instance].rxMsgCount++;

//							dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
							return;
						}
						else
						{
							instance_data[instance].instanceWakeTime = portGetTickCnt(); //ERWIN
							instance_data[instance].timeForceWakeUp = instance_data[instance].instanceWakeTime;
							instance_data[instance].gatewayAnchorAddress = dw_event.msgu.frame[WHOISGATEWAY+fcode_index] | 0x8000;
//							instance_data[instance].getReportReqs = dw_event.msgu.frame[WHOISGATEWAY+fcode_index+1];

							if(instance_data[instance].instanceAddress16 == instance_data[instance].gatewayAnchorAddress)
							{
								instance_data[instance].gatewayAnchor = TRUE;

							}
							else
								instance_data[instance].gatewayAnchor = FALSE;
//#if VERSION == VER_1p0
//							memcpy(&instance_data[instance].TimestampTx1,&dw_event.msgu.frame[POLL_TIMESTAMP1+fcode_index],5);
//							memcpy(&instance_data[instance].TimestampRx2,&dw_event.msgu.frame[POLL_TIMESTAMP2+fcode_index],5);
//							memcpy(&instance_data[instance].TimestampTx3,&dw_event.msgu.frame[POLL_TIMESTAMP3+fcode_index],5);
//#endif
//							memcpy(&instance_data[instance].tempTimestampRx1,&rxTimeStamp,5);
						}

						instance_data[instance].rxRespsIdx = (int8) ((dw_event.msgu.frame[POLL_RNUM+fcode_index] & 0xf)
																+ ((sourceAddress & 0xFF) << 4));
						instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = 0;
						//debug LED on
						led_on(LED_PC9);
						//prepare the response and write it to the tx buffer
						ancprepareresponse(sourceAddress, srcAddr_index, fcode_index, &dw_event.msgu.frame[0], dw_event.uTimeStamp);

						dwt_setrxtimeout((uint16)instance_data[instance].fwtoTimeAnc_sy); //reconfigure the timeout for response

						instance_data[instance].delayedReplyTime = dw_event.timeStamp32h /*+ (instance_data[0].fixedReplyDelayAnc >> 8)*/ ;
//						instance_data[instance].responseTO = NUM_EXPECTED_RESPONSES; //set number of expected responses to 3 (from other anchors)
//						instance_data[instance].responseTO = (instance_data[instance].maxAnchorsTwr-1);
						instance_data[instance].responseTO = (instance_data[instance].maxAnchorsTwr);

						dw_event.type_pend = anctxorrxreenable(instance_data[instance].instanceAddress16, 2+0);

						instance_data[instance].tof[sourceAddress & 0xFF] = INVALID_TOF; //clear ToF ..
						//debug LED off
						led_off(LED_PC9);
					}
					break;

					//we got a response from a "responder" (anchor)
					case RTLS_DEMO_MSG_ANCH_RESP:
					case RTLS_DEMO_MSG_ANCH_RESP2:
					{
						//we are a tag
					    if(instance_data[instance].mode == TAG)
					    {
					    	instance_data[instance].timeForceWakeUp = portGetTickCnt(); //ERWIN
					    	if(destinAddress == instance_data[instance].instanceAddress16)
					    	{
					    		uint8 indexForFinalMsg;

					    		//							if(sourceAddress > 0x8000)
					    		//								aaa++;

					    		instance_data[instance].rxResps[instance_data[instance].rangeNum]++;
					    		dw_event.type_pend = tagrxreenable(sourceAddress); //responseTO decremented above...
					    		indexForFinalMsg = RRXT0 + 5*(sourceAddress & 0xFF);

					    		instance_data[instance].rxResponseMask |= (0x0001 << (sourceAddress & 0xFF)); //add anchor ID to the mask
					    		//							instance_data[instance].getReportResp |= dw_event.msgu.frame[RES_TAG_SLP1+fcode_index+1];
					    		// Write Response RX time field of Final message
					    		memcpy(&(instance_data[instance].msg_f.messageData[indexForFinalMsg]), rxTimeStamp, 5);
#if VERSION == VER_1p0
					    		//							memcpy(&instance_data[instance].TimestampRx1,&dw_event.msgu.frame[RESP_TIMESTAMP1+fcode_index],5);
					    		//							memcpy(&instance_data[instance].TimestampTx2,&dw_event.msgu.frame[RESP_TIMESTAMP2+fcode_index],5);
					    		//							memcpy(&instance_data[instance].TimestampRx3,&dw_event.msgu.frame[RESP_TIMESTAMP3+fcode_index],5);
					    		//
					    		//							instance_data[instance].TimestampTx1 = instance_data[instance].tempTimestampTx1;
					    		//							instance_data[instance].TimestampRx2 = instance_data[instance].tempTimestampRx2;
					    		//							instance_data[instance].TimestampTx3 = instance_data[instance].tempTimestampTx3;

					    		instance_data[instance].tempTimestampRx2 = dw_event.timeStamp;
#endif
					    		//							instance_data[instance].msg_f.messageData[VRESP1] = instance_data[instance].rxResponseMask;
					    		//							instance_data[instance].msg_f.messageData[VRESP2] = instance_data[instance].rxResponseMask >> 8;
					    		instance_data[instance].msg_f.messageData[instance_data[instance].finalTxTimeIdx+5] = instance_data[instance].rxResponseMask;
					    		instance_data[instance].msg_f.messageData[instance_data[instance].finalTxTimeIdx+6] = instance_data[instance].rxResponseMask >> 8;
					    	}
					    	else
					    		//					    	if(dw_event.msgu.frame[TOFRPT_ID+fcode_index] == (uint8)(instance_data[instance].instanceAddress16 & 0x00ff))
					    		if(dw_event.msgu.frame[TOFRPT_ID+fcode_index] == (uint8)(instance_data[instance].instanceAddress16 & 0x00ff) && \
					    				(dw_event.msgu.frame[TOFRPT_N+fcode_index] == instance_data[instance].rangeNum-1))
					    		{
					    			//					    		dwt_forcetrxoff();	//this will clear all events

					    			instance_data[instance].rxRespsImRpt[instance_data[instance].rangeNum]++;

					    			//copy the ToF and put into array (array holds last 4 ToFs)
					    			memcpy(&instance_data[instance].tofArrayImRpt[(sourceAddress & 0xFF)], &(dw_event.msgu.frame[TOFRPT+fcode_index]), 4);

					    			//check if the ToF is valid, this makes sure we only report valid ToFs
					    			//e.g. consider the case of reception of response from anchor a1 (we are anchor a2)
					    			//if a1 got a Poll with previous Range number but got no Final, then the response will have
					    			//the correct range number but the range will be INVALID_TOF
					    			if(instance_data[instance].tofArrayImRpt[(sourceAddress & 0xFF)] != INVALID_TOF)
					    				instance_data[instance].rxResponseMaskImRpt |= (0x0001 << (sourceAddress & 0xFF));

					    			//					    		led_on(LED_PC9);
					    			//					    		//send a response or re-enable rx
					    			dwt_setrxtimeout(0); //reconfigure the timeout
//					    			dwt_setpreambledetecttimeout(0);
					    			dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
					    			//					    		dw_event.type_pend = DWT_SIG_RX_PENDING ;
					    			//					    		instance_data[instance].testAppState = TA_RXE_WAIT;
					    			//					    		led_off(LED_PC9);

					    			//					    		instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
					    			//					    		handle_error_unknownframe(dw_event);
					    			//					    		instance_data[instance].stopTimer = 1;
					    			//					    		instance_data[instance].rxMsgCount++;
					    			return;

					    			//					    		dw_event.type_pend = tagrxreenable(sourceAddress); //responseTO decremented above...
					    		}
					    		else
					    		{
					    			instance_data[instance].tofArrayImRpt[(sourceAddress & 0xFF)] = INVALID_TOF;

					    			instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
					    			handle_error_unknownframe(dw_event);
//					    			instance_data[instance].stopTimer = 1;
					    			instance_data[instance].rxMsgCount++;

					    			//								dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
					    			return;
					    		}
					    }
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
						else if (instance_data[instance].mode == ANCHOR_RNG) //A0 and A1 only when ranging to other anchors
						{
							uint8 index ;
							instance_data[instance].rxResps[instance_data[instance].rangeNumAnc]++;
							dw_event.type_pend = ancsendfinalorrxreenable(sourceAddress);
							index = RRXT0 + 5*(sourceAddress & 0xFF);

							instance_data[instance].rxResponseMaskAnc |= (0x0001 << (sourceAddress & 0xFF)); //add anchor ID to the mask
							// Write Response RX time field of Final message
							memcpy(&(instance_data[instance].msg_f.messageData[index]), rxTimeStamp, 5);
						}
#endif
						else //normal anchor mode
						{
							instance_data[instance].instanceWakeTime = portGetTickCnt(); //ERWIN
							instance_data[instance].timeForceWakeUp = instance_data[instance].instanceWakeTime;

							//got a response... (check if we got a Poll with the same range number as in this response)
							if(RTLS_DEMO_MSG_ANCH_RESP == dw_event.msgu.frame[fcode_index])
							{
								if((instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] >= 0) //we got the poll else ignore this response
									&& (instance_data[instance].responseTO > 0)	) //if responseTO == 0 we have already received all of the responses - meaning should not be here => error
								{
									instance_data[instance].rxResps[instance_data[instance].rxRespsIdx]++; //increment the number of responses received
									instance_data[instance].responseTO--;

									//send a response or re-enable rx
									dw_event.type_pend = anctxorrxreenable(sourceAddress, 4+0);
								}
								else //like a timeout (error) ...
								{
									led_on(LED_PC9);
									//send a response or re-enable rx
									dwt_setrxtimeout(0); //reconfigure the timeout
									dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
									dw_event.type_pend = DWT_SIG_RX_PENDING ;
									led_off(LED_PC9);
								}
							}
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
							else //in anchor mode and got RTLS_DEMO_MSG_ANCH_RESP2
							{
								if((instance_data[instance].gatewayAnchor) &&
									(instance_data[instance].rxResps[instance_data[instance].rangeNumAnc]) == 2)
								{//got two responses A1 and A2 this is third (A2's to A1)
									instance_data[instance].rxResps[instance_data[instance].rangeNumAnc]++;
									instance_data[instance].rxResponseMaskAnc |= 0x8 ;

									dw_event.type_pend = anctxorrxreenable(sourceAddress, 4+1); //re-enable the RX
								}
								//A2 got A1's response to A0 - send A2 response (but only if we got the Poll from A0)
								else if((instance_data[instance].instanceAddress16 == A2_ANCHOR_ADDR) &&
										(instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] >= 0) )
								{
									instance_data[instance].rxResps[instance_data[instance].rxRespsIdx]++;
									instance_data[instance].responseTO--;

									dwt_setrxtimeout(0);
									dwt_setrxaftertxdelay(0); //clear rx on delay as Final will come sooner than if we were waiting for next Response
									dw_event.type_pend = anctxorrxreenable(sourceAddress, 1);
								}
								else // if other anchor A1, A2, A3 .. ignore these responses when in ANCHOR mode
								{
									dwt_setrxtimeout(0); //reconfigure the timeout
									dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
									dw_event.type_pend = DWT_SIG_RX_PENDING ;
								}
							}
#endif
						}

					}
					break;

					case RTLS_DEMO_MSG_TAG_FINAL:
					case RTLS_DEMO_MSG_ANCH_FINAL:
						if(instance_data[instance].mode == TAG) //tag should ignore any other Final from anchors
						{
							instance_data[instance].responseTO++; //as will be decremented in the function and was also decremented above
							handle_error_unknownframe(dw_event);
//							instance_data[instance].stopTimer = 1;
							instance_data[instance].rxMsgCount++;

//							dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
							return;
						}
						else
						{
							instance_data[instance].instanceWakeTime = portGetTickCnt(); //ERWIN
							instance_data[instance].timeForceWakeUp = instance_data[instance].instanceWakeTime;
						}
//						else
//						{
//							int64 Rb, Da, Ra, Db ;
//							uint64 tagFinalTxTime  = 0;
//							uint64 tagFinalRxTime  = 0;
//							uint64 tagPollTxTime  = 0;
//							uint64 anchorRespRxTime  = 0;
//							uint64 tof = INVALID_TOF;
//
//							double RaRbxDaDb = 0;
//							double RbyDb = 0;
//							double RayDa = 0;
//
//							//								instance_data[instance].TimestampTx1 = 0;
//							//								instance_data[instance].TimestampRx2 = 0;
//							//								instance_data[instance].TimestampTx3 = 0;
//							//
//							//								instance_data[instance].TimestampRx1 = 0;
//							//								instance_data[instance].TimestampTx2 = 0;
//							//								instance_data[instance].TimestampRx3 = 0;
//
//							//								uint8 validResp = messageData[VRESP];
//							//								uint16 validResp = messageData[VRESP1] | (uint16)(messageData[VRESP2] << 8);
//							uint16 validResp = dw_event.msgu.frame[fcode_index+instance_data[instance].finalTxTimeIdx+5] | \
//									(uint16)(dw_event.msgu.frame[fcode_index+instance_data[instance].finalTxTimeIdx+6] << 8);
//							uint8 indexFromFinalMsg = RRXT0 + 5*(instance_data[instance].shortAdd_idx);
//
//							//								// get command
//							//								if(instance_data[instance].commandSeqNum < messageData[FINAL_CMDSEQNUM] || \
//							//										(instance_data[instance].commandSeqNum > 250 && messageData[FINAL_CMDSEQNUM] < 5))
//							//								{
//							//									instance_data[instance].commandSeqNum = messageData[FINAL_CMDSEQNUM];
//							//									instance_data[instance].opMode = messageData[FINAL_CMDOPMODE];
//							//									UpdateSettings = 1;
//							//								}
//
//							// get the status IO message
//							//								instance_data[instance].StatusIO1 = messageData[STATUSIO1FINAL];
//							//								instance_data[instance].StatusIO2 = messageData[STATUSIO2FINAL];
//							//								instance_data[instance].StatIOseqnum = messageData[STATIOSEQFINAL];
//
//							//								if(ANCHOR_SYNC == instance_data[instance].mode)
//							//								{
//							//									instance_data[instance].testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
//							//									instance_data[instance].nextState = TWR_SYNC_BCAST ;
//							//									instance_data[instance].instToSleep = TRUE;
//							//									//										instance_data[instance].instToSleep = FALSE;
//							//								}
//							//								else
//							if((RTLS_DEMO_MSG_TAG_FINAL == dw_event.msgu.frame[fcode_index]) &&
//									(instance_data[instance].rangeNumA[sourceAddress & 0xFF] != dw_event.msgu.frame[POLL_RNUM+fcode_index])) //Final's range number needs to match Poll's or else discard this message
//							{
//								instance_data[instance].testAppState = TA_RXE_WAIT ;              // wait for next frame
//								break;
//							}
//
//							/*if((RTLS_DEMO_MSG_ANCH_FINAL == fcode) &&
//																(((instance_data[instance].rangeNumAAnc[tof_idx] != messageData[POLL_RNUM]) //Final's range number needs to match Poll's or else discard this message
//																|| instance_data[instance].gatewayAnchor) //gateway can ignore the Final (from A1 to A2 exchange)
//																|| (A3_ANCHOR_ADDR == instance_data[instance].instanceAddress16))) //A3 does not care about Final from A1 or A0
//														{
//						                                    instance_data[instance].testAppState = TA_RXE_WAIT ;              // wait for next frame
//						                                    break;
//														}*/
//
//							/*if (A1_ANCHOR_ADDR == instance_data[instance].instanceAddress16) //this is A1
//						                                {
//						                                	if(GATEWAY_ANCHOR_ADDR == (srcAddr[0] | ((uint32)(srcAddr[1] << 8)))) //final is from A0
//						                                	{
//						                                		//ENABLE TIMER ONLY IF FINAL RECEIVED
//																instance_data[instance].instanceTimerEn = 1;
//						                                	}
//						                                }*/
//							//output data over USB...
//							instance_data[instance].newRangeAncAddress = instance_data[instance].instanceAddress16;
//
//							//if we got the final, maybe the tag did not get our response, so
//							//we can use other anchors responses/ToF if there are any.. and output..
//							//but we cannot calculate new range
//							if(((validResp & (0x0001<<(instance_data[instance].shortAdd_idx))) != 0))
//							{
//								// time of arrival of Final message
//								tagFinalRxTime = dw_event.timeStamp ; //Final's Rx time
//
//								//printf("FinalRx Timestamp: %4.15e\n", convertdevicetimetosecu(dw_event.timeStamp));
//								instance_data[instance].delayedReplyTime = 0 ;
//
//								// times measured at Tag extracted from the message buffer
//								// extract 40bit times
//								memcpy(&tagPollTxTime, &(dw_event.msgu.frame[PTXT+fcode_index]), 5);
//								memcpy(&anchorRespRxTime, &(dw_event.msgu.frame[indexFromFinalMsg+fcode_index]), 5);
//								//									memcpy(&tagFinalTxTime, &(dw_event.msgu.frame[FTXT+fcode_index]), 5);
//								memcpy(&tagFinalTxTime, &(dw_event.msgu.frame[instance_data[instance].finalTxTimeIdx+fcode_index]), 5);
//#if VERSION == VER_1p0
//								for(uint8 ii=0;ii<9;ii++)
//								{
//									instance_data[instance].TimestampTx1[9-ii] = instance_data[instance].TimestampTx1[8-ii];
//									instance_data[instance].TimestampTx2[9-ii] = instance_data[instance].TimestampTx2[8-ii];
//									instance_data[instance].TimestampTx3[9-ii] = instance_data[instance].TimestampTx3[8-ii];
//									instance_data[instance].TimestampRx1[9-ii] = instance_data[instance].TimestampRx1[8-ii];
//									instance_data[instance].TimestampRx2[9-ii] = instance_data[instance].TimestampRx2[8-ii];
//									instance_data[instance].TimestampRx3[9-ii] = instance_data[instance].TimestampRx3[8-ii];
//								}
//
//								instance_data[instance].TimestampTx1[0] = tagPollTxTime;
//								instance_data[instance].TimestampRx2[0] = anchorRespRxTime;
//								instance_data[instance].TimestampTx3[0] = tagFinalTxTime;
//
//								instance_data[instance].TimestampRx1[0] = instance_data[instance].tagPollRxTime & MASK_40BIT;
//								instance_data[instance].TimestampTx2[0] = instance_data[instance].txu.anchorRespTxTime & MASK_40BIT;
//								instance_data[instance].TimestampRx3[0] = tagFinalRxTime & MASK_40BIT;
//#endif
////									memcpy(&instance_data[instance].TimestampRx1[0],(uint8 *)&(instance_data[instance].tagPollRxTime),5);
//								//									memcpy(&instance_data[instance].TimestampTx2[0],(uint8 *)&(instance_data[instance].txu.anchorRespTxTime),5);
//								//									memcpy(&instance_data[instance].TimestampRx3[0],(uint8 *)&(tagFinalRxTime),5);
//
//								// poll response round trip delay time is calculated as
//								// (anchorRespRxTime - tagPollTxTime) - (anchorRespTxTime - tagPollRxTime)
//								Ra = (int64)((anchorRespRxTime - tagPollTxTime) & MASK_40BIT);
//								Db = (int64)((instance_data[instance].txu.anchorRespTxTime - instance_data[instance].tagPollRxTime) & MASK_40BIT);
//
//								// response final round trip delay time is calculated as
//								// (tagFinalRxTime - anchorRespTxTime) - (tagFinalTxTime - anchorRespRxTime)
//								Rb = (int64)((tagFinalRxTime - instance_data[instance].txu.anchorRespTxTime) & MASK_40BIT);
//								Da = (int64)((tagFinalTxTime - anchorRespRxTime) & MASK_40BIT);
//
//								RaRbxDaDb = (((double)Ra))*(((double)Rb))
//																	- (((double)Da))*(((double)Db));
//
//								RbyDb = ((double)Rb + (double)Db);
//
//								RayDa = ((double)Ra + (double)Da);
//
//								tof = (int32) ( RaRbxDaDb/(RbyDb + RayDa) );
//							}
//
//							//tag to anchor ranging
//							if(RTLS_DEMO_MSG_TAG_FINAL == dw_event.msgu.frame[fcode_index])
//							{
//								instance_data[instance].newRangeTagAddress = sourceAddress;
//								//time-of-flight
//								instance_data[instance].tof[instance_data[instance].newRangeTagAddress & 0xFF] = tof;
//								//calculate all tag - anchor ranges... and report
//								//									instance_data[instance].newRange = instance_calcranges(&instance_data[instance].tofArray[0], MAX_ANCHOR_LIST_SIZE, TOF_REPORT_T2A, &instance_data[instance].rxResponseMask);
//								instance_data[instance].newRange = instance_calcranges(&instance_data[instance].tofArray[0], instance_data[instance].maxAnchorsTwr, TOF_REPORT_T2A, &instance_data[instance].rxResponseMask);
//								instance_data[instance].rxResponseMaskReport = instance_data[instance].rxResponseMask; //copy the valid mask to report
//								instance_data[instance].rxResponseMask = 0;
//								//we have our range - update the own mask entry...
//								if(tof != INVALID_TOF) //check the last ToF entry is valid and copy into the current array
//								{
//									setTagDist(sourceAddress & 0xFF, instance_data[instance].shortAdd_idx); //copy distance from this anchor to the tag into array
//
//									instance_data[instance].rxResponseMask = (0x0001 << instance_data[instance].shortAdd_idx);
//									instance_data[instance].tofArray[instance_data[instance].shortAdd_idx] = tof;
//								}
//								instance_data[instance].newRangeTime = dw_event.uTimeStamp ;
//							}
////#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
////							else //anchor to anchor ranging
////							{
////								instance_data[instance].newRangeTagAddress = sourceAddress;
////								//time-of-flight
////								instance_data[instance].tofAnc[tof_idx] = tof;
////							}
////#endif
//							//reset the response count
//							if(instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] >= 0)
//							{
//								instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = -1 * instance_data[instance].rxResps[instance_data[instance].rxRespsIdx];
//								if(instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] == 0) //as A0 will have this as 0 when ranging to A1
//									instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = -1 ;
//							}
//
//							if (instance_data[instance].mode == ANCHOR)
//							{
//								if (instance_data[instance].enableReport)
//								{
//									instance_data[instance].rxRespsIdx = (int8) ((dw_event.msgu.frame[POLL_RNUM+fcode_index] & 0xf)
//											+ ((sourceAddress & 0xFF) << 4));
//									instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] = 0;
//									//debug LED on
//									led_on(LED_PC9);
//									//prepare the response and write it to the tx buffer
//									ancprepareresponse(sourceAddress, srcAddr_index, fcode_index, &dw_event.msgu.frame[0], dw_event.uTimeStamp);
//
//									dwt_setrxtimeout((uint16)instance_data[instance].fwtoTimeAnc_sy); //reconfigure the timeout for response
//
//									instance_data[instance].delayedReplyTime = dw_event.timeStamp32h /*+ (instance_data[0].fixedReplyDelayAnc >> 8)*/ ;
//									//						instance_data[instance].responseTO = NUM_EXPECTED_RESPONSES; //set number of expected responses to 3 (from other anchors)
//									instance_data[instance].responseTO = (instance_data[instance].maxAnchorsTwr-1);
//
//									dw_event.type_pend = anctxorrxreenable(instance_data[instance].instanceAddress16, 2+0);
//
////									instance_data[instance].tof[sourceAddress & 0xFF] = INVALID_TOF; //clear ToF ..
//									//debug LED off
//									led_off(LED_PC9);
////									instance_data[instance].testAppState = TWR_REPORT_BCAST;
//									// update reply delay
////									instance_data[instance].delayedReplyTime = dw_event.timeStamp32h;
////									instance_data[instance].delayedReplyTime += 2*(instance_data[instance].fixedReplyDelayAnc >> 8);
////									uint32 temp = instance_data[instance].shortAdd_idx*(instance_data[instance].fixedReplyDelayAnc >> 8);
////									instance_data[instance].delayedReplyTime += temp;
//								}
////								else
//									instance_data[instance].testAppState = TA_RXE_WAIT ;              // wait for next frame
//							}
//							//					            if(instance_data[instance].antennaDelayChanged == 1)
//														//					            	instancesetantennadelays(); //this will update the antenna delay if it has changed
//							instancesettxpower(); // configure TX power if it has changed
//
//							//					            usbVCOMout[0] = '$';
//							//					            usbVCOMout[1] = messageData[STATUSIO1FINAL];
//							//					            usbVCOMout[2] = messageData[STATUSIO2FINAL];
//							//					            usbVCOMout[3] = messageData[STATIOSEQFINAL];
//							//					            usbVCOMout[4] = '\r';
//							//					            usbVCOMout[5] = '\n';
//							//					            send_usbmessage(&usbVCOMout[0], 6);
//							//					            usb_run();
//
//							//#if VERSION == VER_1p0
//							//					            memcpy(&instance_data[instance].TimestampRx1,&instance_data[instance].tempTimestampRx1,5);
//							//					            memcpy(&instance_data[instance].TimestampTx2,&instance_data[instance].tempTimestampTx2,5);
//							//					            memcpy(&instance_data[instance].TimestampRx3,&instance_data[instance].tempTimestampRx3,5);
//							//#endif
//
//						}
//
////						else if(instance_data[instance].gatewayAnchor && instance_data[instance].enableReport)
////							memcpy(&instance_data[instance].TimestampRx3,&rxTimeStamp,5);
						break;

//					case RTLS_DEMO_MSG_REPORT_BCAST:
//						//we are a tag
//						if(instance_data[instance].mode == TAG)
//						{
////							uint8 indexForFinalMsg;
//
//							//							if(sourceAddress > 0x8000)
//							//								aaa++;
//
//							instance_data[instance].rxResps[instance_data[instance].rangeNum]++;
//							dw_event.type_pend = tagrxreenable(sourceAddress); //responseTO decremented above...
////							indexForFinalMsg = RRXT0 + 5*(sourceAddress & 0xFF);
//
//							instance_data[instance].rxResponseMask |= (0x0001 << (sourceAddress & 0xFF)); //add anchor ID to the mask
//
//#if VERSION == VER_1p0
//							//							memcpy(&instance_data[instance].TimestampRx1,&dw_event.msgu.frame[RESP_TIMESTAMP1+fcode_index],5);
//							//							memcpy(&instance_data[instance].TimestampTx2,&dw_event.msgu.frame[RESP_TIMESTAMP2+fcode_index],5);
//							//							memcpy(&instance_data[instance].TimestampRx3,&dw_event.msgu.frame[RESP_TIMESTAMP3+fcode_index],5);
//							//
//							//							instance_data[instance].TimestampTx1 = instance_data[instance].tempTimestampTx1;
//							//							instance_data[instance].TimestampRx2 = instance_data[instance].tempTimestampRx2;
//							//							instance_data[instance].TimestampTx3 = instance_data[instance].tempTimestampTx3;
//
//							instance_data[instance].tempTimestampRx2 = dw_event.timeStamp;
//#endif
//							//							instance_data[instance].msg_f.messageData[VRESP1] = instance_data[instance].rxResponseMask;
//							//							instance_data[instance].msg_f.messageData[VRESP2] = instance_data[instance].rxResponseMask >> 8;
////							instance_data[instance].msg_f.messageData[instance_data[instance].finalTxTimeIdx+5] = instance_data[instance].rxResponseMask;
////							instance_data[instance].msg_f.messageData[instance_data[instance].finalTxTimeIdx+6] = instance_data[instance].rxResponseMask >> 8;
//						}
//						else //normal anchor mode
//						{
//							if((instance_data[instance].rxResps[instance_data[instance].rxRespsIdx] >= 0) //we got the poll else ignore this response
//									&& (instance_data[instance].responseTO > 0)	) //if responseTO == 0 we have already received all of the responses - meaning should not be here => error
//							{
//								instance_data[instance].rxResps[instance_data[instance].rxRespsIdx]++; //increment the number of responses received
//								instance_data[instance].responseTO--;
//
//								//send a response or re-enable rx
//								dw_event.type_pend = anctxorrxreenable(sourceAddress, 4+0);
//							}
//							else //like a timeout (error) ...
//							{
//								led_on(LED_PC9);
//								//send a response or re-enable rx
//								dwt_setrxtimeout(0); //reconfigure the timeout
//								dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
//								dw_event.type_pend = DWT_SIG_RX_PENDING ;
//								led_off(LED_PC9);
//							}
//						}
//						break;

//					case RTLS_DEMO_MSG_IOSTATUS:
//						// get the status IO message
//						instance_data[instance].StatusIO1 = dw_event.msgu.frame[10];
//						instance_data[instance].StatusIO2 = dw_event.msgu.frame[11];
//						instance_data[instance].StatIOseqnum = dw_event.msgu.frame[12];
//
//						usbVCOMout[0] = '$';
//						usbVCOMout[1] = instance_data[instance].StatusIO1;
//						usbVCOMout[2] = instance_data[instance].StatusIO2;
//						usbVCOMout[3] = instance_data[instance].StatIOseqnum;
//						usbVCOMout[4] = '\r';
//						usbVCOMout[5] = '\n';
//						send_usbmessage(&usbVCOMout[0], 6);
//						usb_run();
//						break;

					case TOA_DEMO_MSG_SYNC_BCAST:
						led_on(LED_PC9);
						uint8 idDestTagBlink = dw_event.msgu.frame[fcode_index+SYNC_BLINK_TAGID] % MAX_BUFF_SIZE;
						uint8 idPosTagReport = dw_event.msgu.frame[fcode_index+SYNC_REP_TAGID] % MAX_BUFF_SIZE;

						// save Tag ID BLink
						instance_data[instance].TagIdBlink = dw_event.msgu.frame[fcode_index+SYNC_BLINK_TAGID];
						instance_data[instance].TagIdBlinkArray[idDestTagBlink] = instance_data[instance].TagIdBlink;
						// save Tag ID Report
						instance_data[instance].TagIdReport = dw_event.msgu.frame[fcode_index+SYNC_REP_TAGID];
						instance_data[instance].TagIdReportArray[idPosTagReport] = instance_data[instance].TagIdReport;
						// save SYNC RX timestamp
						instance_data[instance].syncRxTimestamp[idDestTagBlink] = dw_event.timeStamp;
						//when receives a SYNC, we need to remember the new sequence number
						memcpy(&instance_data[instance].SeqNumBlink[idDestTagBlink],&dw_event.msgu.frame[fcode_index+SYNC_BLINK_SEQNUM],2);
						// copy sequence number of the position report
						memcpy(&instance_data[instance].SeqNumReport[idPosTagReport],&dw_event.msgu.frame[fcode_index+SYNC_REP_SEQNUM],2);
						// get the position messages
						memcpy(&instance_data[instance].ReportPosX[idPosTagReport],&dw_event.msgu.frame[fcode_index+SYNC_REP_POS_X],sizeof(float));
						memcpy(&instance_data[instance].ReportPosY[idPosTagReport],&dw_event.msgu.frame[fcode_index+SYNC_REP_POS_Y],sizeof(float));
						memcpy(&instance_data[instance].ReportPosZ[idPosTagReport],&dw_event.msgu.frame[fcode_index+SYNC_REP_POS_Z],sizeof(float));
						// get status report whether or not it is valid
						instance_data[instance].ReportValid[idPosTagReport] = dw_event.msgu.frame[fcode_index+SYNC_REP_VALID];
						// reset blink data info
						instance_data[instance].NewBlinkData[idDestTagBlink] = 0;

//						uint8 kk = 0;
//						UART_txbuff[kk++] = 0;
//						UART_txbuff[kk++] = 0xE8;
//						UART_txbuff[kk++] = '~';
//						UART_txbuff[kk++] = instance_data[instance].TagIdBlink;
//						UART_txbuff[kk++] = (instance_data[instance].SeqNumBlink[idDestTagBlink] >> 8);
//						UART_txbuff[kk++] = instance_data[instance].SeqNumBlink[idDestTagBlink];
//						UART_txbuff[kk++] = '\r';
//						UART_txbuff[kk++] = '\n';
//
//						send_uart_txdata(kk);


						// read diagnostics data
#if (READ_DIAGNOSTICS) == 1
//						dwt_rxdiag_t diagn;
						dwt_readdignostics(&instance_data[instance].Adiags_data[sourceAddress & 0xFF]);

//						if(instance_data[instance].mode == TAG)
//						{
//							memcpy(&(instance_data[instance].Adiags_data[sourceAddress & 0xFF]), &diagn, sizeof(diagn));
//						}
//						else // srcAddr[1] == 0 means data coming from tag
//						{
//							uint8 idx;
//							idx = (sourceAddress & 0xFF) % MAX_BUFF_SIZE_TOA;
//							memcpy(&(instance_data[instance].Tdiags_data[idx]), &diagn, sizeof(diagn));
//						}
#endif

						//send response or re-enable rx
						if(instance_data[instance].mode == TAG && \
								instance_data[instance].TagIdBlink == (instance_data[instance].instanceAddress16 & 0xFF))
						{
							// set delayed reply (BLINK)
//							instance_data[instance].delayedReplyTime = \
//									dw_event.timeStamp32h + (instance_data[instance].fixedReplyDelayAnc >> 8);
							instance_data[instance].tagCalculatedFinalTxTime = ((dw_event.timeStamp + instance_data[instance].fixedReplyDelayAnc) & MASK_TXDTS);
							instance_data[instance].delayedReplyTime = instance_data[instance].tagCalculatedFinalTxTime >> 8; // high 32 bits
							// Calculate Time Final message will be sent and write this field of Final message
							// Sending time will be delayedReplyTime, snapped to ~125MHz or ~250MHz boundary by
							// zeroing its low 9 bits, and then having the TX antenna delay added
							// getting antenna delay from the device and add it to the Calculated TX Time
							instance_data[instance].tagCalculatedFinalTxTime += instance_data[instance].txAntennaDelay;
							instance_data[instance].tagCalculatedFinalTxTime &= MASK_40BIT;
							// diffRxTimestamp computed in tag
							instance_data[instance].diffTimestampTag[idDestTagBlink] = instance_data[instance].tagCalculatedFinalTxTime - dw_event.timeStamp;
							// prepare message
							instance_data[instance].msg_f.messageData[FCODE] = TOA_DEMO_MSG_TAG_BLINK;
							//Tag blink sequence number
							memcpy(&instance_data[instance].msg_f.messageData[TAG_BLINK_SEQNUM], \
									&instance_data[instance].SeqNumBlink[idDestTagBlink], 2);
							//Tag blink diff timestamps (computed reply delay)
							memcpy(&instance_data[instance].msg_f.messageData[TAG_BLINK_DIFFTS], \
									&instance_data[instance].diffTimestampTag[idDestTagBlink], 5);
//							instance_data[instance].msg_f.messageData[BLINK_CMDSEQNUM] = instance_data[instance].commandSeqNum;
//							instance_data[instance].msg_f.messageData[BLINK_CMDOPMODE] = instance_data[instance].opMode;
							instance_data[instance].msg_f.seqNum = instance_data[instance].frameSN++; //copy sequence number and then increment
							instance_data[instance].msg_f.sourceAddr[0] = instance_data[instance].eui64[0]; //copy the address
							instance_data[instance].msg_f.sourceAddr[1] = instance_data[instance].eui64[1]; //copy the address
							instance_data[instance].msg_f.destAddr[0] = 0xff;  //set the destination address (broadcast == 0xffff)
							instance_data[instance].msg_f.destAddr[1] = 0xff;  //set the destination address (broadcast == 0xffff)
							instance_data[instance].psduLength = (TAG_BLINK_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
//							dwt_setrxaftertxdelay(instance_data[instance].ancRespRxDelay);
							// write the frame data to DW TX buffer
							dwt_writetxdata(instance_data[instance].psduLength, (uint8 *)  &instance_data[instance].msg_f, 0) ;
							dwt_writetxfctrl(instance_data[instance].psduLength, 0, 0/*ERWIN*/); //write frame control
							// set this to automatically enable receiver after transmission
							instance_data[instance].wait4ack = DWT_RESPONSE_EXPECTED; //response is expected -> automatically enable the receiver
							// send delayed TX BLINK
							dwt_setdelayedtrxtime(instance_data[instance].delayedReplyTime) ;
							if(dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED))
							{
								//if TX has failed - we need to re-enable RX for the next response or final reception...
								dwt_setrxaftertxdelay(0);
								instance_data[instance].wait4ack = 0; //clear the flag as the TX has failed the TRX is off
								instance_data[instance].lateTX++;
								instance_data[instance].delayedReplyTime += 2*(instance_data[instance].fixedReplyDelayAnc >> 8); //to take into account W4R
								ancenablerx();
								dw_event.type_pend = DWT_SIG_RX_PENDING ;
							}
							else
							{
								instance_data[instance].delayedReplyTime += (instance_data[instance].fixedReplyDelayAnc >> 8); //to take into account W4R
								dw_event.type_pend = DWT_SIG_TX_PENDING ; // exit this interrupt and notify the application/instance that TX is in progress.
								instance_data[instance].timeofTx = portGetTickCnt();
								instance_data[instance].monitor = 1;
							}

							instance_data[instance].testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
							instance_data[instance].previousState = TOA_TAG_BLINK;

							instance_data[instance].toaEvtTime[idDestTagBlink] = portGetTickCnt() & 0xffffffff;
						}
						else
						{
							dwt_setrxtimeout(0); //reconfigure the timeout
							dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
							dw_event.type_pend = DWT_SIG_RX_PENDING ;
						}
						led_off(LED_PC9);

//						uint8 kk = 0;
//						kk = 0;
//						UART_txbuff[kk++] = 0;
//						//				UART_txbuff[kk++] = 0xE8;
//						//				UART_txbuff[kk++] = '~';
//						//				UART_txbuff[kk++] = instance_data[instance].TagIdBlinkArray[instance_data[instance].idxBlink];
//						//				UART_txbuff[kk++] = (instance_data[instance].SeqNumBlink[instance_data[instance].idxBlink] >> 8);
//						//				UART_txbuff[kk++] = instance_data[instance].SeqNumBlink[instance_data[instance].idxBlink];
//						UART_txbuff[kk++] = '\r';
//						UART_txbuff[kk++] = '\n';
//
//						send_uart_txdata(kk);
						break;

					case TOA_DEMO_MSG_TAG_BLINK:
						led_on(LED_PC9);

						instance_data[instance].TagIdBlink = (sourceAddress & 0xFF);
						uint8 idSrcTagBlink = instance_data[instance].TagIdBlink % MAX_BUFF_SIZE;

						instance_data[instance].blinkRxTimestamp[idSrcTagBlink] = dw_event.timeStamp ; //save BLINK Rx time

						//when receives BLINK, save the data
						instance_data[instance].TagIdBlinkArray[idSrcTagBlink] = instance_data[instance].TagIdBlink;
						memcpy(&instance_data[instance].SeqNumBlink[idSrcTagBlink],&dw_event.msgu.frame[fcode_index+TAG_BLINK_SEQNUM],2);
						memcpy(&instance_data[instance].diffTimestampTag[idSrcTagBlink],&dw_event.msgu.frame[fcode_index+TAG_BLINK_DIFFTS],5);

						//check from the sequence number
//						uint16 seqNumBlink = (uint16)messageData[TAG_BLINK_SEQNUM] | (uint16)(messageData[TAG_BLINK_SEQNUM+1] << 8);
						//                            	if(instance_data[instance].SeqNumBlink[(srcAddr[0] & 0xFF)] == seqNumBlink) {
//						if(instance_data[instance].blinkRxTimestamp[idSrcTagBlink] >= instance_data[instance].syncRxTimestamp[idSrcTagBlink])
							instance_data[instance].diffRxTimestamp[idSrcTagBlink] = \
							instance_data[instance].blinkRxTimestamp[idSrcTagBlink] - instance_data[instance].syncRxTimestamp[idSrcTagBlink];
//						else
//							instance_data[instance].diffRxTimestamp[idSrcTagBlink] = \
//							instance_data[instance].blinkRxTimestamp[idSrcTagBlink] \
//							+ 0x000000FFFFFFFFFF - instance_data[instance].syncRxTimestamp[idSrcTagBlink];
						//                            	}
//						instance_data[instance].diffRxTimestamp[idSrcTagBlink] -= instance_data[instance].diffTimestampTag[idSrcTagBlink];

						instance_data[instance].toaEvtTime[idSrcTagBlink] = portGetTickCnt() & 0xffffffff;
						// set blink data info
						instance_data[instance].NewBlinkData[idSrcTagBlink] = 1;
						//send a response or re-enable rx
//						if(instance_data[instance].mode == ANCHOR_SYNC)
//							instance_data[instance].stopTimer = 1;
//						dwt_setrxtimeout(0); //reconfigure the timeout
						dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
						dw_event.type_pend = DWT_SIG_RX_PENDING ;

						// read diagnostics data
#if (READ_DIAGNOSTICS) == 1
						if(instance_data[instance].mode != TAG)
							dwt_readdignostics(&instance_data[instance].Tdiags_data[idSrcTagBlink]);

//						if(instance_data[instance].mode != TAG)
//							memcpy(&(instance_data[instance].Tdiags_data[idSrcTagBlink]), &diagn, sizeof(diagn));
#endif
						led_off(LED_PC9);
						break;

					//if anchor fall into case below and process the frame
					default:  //process rx frame
					{
						dw_event.type_pend = DWT_SIG_DW_IDLE;
					}
					break;

				}
			}//end of if not Listener mode
	    	instance_data[instance].stopTimer = 1;

            instance_putevent(dw_event, rxd_event);

			instance_data[instance].rxMsgCount++;
		}
//		else if(rxd_event == DWT_SIG_RX_SYNCLOSS)
//		{
//			dwt_softreset();
//			return;
//		}
		else //if (rxd_event == SIG_RX_UNKNOWN) //need to re-enable the rx (got unknown frame type)
		{
			dwt_softreset();
			handle_error_unknownframe(dw_event);
			return;
		}
	}
	else if (rxd->event == DWT_SIG_RX_TIMEOUT) //if tag and got TO, then did not get any or some responses - check if need to send final.
	{
		dw_event.type_pend = DWT_SIG_DW_IDLE;

		if(instance_data[instance].mode == ANCHOR)
		{
			//check if anchor has received all of the responses from other anchors (it could have received only 1 or 2)
			//it's timed out (re-enable rx or tx response)
			if(instance_data[instance].responseTO > 0)
			{
				instance_data[instance].responseTO--;
				//send a response or re-enable rx
				dw_event.type_pend = anctxorrxreenable(instance_data[instance].instanceAddress16, 6+0);
			}
		}
		dw_event.type = 0;
		dw_event.type_save = DWT_SIG_RX_TIMEOUT;
		dw_event.rxLength = 0;
		dw_event.timeStamp = 0;
		dw_event.timeStamp32l = 0;
		dw_event.timeStamp32h = 0;

		instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);
		//printf("RX timeout while in %d\n", instance_data[instance].testAppState);
	}
	else //assume other events are errors
	{
		//led_on(LED_PC9);
		handle_error_unknownframe(dw_event);
		return;
		//led_off(LED_PC9);
	}
}
#endif

#pragma GCC optimize ("O3")
int instance_peekevent(void)
{
	int instance = 0;
    return instance_data[instance].dwevent[instance_data[instance].dweventPeek].type; //return the type of event that is in front of the queue
}

#pragma GCC optimize ("O3")
void instance_saveevent(event_data_t newevent, uint8 etype)
{
	int instance = 0;

	instance_data[instance].saved_dwevent = newevent;
	instance_data[instance].saved_dwevent.type = etype;
}

#pragma GCC optimize ("O3")
event_data_t instance_getsavedevent(void)
{
	int instance = 0;

	return instance_data[instance].saved_dwevent;
}

#pragma GCC optimize ("O3")
void instance_putevent(event_data_t newevent, uint8 etype)
{
	int instance = 0;
	//newevent.eventtime = portGetTickCnt(); //ERWIN
	newevent.gotit = 0 ; //newevent.eventtimeclr = 0;

	//copy event
	instance_data[instance].dwevent[instance_data[instance].dweventIdxIn] = newevent;

	//set type - this makes it a new event (making sure the event data is copied before event is set as new)
	//to make sure that the get event function does not get an incomplete event
	instance_data[instance].dwevent[instance_data[instance].dweventIdxIn].type = etype;

	instance_data[instance].dweventIdxIn++;

	if(MAX_EVENT_NUMBER == instance_data[instance].dweventIdxIn)
	{
		instance_data[instance].dweventIdxIn = 0;
	}
	//eventIncount++;

	//printf("put %d - in %d out %d @ %d\n", newevent.type, instance_data[instance].dweventCntIn, instance_data[instance].dweventCntOut, ptime);
}

event_data_t dw_event_g;

#pragma GCC optimize ("O3")
event_data_t* instance_getevent(int x)
{
	int instance = 0;
	int indexOut = instance_data[instance].dweventIdxOut;

	//dw_event_g = instance_data[instance].dwevent[instance_data[instance].dweventCntOut]; //this holds any TX/RX events

	//memcpy(&dw_event_g, &instance_data[instance].dwevent[instance_data[instance].dweventCntOut], sizeof(event_data_t));

	if(instance_data[instance].dwevent[indexOut].type == 0) //exit with "no event"
	{
		dw_event_g.type = 0;
		dw_event_g.type_save = 0;
		return &dw_event_g;
	}

	//copy the event
	dw_event_g.type_save = instance_data[instance].dwevent[indexOut].type_save ;
	dw_event_g.type_pend = instance_data[instance].dwevent[indexOut].type_pend ;
	dw_event_g.rxLength = instance_data[instance].dwevent[indexOut].rxLength ;
	dw_event_g.timeStamp = instance_data[instance].dwevent[indexOut].timeStamp ;
	dw_event_g.timeStamp32l = instance_data[instance].dwevent[indexOut].timeStamp32l ;
	dw_event_g.timeStamp32h = instance_data[instance].dwevent[indexOut].timeStamp32h ;
	dw_event_g.uTimeStamp = instance_data[instance].dwevent[indexOut].uTimeStamp ;
	//dw_event_g.eventtime = instance_data[instance].dwevent[indexOut].eventtime ;
	//dw_event_g.eventtimeclr = instance_data[instance].dwevent[indexOut].eventtimeclr ;
	//dw_event_g.gotit = instance_data[instance].dwevent[indexOut].gotit ;

	memcpy(&dw_event_g.msgu, &instance_data[instance].dwevent[indexOut].msgu, sizeof(instance_data[instance].dwevent[indexOut].msgu));

	dw_event_g.type = instance_data[instance].dwevent[indexOut].type ;


	instance_data[instance].dwevent[indexOut].gotit = x;

	//instance_data[instance].dwevent[indexOut].eventtimeclr = portGetTickCnt(); //ERWIN

	instance_data[instance].dwevent[indexOut].type = 0; //clear the event

	instance_data[instance].dweventIdxOut++;
	if(MAX_EVENT_NUMBER == instance_data[instance].dweventIdxOut) //wrap the counter
	{
		instance_data[instance].dweventIdxOut = 0;
	}
	instance_data[instance].dweventPeek = instance_data[instance].dweventIdxOut; //set the new peek value

	//if(dw_event.type) printf("get %d - in %d out %d @ %d\n", dw_event.type, instance_data[instance].dweventCntIn, instance_data[instance].dweventCntOut, ptime);

	//eventOutcount++;


	return &dw_event_g;
}

void instance_clearevents(void)
{
	int i = 0;
	int instance = 0;

	for(i=0; i<MAX_EVENT_NUMBER; i++)
	{
        memset(&instance_data[instance].dwevent[i], 0, sizeof(event_data_t));
	}

	instance_data[instance].dweventIdxIn = 0;
	instance_data[instance].dweventIdxOut = 0;
	instance_data[instance].dweventPeek = 0;

	//eventOutcount = 0;
	//eventIncount = 0;
}

// -------------------------------------------------------------------------------------------------------------------
#pragma GCC optimize ("O3")
int instance_run_TWR(void)
{
#if 1
    int instance = 0 ;
    int done = INST_NOT_DONE_YET;
    int message = instance_peekevent(); //get any of the received events from ISR
//    uint8 reset_trx = 0;
    int32 timeSinceLastWakeUp;
    int32 timeSinceLastForceWakeup;

	while(done == INST_NOT_DONE_YET)
	{
		//int state = instance_data[instance].testAppState;
		done = testapprun_TWR(&instance_data[instance], message) ;                                               // run the communications application

		//we've processed message
		message = 0;

		//force to run scheduling after initialization
		if(instance_data[instance].previousState == TA_INIT)
			done = INST_DONE_WAIT_FOR_NEXT_EVENT_TO;
	}

	instance_data[instance].CurrentTime = portGetTickCnt(); //ERWIN

	timeSinceLastWakeUp = instance_data[instance].CurrentTime - instance_data[instance].instanceWakeTime;
	timeSinceLastForceWakeup = instance_data[instance].CurrentTime - instance_data[instance].timeForceWakeUp;

	//force wake up periodically
//	if(instance_data[instance].mode == TAG)
//	{
		if(instance_data[instance].CurrentTime > instance_data[instance].timeForceWakeUp && timeSinceLastForceWakeup > instance_data[instance].periodForceWakeUp)
		{
			//read device ID to force the chip to wake up
			instancereaddeviceid() ;
			instance_data[instance].timeForceWakeUp = portGetTickCnt(); //ERWIN
		}
//	}
//	else
//	{
//		if(timeSinceLastWakeUp >= instance_data[instance].tagSleepTime_ms)
//			//read device ID to force the chip to wake up
//			instancereaddeviceid() ;
//	}

//	//read device ID to force the chip to wake up
//	instancereaddeviceid() ;

	instance_data[instance].instanceTimerEn = 1; //start timer
    if(done == INST_DONE_WAIT_FOR_NEXT_EVENT_TO) //we are in RX and need to timeout (Tag needs to send another poll if no Rx frame)
    {
//    	instance_data[instance].instanceTimerEn = 1; //start timer
        if(instance_data[instance].mode == TAG) //Tag (is either in RX or sleeping)
        {
        	int32 nextPeriod ;
        	int offset = 0; // this offset come from trial and error (13 means 1.3 ms)

        	// next period will be a positive number because correction is -0.5 to +1.5 periods, (and tagSleepTime_ms is the period)
        	nextPeriod = instance_data[instance].tagSleepRnd + instance_data[instance].tagSleepTime_ms + instance_data[instance].tagSleepCorrection;

        	instance_data[instance].nextSleepPeriod = (uint32) nextPeriod ; //set timeout time, CAST the positive period to UINT for correct wrapping.
        	instance_data[instance].tagSleepCorrection2 = instance_data[instance].tagSleepCorrection;
        	instance_data[instance].tagSleepCorrection = 0; //clear the correction

//        	if(instance_data[instance].getReportReqs == 1)
        	instance_data[instance].nextReportPeriod = (instance_data[instance].nextSleepPeriod) - \
        			(instance_data[instance].numSlots-1)*(instance_data[instance].slotPeriod) + offset;
//        	reset_trx = 1;

            instance_data[instance].instanceTimerEn = 1; //start timer
        }
//        else if(instance_data[instance].mode == ANCHOR_SYNC) //Anchor Sync will send SYNC broadcast and wait for TAG's POLL (is either in TX or RX)
//        	instance_data[instance].instanceTimerEn = 1; //start timer

        instance_data[instance].stopTimer = 0 ; //clear the flag - timer can run if instancetimer_en set (set above)
        instance_data[instance].done = INST_NOT_DONE_YET;
    }

    //check if timer has expired
    if((instance_data[instance].instanceTimerEn == 1) && (instance_data[instance].stopTimer == 0))
    {
//    	instance_data[instance].CurrentTime = portGetTickCnt(); //ERWIN
//
//    	uint32 timeSinceLastWakeUp = instance_data[instance].CurrentTime - instance_data[instance].instanceWakeTime;

//    	if(instance_data[instance].getReportReqs == 0)
//    		instance_data[instance].nextReportPeriod = (instance_data[instance].nextSleepPeriod >> 1) - \
//			(instance_data[instance].numSlots-1)*(instance_data[instance].slotPeriod >> 1) -25;
//    	if(instance_data[instance].nextReportPeriod < 0)


        if(instance_data[instance].mode == TAG)
        {
        	if(instance_data[instance].enableReport == 1 && instance_data[instance].getReportReqs == 1)
        	{
        		if(instance_data[instance].CurrentTime > instance_data[instance].instanceWakeTime && \
        				timeSinceLastWakeUp >= instance_data[instance].nextReportPeriod) // && timeSinceLastWakeUp < instance_data[instance].nextSleepPeriod)
        		{
//        			event_data_t dw_event;
//        			if(reset_trx)
//        			{
//        				uint32 devID ;
        				//    instanceConfig_t uwbPhyConfig;
//        				int result;

//        				SPI_ConfigFastRate(SPI_BaudRatePrescaler_32);  //max SPI before PLLs configured is ~4M

//        				dwt_forcetrxoff();	//this will clear all events
        				//this is called here to wake up the device (i.e. if it was in sleep mode before the restart)
//        				devID = dwt_readdevid() ;

//        				//reset the DW1000 by driving the RSTn line low
//        				reset_DW1000();
//
////        				SPI_ConfigFastRate(SPI_BaudRatePrescaler_4); //increase SPI to max
//        				devID = instancereaddeviceid() ;

//        				event_data_t dw_event;
//        				dwt_forcetrxoff();	//this will clear all events
//        				instance_data[instance].instanceTimerEn = 0;
//        				dw_event.rxLength = 0;
//        				dw_event.type = 0;
//        				dw_event.type_save = 0x80 | DWT_SIG_RX_TIMEOUT;
//        				//printf("PC timeout DWT_SIG_RX_TIMEOUT\n");
//        				instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);

//        				dwt_forcetrxoff();	//this will clear all events
//        				dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering
//        				dwt_setrxaftertxdelay(0); //no delay of turning on of RX
        				dwt_setrxtimeout(0);
//        				dwt_setpreambledetecttimeout(0);
        				//change to next state - wait to receive a message
//        				instance_data[instance].testAppState = TA_RXE_WAIT ;
        				dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
//        				reset_trx = 0;
//        				instance_data[instance].nextReportPeriod += instance_data[instance].slotPeriod;
//        			}
//        			instance_data[instance].testAppState = TA_RXE_WAIT;
//        			instance_putevent(dw_event, DWT_SIG_RX_PENDING);

//        			instance_data[instance].getReportReqs = 1;

        			instance_data[instance].nextReportPeriod += instance_data[instance].slotPeriod;

        			if(instance_data[instance].nextReportPeriod >= instance_data[instance].nextSleepPeriod)
        				instance_data[instance].getReportReqs = 0;

        			instance_data[instance].rxRespsImRpt[instance_data[instance].rangeNum] = 0; //reset the number of received responses

//        			instance_data[instance].responseTO = (instance_data[instance].maxAnchorsTwr + 2)*(instance_data[instance].maxTagsTwr - 1); //expecting 4 responses
//        			dwt_setrxtimeout((uint16)instance_data[instance].fwtoTime_sy * instance_data[instance].maxAnchorsTwr);  //configure the RX FWTO
//        			led_on(LED_PC9);
        			//send a response or re-enable rx
//        			dwt_setdelayedtrxtime(instance_data[instance].delayedReplyTime+RX_RESPONSE1_TURNAROUND) ;
//        			dwt_rxenable(DWT_START_RX_DELAYED);
//        			dwt_setrxtimeout(0); //reconfigure the timeout
//        			dwt_setrxaftertxdelay((uint32)3*RX_RESPONSE1_TURNAROUND);
//        			dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
        			//					dw_event->type_pend = DWT_SIG_RX_PENDING ;
//        			led_off(LED_PC9);

//        			instance_data[instance].rxResponseMaskImRpt = 0;	//reset/clear the mask of received responses when tx poll
        			instance_data[instance].newRangeImRpt = 0;
//        			instance_data[instance].newRangeImRpt = 0;

        			instance_data[instance].newRangeImRpt = instance_calcranges(&instance_data[instance].tofArrayImRpt[0], \
        					instance_data[instance].maxAnchorsTwr, TOF_REPORT_T2A, &instance_data[instance].rxResponseMaskImRpt, 1);

        			instance_data[instance].rxResponseMaskReportImRpt = instance_data[instance].rxResponseMaskImRpt;

        			instance_data[instance].rxResponseMaskImRpt = 0;

        			if(instance_data[instance].newRangeImRpt)
        				instance_data[instance].newRangeTimeImRpt = portGetTickCnt(); //ERWIN

        			instance_data[instance].testAppState = TA_SLEEP_DONE;
        			instance_data[instance].nextState = TA_TXPOLL_WAIT_SEND;
        			instance_data[instance].previousState = TA_TXFINAL_WAIT_SEND;

//        			instance_data[instance].getReportReqs = 0;

//        			dwt_forcetrxoff();	//this will clear all events
//        			instance_data[instance].instToSleep = TRUE ;
        		}
        	}
//        	else
//        	{
//        	if((instance_data[instance].CurrentTime - instance_data[instance].instanceWakeTime) >= \
//        			instance_data[instance].nextSleepPeriod || \
//					(instance_data[instance].getReportReqs && \
//							(instance_data[instance].CurrentTime - instance_data[instance].instanceWakeTime) >= \
//							instance_data[instance].nextReportPeriod))
        	if(instance_data[instance].CurrentTime > instance_data[instance].instanceWakeTime && \
        			timeSinceLastWakeUp >= instance_data[instance].nextSleepPeriod)
        	{
        		instance_data[instance].testAppState = TA_SLEEP_DONE;
        		instance_data[instance].nextState = TA_TXPOLL_WAIT_SEND;
        		instance_data[instance].previousState = TA_TXFINAL_WAIT_SEND;

        		event_data_t dw_event;
//        		dwt_forcetrxoff();	//this will clear all events
        		instance_data[instance].instanceTimerEn = 0;
        		dw_event.rxLength = 0;
        		dw_event.type = 0;
        		dw_event.type_save = 0x80 | DWT_SIG_RX_TIMEOUT;
        		//printf("PC timeout DWT_SIG_RX_TIMEOUT\n");
        		instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);
        		//        			inst_processrxtimeout(&instance_data[instance]);
        		instance_data[instance].rxResponseMaskImRpt = 0;	//reset/clear the mask of received responses when tx poll
        		if(instance_data[instance].enableReport == 1)
        			instance_data[instance].getReportReqs = 1;
        	}
//        	}
        }
//        else if(instance_data[instance].mode == ANCHOR_SYNC)
//        {
//        	instance_data[instance].CurrentTime = portGetTickCnt(); //ERWIN
//        	if((instance_data[instance].CurrentTime - instance_data[instance].instanceWakeTime) > instance_data[instance].slotPeriod)
//        	{
//        		event_data_t dw_event;
//        		instance_data[instance].instanceTimerEn = 0;
//        		dw_event.rxLength = 0;
//        		dw_event.type = 0;
//        		dw_event.type_save = 0x80 | DWT_SIG_RX_TIMEOUT;
//        		//printf("PC timeout DWT_SIG_RX_TIMEOUT\n");
//        		instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);
//
//        		//				instance_data[instance].testAppState = TOA_SYNC_BCAST;
//        	}
//        }
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
        else if(instance_data[instance].mode == ANCHOR)
        {
        	uint32_t slotTime = portGetTickCnt() % instance_data[instance].sframePeriod; //ERWIN

        	if(instance_data[instance].gatewayAnchor)
        	{
				//if we are in the last slot - then A0 ranges to A1 and A2
				if( slotTime >= instance_data[instance].a0SlotTime)
				{
					nrf_drv_gpiote_in_event_disable(DW1000_IRQ); //ERWIN todo port_DisableEXT_IRQ(); //enable ScenSor IRQ before starting
					//anchor0 sends poll to anchor1
					instance_data[instance].mode = ANCHOR_RNG; //change to ranging initiator
					dwt_forcetrxoff(); //disable DW1000
					instance_clearevents(); //clear any events
					//change state to send a Poll
					instance_data[instance].testAppState = TA_TXPOLL_WAIT_SEND ;
					instance_data[instance].msg_f.destAddr[0] = 0x1 ;
					instance_data[instance].msg_f.destAddr[1] = (GATEWAY_ANCHOR_ADDR >> 8);
					instance_data[instance].instanceTimerEn = 0;
					instance_data[instance].rangeNumAnc++;
					nrf_drv_gpiote_in_event_enable(DW1000_IRQ, true); //ERWIN todo port_EnableEXT_IRQ(); //enable ScenSor IRQ before starting
				}
        	}
			else if (instance_data[instance].instanceAddress16 == A1_ANCHOR_ADDR) //A1 ranges to A2 in the 2nd half of last slot
			{
				if(portGetTickCnt() >= instance_data[instance].a1SlotTime) //ERWIN
				{
					nrf_drv_gpiote_in_event_disable(DW1000_IRQ); //ERWIN todo port_DisableEXT_IRQ(); //enable ScenSor IRQ before starting
					//anchor1 sends poll to anchor2
					instance_data[instance].mode = ANCHOR_RNG; //change to ranging initiator
					dwt_forcetrxoff(); //disable DW1000
					instance_clearevents(); //clear any events
					//change state to send a Poll
					instance_data[instance].testAppState = TA_TXPOLL_WAIT_SEND ;
					instance_data[instance].msg_f.destAddr[0] = 0x2 ;
					instance_data[instance].msg_f.destAddr[1] = (GATEWAY_ANCHOR_ADDR >> 8);

					instance_data[instance].instanceTimerEn = 0;
					//instance_data[instance].a1SlotTime = 0;
					nrf_drv_gpiote_in_event_enable(DW1000_IRQ, true); //ERWIN todo port_EnableEXT_IRQ(); //enable ScenSor IRQ before starting
				}
			}
		}
#endif
    }

#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
    else if (instance_data[instance].instanceTimerEn == 0)
    {
        if((instance_data[instance].mode == ANCHOR) && (instance_data[instance].gatewayAnchor))
        {
        	uint32_t slotTime = portGetTickCnt() % instance_data[instance].sframePeriod; //ERWIN
        	//enable the timer in 1st slot
        	if(slotTime < instance_data[instance].slotPeriod)
        	{
        		instance_data[instance].instanceTimerEn = 1;
        	}
        }
    }
#endif
#endif
    return 0 ;
}

#pragma GCC optimize ("O3")
int instance_run_TOA(void)
{
    int instance = 0 ;
    int done = INST_NOT_DONE_YET;
    int message = instance_peekevent(); //get any of the received events from ISR


	while(done == INST_NOT_DONE_YET)
	{
		//int state = instance_data[instance].testAppState;
		done = testapprun_TOA(&instance_data[instance], message) ;                                               // run the communications application

		//we've processed message
		message = 0;
	}



    if(done == INST_DONE_WAIT_FOR_NEXT_EVENT_TO) //we are in RX and need to timeout (Tag needs to send another poll if no Rx frame)
    {
        if(instance_data[instance].mode == ANCHOR_SYNC) //Anchor Sync will send SYNC broadcast and wait for TAG's BLINK (is either in TX or RX)
        	instance_data[instance].instanceTimerEn = 1; //start timer

        instance_data[instance].stopTimer = 0 ; //clear the flag - timer can run if instancetimer_en set (set above)
        instance_data[instance].done = INST_NOT_DONE_YET;
    }

    //check if timer has expired
    if((instance_data[instance].instanceTimerEn == 1) && (instance_data[instance].stopTimer == 0))
    {
        if(instance_data[instance].mode == ANCHOR_SYNC)
        {
        	instance_data[instance].CurrentTime = portGetTickCnt(); //ERWIN
			if((instance_data[instance].CurrentTime - instance_data[instance].instanceWakeTime) > instance_data[instance].PeriodSync)
			{
				event_data_t dw_event;
				instance_data[instance].instanceTimerEn = 0;
				dw_event.rxLength = 0;
				dw_event.type = 0;
				dw_event.type_save = 0x80 | DWT_SIG_RX_TIMEOUT;
				//printf("PC timeout DWT_SIG_RX_TIMEOUT\n");
				instance_putevent(dw_event, DWT_SIG_RX_TIMEOUT);

//				instance_data[instance].testAppState = TOA_SYNC_BCAST;
			}
        }
    }

    return 0 ;
}


void instance_close(void)
{
    //wake up device from low power mode
    //NOTE - in the ARM  code just drop chip select for 200us
#if 0 //ERWIN todo check CS aka SS, but it seems there isn't a SS
    nrf_gpio_pin_clear(NRF_DRV_SPI_PIN_NOT_USED); //ERWIN port_SPIx_clear_chip_select();  //CS low
    Sleep(1);   //200 us to wake up then waits 5ms for DW1000 XTAL to stabilise
    nrf_gpio_pin_clear(NRF_DRV_SPI_PIN_NOT_USED); //ERWIN port_SPIx_set_chip_select();  //CS high
#endif
    Sleep(5);
    dwt_entersleepaftertx(0); // clear the "enter deep sleep after tx" bit

    dwt_setinterrupt(0xFFFFFFFF, 0); //don't allow any interrupts

}


void instance_notify_DW1000_inIDLE(int idle)
{
	instance_data[0].dwIDLE = idle;
}

void instanceconfigtxpower(uint32 txpower)
{
	instance_data[0].txPower = txpower ;

	instance_data[0].txPowerChanged = 1;

}

void instancesettxpower(void)
{
	if(instance_data[0].txPowerChanged == 1)
	{
	    //Configure TX power
	    dwt_write32bitreg(0x1E, instance_data[0].txPower);

		instance_data[0].txPowerChanged = 0;
	}
}

void instanceconfigantennadelays(uint16 tx, uint16 rx)
{
	instance_data[0].txAntennaDelay = tx ;
	instance_data[0].rxAntennaDelay = rx ;

	instance_data[0].antennaDelayChanged = 1;
}

void instancesetantennadelays(void)
{
//	if(instance_data[0].antennaDelayChanged == 1)
//	{
		dwt_setrxantennadelay(instance_data[0].rxAntennaDelay);
		dwt_settxantennadelay(instance_data[0].txAntennaDelay);

//		instance_data[0].antennaDelayChanged = 0;
//	}
}


uint16 instancetxantdly(void)
{
	return instance_data[0].txAntennaDelay;
}

uint16 instancerxantdly(void)
{
	return instance_data[0].rxAntennaDelay;
}

uint16 instancevalidranges(void)
{
	uint16 x = instance_data[0].rxResponseMaskReport;
	instance_data[0].rxResponseMaskReport = 0; //reset mask as we have printed out the ToFs
	return x;
}
//#endif


/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
