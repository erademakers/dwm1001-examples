/*! ----------------------------------------------------------------------------
 *  @file    instance.c
 *  @brief   DecaWave application level message exchange for ranging demo
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
//ERWIN #include "deca_spi.h"
#include "deca_regs.h"

#include "instance.h"

extern uint8 UART_txbuff[20];
extern void send_uart_txdata(uint8 len);
//extern uint8 UpdateSettings;
//extern uint8 usbVCOMout[400];
extern sfConfig_t sfConfigTwr;

// -------------------------------------------------------------------------------------------------------------------


// -------------------------------------------------------------------------------------------------------------------
//      Data Definitions
// -------------------------------------------------------------------------------------------------------------------

const uint16 rfDelays[2];
const uint16 rfDelaysTREK[2];
const tx_struct txSpectrumConfig[8];

const antdelay_struct rfDelaysAntOrig[8];
const antdelay_struct rfDelaysAntTaoglas[8];

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE: the maximum RX timeout is ~ 65ms
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


// -------------------------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------
//
// function to construct the message/frame header bytes
//
// -------------------------------------------------------------------------------------------------------------------
//
void instanceconfigframeheader16(instance_data_t *inst)
{
    //set frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
    inst->msg_f.frameCtrl[0] = 0x1 /*frame type 0x1 == data*/ | 0x40 /*PID comp*/;

	//source/dest addressing modes and frame version
	inst->msg_f.frameCtrl[1] = 0x8 /*dest extended address (16bits)*/ | 0x80 /*src extended address (16bits)*/;

	inst->msg_f.panID[0] = (inst->panID) & 0xff;
	inst->msg_f.panID[1] = inst->panID >> 8;

    inst->msg_f.seqNum = 0;
}

int instancesenddlypacket(instance_data_t *inst, int delayedTx)
{
    int result = 0;

    dwt_writetxfctrl(inst->psduLength, 0, 0);
    if(delayedTx == DWT_START_TX_DELAYED)
    {
        dwt_setdelayedtrxtime(inst->delayedReplyTime) ; //should be high 32-bits of delayed TX TS
    }

    //begin delayed TX of frame
    if (dwt_starttx(delayedTx | inst->wait4ack))  // delayed start was too late
    {
        result = 1; //late/error
        inst->lateTX++;
    }
    else
    {
    	inst->timeofTx = portGetTickCnt();
        inst->monitor = 1;
    }
    return result;                                              // state changes
}


int instance_calcranges(uint32 *array, uint16 size, int reportRange, uint16* mask, uint8 isImRpt)
{
	int i;
	int newRange = TOF_REPORT_NUL;
	int distance = 0;

	for(i=0; i<size; i++)
	{
		uint32 tofx = array[i];
		if(tofx != INVALID_TOF) //if ToF == 0 - then no new range to report
		{
			distance = reportTOF(i, tofx, isImRpt);
		}

		if(distance == 1)
		{
			newRange = reportRange;
		}
		else
		{
			//clear mask
			*mask &= ~(0x0001 << i) ;
			if(isImRpt)
				clearDistTableImRpt(i);
			else
				clearDistTable(i);
		}
		array[i] = INVALID_TOF;

		distance = 0;
	}

	return newRange;
}

// -------------------------------------------------------------------------------------------------------------------
//
// the main instance state machine (all the instance modes Tag, Anchor or Listener use the same statemachine....)
//
// -------------------------------------------------------------------------------------------------------------------
//
int testapprun_TWR(instance_data_t *inst, int message)
{
#if 1
    switch (inst->testAppState)
    {
        case TA_INIT :
            // printf("TA_INIT") ;
        	memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);
        	dwt_seteui(inst->eui64);

        	dwt_setpanid(inst->panID);

        	inst->shortAdd_idx = (inst->instanceAddress16 & 0xFF) ;
        	dwt_setaddress16(inst->instanceAddress16);

        	dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering

//        	if(inst->mode == ANCHOR && inst->instanceAddress16 == inst->gatewayAnchorAddress)
//        		inst->mode = ANCHOR_SYNC;

            switch (inst->mode)
            {
//            	case ANCHOR_SYNC:
//            	{
//            		uint16 sleep_mode = 0;
//
////            		dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;
////            		dwt_setpanid(inst->panID);
////
////            		memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);
////            		dwt_seteui(inst->eui64);
////
////            		//set source address
////					//                    inst->newRangeTagAddress = inst->instanceAddress16 ;
////            		dwt_setaddress16(inst->instanceAddress16);
//
//            		//Start off by Sleeping 1st -> set instToSleep to TRUE
//            		inst->nextState = TWR_SYNC_BCAST;
//            		inst->testAppState = TA_TXE_WAIT;
////            		inst->instToSleep = TRUE ;
////            		inst->nextState = 0;
////            		inst->testAppState = TOA_SYNC_BCAST;
//            		inst->instToSleep = FALSE ;
//
//            		inst->rangeNum = 0;
//            		inst->tagSleepCorrection = 0;
//
//            		sleep_mode = (DWT_LOADUCODE|DWT_PRESRV_SLEEP|DWT_CONFIG|DWT_TANDV);
//
//            		if((dwt_getldotune() != 0)) //if we need to use LDO tune value from OTP kick it after sleep
//            			sleep_mode |= DWT_LOADLDO;
//
//            		if(inst->configData.txPreambLength == DWT_PLEN_64)  //if using 64 length preamble then use the corresponding OPSet
//            			sleep_mode |= DWT_LOADOPSET;
//
//#if (DEEP_SLEEP == 1)
//            		dwt_configuresleep(sleep_mode, DWT_WAKE_WK|DWT_WAKE_CS|DWT_SLP_EN); //configure the on wake parameters (upload the IC config settings)
//#endif
//            		instanceconfigframeheader16(inst);
//            		inst->instanceWakeTime = portGetTickCnt(); //ERWIN
//            	}
//            	break;
                case TAG:
                {
                	uint16 sleep_mode = 0;

//                    dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;
//                    dwt_setpanid(inst->panID);
//
//                    memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);
//                    dwt_seteui(inst->eui64);

                    //set source address
                    inst->newRangeTagAddress = inst->instanceAddress16 ;
//                    dwt_setaddress16(inst->instanceAddress16);

                    //Start off by Sleeping 1st -> set instToSleep to TRUE
                    inst->nextState = TA_TXPOLL_WAIT_SEND;
                    inst->testAppState = TA_TXE_WAIT;
                    inst->previousState = TA_INIT ;
                    inst->instToSleep = TRUE ;

                    inst->rangeNum = 0;
                    inst->tagSleepCorrection = 0;

                    sleep_mode = (DWT_LOADUCODE|DWT_PRESRV_SLEEP|DWT_CONFIG|DWT_TANDV);
/*ERWIN
					if((dwt_getldotune() != 0)) //if we need to use LDO tune value from OTP kick it after sleep
						sleep_mode |= DWT_LOADLDO;
*/
					if(inst->configData.txPreambLength == DWT_PLEN_64)  //if using 64 length preamble then use the corresponding OPSet
						sleep_mode |= DWT_LOADOPSET;

#if (DEEP_SLEEP == 1)
				    dwt_configuresleep(sleep_mode, DWT_WAKE_WK|DWT_WAKE_CS|DWT_SLP_EN); //configure the on wake parameters (upload the IC config settings)
#endif
				    instanceconfigframeheader16(inst);

				    inst->instanceWakeTime = portGetTickCnt(); //ERWIN
				    inst->timeForceWakeUp = inst->instanceWakeTime;
                }
                break;
                case ANCHOR:
                {
//                    memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);
//                    dwt_seteui(inst->eui64);
//
//                    dwt_setpanid(inst->panID);

                    //set source address
//                    inst->shortAdd_idx = (inst->instanceAddress16 & 0x3) ;
//                    inst->shortAdd_idx = (inst->instanceAddress16 & 0xFF) ;
//                    dwt_setaddress16(inst->instanceAddress16);

                	//if address = 0x8000
//                	if(inst->instanceAddress16 == GATEWAY_ANCHOR_ADDR)
//                	{
//                		inst->gatewayAnchor = TRUE;
//                	}

//                	dwt_enableframefilter(DWT_FF_NOTYPE_EN); //allow data, ack frames;

                	// First time anchor listens we don't do a delayed RX
					dwt_setrxaftertxdelay(0);
                    //change to next state - wait to receive a message
                    inst->testAppState = TA_RXE_WAIT ;

                    dwt_setrxtimeout(0);
                    dwt_setpreambledetecttimeout(0);
                    instanceconfigframeheader16(inst);

                    inst->instanceWakeTime = portGetTickCnt(); //ERWIN
                    inst->timeForceWakeUp = inst->instanceWakeTime;
                }
                break;
                case LISTENER:
                {
                    dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering
					dwt_setrxaftertxdelay(0); //no delay of turning on of RX
                    dwt_setrxtimeout(0);
                    dwt_setpreambledetecttimeout(0);
                    //change to next state - wait to receive a message
                    inst->testAppState = TA_RXE_WAIT ;
                }
                break ; // end case TA_INIT
                default:
                break;
            }
            break; // end case TA_INIT

        case TA_SLEEP_DONE :
        {
        	event_data_t* dw_event = instance_getevent(10); //clear the event from the queue
			// waiting for timout from application to wakup IC
			if (dw_event->type != DWT_SIG_RX_TIMEOUT && inst->previousState != TA_INIT)
			{
				// if no pause and no wake-up timeout continu waiting for the sleep to be done.
                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //wait here for sleep timeout
                break;
            }

            inst->done = INST_NOT_DONE_YET;
            inst->instToSleep = FALSE ;
            inst->testAppState = inst->nextState;
//            inst->nextState = 0; //clear
//            if(inst->enableReport)
//            {
//            	if(inst->getReportReqs == 0)
//            		inst->instanceWakeTime = portGetTickCnt(); //ERWIN // Record the time count when we wake-up
//            }
//            else
            	inst->instanceWakeTime = portGetTickCnt(); //ERWIN // Record the time count when we wake-up
            	inst->timeForceWakeUp = inst->instanceWakeTime;
#if (DEEP_SLEEP == 1)
            {
            	uint32 x = 0;
                
                //wake up device from low power mode
                //NOTE - in the ARM  code just drop chip select for 200us
            	led_on(LED_PC9);
                port_SPIx_clear_chip_select();  //CS low
                instance_data[0].dwIDLE = 0; //reset DW1000 IDLE flag

                setup_DW1000RSTnIRQ(1); //enable RSTn IRQ

                Sleep(2);   //200 us to wake up - need 2 as Sleep(1) is ~ 175 us
                //then wait 5ms for DW1000 XTAL to stabilise - instead of wait we wait for RSTn to go high
                //Sleep(5);

                //need to poll to check when the DW1000 is in IDLE, the CPLL interrupt is not reliable
                //when RSTn goes high the DW1000 is in INIT, it will enter IDLE after PLL lock (in 5 us)
                while(instance_data[0].dwIDLE == 0) // this variable will be sent in the IRQ (process_dwRSTn_irq)
                {
                	 //wait for DW1000 to go to IDLE state RSTn pin to go high
                	x++;
                }
                setup_DW1000RSTnIRQ(0); //disable RSTn IRQ
                port_SPIx_set_chip_select();  //CS high

                //!!! NOTE it takes ~35us for the DW1000 to download AON and lock the PLL and be in IDLE state
                //do some dummy reads of the dev ID register to make sure DW1000 is in IDLE before setting LEDs
            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)

            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
                /*if(x != DWT_DEVICE_ID)
                {
                	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
                }*/
                led_off(LED_PC9);
                //this is platform dependent - only program if DW EVK/EVB
                dwt_setleds(1);

                //MP bug - TX antenna delay needs reprogramming as it is not preserved (only RX)
                dwt_settxantennadelay(inst->txAntennaDelay) ;

                //set EUI as it will not be preserved unless the EUI is programmed and loaded from NVM
				dwt_seteui(inst->eui64);
            }
#else
            Sleep(3); //to approximate match the time spent in the #if above
#endif

//            if(inst->antennaDelayChanged == 1)
//            	instancesetantennadelays(); //this will update the antenna delay if it has changed
            instancesettxpower(); //configure TX power if it has changed

       }
            break;

        case TA_TXE_WAIT : //either go to sleep or proceed to TX a message
            // printf("TA_TXE_WAIT") ;
            //if we are scheduled to go to sleep before next transmission then sleep first.
        	if((inst->nextState == TA_TXPOLL_WAIT_SEND)
                    && (inst->instToSleep)  //go to sleep before sending the next poll/ starting new ranging exchange
                    )
            {
//        		if(inst->getReportReqs == 0)
        			inst->rangeNum++; //increment the range number before going to sleep
                //the app should put chip into low power state and wake up after tagSleepTime_ms time...
                //the app could go to *_IDLE state and wait for uP to wake it up...
                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT_TO; //don't sleep here but kick off the Sleep timer countdown
                inst->testAppState = TA_SLEEP_DONE;

                {
#if (DEEP_SLEEP == 1)
                	//put device into low power mode
					dwt_entersleep(); //go to sleep
#endif

					// check which anchors are active
					uint8 i = 0;
					if(inst->rxResponseMask > 0)
					{
						// check from MSB first (A15 to A0)
						for(i=0;i<16;i++)
							if(((inst->rxResponseMask & (0x8000 >> i)) > 0))
								inst->gatewayAnchorAddress = 0x8000 | (15-i);
					}
					else
						inst->gatewayAnchorAddress = 0x8000;

					//DW1000 gone to sleep - report the received range
//					inst->newRange = instance_calcranges(&inst->tofArray[0], MAX_ANCHOR_LIST_SIZE, TOF_REPORT_T2A, &inst->rxResponseMask);
					inst->newRange = instance_calcranges(&inst->tofArray[0], inst->maxAnchorsTwr, TOF_REPORT_T2A, &inst->rxResponseMask, 0);
					inst->rxResponseMaskReport = inst->rxResponseMask;

//#if VERSION == VER_1p0
//					memcpy(&inst->TimestampTx1,&inst->tempTimestampTx1,5);
//					memcpy(&inst->TimestampRx2,&inst->tempTimestampRx2,5);
//					memcpy(&inst->TimestampTx3,&inst->tempTimestampTx3,5);
//#endif

//					// check which anchors are active
//					uint8 i = 0;
//					if(inst->rxResponseMask > 0)
//					{
//						// check from MSB first (A15 to A0)
//						for(i=0;i<16;i++)
//							if(((inst->rxResponseMask & (0x8000 >> i)) > 0))
//								inst->gatewayAnchorAddress = 0x8000 | (15-i);
//					}
//					else
//						inst->gatewayAnchorAddress = 0x8000;

					inst->rxResponseMask = 0;
					inst->newRangeTime = portGetTickCnt(); //ERWIN
                }

            }
            else //proceed to configuration and transmission of a frame
            {
                inst->testAppState = inst->nextState;
//                inst->nextState = 0; //clear
            }
            break ; // end case TA_TXE_WAIT

        case TA_TXPOLL_WAIT_SEND :
            {
//            	inst->msg_f.messageData[POLL_RNUM] = (inst->mode == TAG) ? inst->rangeNum : inst->rangeNumAnc; //copy new range number
//            	inst->msg_f.messageData[FCODE] = (inst->mode == TAG) ? RTLS_DEMO_MSG_TAG_POLL : RTLS_DEMO_MSG_ANCH_POLL; //message function code (specifies if message is a poll, response or other...)
            	inst->msg_f.messageData[POLL_RNUM] = inst->rangeNum; //copy new range number
            	inst->msg_f.messageData[FCODE] = RTLS_DEMO_MSG_TAG_POLL; //message function code (specifies if message is a poll, response or other...)
//            	inst->msg_f.messageData[STATUSIO1POLL] = inst->StatusIO1;
//            	inst->msg_f.messageData[STATUSIO2POLL] = inst->StatusIO2;
//            	inst->msg_f.messageData[STATIOSEQPOLL] = inst->StatIOseqnum;
            	inst->msg_f.messageData[WHOISGATEWAY] = inst->gatewayAnchorAddress & 0x00FF;
//            	inst->msg_f.messageData[WHOISGATEWAY+1] = inst->getReportReqs;
//            	inst->msg_f.messageData[POLL_CMDSEQNUM] = inst->commandSeqNum;
//            	inst->msg_f.messageData[POLL_CMDOPMODE] = inst->opMode;
//#if VERSION == VER_1p0
//            	memcpy(&inst->msg_f.messageData[POLL_TIMESTAMP1],(uint8*)&inst->TimestampTx1[0],5);
//            	memcpy(&inst->msg_f.messageData[POLL_TIMESTAMP2],(uint8*)&inst->TimestampRx2[0],5);
//            	memcpy(&inst->msg_f.messageData[POLL_TIMESTAMP3],(uint8*)&inst->TimestampTx3[0],5);
//#endif
            	inst->psduLength = (TAG_POLL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
                inst->msg_f.seqNum = inst->frameSN++; //copy sequence number and then increment
                inst->msg_f.sourceAddr[0] = inst->eui64[0]; //copy the address
                inst->msg_f.sourceAddr[1] = inst->eui64[1]; //copy the address
            	inst->msg_f.destAddr[0] = 0xff;  //set the destination address (broadcast == 0xffff)
            	inst->msg_f.destAddr[1] = 0xff;  //set the destination address (broadcast == 0xffff)
                dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg_f, 0) ;	// write the frame data

				//set the delayed rx on time (the response message will be sent after this delay (from A0))
				dwt_setrxaftertxdelay((uint32)RX_RESPONSE1_TURNAROUND);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)
//				dwt_setrxaftertxdelay((uint32)inst->trxResponseTrunaround);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)

//				if(inst->enableReport)
//					inst->getReportResp = 0;

				if(inst->mode == TAG)
				{
					inst->rxResps[inst->rangeNum] = 0; //reset the number of received responses
//					inst->responseTO = MAX_ANCHOR_LIST_SIZE; //expecting 4 responses
					inst->responseTO = inst->maxAnchorsTwr; //expecting 4 responses
#if VERSION == VER_1p0
					dwt_setrxtimeout((uint16)inst->fwtoTime_sy * 2);  //configure the RX FWTO
#else
//					dwt_setrxtimeout((uint16)inst->fwtoTime_sy * MAX_ANCHOR_LIST_SIZE);  //configure the RX FWTO
					dwt_setrxtimeout((uint16)inst->fwtoTime_sy * (inst->maxAnchorsTwr+1));  //configure the RX FWTO
#endif
				}
#if(ANCTOANCTWR == 1) //allow anchor to anchor ranging
				else
				{
					inst->rxResps[inst->rangeNumAnc] = 0; //reset number of responses
					inst->responseTO = NUM_EXPECTED_RESPONSES_ANC0; //2 responses A1, A2
					dwt_setrxtimeout((uint16)inst->fwtoTime_sy * (NUM_EXPECTED_RESPONSES_ANC0));  //units are
				}
#endif
				inst->rxResponseMask = 0;	//reset/clear the mask of received responses when tx poll
				inst->rxResponseMaskAnc = 0;

				inst->wait4ack = DWT_RESPONSE_EXPECTED; //response is expected - automatically enable the receiver

				dwt_writetxfctrl(inst->psduLength, 0, 0 /*ERWIN*/); //write frame control

				dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED); //transmit the frame

                inst->testAppState = TA_TX_WAIT_CONF ;  // wait confirmation
                inst->previousState = TA_TXPOLL_WAIT_SEND ;
                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set above)

            }
            break;

        case TA_TXFINAL_WAIT_SEND :
            {
            	//the final has the same range number as the poll (part of the same ranging exchange)
//                inst->msg_f.messageData[POLL_RNUM] = (inst->mode == TAG) ? inst->rangeNum : inst->rangeNumAnc;
                inst->msg_f.messageData[POLL_RNUM] = inst->rangeNum;
                //the mask is sent so the anchors know whether the response RX time is valid
//                inst->msg_f.messageData[VRESP] = (inst->mode == TAG) ? inst->rxResponseMask : inst->rxResponseMaskAnc;
//				inst->msg_f.messageData[VRESP1] = (inst->mode == TAG) ? inst->rxResponseMask: inst->rxResponseMaskAnc;
//				inst->msg_f.messageData[VRESP2] = (inst->mode == TAG) ? inst->rxResponseMask >> 8: inst->rxResponseMaskAnc >> 8;
//				inst->msg_f.messageData[STATUSIO1FINAL] = inst->StatusIO1;
//				inst->msg_f.messageData[STATUSIO2FINAL] = inst->StatusIO2;
//				inst->msg_f.messageData[STATIOSEQFINAL] = inst->StatIOseqnum;
//            	inst->msg_f.messageData[FCODE] = (inst->mode == TAG) ? RTLS_DEMO_MSG_TAG_FINAL : RTLS_DEMO_MSG_ANCH_FINAL; //message function code (specifies if message is a poll, response or other...)
            	inst->msg_f.messageData[FCODE] = RTLS_DEMO_MSG_TAG_FINAL; //message function code (specifies if message is a poll, response or other...)
//            	inst->msg_f.messageData[FINAL_CMDSEQNUM] = inst->commandSeqNum;
//            	inst->msg_f.messageData[FINAL_CMDOPMODE] = inst->opMode;
//                inst->psduLength = (TAG_FINAL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
                inst->psduLength = (inst->twrFinalMsgLen + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
                inst->msg_f.seqNum = inst->frameSN++;
				dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg_f, 0) ;	// write the frame data

//				if(inst->enableReport)
//					inst->getReportReqs = 1;
//				else
//					inst->getReport = 0;

//				if(inst->enableReport == 0)
//				{
					inst->wait4ack = 0; //clear the flag not using wait for response as this message ends the ranging exchange
//					inst->wait4ack = DWT_RESPONSE_EXPECTED;

//					dwt_setrxtimeout(0); //reconfigure the timeout
//					dwt_setrxaftertxdelay((uint32)3*RX_RESPONSE1_TURNAROUND);

					if(instancesenddlypacket(inst, DWT_START_TX_DELAYED))
					{
						// initiate the re-transmission
						if(inst->mode == TAG)
						{
							inst->testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
							inst->nextState = TA_TXPOLL_WAIT_SEND ;
						}
						else
						{
							//A0 - failed to send Final
							//A1 - failed to send Final
							//go back to RX and behave as anchor
							instance_backtoanchor(inst);
						}
						break; //exit this switch case...
					}
					else
					{

						inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
						inst->previousState = TA_TXFINAL_WAIT_SEND;

					}
					if(inst->mode == TAG)
					{
						inst->instToSleep = TRUE ;
					}
					inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set above)
//				}
//				else
//            	if(inst->enableReport)
//				{
//					if(inst->enableReport)
//						inst->getReportReqs = 1;
//					inst->rxRespsImRpt[inst->rangeNum] = 0; //reset the number of received responses
					//					inst->responseTO = MAX_ANCHOR_LIST_SIZE; //expecting 4 responses
//					inst->responseTO = (inst->maxAnchorsTwr + 2)*(inst->maxTagsTwr - 1); //expecting 4 responses
//					dwt_setrxtimeout((uint16)inst->fwtoTime_sy * inst->maxAnchorsTwr);  //configure the RX FWTO
//					led_on(LED_PC9);
//					//send a response or re-enable rx
//					dwt_setdelayedtrxtime(inst->delayedReplyTime+RX_RESPONSE1_TURNAROUND) ;
//					dwt_rxenable(DWT_START_RX_DELAYED);
//					dwt_setrxtimeout(0); //reconfigure the timeout
//					dwt_setrxaftertxdelay((uint32)3*RX_RESPONSE1_TURNAROUND);
//					dwt_rxenable(DWT_START_RX_IMMEDIATE) ;
////					dw_event->type_pend = DWT_SIG_RX_PENDING ;
//					led_off(LED_PC9);

//					inst->rxResponseMaskImRpt = 0;	//reset/clear the mask of received responses when tx poll
//					inst->newRangeImRpt = 0;
//					inst->rxResponseMaskAnc = 0;

//					inst->wait4ack = DWT_RESPONSE_EXPECTED; //response is expected - automatically enable the receiver
//
//					if(instancesenddlypacket(inst, DWT_START_TX_DELAYED))
//					{
//						// initiate the re-transmission
//						if(inst->mode == TAG)
//						{
//							inst->testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
//							inst->nextState = TA_TXPOLL_WAIT_SEND ;
//						}
//						else
//						{
//							//A0 - failed to send Final
//							//A1 - failed to send Final
//							//go back to RX and behave as anchor
//							instance_backtoanchor(inst);
//						}
//						break; //exit this switch case...
//					}
//					else
//					{
//
//						inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
//						inst->previousState = TA_TXFINAL_WAIT_SEND;
//
//					}
//					if(inst->mode == TAG)
//					{
//						inst->instToSleep = FALSE ;
//					}
//					inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set above)
//				}
            }
            break;

//        case TWR_REPORT_BCAST :
//        	{
//        		inst->msg_f.messageData[FCODE] = RTLS_DEMO_MSG_REPORT_BCAST; //message function code (specifies if message is a poll, response or other...)
//        		inst->msg_f.messageData[TWR_REPORT_TAGID] = inst->TagIdBlinkArray[inst->idxBlinkInBuff]; 				// Tag id to blink
//        		memcpy(&inst->msg_f.messageData[TWR_REPORT_SEQNUM], &inst->rangeNumA[inst->idxBlinkInBuff], 2); // Blink seq num for that tag
//        		inst->msg_f.messageData[SYNC_REP_TAGID] = inst->TagIdReportArray[inst->idxReportInBuff]; 				// Tag id to be reported
//        		memcpy(&inst->msg_f.messageData[SYNC_REP_SEQNUM], &inst->SeqNumReport[inst->idxReportInBuff], 2); 	// Report seq num for that tag
//        		memcpy(&inst->msg_f.messageData[SYNC_REP_POS_X], &inst->ReportPosX[inst->idxReportInBuff], sizeof(float)); 	// Report position X
//        		memcpy(&inst->msg_f.messageData[SYNC_REP_POS_Y], &inst->ReportPosY[inst->idxReportInBuff], sizeof(float)); 	// Report position Y
//        		memcpy(&inst->msg_f.messageData[SYNC_REP_POS_Z], &inst->ReportPosZ[inst->idxReportInBuff], sizeof(float)); 	// Report position Z
//        		inst->msg_f.messageData[SYNC_REP_VALID] = inst->ReportValid[inst->idxReportInBuff]; 				// if 0 then it's not valid
//
//        		inst->psduLength = (ANCH_SYNC_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
//        		inst->msg_f.seqNum = inst->frameSN++; //copy sequence number and then increment
//        		inst->msg_f.sourceAddr[0] = inst->eui64[0]; //copy the address
//        		inst->msg_f.sourceAddr[1] = inst->eui64[1]; //copy the address
//        		inst->msg_f.destAddr[0] = 0xff;  //set the destination address (broadcast == 0xffff)
//        		inst->msg_f.destAddr[1] = 0xff;  //set the destination address (broadcast == 0xffff)
//        		dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg_f, 0) ;	// write the frame data
//
//        		//set the delayed rx on time (the response message will be sent after this delay (from A0))
//        		dwt_setrxaftertxdelay(0);
//        		dwt_setrxtimeout((uint16)inst->fwtoTime_sy);  //configure the RX FWTO
//        		inst->wait4ack = DWT_RESPONSE_EXPECTED; //response is expected - automatically enable the receiver
//
//        		dwt_writetxfctrl(inst->psduLength, 0); //write frame control
//
//        		dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED); //transmit the frame
//
//        		inst->testAppState = TA_TX_WAIT_CONF ;  // wait confirmation
//        		inst->previousState = TOA_SYNC_BCAST ;
//        		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set above)
//
////        		uint8 kk = 0;
////        		UART_txbuff[kk++] = 0;
////        		UART_txbuff[kk++] = 0xE8;
////        		UART_txbuff[kk++] = '~';
////        		UART_txbuff[kk++] = inst->TagIdBlinkArray[inst->idxBlinkInBuff];
////        		UART_txbuff[kk++] = (inst->SeqNumBlink[inst->idxBlinkInBuff] >> 8);
////        		UART_txbuff[kk++] = inst->SeqNumBlink[inst->idxBlinkInBuff];
////        		UART_txbuff[kk++] = '\r';
////        		UART_txbuff[kk++] = '\n';
////
////        		send_uart_txdata(kk);
//
//        	}
//        	break;


        case TA_TX_WAIT_CONF :
		   //printf("TA_TX_WAIT_CONF %d m%d %d states %08x %08x\n", inst->previousState, message, inst->newReportSent, dwt_read32bitreg(0x19), dwt_read32bitreg(0x0f)) ;

                {
				event_data_t* dw_event = instance_getevent(11); //get and clear this event

                //NOTE: Can get the ACK before the TX confirm event for the frame requesting the ACK
                //this happens because if polling the ISR the RX event will be processed 1st and then the TX event
                //thus the reception of the ACK will be processed before the TX confirmation of the frame that requested it.
				if(dw_event->type != DWT_SIG_TX_DONE) //wait for TX done confirmation
                {
					if(dw_event->type != 0)
					{
						if(dw_event->type == DWT_SIG_RX_TIMEOUT) //got RX timeout - i.e. did not get the response (e.g. ACK)
						{
							//printf("RX timeout in TA_TX_WAIT_CONF (%d)\n", inst->previousState);
							//we need to wait for SIG_TX_DONE and then process the timeout and re-send the frame if needed
							inst->gotTO = 1;
						}
						else
						{
							inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
						}
					}

                    inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                        break;

                }

                inst->done = INST_NOT_DONE_YET;

                if(inst->previousState == TA_TXFINAL_WAIT_SEND)
                {
                    if(inst->mode == TAG)
                    {
//                    	if(inst->enableReport)
//                    	{
//                    		inst->testAppState = TA_RXE_WAIT ;
//                    		inst->nextState = TA_TXPOLL_WAIT_SEND ;
//                    		break;
//                    	}
//                    	else
//                    	{
                    		inst->testAppState = TA_TXE_WAIT ;
                    		inst->nextState = TA_TXPOLL_WAIT_SEND ;
                    		break;
//                    	}
                    }
                    else
                    {
                    	instance_backtoanchor(inst);
					}
                }
                else if (inst->gotTO == 1) //timeout
                {
					//printf("got TO in TA_TX_WAIT_CONF\n");
                    inst_processrxtimeout(inst);
                    inst->gotTO = 0;
					inst->wait4ack = 0 ; //clear this
					break;
                }
                else
                {
//					inst->txu.txTimeStamp = dw_event->timeStamp;


					if(inst->previousState == TA_TXPOLL_WAIT_SEND)
					{
//		                uint64 tagCalculatedFinalTxTime ;

		                // Embed into Final message: 40-bit pollTXTime,  40-bit respRxTime,  40-bit finalTxTime
		                if(inst->mode == TAG)
		                {
		                	inst->tagCalculatedFinalTxTime =  (inst->txu.txTimeStamp + inst->pollTx2FinalTxDelay) & MASK_TXDTS;
		                }
		                else //for anchor make the final half the delay ..... (this is ok, as A0 awaits 2 responses)
		                {
		                	inst->tagCalculatedFinalTxTime =  (inst->txu.txTimeStamp + inst->pollTx2FinalTxDelayAnc) & MASK_TXDTS;
		                }
		                inst->delayedReplyTime = inst->tagCalculatedFinalTxTime >> 8; //high 32-bits
		                // Calculate Time Final message will be sent and write this field of Final message
		                // Sending time will be delayedReplyTime, snapped to ~125MHz or ~250MHz boundary by
		                // zeroing its low 9 bits, and then having the TX antenna delay added
		                // getting antenna delay from the device and add it to the Calculated TX Time
		                inst->tagCalculatedFinalTxTime = inst->tagCalculatedFinalTxTime + inst->txAntennaDelay;
		                inst->tagCalculatedFinalTxTime &= MASK_40BIT;

		                // Write Calculated TX time field of Final message
//						memcpy(&(inst->msg_f.messageData[FTXT]), (uint8 *)&inst->tagCalculatedFinalTxTime, 5);
						memcpy(&(inst->msg_f.messageData[inst->finalTxTimeIdx]), (uint8 *)&inst->tagCalculatedFinalTxTime, 5);
		                // Write Poll TX time field of Final message
						memcpy(&(inst->msg_f.messageData[PTXT]), (uint8 *)&inst->txu.tagPollTxTime, 5);
#if VERSION == VER_1p0
						inst->tempTimestampTx1 = inst->txu.tagPollTxTime;
						inst->tempTimestampTx3 = inst->tagCalculatedFinalTxTime;
#endif
						//change the w4r for the second and remaining anchors to 50 us
						//dwt_setrxaftertxdelay((uint32)RX_RESPONSEX_TURNAROUND);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)
					}

		            if(inst->previousState == TA_TXRESPONSE_SENT_TORX)
		            {
		            	inst->previousState = TA_TXRESPONSE_WAIT_SEND ;
		            	if(inst->mode == ANCHOR)
		            	{
		            		instance_backtoanchor(inst);
		            		dwt_rxenable(DWT_START_RX_IMMEDIATE);
		            	}
		            }
                    inst->testAppState = TA_RXE_WAIT ;                      // After sending, tag expects response/report, anchor waits to receive a final/new poll

                    message = 0;
                    //fall into the next case (turn on the RX)
                }

            } break;

            //break ; // end case TA_TX_WAIT_CONF


        case TA_RXE_WAIT :
        // printf("TA_RXE_WAIT") ;
        {

            if(inst->wait4ack == 0) //if this is set the RX will turn on automatically after TX
            {
                //turn RX on
            	dwt_rxenable(DWT_START_RX_IMMEDIATE) ;  // turn RX on, without delay
            }
            else
            {
                inst->wait4ack = 0 ; //clear the flag, the next time we want to turn the RX on it might not be auto
            }

            if (inst->mode != LISTENER)
            {
            	inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //using RX FWTO
            }

            inst->testAppState = TA_RX_WAIT_DATA;   // let this state handle it

            // end case TA_RXE_WAIT, don't break, but fall through into the TA_RX_WAIT_DATA state to process it immediately.
            if(message == 0) break;
        } break;

        case TA_RX_WAIT_DATA :                                                                     // Wait RX data
		   //printf("TA_RX_WAIT_DATA %d", message) ;

            switch (message)
            {

				//if we have received a DWT_SIG_RX_OKAY event - this means that the message is IEEE data type - need to check frame control to know which addressing mode is used
                case DWT_SIG_RX_OKAY :
                {
					event_data_t* dw_event = instance_getevent(15); //get and clear this event
					uint8  srcAddr[8] = {0,0,0,0,0,0,0,0};
					uint8  dstAddr[8] = {0,0,0,0,0,0,0,0};
                    int fcode = 0;
					int fn_code = 0;
					//int srclen = 0;
					//int fctrladdr_len;
					uint8 tof_idx  = 0;
					uint8 *messageData;
					uint16 srcAddr16;
					uint16 dstAddr16;

					inst->stopTimer = 0; //clear the flag, as we have received a message

                    // handle 16 and 64 bit source and destination addresses
					switch(dw_event->msgu.frame[1] & 0xCC)
					{
						case 0xCC: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ll.sourceAddr[0]), ADDR_BYTE_SIZE_L);
							memcpy(&dstAddr[0], &(dw_event->msgu.rxmsg_ll.destAddr[0]), ADDR_BYTE_SIZE_L);
							fn_code = dw_event->msgu.rxmsg_ll.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ll.messageData[0];
							//srclen = ADDR_BYTE_SIZE_L;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_L;
							break;
						case 0xC8: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_sl.sourceAddr[0]), ADDR_BYTE_SIZE_L);
							memcpy(&dstAddr[0], &(dw_event->msgu.rxmsg_sl.destAddr[0]), ADDR_BYTE_SIZE_S);
							fn_code = dw_event->msgu.rxmsg_sl.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_sl.messageData[0];
							//srclen = ADDR_BYTE_SIZE_L;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_LS;
							break;
						case 0x8C: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ls.sourceAddr[0]), ADDR_BYTE_SIZE_S);
							memcpy(&dstAddr[0], &(dw_event->msgu.rxmsg_ls.destAddr[0]), ADDR_BYTE_SIZE_L);
							fn_code = dw_event->msgu.rxmsg_ls.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ls.messageData[0];
							//srclen = ADDR_BYTE_SIZE_S;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_LS;
							break;
						case 0x88: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ss.sourceAddr[0]), ADDR_BYTE_SIZE_S);
							memcpy(&dstAddr[0], &(dw_event->msgu.rxmsg_ss.destAddr[0]), ADDR_BYTE_SIZE_S);
							fn_code = dw_event->msgu.rxmsg_ss.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ss.messageData[0];
							//srclen = ADDR_BYTE_SIZE_S;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_S;
							break;
					}
					srcAddr16 = srcAddr[0] | ((uint16)srcAddr[1] << 8);
					dstAddr16 = dstAddr[0] | ((uint16)dstAddr[1] << 8);

					// read diagnostics data
//#if (READ_DIAGNOSTICS) == 1
//					dwt_rxdiag_t diagn;
//					//uint8 aaa = 0;
//					dwt_readdignostics(&diagn);
//
//					//if(srcAddr[0] > 7)
//					//	aaa = 1;
//
//					if(inst->mode == TAG)
//					{
//						memcpy(&(inst->Adiags_data[srcAddr[0] & 0xFF]), &diagn, sizeof(diagn));
//					}
//					else
//					{
//						memcpy(&(inst->Tdiags_data[srcAddr[0] & 0xFF]), &diagn, sizeof(diagn));
//					}
//#endif
					if((inst->instToSleep == FALSE) && (inst->mode == LISTENER))//update received data, and go back to receiving frames
					{
						//do something with message data (e.g. could extract any ToFs and print them)
						inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
						dwt_setrxaftertxdelay(0);
					}
					else
                    {
						//process ranging messages
						fcode = fn_code;
//						tof_idx = srcAddr[0] & 0x3 ;
						tof_idx = srcAddr[0] & 0xFF ;

                        switch(fcode)
                        {

                            case RTLS_DEMO_MSG_ANCH_POLL:
                            case RTLS_DEMO_MSG_TAG_POLL:
                            {
            					inst->tagPollRxTime = dw_event->timeStamp ; //save Poll's Rx time
								if(fcode == RTLS_DEMO_MSG_TAG_POLL) //got poll from Tag
								{
									inst->rangeNumA[srcAddr[0] & 0xFF] = messageData[POLL_RNUM]; //when anchor receives a poll, we need to remember the new range number
								}
								else //got poll from Anchor (initiator)
								{
									inst->rangeNumAAnc[tof_idx] = messageData[POLL_RNUM]; //when anchor receives poll from another anchor - save the range number
								}

								/*if (A1_ANCHOR_ADDR == inst->instanceAddress16) //this is A1
                                {
                                	if(GATEWAY_ANCHOR_ADDR == (srcAddr[0] | ((uint32)(srcAddr[1] << 8)))) //poll is from A0
                                	{
                                		//configure the time A1 will poll A2 (it should be in half slot time from now)
										inst->a1SlotTime = dw_event->uTimeStamp + (inst->slotPeriod);

										//inst->instanceTimerEn = 1; - THIS IS ENABLED BELOW AFTER FINAL
										// - means that if final is not received then A1 will not range to A2
                                	}
                                }*/

								//the response has been sent - await TX done event
                                if(dw_event->type_pend == DWT_SIG_TX_PENDING)
                                {
                                    inst->testAppState = TA_TX_WAIT_CONF;                // wait confirmation
                                    inst->previousState = TA_TXRESPONSE_SENT_POLLRX ;    //wait for TX confirmation of sent response
                                }
                                //already re-enabled the receiver
                                else if (dw_event->type_pend == DWT_SIG_RX_PENDING)
                                {
                                	//stay in RX wait for next frame...
                                	//RX is already enabled...
                                	inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                                }
                                else //the DW1000 is idle (re-enable from the application level)
                                {
                                	//stay in RX wait for next frame...
                                	inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
                                }

//                                // get command
//                                if(inst->commandSeqNum < messageData[POLL_CMDSEQNUM] || \
//                                		(inst->commandSeqNum > 250 && messageData[POLL_CMDSEQNUM] < 5))
//                                {
//                                	inst->commandSeqNum = messageData[POLL_CMDSEQNUM];
//                                	inst->opMode = messageData[POLL_CMDOPMODE];
//                                	UpdateSettings = 1;
//                                }

                                // get the status IO message
//                                inst->StatusIO1 = messageData[STATUSIO1POLL];
//                                inst->StatusIO2 = messageData[STATUSIO2POLL];
//                                inst->StatIOseqnum = messageData[STATIOSEQPOLL];
//
//                                usbVCOMout[0] = '$';
//                                usbVCOMout[1] = messageData[STATUSIO1POLL];
//                                usbVCOMout[2] = messageData[STATUSIO2POLL];
//                                usbVCOMout[3] = messageData[STATIOSEQPOLL];
//                                usbVCOMout[4] = '\r';
//                                usbVCOMout[5] = '\n';
//                                send_usbmessage(&usbVCOMout[0], 6);
//                                usb_run();


                            }
                            break; //RTLS_DEMO_MSG_TAG_POLL

                            case RTLS_DEMO_MSG_ANCH_RESP2:
                            case RTLS_DEMO_MSG_ANCH_RESP:
                            {
                            	uint8 currentRangeNum = (messageData[TOFRN] + 1); //current = previous + 1

//                            	// get command
//                            	if(inst->commandSeqNum < messageData[RESP_CMDSEQNUM] || \
//                            			(inst->commandSeqNum > 250 && messageData[RESP_CMDSEQNUM] < 5))
//                            	{
//                            		inst->commandSeqNum = messageData[RESP_CMDSEQNUM];
//                            		inst->opMode = messageData[RESP_CMDOPMODE];
//                            		UpdateSettings = 1;
//                            	}

//                            	if(GATEWAY_ANCHOR_ADDR == (srcAddr[0] | ((uint32)(srcAddr[1] << 8)))) //if response from gateway then use the correction factor
								if(inst->gatewayAnchorAddress == (srcAddr[0] | ((uint32)(srcAddr[1] << 8)))) //if response from gateway then use the correction factor
                            	{
									if(inst->mode == TAG && dstAddr16 == inst->instanceAddress16)
									{
										// casting received bytes to int because this is a signed correction -0.5 periods to +1.5 periods
										inst->tagSleepCorrection = (int16) (((uint16) messageData[RES_TAG_SLP1] << 8) + messageData[RES_TAG_SLP0]);
										inst->tagSleepRnd = 0; // once we have initial response from Anchor #0 the slot correction acts and we don't need this anymore
									}
								}

                            	//the response has been sent - await TX done event
                            	if(dw_event->type_pend == DWT_SIG_TX_PENDING) //anchor received response from anchor ID - 1 so is sending it's response now back to tag
								{
		                			inst->testAppState = TA_TX_WAIT_CONF;                // wait confirmation
		                			inst->previousState = TA_TXRESPONSE_SENT_RESPRX ;    //wait for TX confirmation of sent response
								}
                            	//already re-enabled the receiver
                            	else if(dw_event->type_pend == DWT_SIG_RX_PENDING)
								{
									// stay in TA_RX_WAIT_DATA - receiver is already enabled.
								}
                            	//DW1000 idle - send the final
								else //if(dw_event->type_pend == DWT_SIG_DW_IDLE)
								{
//									if(((TAG == inst->mode) && (inst->rxResponseMask & 0x0001)) //if A0's response received send the final
//											|| ((A1_ANCHOR_ADDR == inst->instanceAddress16) && (inst->rxResponseMaskAnc & 0x4))
//											|| ((GATEWAY_ANCHOR_ADDR == inst->instanceAddress16) && (inst->rxResponseMaskAnc & 0x2)) ) //if A1's response received
									if((TAG == inst->mode) && (inst->rxResponseMask > 0) && (dstAddr16 == inst->instanceAddress16))
									{
										inst->testAppState = TA_TXFINAL_WAIT_SEND ; // send our response / the final
									}
									else //go to sleep
									{
										if(TAG == inst->mode)
										{
											inst->testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
											inst->nextState = TA_TXPOLL_WAIT_SEND ;
											inst->instToSleep = TRUE;
										}
										else
										{
											instance_backtoanchor(inst);
										}
									}
								}
								/*else
								{
                                	//stay in RX wait for next frame...
                                	inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
								}*/

								if(fcode == RTLS_DEMO_MSG_ANCH_RESP) //tag to anchor mode
								{
									/*if(currentRangeNum == inst->rangeNum) //these are the previous ranges...
									{
										//copy the ToF and put into array (array holds last 4 ToFs)
										memcpy(&inst->tofArray[(srcAddr[0]&0x3)], &(messageData[TOFR]), 4);

										//check if the ToF is valid, this makes sure we only report valid ToFs
										//e.g. consider the case of reception of response from anchor a1 (we are anchor a2)
										//if a1 got a Poll with previous Range number but got no Final, then the response will have
										//the correct range number but the range will be INVALID_TOF
										if(inst->tofArray[(srcAddr[0]&0x3)] != INVALID_TOF)
										{
											inst->rxResponseMask |= (0x1 << (srcAddr[0]&0x3));
										}

									}
									else
									{
										if(inst->tofArray[(srcAddr[0]&0x3)] != INVALID_TOF)
										{
											inst->tofArray[(srcAddr[0]&0x3)] = INVALID_TOF;
										}
									}*/

									if(currentRangeNum == inst->rangeNum) //these are the previous ranges...
									{
										if(inst->mode == ANCHOR || (inst->mode == TAG && dstAddr16 == inst->instanceAddress16))
										{
											//copy the ToF and put into array (array holds last 4 ToFs)
											memcpy(&inst->tofArray[(srcAddr[0] & 0xFF)], &(messageData[TOFR]), 4);

											//check if the ToF is valid, this makes sure we only report valid ToFs
											//e.g. consider the case of reception of response from anchor a1 (we are anchor a2)
											//if a1 got a Poll with previous Range number but got no Final, then the response will have
											//the correct range number but the range will be INVALID_TOF
											if(inst->tofArray[(srcAddr[0] & 0xFF)] != INVALID_TOF)
											{
												inst->rxResponseMask |= (0x0001 << (srcAddr[0] & 0xFF));
											}
//											if(inst->enableReport)
//												inst->getReportResp |= messageData[RES_TAG_SLP1+1];
#if VERSION == VER_1p0
//											inst->TimestampTx1 = 0;
//											inst->TimestampRx2 = 0;
//											inst->TimestampTx3 = 0;
//
//											inst->TimestampRx1 = 0;
//											inst->TimestampTx2 = 0;
//											inst->TimestampRx3 = 0;

											for(uint8 ii=0;ii<9;ii++)
											{
												inst->TimestampTx1[9-ii] = inst->TimestampTx1[8-ii];
												inst->TimestampTx2[9-ii] = inst->TimestampTx2[8-ii];
												inst->TimestampTx3[9-ii] = inst->TimestampTx3[8-ii];
												inst->TimestampRx1[9-ii] = inst->TimestampRx1[8-ii];
												inst->TimestampRx2[9-ii] = inst->TimestampRx2[8-ii];
												inst->TimestampRx3[9-ii] = inst->TimestampRx3[8-ii];
											}

											memcpy(&inst->TimestampRx1[0],&messageData[RESP_TIMESTAMP1],5);
											memcpy(&inst->TimestampTx2[0],&messageData[RESP_TIMESTAMP2],5);
											memcpy(&inst->TimestampRx3[0],&messageData[RESP_TIMESTAMP3],5);

											inst->TimestampTx1[0] = inst->tempTimestampTx1 & MASK_40BIT;
											inst->TimestampRx2[0] = inst->tempTimestampRx2 & MASK_40BIT;
											inst->TimestampTx3[0] = inst->tempTimestampTx3 & MASK_40BIT;
#endif
										}
									}
									else
									{
										if(inst->tofArray[(srcAddr[0] & 0xFF)] != INVALID_TOF)
										{
											inst->tofArray[(srcAddr[0] & 0xFF)] = INVALID_TOF;
										}
									}


								}
								/*else //anchor to anchor (only gateway processes anchor to anchor ToFs)
								{
									//report the correct set of ranges (ranges from anchors A1, A2 need to match owns range number)
									if((inst->gatewayAnchor)&&(currentRangeNum == inst->rangeNumAnc)) //these are the previous ranges...
									{
										inst->rangeNumAAnc[0] = inst->rangeNumAnc ;

										//once A0 receives A2's response then it can report the 3 ToFs.
										if(inst->rxResps[inst->rangeNumAnc] == 3)
										//if(A2_ANCHOR_ADDR == (srcAddr[0] | ((uint32)(srcAddr[1] << 8))))
										{
											//copy the ToF and put into array, the array should have 3 ToFs A0-A1, A0-A2 and A1-A2
											memcpy(&inst->tofArrayAnc[(srcAddr[0]+dstAddr[0])&0x3], &(messageData[TOFR]), 4);
											//calculate all anchor - anchor ranges... and report
											inst->newRange = instance_calcranges(&inst->tofArrayAnc[0], MAX_ANCHOR_LIST_SIZE, TOF_REPORT_A2A, &inst->rxResponseMaskAnc);
											inst->rxResponseMaskReport = inst->rxResponseMaskAnc;
											inst->rxResponseMaskAnc = 0;
											inst->newRangeTime = dw_event->uTimeStamp ;
										}
										else
										{
											//copy the ToF and put into array (array holds last 4 ToFs)
											memcpy(&inst->tofArrayAnc[(srcAddr[0]+dstAddr[0])&0x3], &(messageData[TOFR]), 4);
										}
									}
								}*/

                            }
                            break; //RTLS_DEMO_MSG_ANCH_RESP


                            case RTLS_DEMO_MSG_ANCH_FINAL:
                            case RTLS_DEMO_MSG_TAG_FINAL:
                            {
                                int64 Rb, Da, Ra, Db ;
                                uint64 tagFinalTxTime  = 0;
                                uint64 tagFinalRxTime  = 0;
                                uint64 tagPollTxTime  = 0;
                                uint64 anchorRespRxTime  = 0;
                                uint64 tof = INVALID_TOF;

                                double RaRbxDaDb = 0;
								double RbyDb = 0;
								double RayDa = 0;

//								inst->TimestampTx1 = 0;
//								inst->TimestampRx2 = 0;
//								inst->TimestampTx3 = 0;
//
//								inst->TimestampRx1 = 0;
//								inst->TimestampTx2 = 0;
//								inst->TimestampRx3 = 0;

//								uint8 validResp = messageData[VRESP];
//								uint16 validResp = messageData[VRESP1] | (uint16)(messageData[VRESP2] << 8);
								uint16 validResp = messageData[inst->finalTxTimeIdx+5] | \
										(uint16)(messageData[inst->finalTxTimeIdx+6] << 8);
								uint8 index = RRXT0 + 5*(inst->shortAdd_idx);

//								// get command
//								if(inst->commandSeqNum < messageData[FINAL_CMDSEQNUM] || \
//										(inst->commandSeqNum > 250 && messageData[FINAL_CMDSEQNUM] < 5))
//								{
//									inst->commandSeqNum = messageData[FINAL_CMDSEQNUM];
//									inst->opMode = messageData[FINAL_CMDOPMODE];
//									UpdateSettings = 1;
//								}

								// get the status IO message
//								inst->StatusIO1 = messageData[STATUSIO1FINAL];
//								inst->StatusIO2 = messageData[STATUSIO2FINAL];
//								inst->StatIOseqnum = messageData[STATIOSEQFINAL];

//								if(ANCHOR_SYNC == inst->mode)
//								{
//									inst->testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
//									inst->nextState = TWR_SYNC_BCAST ;
//									inst->instToSleep = TRUE;
//									//										inst->instToSleep = FALSE;
//								}
//								else
								if((RTLS_DEMO_MSG_TAG_FINAL == fcode) &&
										(inst->rangeNumA[srcAddr[0] & 0xFF] != messageData[POLL_RNUM])) //Final's range number needs to match Poll's or else discard this message
								{
                                    inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
                                    break;
								}

								/*if((RTLS_DEMO_MSG_ANCH_FINAL == fcode) &&
										(((inst->rangeNumAAnc[tof_idx] != messageData[POLL_RNUM]) //Final's range number needs to match Poll's or else discard this message
										|| inst->gatewayAnchor) //gateway can ignore the Final (from A1 to A2 exchange)
										|| (A3_ANCHOR_ADDR == inst->instanceAddress16))) //A3 does not care about Final from A1 or A0
								{
                                    inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
                                    break;
								}*/

								/*if (A1_ANCHOR_ADDR == inst->instanceAddress16) //this is A1
                                {
                                	if(GATEWAY_ANCHOR_ADDR == (srcAddr[0] | ((uint32)(srcAddr[1] << 8)))) //final is from A0
                                	{
                                		//ENABLE TIMER ONLY IF FINAL RECEIVED
										inst->instanceTimerEn = 1;
                                	}
                                }*/
            					//output data over USB...
                                inst->newRangeAncAddress = inst->instanceAddress16;

								//if we got the final, maybe the tag did not get our response, so
								//we can use other anchors responses/ToF if there are any.. and output..
								//but we cannot calculate new range
								if(((validResp & (0x0001<<(inst->shortAdd_idx))) != 0))
								{
									// time of arrival of Final message
									tagFinalRxTime = dw_event->timeStamp ; //Final's Rx time

									//printf("FinalRx Timestamp: %4.15e\n", convertdevicetimetosecu(dw_event.timeStamp));
									inst->delayedReplyTime = 0 ;

									// times measured at Tag extracted from the message buffer
									// extract 40bit times
									memcpy(&tagPollTxTime, &(messageData[PTXT]), 5);
									memcpy(&anchorRespRxTime, &(messageData[index]), 5);
//									memcpy(&tagFinalTxTime, &(messageData[FTXT]), 5);
									memcpy(&tagFinalTxTime, &(messageData[inst->finalTxTimeIdx]), 5);
#if VERSION == VER_1p0
									for(uint8 ii=0;ii<9;ii++)
									{
										inst->TimestampTx1[9-ii] = inst->TimestampTx1[8-ii];
										inst->TimestampTx2[9-ii] = inst->TimestampTx2[8-ii];
										inst->TimestampTx3[9-ii] = inst->TimestampTx3[8-ii];
										inst->TimestampRx1[9-ii] = inst->TimestampRx1[8-ii];
										inst->TimestampRx2[9-ii] = inst->TimestampRx2[8-ii];
										inst->TimestampRx3[9-ii] = inst->TimestampRx3[8-ii];
									}
#endif
									inst->TimestampTx1[0] = tagPollTxTime;
									inst->TimestampRx2[0] = anchorRespRxTime;
									inst->TimestampTx3[0] = tagFinalTxTime;

									inst->TimestampRx1[0] = inst->tagPollRxTime & MASK_40BIT;
									inst->TimestampTx2[0] = inst->txu.anchorRespTxTime & MASK_40BIT;
									inst->TimestampRx3[0] = tagFinalRxTime & MASK_40BIT;
//#endif
//									memcpy(&inst->TimestampRx1[0],(uint8 *)&(inst->tagPollRxTime),5);
//									memcpy(&inst->TimestampTx2[0],(uint8 *)&(inst->txu.anchorRespTxTime),5);
//									memcpy(&inst->TimestampRx3[0],(uint8 *)&(tagFinalRxTime),5);

									// poll response round trip delay time is calculated as
									// (anchorRespRxTime - tagPollTxTime) - (anchorRespTxTime - tagPollRxTime)
									Ra = (int64)((anchorRespRxTime - tagPollTxTime) & MASK_40BIT);
									Db = (int64)((inst->txu.anchorRespTxTime - inst->tagPollRxTime) & MASK_40BIT);

									// response final round trip delay time is calculated as
									// (tagFinalRxTime - anchorRespTxTime) - (tagFinalTxTime - anchorRespRxTime)
									Rb = (int64)((tagFinalRxTime - inst->txu.anchorRespTxTime) & MASK_40BIT);
									Da = (int64)((tagFinalTxTime - anchorRespRxTime) & MASK_40BIT);

									RaRbxDaDb = (((double)Ra))*(((double)Rb))
									- (((double)Da))*(((double)Db));

									RbyDb = ((double)Rb + (double)Db);

									RayDa = ((double)Ra + (double)Da);

									tof = (int32) ( RaRbxDaDb/(RbyDb + RayDa) );
								}

								//tag to anchor ranging
								if(RTLS_DEMO_MSG_TAG_FINAL == fcode)
								{
									inst->newRangeTagAddress = srcAddr[0] + ((uint16) srcAddr[1] << 8);
									//time-of-flight
									inst->tof[inst->newRangeTagAddress & 0xFF] = tof;
									//calculate all tag - anchor ranges... and report
//									inst->newRange = instance_calcranges(&inst->tofArray[0], MAX_ANCHOR_LIST_SIZE, TOF_REPORT_T2A, &inst->rxResponseMask);
									inst->newRange = instance_calcranges(&inst->tofArray[0], inst->maxAnchorsTwr, TOF_REPORT_T2A, &inst->rxResponseMask, 0);
									inst->rxResponseMaskReport = inst->rxResponseMask; //copy the valid mask to report
									inst->rxResponseMask = 0;
									//we have our range - update the own mask entry...
									if(tof != INVALID_TOF) //check the last ToF entry is valid and copy into the current array
									{
										setTagDist(srcAddr[0], inst->shortAdd_idx); //copy distance from this anchor to the tag into array

										inst->rxResponseMask = (0x0001 << inst->shortAdd_idx);
										inst->tofArray[inst->shortAdd_idx] = tof;
									}
									inst->newRangeTime = dw_event->uTimeStamp ;
								}
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
								else //anchor to anchor ranging
								{
									inst->newRangeTagAddress = srcAddr[0] + ((uint16) srcAddr[1] << 8);
									//time-of-flight
									inst->tofAnc[tof_idx] = tof;
								}
#endif
					            //reset the response count
					            if(inst->rxResps[inst->rxRespsIdx] >= 0)
					            {
					            	inst->rxResps[inst->rxRespsIdx] = -1 * inst->rxResps[inst->rxRespsIdx];
					            	if(inst->rxResps[inst->rxRespsIdx] == 0) //as A0 will have this as 0 when ranging to A1
					            		inst->rxResps[inst->rxRespsIdx] = -1 ;
					            }

//					            if(inst->antennaDelayChanged == 1)
//					            	instancesetantennadelays(); //this will update the antenna delay if it has changed
					            instancesettxpower(); // configure TX power if it has changed

					            if (inst->mode == ANCHOR)
					            {
//					            	if (inst->enableReport)
//					            	{
//					            		inst->testAppState = TWR_REPORT_BCAST;
//					            		// update reply delay
//					            		inst->delayedReplyTime = dw_event->timeStamp32h;
//					            		inst->delayedReplyTime += 2*(inst->fixedReplyDelayAnc >> 8);
//					            		uint32 temp = inst->shortAdd_idx*(inst->fixedReplyDelayAnc >> 8);
//					            		inst->delayedReplyTime += temp;
//					            	}
//					            	else
					            		inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
					            }

//					            usbVCOMout[0] = '$';
//					            usbVCOMout[1] = messageData[STATUSIO1FINAL];
//					            usbVCOMout[2] = messageData[STATUSIO2FINAL];
//					            usbVCOMout[3] = messageData[STATIOSEQFINAL];
//					            usbVCOMout[4] = '\r';
//					            usbVCOMout[5] = '\n';
//					            send_usbmessage(&usbVCOMout[0], 6);
//					            usb_run();

//#if VERSION == VER_1p0
//					            memcpy(&inst->TimestampRx1,&inst->tempTimestampRx1,5);
//					            memcpy(&inst->TimestampTx2,&inst->tempTimestampTx2,5);
//					            memcpy(&inst->TimestampRx3,&inst->tempTimestampRx3,5);
//#endif

                            }
//                            	inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
                            break; //RTLS_DEMO_MSG_TAG_FINAL

//                            case RTLS_DEMO_MSG_REPORT_BCAST:
//                            {
//                            	uint8 currentRangeNum = (messageData[TOFRN]);
//
////                            	if(inst->gatewayAnchorAddress == (srcAddr[0] | ((uint32)(srcAddr[1] << 8)))) //if response from gateway then use the correction factor
////                            	{
////                            		if(inst->mode == TAG)
////                            		{
////                            			// casting received bytes to int because this is a signed correction -0.5 periods to +1.5 periods
////                            			inst->tagSleepCorrection = (int16) (((uint16) messageData[RES_TAG_SLP1] << 8) + messageData[RES_TAG_SLP0]);
////                            			inst->tagSleepRnd = 0; // once we have initial response from Anchor #0 the slot correction acts and we don't need this anymore
////                            		}
////                            	}
//
//                            	//the response has been sent - await TX done event
//                            	if(dw_event->type_pend == DWT_SIG_TX_PENDING) //anchor received response from anchor ID - 1 so is sending it's response now back to tag
//                            	{
//                            		inst->testAppState = TA_TX_WAIT_CONF;                // wait confirmation
//                            		inst->previousState = TA_TXRESPONSE_SENT_RESPRX ;    //wait for TX confirmation of sent response
//                            	}
//                            	//already re-enabled the receiver
//                            	else if(dw_event->type_pend == DWT_SIG_RX_PENDING)
//                            	{
//                            		// stay in TA_RX_WAIT_DATA - receiver is already enabled.
//                            	}
//                            	//DW1000 idle - send the final
//                            	else //if(dw_event->type_pend == DWT_SIG_DW_IDLE)
//                            	{
////                            		if((TAG == inst->mode) && (inst->rxResponseMask > 0))
////                            		{
////                            			inst->testAppState = TA_TXFINAL_WAIT_SEND ; // send our response / the final
////                            		}
////                            		else //go to sleep
////                            		{
//                            			if(TAG == inst->mode)
//                            			{
//                            				inst->testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
//                            				inst->nextState = TA_TXPOLL_WAIT_SEND ;
//                            				inst->instToSleep = TRUE;
//                            			}
//                            			else
//                            			{
//                            				instance_backtoanchor(inst);
//                            			}
////                            		}
//                            	}
//                            	/*else
//                            								{
//                                                            	//stay in RX wait for next frame...
//                                                            	inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
//                            								}*/
//
//                            	if(fcode == RTLS_DEMO_MSG_REPORT_BCAST) //tag to anchor mode
//                            	{
//                            		/*if(currentRangeNum == inst->rangeNum) //these are the previous ranges...
//                            									{
//                            										//copy the ToF and put into array (array holds last 4 ToFs)
//                            										memcpy(&inst->tofArray[(srcAddr[0]&0x3)], &(messageData[TOFR]), 4);
//
//                            										//check if the ToF is valid, this makes sure we only report valid ToFs
//                            										//e.g. consider the case of reception of response from anchor a1 (we are anchor a2)
//                            										//if a1 got a Poll with previous Range number but got no Final, then the response will have
//                            										//the correct range number but the range will be INVALID_TOF
//                            										if(inst->tofArray[(srcAddr[0]&0x3)] != INVALID_TOF)
//                            										{
//                            											inst->rxResponseMask |= (0x1 << (srcAddr[0]&0x3));
//                            										}
//
//                            									}
//                            									else
//                            									{
//                            										if(inst->tofArray[(srcAddr[0]&0x3)] != INVALID_TOF)
//                            										{
//                            											inst->tofArray[(srcAddr[0]&0x3)] = INVALID_TOF;
//                            										}
//                            									}*/
//
//                            		if(currentRangeNum == inst->rangeNum) //these are the previous ranges...
//                            		{
//                            			//copy the ToF and put into array (array holds last 4 ToFs)
//                            			memcpy(&inst->tofArray[(srcAddr[0] & 0xFF)], &(messageData[TOFR]), 4);
//
//                            			//check if the ToF is valid, this makes sure we only report valid ToFs
//                            			//e.g. consider the case of reception of response from anchor a1 (we are anchor a2)
//                            			//if a1 got a Poll with previous Range number but got no Final, then the response will have
//                            			//the correct range number but the range will be INVALID_TOF
//                            			if(inst->tofArray[(srcAddr[0] & 0xFF)] != INVALID_TOF)
//                            			{
//                            				inst->rxResponseMask |= (0x0001 << (srcAddr[0] & 0xFF));
//                            			}
//#if VERSION == VER_1p0
//                            			//										inst->TimestampTx1 = 0;
//                            			//										inst->TimestampRx2 = 0;
//                            			//										inst->TimestampTx3 = 0;
//                            			//
//                            			//										inst->TimestampRx1 = 0;
//                            			//										inst->TimestampTx2 = 0;
//                            			//										inst->TimestampRx3 = 0;
//
//                            			for(uint8 ii=0;ii<9;ii++)
//                            			{
//                            				inst->TimestampTx1[9-ii] = inst->TimestampTx1[8-ii];
//                            				inst->TimestampTx2[9-ii] = inst->TimestampTx2[8-ii];
//                            				inst->TimestampTx3[9-ii] = inst->TimestampTx3[8-ii];
//                            				inst->TimestampRx1[9-ii] = inst->TimestampRx1[8-ii];
//                            				inst->TimestampRx2[9-ii] = inst->TimestampRx2[8-ii];
//                            				inst->TimestampRx3[9-ii] = inst->TimestampRx3[8-ii];
//                            			}
//
//                            			memcpy(&inst->TimestampRx1[0],&messageData[RESP_TIMESTAMP1],5);
//                            			memcpy(&inst->TimestampTx2[0],&messageData[RESP_TIMESTAMP2],5);
//                            			memcpy(&inst->TimestampRx3[0],&messageData[RESP_TIMESTAMP3],5);
//
//                            			inst->TimestampTx1[0] = inst->tempTimestampTx1 & MASK_40BIT;
//                            			inst->TimestampRx2[0] = inst->tempTimestampRx2 & MASK_40BIT;
//                            			inst->TimestampTx3[0] = inst->tempTimestampTx3 & MASK_40BIT;
//#endif
//                            		}
//                            		else
//                            		{
//                            			if(inst->tofArray[(srcAddr[0] & 0xFF)] != INVALID_TOF)
//                            			{
//                            				inst->tofArray[(srcAddr[0] & 0xFF)] = INVALID_TOF;
//                            			}
//                            		}
//
//
//                            	}
//
//                            }
//                            break; //RTLS_DEMO_MSG_REPORT_BCAST


                            default:
                            {
                                //only enable receiver when not using double buffering
                                inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
								dwt_setrxaftertxdelay(0);

                            }
                            break;
						} //end switch (fcode)

						if(dw_event->msgu.frame[0] & 0x20)
						{
							//as we only pass the received frame with the ACK request bit set after the ACK has been sent
							instance_getevent(16); //get and clear the ACK sent event
						}
					} //end else

                }
				break ; //end of DWT_SIG_RX_OKAY

                case DWT_SIG_RX_TIMEOUT :
                	{
                		event_data_t* dw_event = instance_getevent(17); //get and clear this event

						//printf("PD_DATA_TIMEOUT %d\n", inst->previousState) ;

                		//Anchor can time out and then need to send response - so will be in TX pending
                		if(dw_event->type_pend == DWT_SIG_TX_PENDING)
                		{
                			inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
                			inst->previousState = TA_TXRESPONSE_SENT_TORX ;    //wait for TX confirmation of sent response
                		}
                		else if(dw_event->type_pend == DWT_SIG_DW_IDLE) //if timed out and back in receive then don't process as timeout
						{
							inst_processrxtimeout(inst);
						}
                		//else if RX_PENDING then wait for next RX event...
						message = 0; //clear the message as we have processed the event
                	}
                break ;

                case DWT_SIG_TX_AA_DONE: //ignore this event - just process the rx frame that was received before the ACK response
				case 0:
				default :
                {
                    if(message) // == DWT_SIG_TX_DONE)
                    {
                    	inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                    }

                	if(inst->done == INST_NOT_DONE_YET) inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                }
                break;

            }
            break ; // end case TA_RX_WAIT_DATA
            default:
                //printf("\nERROR - invalid state %d - what is going on??\n", inst->testAppState) ;
            break;
    } // end switch on testAppState
#endif
    return inst->done;
} // end testapprun()

// -------------------------------------------------------------------------------------------------------------------
//
// the main instance state machine (all the instance modes Tag, Anchor or Listener use the same statemachine....)
//
// -------------------------------------------------------------------------------------------------------------------
//
int testapprun_TOA(instance_data_t *inst, int message)
{
#if 0
    switch (inst->testAppState)
    {
        case TOA_INIT :

//        	inst->PeriodSync = TOA_DEFAULT_SYNC_PERIOD;
        	memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);
        	dwt_seteui(inst->eui64);

        	dwt_setpanid(inst->panID);

        	inst->shortAdd_idx = (inst->instanceAddress16 & 0xFF) ;
        	dwt_setaddress16(inst->instanceAddress16);

        	dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering

        	if(inst->mode == ANCHOR && inst->instanceAddress16 == inst->gatewayAnchorAddress)
        		inst->mode = ANCHOR_SYNC;

        	inst->idxBlinkInBuff = 0;
        	inst->idxReportInBuff = 0;

            switch (inst->mode)
            {
                case ANCHOR_SYNC:
                {
                	uint16 sleep_mode = 0;

//                    dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;
//                    dwt_setpanid(inst->panID);
//
//                    memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);
//                    dwt_seteui(inst->eui64);
//
//                    //set source address
////                    inst->newRangeTagAddress = inst->instanceAddress16 ;
//                    dwt_setaddress16(inst->instanceAddress16);

                    //Start off by Sleeping 1st -> set instToSleep to TRUE
                    inst->nextState = TOA_SYNC_BCAST;
                    inst->testAppState = TA_TXE_WAIT;
//                    inst->instToSleep = TRUE ;
//                    inst->nextState = 0;
//                    inst->testAppState = TOA_SYNC_BCAST;
                    inst->instToSleep = FALSE ;

                    inst->rangeNum = 0;
                    inst->tagSleepCorrection = 0;

                    sleep_mode = (DWT_LOADUCODE|DWT_PRESRV_SLEEP|DWT_CONFIG|DWT_TANDV);
/*ERWIN
					if((dwt_getldotune() != 0)) //if we need to use LDO tune value from OTP kick it after sleep
						sleep_mode |= DWT_LOADLDO;
*/
					if(inst->configData.txPreambLength == DWT_PLEN_64)  //if using 64 length preamble then use the corresponding OPSet
						sleep_mode |= DWT_LOADOPSET;

#if (DEEP_SLEEP == 1)
				    dwt_configuresleep(sleep_mode, DWT_WAKE_WK|DWT_WAKE_CS|DWT_SLP_EN); //configure the on wake parameters (upload the IC config settings)
#endif
				    instanceconfigframeheader16(inst);
				    inst->instanceWakeTime = portGetTickCnt(); //ERWIN
                }
                break;
                case TAG:
                case ANCHOR:
                {
//                    memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);
//                    dwt_seteui(inst->eui64);
//
//                    dwt_setpanid(inst->panID);
//
//                    //set source address
////                    inst->shortAdd_idx = (inst->instanceAddress16 & 0x3) ;
//                    inst->shortAdd_idx = (inst->instanceAddress16 & 0xFF) ;
//                    dwt_setaddress16(inst->instanceAddress16);

                	//if address = 0x8000
//                	if(inst->instanceAddress16 == GATEWAY_ANCHOR_ADDR)
//                	{
//                		inst->gatewayAnchor = TRUE;
//                	}

//                	dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering

                	// First time anchor listens we don't do a delayed RX
					dwt_setrxaftertxdelay(0);
                    //change to next state - wait to receive a message
                    inst->testAppState = inst->nextState = TA_RXE_WAIT ;

                    dwt_setrxtimeout(0);
                    dwt_setpreambledetecttimeout(0);
                    instanceconfigframeheader16(inst);

                }
                break;
                case LISTENER:
                {
                    dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering
					dwt_setrxaftertxdelay(0); //no delay of turning on of RX
                    dwt_setrxtimeout(0);
                    dwt_setpreambledetecttimeout(0);
                    //change to next state - wait to receive a message
                    inst->testAppState = TA_RXE_WAIT ;
                }
                break ; // end case TA_INIT
                default:
                break;
            }
            break; // end case TOA_INIT

        case TA_SLEEP_DONE :
        {
        	event_data_t* dw_event = instance_getevent(10); //clear the event from the queue
			// waiting for timout from application to wakup IC
			if (dw_event->type != DWT_SIG_RX_TIMEOUT)
			{
				// if no pause and no wake-up timeout continu waiting for the sleep to be done.
                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //wait here for sleep timeout
                break;
            }

            inst->done = INST_NOT_DONE_YET;
            inst->instToSleep = FALSE ;
            inst->testAppState = inst->nextState;
//            inst->nextState = 0; //clear
            inst->nextState = TOA_INIT; //clear
			inst->instanceWakeTime = portGetTickCnt(); //ERWIN // Record the time count when we wake-up
#if (DEEP_SLEEP == 1)
            {
            	uint32 x = 0;

                //wake up device from low power mode
                //NOTE - in the ARM  code just drop chip select for 200us
            	led_on(LED_PC9);
                port_SPIx_clear_chip_select();  //CS low
                instance_data[0].dwIDLE = 0; //reset DW1000 IDLE flag

                setup_DW1000RSTnIRQ(1); //enable RSTn IRQ

                Sleep(2);   //200 us to wake up - need 2 as Sleep(1) is ~ 175 us
                //then wait 5ms for DW1000 XTAL to stabilise - instead of wait we wait for RSTn to go high
                //Sleep(5);

                //need to poll to check when the DW1000 is in IDLE, the CPLL interrupt is not reliable
                //when RSTn goes high the DW1000 is in INIT, it will enter IDLE after PLL lock (in 5 us)
                while(instance_data[0].dwIDLE == 0) // this variable will be sent in the IRQ (process_dwRSTn_irq)
                {
                	 //wait for DW1000 to go to IDLE state RSTn pin to go high
                	x++;
                }
                setup_DW1000RSTnIRQ(0); //disable RSTn IRQ
                port_SPIx_set_chip_select();  //CS high

                //!!! NOTE it takes ~35us for the DW1000 to download AON and lock the PLL and be in IDLE state
                //do some dummy reads of the dev ID register to make sure DW1000 is in IDLE before setting LEDs
            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)

            	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
                /*if(x != DWT_DEVICE_ID)
                {
                	x = dwt_readdevid(); //dummy read... need to wait for 5 us to exit INIT state (5 SPI bytes @ ~18 MHz)
                }*/
                led_off(LED_PC9);
                //this is platform dependent - only program if DW EVK/EVB
                dwt_setleds(1);

                //MP bug - TX antenna delay needs reprogramming as it is not preserved (only RX)
                dwt_settxantennadelay(inst->txAntennaDelay) ;

                //set EUI as it will not be preserved unless the EUI is programmed and loaded from NVM
				dwt_seteui(inst->eui64);
            }
#else
            Sleep(1); //to approximate match the time spent in the #if above
#endif

//            if(inst->antennaDelayChanged == 1)
//            	instancesetantennadelays(); //this will update the antenna delay if it has changed
            instancesettxpower(); //configure TX power if it has changed

       }
            break;

        case TA_TXE_WAIT : //either go to sleep or proceed to TX a message
            // printf("TA_TXE_WAIT") ;
            //if we are scheduled to go to sleep before next transmission then sleep first.
        	if((inst->nextState == TOA_SYNC_BCAST)
                    && (inst->instToSleep)  //go to sleep before sending the next poll/ starting new ranging exchange
                    )
//        	if(inst->nextState == TOA_SYNC_BCAST)
        	{
        		inst->TagIdBlink++; //increment the range number before going to sleep
        		inst->TagIdBlink = inst->TagIdBlink % inst->maxSupportedTagTOA;
        		inst->idxBlinkInBuff = inst->TagIdBlink % MAX_BUFF_SIZE;
        		inst->TagIdBlinkArray[inst->idxBlinkInBuff] = inst->TagIdBlink;
        		// if a cycle has been passed (all tags have been polled with SYNC message), increase sequence number
        		if(inst->TagIdBlink == 0)
        			inst->SeqNumBlinkGlobal++;
        		inst->SeqNumBlink[inst->idxBlinkInBuff] = inst->SeqNumBlinkGlobal;
                //the app should put chip into low power state and wake up after tagSleepTime_ms time...
                //the app could go to *_IDLE state and wait for uP to wake it up...
                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT_TO; //don't sleep here but kick off the Sleep timer countdown
                inst->testAppState = TA_SLEEP_DONE;

                {
#if (DEEP_SLEEP == 1)
                	//put device into low power mode
					dwt_entersleep(); //go to sleep
#endif
					//DW1000 gone to sleep - report the received range
//					inst->newRange = instance_calcranges(&inst->tofArray[0], MAX_ANCHOR_LIST_SIZE, TOF_REPORT_T2A, &inst->rxResponseMask);
//					inst->rxResponseMaskReport = inst->rxResponseMask;

//#if VERSION == VER_1p0
//					memcpy(&inst->TimestampTx1,&inst->tempTimestampTx1,5);
//					memcpy(&inst->TimestampRx2,&inst->tempTimestampRx2,5);
//					memcpy(&inst->TimestampTx3,&inst->tempTimestampTx3,5);
//#endif

//					// check which anchors are active
//					uint8 i = 0;
//					if(inst->rxResponseMask > 0)
//					{
//						// check from MSB first (A15 to A0)
//						for(i=0;i<16;i++)
//							if(((inst->rxResponseMask & (0x8000 >> i)) > 0))
//								inst->gatewayAnchorAddress = 0x8000 | (15-i);
//					}
//					else
//						inst->gatewayAnchorAddress = 0x8000;
//
//					inst->rxResponseMask = 0;
//					inst->newRangeTime = portGetTickCnt(); //ERWIN
                }

            }
            else //proceed to configuration and transmission of a frame
            {
                inst->testAppState = inst->nextState;
//                inst->nextState = 0; //clear
                inst->nextState = TOA_INIT; //clear
            }
            break ; // end case TA_TXE_WAIT

        case TOA_SYNC_BCAST :
            {
//            	inst->TagIdBlink++; //increment the range number before going to sleep
//            	inst->TagIdBlink = inst->TagIdBlink % MAX_TAG_LIST_SIZE_TOA;
//            	inst->SeqNumBlink[inst->TagIdBlink]++;

            	inst->msg_f.messageData[FCODE] = TOA_DEMO_MSG_SYNC_BCAST; //message function code (specifies if message is a poll, response or other...)
            	inst->msg_f.messageData[SYNC_BLINK_TAGID] = inst->TagIdBlinkArray[inst->idxBlinkInBuff]; 				// Tag id to blink
            	memcpy(&inst->msg_f.messageData[SYNC_BLINK_SEQNUM], &inst->SeqNumBlink[inst->idxBlinkInBuff], 2); // Blink seq num for that tag
            	inst->msg_f.messageData[SYNC_REP_TAGID] = inst->TagIdReportArray[inst->idxReportInBuff]; 				// Tag id to be reported
            	memcpy(&inst->msg_f.messageData[SYNC_REP_SEQNUM], &inst->SeqNumReport[inst->idxReportInBuff], 2); 	// Report seq num for that tag
            	memcpy(&inst->msg_f.messageData[SYNC_REP_POS_X], &inst->ReportPosX[inst->idxReportInBuff], sizeof(float)); 	// Report position X
            	memcpy(&inst->msg_f.messageData[SYNC_REP_POS_Y], &inst->ReportPosY[inst->idxReportInBuff], sizeof(float)); 	// Report position Y
            	memcpy(&inst->msg_f.messageData[SYNC_REP_POS_Z], &inst->ReportPosZ[inst->idxReportInBuff], sizeof(float)); 	// Report position Z
            	inst->msg_f.messageData[SYNC_REP_VALID] = inst->ReportValid[inst->idxReportInBuff]; 				// if 0 then it's not valid
//            	inst->msg_f.messageData[SYNC_CMDSEQNUM] = inst->commandSeqNum;
//            	inst->msg_f.messageData[SYNC_CMDOPMODE] = inst->opMode;

            	inst->psduLength = (ANCH_SYNC_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
                inst->msg_f.seqNum = inst->frameSN++; //copy sequence number and then increment
                inst->msg_f.sourceAddr[0] = inst->eui64[0]; //copy the address
                inst->msg_f.sourceAddr[1] = inst->eui64[1]; //copy the address
            	inst->msg_f.destAddr[0] = 0xff;  //set the destination address (broadcast == 0xffff)
            	inst->msg_f.destAddr[1] = 0xff;  //set the destination address (broadcast == 0xffff)
                dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg_f, 0) ;	// write the frame data

//                //set the delayed rx on time (the response message will be sent after this delay (from A0))
//                dwt_setrxaftertxdelay((uint32)RX_RESPONSE1_TURNAROUND);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)
//
//                dwt_setrxaftertxdelay(0); //no delay of turning on of RX
//                dwt_setrxtimeout(0);
//                dwt_setpreambledetecttimeout(0);

				//set the delayed rx on time (the response message will be sent after this delay (from A0))
//				dwt_setrxaftertxdelay((uint32)RX_RESPONSE1_TURNAROUND_6M81);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)
				dwt_setrxaftertxdelay(0);
//
//				inst->rxResps[inst->rangeNum] = 0; //reset the number of received responses
//				inst->responseTO = 1; //expecting 4 responses
				dwt_setrxtimeout((uint16)inst->fwtoTime_sy);  //configure the RX FWTO
//				dwt_setrxtimeout(0);

//				inst->rxResponseMask = 0;	//reset/clear the mask of received responses when tx poll
//				inst->rxResponseMaskAnc = 0;

				inst->wait4ack = DWT_RESPONSE_EXPECTED; //response is expected - automatically enable the receiver

				dwt_writetxfctrl(inst->psduLength, 0, 0 /*ERWIN*/); //write frame control

				dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED); //transmit the frame

                inst->testAppState = TA_TX_WAIT_CONF ;  // wait confirmation
                inst->previousState = TOA_SYNC_BCAST ;
                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set above)

//            	inst->TagIdBlink++; //increment the range number before starting the next cycle
//            	inst->TagIdBlink %= MAX_TAG_LIST_SIZE_TOA;
//            	inst->SeqNumBlink[inst->TagIdBlink]++;

                uint8 kk = 0;
                UART_txbuff[kk++] = 0;
                UART_txbuff[kk++] = 0xE8;
                UART_txbuff[kk++] = '~';
                UART_txbuff[kk++] = inst->TagIdBlinkArray[inst->idxBlinkInBuff];
                UART_txbuff[kk++] = (inst->SeqNumBlink[inst->idxBlinkInBuff] >> 8);
                UART_txbuff[kk++] = inst->SeqNumBlink[inst->idxBlinkInBuff];
                UART_txbuff[kk++] = '\r';
                UART_txbuff[kk++] = '\n';

                send_uart_txdata(kk);

            }
            break;

        case TOA_TAG_BLINK :
            {
//            	inst->msg_f.messageData[FCODE] = TOA_DEMO_MSG_TAG_BLINK;
//            	//Tag blink sequence number
//                memcpy(&inst->msg_f.messageData[TAG_BLINK_SEQNUM], &inst->SeqNumBlink[inst->instanceAddress16 & 0xFF], 2); // Blink seq num for that tag
//                //the mask is sent so the anchors know whether the response RX time is valid
////                inst->msg_f.messageData[VRESP] = (inst->mode == TAG) ? inst->rxResponseMask : inst->rxResponseMaskAnc;
////				inst->msg_f.messageData[VRESP1] = (inst->mode == TAG) ? inst->rxResponseMask: inst->rxResponseMaskAnc;
////				inst->msg_f.messageData[VRESP2] = (inst->mode == TAG) ? inst->rxResponseMask >> 8: inst->rxResponseMaskAnc >> 8;
////				inst->msg_f.messageData[STATUSIO1FINAL] = inst->StatusIO1;
////				inst->msg_f.messageData[STATUSIO2FINAL] = inst->StatusIO2;
////				inst->msg_f.messageData[STATIOSEQFINAL] = inst->StatIOseqnum;
////            	inst->msg_f.messageData[FCODE] = (inst->mode == TAG) ? RTLS_DEMO_MSG_TAG_FINAL : RTLS_DEMO_MSG_ANCH_FINAL; //message function code (specifies if message is a poll, response or other...)
//                inst->psduLength = (TAG_BLINK_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
//                inst->msg_f.seqNum = inst->frameSN++; //copy sequence number and then increment
//                inst->msg_f.sourceAddr[0] = inst->eui64[0]; //copy the address
//                inst->msg_f.sourceAddr[1] = inst->eui64[1]; //copy the address
//                inst->msg_f.destAddr[0] = 0xff;  //set the destination address (broadcast == 0xffff)
//                inst->msg_f.destAddr[1] = 0xff;  //set the destination address (broadcast == 0xffff)
//				dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg_f, 0) ;	// write the frame data
//
////				inst->wait4ack = 0; //clear the flag not using wait for response as this message ends the ranging exchange
////				inst->wait4ack = DWT_RESPONSE_EXPECTED; //response is expected - automatically enable the receiver
//
////				inst->delayedReplyTime = RX_RESPONSE1_TURNAROUND_6M81;
////
////				if(instancesenddlypacket(inst, DWT_START_TX_DELAYED))
////				{
////					inst->testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
////					inst->nextState = TA_TX_WAIT_CONF;                                               // wait confirmation
////					inst->previousState = TOA_TAG_BLINK;
////				}
////				else
////				{
////					inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
////					inst->previousState = TOA_TAG_BLINK;
////				}
//
////				if(instancesenddlypacket(inst, DWT_START_TX_DELAYED))
////                {
////                    // initiate the re-transmission
////					if(inst->mode == TAG)
////					{
////						inst->testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
////						inst->nextState = TA_RXE_WAIT ;
////					}
////					else
////					{
////						//A0 - failed to send Final
////						//A1 - failed to send Final
////						//go back to RX and behave as anchor
////						instance_backtoanchor(inst);
////					}
////                    break; //exit this switch case...
////                }
////                else
////                {
////                    inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
////                    inst->previousState = TOA_TAG_BLINK;
////                }
//
//				inst->wait4ack = DWT_RESPONSE_EXPECTED; //response is expected - automatically enable the receiver
//
//				dwt_writetxfctrl(inst->psduLength, 0); //write frame control
//
//				dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED); //transmit the frame
//
//				inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
//				inst->previousState = TOA_TAG_BLINK;
//
//				inst->toaEvtTime[inst->instanceAddress16 & 0xFF] = portGetTickCnt() & 0xffffffff;
//
////            	if(inst->mode == TAG)
////            	{
////            		inst->instToSleep = TRUE ;
////            	}
				inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set above)

				uint8 kk = 0;
				UART_txbuff[kk++] = 0;
//				UART_txbuff[kk++] = 0xE8;
//				UART_txbuff[kk++] = '~';
//				UART_txbuff[kk++] = inst->TagIdBlinkArray[inst->idxBlink];
//				UART_txbuff[kk++] = (inst->SeqNumBlink[inst->idxBlink] >> 8);
//				UART_txbuff[kk++] = inst->SeqNumBlink[inst->idxBlink];
				UART_txbuff[kk++] = '\r';
				UART_txbuff[kk++] = '\n';

				send_uart_txdata(kk);
            }
            break;


        case TA_TX_WAIT_CONF :
		   //printf("TA_TX_WAIT_CONF %d m%d %d states %08x %08x\n", inst->previousState, message, inst->newReportSent, dwt_read32bitreg(0x19), dwt_read32bitreg(0x0f)) ;

            {
				event_data_t* dw_event = instance_getevent(11); //get and clear this event
				inst->txu.txTimeStamp = dw_event->timeStamp; // get TX timestamp

				if(inst->mode == ANCHOR_SYNC)
					inst->syncRxTimestamp[inst->idxBlinkInBuff] = dw_event->timeStamp ; //save SYNC Rx time

                //NOTE: Can get the ACK before the TX confirm event for the frame requesting the ACK
                //this happens because if polling the ISR the RX event will be processed 1st and then the TX event
                //thus the reception of the ACK will be processed before the TX confirmation of the frame that requested it.
				if(dw_event->type != DWT_SIG_TX_DONE) //wait for TX done confirmation
                {
					if(dw_event->type != 0)
					{
						if(dw_event->type == DWT_SIG_RX_TIMEOUT) //got RX timeout - i.e. did not get the response (e.g. ACK)
						{
							//printf("RX timeout in TA_TX_WAIT_CONF (%d)\n", inst->previousState);
							//we need to wait for SIG_TX_DONE and then process the timeout and re-send the frame if needed
							inst->gotTO = 1;
//							inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT_TO;
						}
						else
						{
							inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
						}
					}

                    inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                        break;

                }

                inst->done = INST_NOT_DONE_YET;
                inst->testAppState = TA_RXE_WAIT ;
                message = 0;

                if(inst->previousState == TOA_SYNC_BCAST)
                {
//                    if(inst->mode == TAG)
//                    {
//                    	inst->testAppState = TA_TXE_WAIT ;
//                    	inst->nextState = TA_RXE_WAIT ;
                    	inst->testAppState = TA_RXE_WAIT ;
                    	inst->nextState = TA_TXE_WAIT ;
                        break;
//                    }
//                    else
//                    {
//                    	instance_backtoanchor(inst);
//					}
                }
//                else if (inst->gotTO == 1) //timeout
//                {
//					//printf("got TO in TA_TX_WAIT_CONF\n");
//                    inst_processrxtimeout(inst);
//                    inst->gotTO = 0;
//					inst->wait4ack = 0 ; //clear this
//					break;
//                }
//                else
//                {
////					inst->txu.txTimeStamp = dw_event->timeStamp;
////
////					if(inst->previousState == TA_TXPOLL_WAIT_SEND)
////					{
////		                uint64 tagCalculatedFinalTxTime ;
////
////		                // Embed into Final message: 40-bit pollTXTime,  40-bit respRxTime,  40-bit finalTxTime
////		                if(inst->mode == TAG)
////		                {
////		                	tagCalculatedFinalTxTime =  (inst->txu.txTimeStamp + inst->pollTx2FinalTxDelay) & MASK_TXDTS;
////		                }
////		                else //for anchor make the final half the delay ..... (this is ok, as A0 awaits 2 responses)
////		                {
////		                	tagCalculatedFinalTxTime =  (inst->txu.txTimeStamp + inst->pollTx2FinalTxDelayAnc) & MASK_TXDTS;
////		                }
////		                inst->delayedReplyTime = tagCalculatedFinalTxTime >> 8; //high 32-bits
////		                // Calculate Time Final message will be sent and write this field of Final message
////		                // Sending time will be delayedReplyTime, snapped to ~125MHz or ~250MHz boundary by
////		                // zeroing its low 9 bits, and then having the TX antenna delay added
////		                // getting antenna delay from the device and add it to the Calculated TX Time
////		                tagCalculatedFinalTxTime = tagCalculatedFinalTxTime + inst->txAntennaDelay;
////		                tagCalculatedFinalTxTime &= MASK_40BIT;
////
////		                // Write Calculated TX time field of Final message
////						memcpy(&(inst->msg_f.messageData[FTXT]), (uint8 *)&tagCalculatedFinalTxTime, 5);
////		                // Write Poll TX time field of Final message
////						memcpy(&(inst->msg_f.messageData[PTXT]), (uint8 *)&inst->txu.tagPollTxTime, 5);
////#if VERSION == VER_1p0
////						inst->tempTimestampTx1 = inst->txu.tagPollTxTime;
////						inst->tempTimestampTx3 = tagCalculatedFinalTxTime;
////#endif
////						//change the w4r for the second and remaining anchors to 50 us
////						//dwt_setrxaftertxdelay((uint32)RX_RESPONSEX_TURNAROUND);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)
////					}
////
////		            if(inst->previousState == TA_TXRESPONSE_SENT_TORX)
////		            {
////		            	inst->previousState = TA_TXRESPONSE_WAIT_SEND ;
////		            }
////                    inst->testAppState = TA_RXE_WAIT ;                      // After sending, tag expects response/report, anchor waits to receive a final/new poll
////
////                    message = 0;
////                    //fall into the next case (turn on the RX)
//                }

            } break;

            //break ; // end case TA_TX_WAIT_CONF


        case TA_RXE_WAIT :
        // printf("TA_RXE_WAIT") ;
        {

            if(inst->wait4ack == 0) //if this is set the RX will turn on automatically after TX
            {
                //turn RX on
            	dwt_rxenable(DWT_START_RX_IMMEDIATE) ;  // turn RX on, without delay
            }
            else
            {
                inst->wait4ack = 0 ; //clear the flag, the next time we want to turn the RX on it might not be auto
            }

            if (inst->mode != LISTENER)
            {
            	inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //using RX FWTO
            }

            inst->testAppState = TA_RX_WAIT_DATA;   // let this state handle it

            // end case TA_RXE_WAIT, don't break, but fall through into the TA_RX_WAIT_DATA state to process it immediately.
            if(message == 0) break;
        } break;

        case TA_RX_WAIT_DATA :                                                                     // Wait RX data
		   //printf("TA_RX_WAIT_DATA %d", message) ;

            switch (message)
            {

				//if we have received a DWT_SIG_RX_OKAY event - this means that the message is IEEE data type - need to check frame control to know which addressing mode is used
                case DWT_SIG_RX_OKAY :
                {
					event_data_t* dw_event = instance_getevent(15); //get and clear this event
					uint8  srcAddr[8] = {0,0,0,0,0,0,0,0};
					uint8  dstAddr[8] = {0,0,0,0,0,0,0,0};
                    int fcode = 0;
					int fn_code = 0;
					//int srclen = 0;
					//int fctrladdr_len;
//					uint8 tof_idx  = 0;
					uint8 *messageData;

					inst->stopTimer = 0; //clear the flag, as we have received a message

                    // handle 16 and 64 bit source and destination addresses
					switch(dw_event->msgu.frame[1] & 0xCC)
					{
						case 0xCC: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ll.sourceAddr[0]), ADDR_BYTE_SIZE_L);
							memcpy(&dstAddr[0], &(dw_event->msgu.rxmsg_ll.destAddr[0]), ADDR_BYTE_SIZE_L);
							fn_code = dw_event->msgu.rxmsg_ll.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ll.messageData[0];
							//srclen = ADDR_BYTE_SIZE_L;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_L;
							break;
						case 0xC8: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_sl.sourceAddr[0]), ADDR_BYTE_SIZE_L);
							memcpy(&dstAddr[0], &(dw_event->msgu.rxmsg_sl.destAddr[0]), ADDR_BYTE_SIZE_S);
							fn_code = dw_event->msgu.rxmsg_sl.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_sl.messageData[0];
							//srclen = ADDR_BYTE_SIZE_L;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_LS;
							break;
						case 0x8C: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ls.sourceAddr[0]), ADDR_BYTE_SIZE_S);
							memcpy(&dstAddr[0], &(dw_event->msgu.rxmsg_ls.destAddr[0]), ADDR_BYTE_SIZE_L);
							fn_code = dw_event->msgu.rxmsg_ls.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ls.messageData[0];
							//srclen = ADDR_BYTE_SIZE_S;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_LS;
							break;
						case 0x88: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ss.sourceAddr[0]), ADDR_BYTE_SIZE_S);
							memcpy(&dstAddr[0], &(dw_event->msgu.rxmsg_ss.destAddr[0]), ADDR_BYTE_SIZE_S);
							fn_code = dw_event->msgu.rxmsg_ss.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ss.messageData[0];
							//srclen = ADDR_BYTE_SIZE_S;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_S;
							break;
					}

//					// read diagnostics data
//#if (READ_DIAGNOSTICS) == 1
//					dwt_rxdiag_t diagn;
//					//uint8 aaa = 0;
//					dwt_readdignostics(&diagn);
//
//					//if(srcAddr[0] > 7)
//					//	aaa = 1;
//
//					if(inst->mode == TAG)
//					{
//						memcpy(&(inst->Adiags_data[srcAddr[0] & 0xFF]), &diagn, sizeof(diagn));
//					}
//					else // srcAddr[1] == 0 means data coming from tag
//					{
//						uint8 idx;
//						idx = (srcAddr[0] & 0xFF) % MAX_BUFF_SIZE_TOA;
//						memcpy(&(inst->Tdiags_data[idx]), &diagn, sizeof(diagn));
//					}
//#endif
					if((inst->instToSleep == FALSE) && (inst->mode == LISTENER))//update received data, and go back to receiving frames
					{
						//do something with message data (e.g. could extract any ToFs and print them)
						inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
						dwt_setrxaftertxdelay(0);
					}
					else
                    {
						//process ranging messages
						fcode = fn_code;

                        switch(fcode)
                        {
                            case TOA_DEMO_MSG_SYNC_BCAST:
                            {
//            					inst->syncRxTimestamp[messageData[SYNC_BLINK_TAGID]] = dw_event->timeStamp ; //save SYNC Rx time

								//the response has been sent - await TX done event
                                if(dw_event->type_pend == DWT_SIG_TX_PENDING)
                                {
                                    inst->testAppState = TA_TX_WAIT_CONF;                // wait confirmation
                                    inst->previousState = TA_TXRESPONSE_SENT_RESPRX ;    //wait for TX confirmation of sent response
//                                    inst->previousState = TA_TXRESPONSE_SENT_POLLRX ;    //wait for TX confirmation of sent response
                                }
                                //already re-enabled the receiver
                                else if (dw_event->type_pend == DWT_SIG_RX_PENDING)
                                {
                                	//stay in RX wait for next frame...
                                	//RX is already enabled...
                                	inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                                }
                                else //the DW1000 is idle (re-enable from the application level)
                                {
                                	//stay in RX wait for next frame...
                                	inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
                                }

//                                // get command
//                                if(inst->commandSeqNum < messageData[SYNC_CMDSEQNUM] || \
//                                		(inst->commandSeqNum > 250 && messageData[SYNC_CMDSEQNUM] < 5))
//                                {
//                                	inst->commandSeqNum = messageData[SYNC_CMDSEQNUM];
//                                	inst->opMode = messageData[SYNC_CMDOPMODE];
//                                	UpdateSettings = 1;
//                                }


//                                //when receives a SYNC, we need to remember the new sequence number
//                                memcpy(&inst->SeqNumBlink[messageData[SYNC_BLINK_TAGID]],&messageData[SYNC_BLINK_SEQNUM],2);
//
//                                if(inst->mode == TAG && messageData[SYNC_BLINK_TAGID] == (inst->instanceAddress16 & 0xFF))
//                                {
//                                	inst->testAppState = TOA_TAG_BLINK ; // go to BLINK state for tag
//                                }
//
//                                // copy sequence number of the position report
//                                memcpy(&inst->SeqNumReport[messageData[SYNC_REP_TAGID]],&messageData[SYNC_REP_SEQNUM],2);
//                                // get the position messages
//                                memcpy(&inst->ReportPosX[messageData[SYNC_REP_TAGID]],&messageData[SYNC_REP_POS_X],sizeof(float));
//                                memcpy(&inst->ReportPosY[messageData[SYNC_REP_TAGID]],&messageData[SYNC_REP_POS_Y],sizeof(float));
//                                memcpy(&inst->ReportPosZ[messageData[SYNC_REP_TAGID]],&messageData[SYNC_REP_POS_Z],sizeof(float));
//                                // get status report whether or not it is valid
//                                inst->ReportValid[messageData[SYNC_REP_TAGID]] = messageData[SYNC_REP_VALID];
//
//                                // reset blink data info
//                                inst->NewBlinkData[messageData[SYNC_BLINK_TAGID]] = 0;

                                // get the status IO message
//                                inst->StatusIO1 = messageData[STATUSIO1POLL];
//                                inst->StatusIO2 = messageData[STATUSIO2POLL];
//                                inst->StatIOseqnum = messageData[STATIOSEQPOLL];
//
//                                usbVCOMout[0] = '$';
//                                usbVCOMout[1] = messageData[STATUSIO1POLL];
//                                usbVCOMout[2] = messageData[STATUSIO2POLL];
//                                usbVCOMout[3] = messageData[STATIOSEQPOLL];
//                                usbVCOMout[4] = '\r';
//                                usbVCOMout[5] = '\n';
//                                send_usbmessage(&usbVCOMout[0], 6);
//                                usb_run();

                                uint8 kk = 0;
                                UART_txbuff[kk++] = 0;
                                UART_txbuff[kk++] = 0xE8;
                                UART_txbuff[kk++] = '~';
                                UART_txbuff[kk++] = messageData[SYNC_BLINK_TAGID];
                                UART_txbuff[kk++] = (messageData[SYNC_BLINK_SEQNUM] >> 8);
                                UART_txbuff[kk++] = messageData[SYNC_BLINK_SEQNUM];
//                                UART_txbuff[kk++] = 0;
//                                UART_txbuff[kk++] = 0;
//                                UART_txbuff[kk++] = 0;
//                                UART_txbuff[kk++] = 0;
//                                UART_txbuff[kk++] = 0;
                                UART_txbuff[kk++] = '\r';
                                UART_txbuff[kk++] = '\n';

                                send_uart_txdata(kk);
                            }
                            break; //TOA_DEMO_MSG_SYNC_BCAST

                            case TOA_DEMO_MSG_TAG_BLINK:
                            {
//                            	inst->blinkRxTimestamp[(srcAddr[0] & 0xFF)] = dw_event->timeStamp ; //save BLINK Rx time
//
//                            	//check from the sequence number
//                            	uint16 seqNumBlink = (uint16)messageData[TAG_BLINK_SEQNUM] | (uint16)(messageData[TAG_BLINK_SEQNUM+1] << 8);
////                            	if(inst->SeqNumBlink[(srcAddr[0] & 0xFF)] == seqNumBlink) {
//                            		if(inst->blinkRxTimestamp[(srcAddr[0] & 0xFF)] >= inst->syncRxTimestamp[(srcAddr[0] & 0xFF)])
//                            			inst->diffRxTimestamp[(srcAddr[0] & 0xFF)] = inst->blinkRxTimestamp[(srcAddr[0] & 0xFF)] - inst->syncRxTimestamp[(srcAddr[0] & 0xFF)];
//                            		else
//                            			inst->diffRxTimestamp[(srcAddr[0] & 0xFF)] = inst->blinkRxTimestamp[(srcAddr[0] & 0xFF)] + 0x000000FFFFFFFFFF - inst->syncRxTimestamp[(srcAddr[0] & 0xFF)];
////                            	}

                            	//the response has been sent - await TX done event
                            	if(dw_event->type_pend == DWT_SIG_TX_PENDING) //anchor received response from anchor ID - 1 so is sending it's response now back to tag
								{
		                			inst->testAppState = TA_TX_WAIT_CONF;                // wait confirmation
		                			inst->previousState = TA_TXRESPONSE_SENT_RESPRX ;    //wait for TX confirmation of sent response
								}
//                            	//already re-enabled the receiver
//                            	else if(dw_event->type_pend == DWT_SIG_RX_PENDING)
//								{
//									// stay in TA_RX_WAIT_DATA - receiver is already enabled.
//								}

								else //if(dw_event->type_pend == DWT_SIG_DW_IDLE)
								{
									if(ANCHOR_SYNC == inst->mode)
									{
										inst->testAppState = TA_TXE_WAIT ; //go to TA_TXE_WAIT first to check if it's sleep time
										inst->nextState = TOA_SYNC_BCAST ;
										inst->instToSleep = TRUE;
//										inst->instToSleep = FALSE;
									}
									else
									{
										//stay in RX
										inst->testAppState = TA_RXE_WAIT ;
										dwt_setrxtimeout(0);
										dwt_setpreambledetecttimeout(0);
										dwt_setrxaftertxdelay(0);
									}
								}

//                            	// get command
//                            	if(inst->commandSeqNum < messageData[BLINK_CMDSEQNUM] || \
//                            			(inst->commandSeqNum > 250 && messageData[BLINK_CMDSEQNUM] < 5))
//                            	{
//                            		inst->commandSeqNum = messageData[BLINK_CMDSEQNUM];
//                            		inst->opMode = messageData[BLINK_CMDOPMODE];
//                            		UpdateSettings = 1;
//                            	}

//                            	inst->toaEvtTime[(srcAddr[0] & 0xFF)] = portGetTickCnt() & 0xffffffff;
//
//                            	// set blink data info
//                            	inst->NewBlinkData[(srcAddr[0] & 0xFF)] = 1;

								/*else
								{
                                	//stay in RX wait for next frame...
                                	inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
								}*/

//								if(fcode == RTLS_DEMO_MSG_ANCH_RESP) //tag to anchor mode
//								{
//									/*if(currentRangeNum == inst->rangeNum) //these are the previous ranges...
//									{
//										//copy the ToF and put into array (array holds last 4 ToFs)
//										memcpy(&inst->tofArray[(srcAddr[0]&0x3)], &(messageData[TOFR]), 4);
//
//										//check if the ToF is valid, this makes sure we only report valid ToFs
//										//e.g. consider the case of reception of response from anchor a1 (we are anchor a2)
//										//if a1 got a Poll with previous Range number but got no Final, then the response will have
//										//the correct range number but the range will be INVALID_TOF
//										if(inst->tofArray[(srcAddr[0]&0x3)] != INVALID_TOF)
//										{
//											inst->rxResponseMask |= (0x1 << (srcAddr[0]&0x3));
//										}
//
//									}
//									else
//									{
//										if(inst->tofArray[(srcAddr[0]&0x3)] != INVALID_TOF)
//										{
//											inst->tofArray[(srcAddr[0]&0x3)] = INVALID_TOF;
//										}
//									}*/
//
//									if(currentRangeNum == inst->rangeNum) //these are the previous ranges...
//									{
//										//copy the ToF and put into array (array holds last 4 ToFs)
//										memcpy(&inst->tofArray[(srcAddr[0] & 0xFF)], &(messageData[TOFR]), 4);
//
//										//check if the ToF is valid, this makes sure we only report valid ToFs
//										//e.g. consider the case of reception of response from anchor a1 (we are anchor a2)
//										//if a1 got a Poll with previous Range number but got no Final, then the response will have
//										//the correct range number but the range will be INVALID_TOF
//										if(inst->tofArray[(srcAddr[0] & 0xFF)] != INVALID_TOF)
//										{
//											inst->rxResponseMask |= (0x0001 << (srcAddr[0] & 0xFF));
//										}
//#if VERSION == VER_1p0
////										inst->TimestampTx1 = 0;
////										inst->TimestampRx2 = 0;
////										inst->TimestampTx3 = 0;
////
////										inst->TimestampRx1 = 0;
////										inst->TimestampTx2 = 0;
////										inst->TimestampRx3 = 0;
//
//										for(uint8 ii=0;ii<9;ii++)
//										{
//											inst->TimestampTx1[9-ii] = inst->TimestampTx1[8-ii];
//											inst->TimestampTx2[9-ii] = inst->TimestampTx2[8-ii];
//											inst->TimestampTx3[9-ii] = inst->TimestampTx3[8-ii];
//											inst->TimestampRx1[9-ii] = inst->TimestampRx1[8-ii];
//											inst->TimestampRx2[9-ii] = inst->TimestampRx2[8-ii];
//											inst->TimestampRx3[9-ii] = inst->TimestampRx3[8-ii];
//										}
//
//										memcpy(&inst->TimestampRx1[0],&messageData[RESP_TIMESTAMP1],5);
//										memcpy(&inst->TimestampTx2[0],&messageData[RESP_TIMESTAMP2],5);
//										memcpy(&inst->TimestampRx3[0],&messageData[RESP_TIMESTAMP3],5);
//
//										inst->TimestampTx1[0] = inst->tempTimestampTx1 & MASK_40BIT;
//										inst->TimestampRx2[0] = inst->tempTimestampRx2 & MASK_40BIT;
//										inst->TimestampTx3[0] = inst->tempTimestampTx3 & MASK_40BIT;
//#endif
//									}
//									else
//									{
//										if(inst->tofArray[(srcAddr[0] & 0xFF)] != INVALID_TOF)
//										{
//											inst->tofArray[(srcAddr[0] & 0xFF)] = INVALID_TOF;
//										}
//									}
//
//
//								}
                            	uint8 kk = 0;
                            	UART_txbuff[kk++] = 0;
//                            	UART_txbuff[kk++] = 0xE8;
//                            	UART_txbuff[kk++] = '~';
//                            	UART_txbuff[kk++] = (srcAddr[0] & 0xFF);
//                            	UART_txbuff[kk++] = (messageData[TAG_BLINK_SEQNUM] >> 8);
//                            	UART_txbuff[kk++] = messageData[TAG_BLINK_SEQNUM];
                            	UART_txbuff[kk++] = '\r';
                            	UART_txbuff[kk++] = '\n';

                            	send_uart_txdata(kk);

                            }
                            break; //TOA_DEMO_MSG_TAG_BLINK

                            default:
                            {
                                //only enable receiver when not using double buffering
                                inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
								dwt_setrxaftertxdelay(0);

                            }
                            break;
						} //end switch (fcode)

						if(dw_event->msgu.frame[0] & 0x20)
						{
							//as we only pass the received frame with the ACK request bit set after the ACK has been sent
							instance_getevent(16); //get and clear the ACK sent event
						}
					} //end else

                }
				break ; //end of DWT_SIG_RX_OKAY

                case DWT_SIG_RX_TIMEOUT :
                	{
                		event_data_t* dw_event = instance_getevent(17); //get and clear this event

						//printf("PD_DATA_TIMEOUT %d\n", inst->previousState) ;

                		//Anchor can time out and then need to send response - so will be in TX pending
                		if(dw_event->type_pend == DWT_SIG_TX_PENDING)
                		{
                			inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
                			inst->previousState = TA_TXRESPONSE_SENT_TORX ;    //wait for TX confirmation of sent response
                		}
                		else if(dw_event->type_pend == DWT_SIG_DW_IDLE) //if timed out and back in receive then don't process as timeout
						{
							inst_processrxtimeout(inst);
						}
                		//else if RX_PENDING then wait for next RX event...
						message = 0; //clear the message as we have processed the event
                	}
                break ;

                case DWT_SIG_TX_AA_DONE: //ignore this event - just process the rx frame that was received before the ACK response
				case 0:
				default :
                {
                    if(message) // == DWT_SIG_TX_DONE)
                    {
                    	inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                    }

                	if(inst->done == INST_NOT_DONE_YET) inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                }
                break;

            }
            break ; // end case TA_RX_WAIT_DATA
            default:
                //printf("\nERROR - invalid state %d - what is going on??\n", inst->testAppState) ;
            break;
    } // end switch on testAppState
#endif
    return inst->done;
} // end testapprun_TOA()

// -------------------------------------------------------------------------------------------------------------------
#if NUM_INST != 1
#error These functions assume one instance only
#else


// -------------------------------------------------------------------------------------------------------------------
// function to set the fixed reply delay time (in us)
//
// This sets delay for RX to TX - Delayed Send, and for TX to RX delayed receive (wait for response) functionality,
// and the frame wait timeout value to use.  This is a function of data rate, preamble length, and PRF

extern uint8 dwnsSFDlen[];

void instancesetreplydelay(int delayus, int maxAnchor) //delay in us
{
    int instance = 0;

    int margin = 3000; //2000 symbols
    int respframe = 0;
    int respframe_sy = 0;

	//configure the rx delay receive delay time, it is dependent on the message length
	float msgdatalen = 0;
	float preamblelen = 0;
	int sfdlen = 0;
	int x = 0;

	uint32 slotPeriod_us;
	uint32 minimumDelay = 0;

	//Set the RX timeouts based on the longest expected message - the Final message
	//Poll = 13, Response = 20, Final = 44 bytes
//	msgdatalen = TAG_FINAL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;
//	msgdatalen = MAX_MAC_MSG_DATA_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;
	msgdatalen = instance_data[instance].maxMessageLength + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;

	//msgdatalen = ANCH_RESPONSE_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;

	x = (int) ceil(msgdatalen*8/330.0f);

	msgdatalen = msgdatalen*8 + x*48;

	//add some margin so we don't timeout too soon
	margin = 0; //(TAG_FINAL_MSG_LEN - TAG_POLL_MSG_LEN); //0;

	x = (int) ceil(margin*8/330.0f);

	margin = margin*8 + x*48;

	//assume PHR length is 172308ns for 110k and 21539ns for 850k/6.81M
	if(instance_data[instance].configData.dataRate == DWT_BR_110K)
    {
		msgdatalen *= 8205.13f;
		msgdatalen += 172308; // PHR length in nanoseconds

		margin *= 8205.13f;

    }
	else if(instance_data[instance].configData.dataRate == DWT_BR_850K)
    {
		msgdatalen *= 1025.64f;
		msgdatalen += 21539; // PHR length in nanoseconds

		margin *= 1025.64f;
    }
	else
    {
		msgdatalen *= 128.21f;
		msgdatalen += 21539; // PHR length in nanoseconds

		margin *= 128.21f;
	}

	//SFD length is 64 for 110k (always)
	//SFD length is 8 for 6.81M, and 16 for 850k, but can vary between 8 and 16 bytes
	sfdlen = dwnsSFDlen[instance_data[instance].configData.dataRate];

	switch (instance_data[instance].configData.txPreambLength)
    {
    case DWT_PLEN_4096 : preamblelen = 4096.0f; break;
    case DWT_PLEN_2048 : preamblelen = 2048.0f; break;
    case DWT_PLEN_1536 : preamblelen = 1536.0f; break;
    case DWT_PLEN_1024 : preamblelen = 1024.0f; break;
    case DWT_PLEN_512  : preamblelen = 512.0f; break;
    case DWT_PLEN_256  : preamblelen = 256.0f; break;
    case DWT_PLEN_128  : preamblelen = 128.0f; break;
    case DWT_PLEN_64   : preamblelen = 64.0f; break;
            }

	//preamble  = plen * (994 or 1018) depending on 16 or 64 PRF
	if(instance_data[instance].configData.prf == DWT_PRF_16M)
	{
		preamblelen = (sfdlen + preamblelen) * 0.99359f;
	}
	else
	{
		preamblelen = (sfdlen + preamblelen) * 1.01763f;
	}

	respframe_sy = (16 + (int)((preamblelen + ((msgdatalen + margin)/1000.0))/ 1.0256)) ;

//	//this is the delay used for the delayed transmit (when sending the response, and final messages)
//	instance_data[instance].pollTx2FinalTxDelay = convertmicrosectodevicetimeu (delayus);
//	//the anchor to anchor ranging consist of A0 ranging to A1 and A2 and A1 ranging to A2
//	//as there are less messages the ranging time is shorter (thus divide by 2)
//	instance_data[instance].pollTx2FinalTxDelayAnc = convertmicrosectodevicetimeu (delayus/2 + 100);

	//this is the delay the anchors 1, 2, etc.. will send the response back at...
	//anchor 2 will have the delay set to 2 * fixedReplyDelayAnc
	//andhor 3 will have the delay set to 3 * fixedReplyDelayAnc and so on...
	//this delay depends on how quickly the tag can receive and process the message from previous anchor
	//(and also the frame length of course)
	respframe = (int)(preamblelen + (msgdatalen/1000.0)); //length of response frame (micro seconds)

//	if(instance_data[instance].configData.dataRate == DWT_BR_110K)
//	{
//		//set the frame wait timeout time - total time the frame takes in symbols
//		instance_data[instance].fwtoTime_sy = respframe_sy + RX_RESPONSE1_TURNAROUND_110K + 400; //add some margin because of the resp to resp RX turn on time
//
//		instance_data[instance].fwtoTimeAnc_sy = respframe_sy; //add some margin so we don't timeout too soon
//		instance_data[instance].fixedReplyDelayAnc = convertmicrosectodevicetimeu (respframe + RX_RESPONSE1_TURNAROUND_110K);
//		instance_data[instance].fixedReplyDelayAncP = (uint32) (((uint64) convertmicrosectodevicetimeu (preamblelen)) >> 8) + 16;
//
//		instance_data[instance].ancRespRxDelay = RX_RESPONSE1_TURNAROUND_110K ;
//	}
//	else
//	{
//		//set the frame wait timeout time - total time the frame takes in symbols
//		instance_data[instance].fwtoTime_sy = respframe_sy + RX_RESPONSE1_TURNAROUND_6M81; //add some margin because of the resp to resp RX turn on time
//
//		instance_data[instance].fwtoTimeAnc_sy =  respframe_sy;
//		instance_data[instance].fixedReplyDelayAnc = convertmicrosectodevicetimeu (respframe + RX_RESPONSE1_TURNAROUND_6M81);
//		instance_data[instance].fixedReplyDelayAncP = (uint32) (((uint64) convertmicrosectodevicetimeu (preamblelen)) >> 8) + 16;
//
//		instance_data[instance].ancRespRxDelay = RX_RESPONSE1_TURNAROUND_6M81 ;
//	}

	if(instance_data[instance].configData.dataRate == DWT_BR_110K)
	{
		//set the frame wait timeout time - total time the frame takes in symbols
		instance_data[instance].fwtoTime_sy = respframe_sy + instance_data[instance].trxResponseTurnaround + 400; //add some margin because of the resp to resp RX turn on time

		instance_data[instance].fwtoTimeAnc_sy = respframe_sy; //add some margin so we don't timeout too soon
		instance_data[instance].fixedReplyDelayAnc = convertmicrosectodevicetimeu (respframe + instance_data[instance].trxResponseTurnaround);
		instance_data[instance].fixedReplyDelayAncP = (uint32) (((uint64) convertmicrosectodevicetimeu (preamblelen)) >> 8) + 16;

		instance_data[instance].ancRespRxDelay = instance_data[instance].trxResponseTurnaround ;
	}
	else
	{
		//set the frame wait timeout time - total time the frame takes in symbols
		instance_data[instance].fwtoTime_sy = respframe_sy + instance_data[instance].trxResponseTurnaround; //add some margin because of the resp to resp RX turn on time

		instance_data[instance].fwtoTimeAnc_sy =  respframe_sy;
		instance_data[instance].fixedReplyDelayAnc = convertmicrosectodevicetimeu (respframe + instance_data[instance].trxResponseTurnaround);
		instance_data[instance].fixedReplyDelayAncP = (uint32) (((uint64) convertmicrosectodevicetimeu (preamblelen)) >> 8) + 16;

		instance_data[instance].ancRespRxDelay = instance_data[instance].trxResponseTurnaround ;
	}

	instance_data[instance].fixedRplyDly_us = respframe + instance_data[instance].trxResponseTurnaround; // fixed reply delay in microseconds
	instance_data[instance].maxMessageDuration = respframe;

	minimumDelay = instance_data[instance].fixedRplyDly_us*maxAnchor + 1000;
	slotPeriod_us = sfConfigTwr.slotPeriod*100;
	if(minimumDelay > delayus && instance_data[instance].opMode == TWR)
	{
		delayus = minimumDelay;
		sfConfigTwr.replyDly = delayus;
		if(delayus > slotPeriod_us)
			sfConfigTwr.slotPeriod = (delayus+1000)/100;
	}
	//this is the delay used for the delayed transmit (when sending the response, and final messages)
	instance_data[instance].pollTx2FinalTxDelay = convertmicrosectodevicetimeu (delayus);
	//the anchor to anchor ranging consist of A0 ranging to A1 and A2 and A1 ranging to A2
	//as there are less messages the ranging time is shorter (thus divide by 2)
	instance_data[instance].pollTx2FinalTxDelayAnc = convertmicrosectodevicetimeu (delayus/2 + 100);

	//	instance_data[instance].sframePeriod = sfConfigTwr->sfPeriod;
	instance_data[instance].slotPeriod = sfConfigTwr.slotPeriod;
	instance_data[instance].tagSleepRnd = sfConfigTwr.slotPeriod;
	//	instance_data[instance].numSlots = sfConfigTwr->numSlots;
	instance_data[instance].numSlots = sfConfigTwr.maxTags;
	instance_data[instance].sframePeriod = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;

	sfConfigTwr.sfPeriod = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;
	sfConfigTwr.pollSleepDly = sfConfigTwr.numSlots*sfConfigTwr.slotPeriod;

	instance_data[instance].tagSleepTime_ms = sfConfigTwr.pollSleepDly; //set the Tag sleep time

}

// -------------------------------------------------------------------------------------------------------------------
//
// Set Payload parameters for the instance
//
// -------------------------------------------------------------------------------------------------------------------
void instancesetaddresses(uint16 address)
{
    int instance = 0 ;

    instance_data[instance].instanceAddress16 = address ;       // copy configurations
}


#endif


/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
