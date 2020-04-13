/*! ----------------------------------------------------------------------------
 *  @file    instance.h
 *  @brief   DecaWave header for application level instance
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "deca_types.h"
#include "deca_device_api.h"

/******************************************************************************************************************
********************* NOTES on DW (MP) features/options ***********************************************************
*******************************************************************************************************************/
#define VER_1p0			0
#define VER_1p1			1
#define VER_1p2			2
#define VER_1p3			3
#define VER_2p0			5

#define ANTENNA_ORIG		0
#define ANTENNA_TAOGLAS		1

#define ANTENNA_USED		ANTENNA_ORIG
//#define ANTENNA_USED		ANTENNA_TAOGLAS

//#define VERSION			VER_1p0
//#define VERSION			VER_1p2
//#define VERSION			VER_1p3
#define VERSION			VER_2p0

#define DEEP_SLEEP (0) //To enable deep-sleep set this to 1
//DEEP_SLEEP mode can be used, for example, by a Tag instance to put the DW1000 into low-power deep-sleep mode:
// when the Anchor is sending the range report back to the Tag, the Tag will enter sleep after a ranging exchange is finished
// once it receives a report or times out, before the next poll message is sent (before next ranging exchange is started).

#define CORRECT_RANGE_BIAS  (1)     // Compensate for small bias due to uneven accumulator growth at close up high power
#define BIAS_TABLE_SIZE (70)

#define ANCTOANCTWR (0) 		//if set to 1 then anchor to anchor TWR will be done in the last slot

#if VERSION == VER_1p0
#define READ_DIAGNOSTICS (1)    //if set to 1 then diagnostics data will be read and stored in the memory
#define ENABLE_DIAGNOSTICS (1)  //if set to 1 then diagnostics data will be sent via USB to PC
#define ENABLE_CIR_DATA	(1)     //if set to 1 then CIR data will be sent via USB to PC
//#define USB_MSG_BINARY (0)		//if set to 1 then USB message is in binary format, otherwise it is in ASCII format
#define USB_MSG_BINARY (1)		//if set to 1 then USB message is in binary format, otherwise it is in ASCII format
#else
//#if VERSION == VER_1p2
//#define READ_DIAGNOSTICS (1)    //if set to 1 then diagnostics data will be read and stored in the memory
//#define ENABLE_DIAGNOSTICS (1)  //if set to 1 then diagnostics data will be sent via USB to PC
//#define ENABLE_CIR_DATA	(0)     //if set to 1 then CIR data will be sent via USB to PC
//#define USB_MSG_BINARY (1)		//if set to 1 then USB message is in binary format, otherwise it is in ASCII format
//#endif
//#if VERSION == VER_1p3
#define READ_DIAGNOSTICS (1)    //if set to 1 then diagnostics data will be read and stored in the memory
#define ENABLE_DIAGNOSTICS (1)  //if set to 1 then diagnostics data will be sent via USB to PC
#define ENABLE_CIR_DATA	(1)     //if set to 1 then CIR data will be sent via USB to PC
#define USB_MSG_BINARY (1)		//if set to 1 then USB message is in binary format, otherwise it is in ASCII format
#endif
/******************************************************************************************************************
*******************************************************************************************************************
*******************************************************************************************************************/

#define NUM_INST            1
#define SPEED_OF_LIGHT      (299702547.0)     // in m/s in air
#define MASK_40BIT			(0x00FFFFFFFFFF)  // DW1000 counter is 40 bits
#define MASK_TXDTS			(0x00FFFFFFFE00)  //The TX timestamp will snap to 8 ns resolution - mask lower 9 bits.

#define SIG_RX_UNKNOWN			99		// Received an unknown frame

//DecaRTLS TWR frame function codes
#define RTLS_DEMO_MSG_REPORT_BCAST          (0x91)          // broadcast report message in TWR
#define RTLS_DEMO_MSG_TAG_POLL              (0x81)          // Tag poll message
#define RTLS_DEMO_MSG_ANCH_RESP             (0x70)          // Anchor response to poll
#define RTLS_DEMO_MSG_ANCH_POLL				(0x71)			// Anchor to anchor poll message
#define RTLS_DEMO_MSG_ANCH_RESP2            (0x72)          // Anchor response to poll from anchor
#define RTLS_DEMO_MSG_ANCH_FINAL            (0x73)          // Anchor final massage back to Anchor
#define RTLS_DEMO_MSG_TAG_FINAL             (0x82)          // Tag final massage back to Anchor
#define RTLS_DEMO_MSG_IOSTATUS				(0x83)			// Tag send I/O status

//DecaRTLS TOA frame function codes
#define TOA_DEMO_MSG_SYNC_BCAST             (0x90)          // Sync node (anchor) SYNC broadcast message
#define TOA_DEMO_MSG_TAG_BLINK             	(0x60)          // Tag BLINK message after receives SYNC

//Command message
#define COMMAND_MESSAGE		             	(0x88)          // command message


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE: the maximum RX timeout is ~ 65ms
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define INST_DONE_WAIT_FOR_NEXT_EVENT   	1   //this signifies that the current event has been processed and instance is ready for next one
#define INST_DONE_WAIT_FOR_NEXT_EVENT_TO    2   //this signifies that the current event has been processed and that instance is waiting for next one with a timeout
                                        		//which will trigger if no event coming in specified time
#define INST_NOT_DONE_YET               	0   //this signifies that the instance is still processing the current event

//this it the delay used for configuring the receiver on delay (wait for response delay)
//NOTE: this RX_RESPONSE_TURNAROUND is dependent on the microprocessor and code optimisations
#define RX_RESPONSEX_TURNAROUND (50) //takes about 100 us for response to come back
#define RX_RESPONSE1_TURNAROUND (200) //takes about 200 us for the 1st response to come back (from A0)
#if VERSION == VER_1p0
#define RX_RESPONSE1_TURNAROUND_6M81 (200)//(300) //takes about 100 us for response to come back
#else
#define RX_RESPONSE1_TURNAROUND_6M81 (300)//(300) //takes about 100 us for response to come back
//#define RX_RESPONSE1_TURNAROUND_6M81 (200)//(300) //takes about 100 us for response to come back
#endif
#define RX_RESPONSE1_TURNAROUND_110K (300)//(600) //takes about 100 us for response to come back

#define TRX_RESP_TURNAROUND_DEFAULT	(300)

typedef enum instanceModes{LISTENER, TAG, ANCHOR, ANCHOR_RNG, ANCHOR_SYNC, NUM_MODES} INST_MODE;
typedef enum oprModes{TWR, TOA} OPR_MODE;
//typedef enum instanceModesTOA{LISTENER, TAG, ANCHOR, ANCHOR_SYNC, NUM_MODES} INST_MODE_TOA;
//Listener = in this mode, the instance only receives frames, does not respond
//Tag = Exchanges DecaRanging messages (Poll-Response-Final) with Anchor and enabling Anchor to calculate the range between the two instances
//Anchor = see above
//Anchor_Rng = the anchor (assumes a tag function) and ranges to another anchor - used in Anchor to Anchor TWR for auto positioning function

#if VERSION == VER_1p0
#define MAX_TAG_LIST_SIZE				(1) //(choose: 1 to 16)
#define MAX_ANCHOR_LIST_SIZE			(1) //(choose: 1 to 16)
//#define NUM_EXPECTED_RESPONSES			(MAX_ANCHOR_LIST_SIZE - 1) //e.g. MAX_ANCHOR_LIST_SIZE - 1
#define NUM_EXPECTED_RESPONSES			(1) //e.g. MAX_ANCHOR_LIST_SIZE - 1
#else
#define MAX_TAG_LIST_SIZE				(8) //(choose: 1 to 16)
#define MAX_ANCHOR_LIST_SIZE			(8) //(choose: 1 to 16)
#define NUM_EXPECTED_RESPONSES			(MAX_ANCHOR_LIST_SIZE - 1) //e.g. MAX_ANCHOR_LIST_SIZE - 1
//#define NUM_EXPECTED_RESPONSES			(1) //e.g. MAX_ANCHOR_LIST_SIZE - 1
#endif
#define NUM_EXPECTED_RESPONSES_ANC		(1) //anchors A0, A1 and A2 are involved in anchor to anchor ranging
#define NUM_EXPECTED_RESPONSES_ANC0		(2) //anchor A0 expects response from A1 and A2

//#define MAX_BUFF_SIZE_TWR				MAX_TAG_LIST_SIZE

#define MAX_TAG_LIST_SIZE_TOA			(10) //(choose: 1 to 65536)
#define MAX_ANCHOR_LIST_SIZE_TOA		(50) //(choose: any)

#define MAX_BUFF_SIZE					16

#define NUM_SLOT_6M81					MAX_TAG_LIST_SIZE
#define NUM_SLOT_110k					MAX_TAG_LIST_SIZE
//Poll to Final delay in the tag in microseconds. Add few ms of margin. These values also depend on microprocessor & code optimization
//When tag range to N anchors,
//each ranging exchange will consist of minimum of 3 messages (Poll, Response, Final)
//and a maximum of 2 + N messages (Poll, Response x N, Final)
#if (MAX_ANCHOR_LIST_SIZE <= 1)
#define POLL_TO_FINAL_DLY_6M81				1400
#define POLL_TO_FINAL_DLY_850K				8000
#define POLL_TO_FINAL_DLY_110K				30000
#else

#define POLL_TO_FINAL_DLY_6M81				8000
#define SLOT_PERIOD_6M81 					100
#define POLL_TO_FINAL_DLY_850K				18000
#define SLOT_PERIOD_850K 					200
#define POLL_TO_FINAL_DLY_110K				78000
#define SLOT_PERIOD_110K 					800

#define SLOT_CIR_MIN 						500 // minimum time slot for enabling CIR reading and transfer via USB
#define SLOT_CIR_MARGIN 				 	400 // additional time slot for enabling CIR reading and transfer via USB
#endif
//#if(ENABLE_CIR_DATA == 1)
//#define SLOT_PERIOD_6M81 				500
//#define SLOT_PERIOD_850K 				500
//#define SLOT_PERIOD_110K 				500
////#else
////#define SLOT_PERIOD_6M81 				50
////#define SLOT_PERIOD_850K 				100
////#define SLOT_PERIOD_110K 				200
////#endif
//#endif
//#if (MAX_ANCHOR_LIST_SIZE > 1 && MAX_ANCHOR_LIST_SIZE < 5)
//#define POLL_TO_FINAL_DLY_6M81				2500
//#define SLOT_PERIOD_6M81 					40
//#define POLL_TO_FINAL_DLY_850K				8000
//#define SLOT_PERIOD_850K 					100
//#define POLL_TO_FINAL_DLY_110K				36000
//#define SLOT_PERIOD_110K 					400
//#endif
//#if (MAX_ANCHOR_LIST_SIZE > 4 && MAX_ANCHOR_LIST_SIZE < 9)
//#define POLL_TO_FINAL_DLY_6M81				8000
//#define SLOT_PERIOD_6M81 					100
//#define POLL_TO_FINAL_DLY_850K				18000
//#define SLOT_PERIOD_850K 					200
//#define POLL_TO_FINAL_DLY_110K				78000
//#define SLOT_PERIOD_110K 					800
//#endif
//#if (MAX_ANCHOR_LIST_SIZE > 8 && MAX_ANCHOR_LIST_SIZE < 17)
//#define POLL_TO_FINAL_DLY_6M81					18000
//#define SLOT_PERIOD_6M81 					200
//#define POLL_TO_FINAL_DLY_850K				38000
//#define SLOT_PERIOD_850K 					400
//#define POLL_TO_FINAL_DLY_110K				158000
//#define SLOT_PERIOD_110K 					1600
//#endif
//#if (MAX_ANCHOR_LIST_SIZE > 16)
//#define POLL_TO_FINAL_DLY				20000
//#define SLOT_PERIOD_6M81 				20
//#endif

//When tag range to N anchors,
//each ranging exchange will consist of minimum of 3 messages (Poll, Response, Final)
//and a maximum of 2 + N messages (Poll, Response x N, Final)
//#if(ENABLE_CIR_DATA == 1)
//#define SLOT_PERIOD_6M81 				60//POLL_TO_FINAL_DLY + 20//10   // minimum slot period in ms for system with 4 anchors
//#define SLOT_PERIOD_110k				30//28   // minimum slot period in ms for system with 4 anchors
//#else
////#define SLOT_PERIOD_6M81 				5//POLL_TO_FINAL_DLY + 5//10   // minimum slot period in ms for system with 4 anchors
//#if(MAX_ANCHOR_LIST_SIZE < 4)
//#define SLOT_PERIOD_6M81 				5 //4+(0.75*MAX_ANCHOR_LIST_SIZE)   //20//POLL_TO_FINAL_DLY + 5//10   // minimum slot period in ms for system with 4 anchors
//#else
//#define SLOT_PERIOD_6M81 				2 + POLL_TO_FINAL_DLY/1000 //1.25*MAX_ANCHOR_LIST_SIZE //4+(0.75*MAX_ANCHOR_LIST_SIZE)
//#endif
//#define SLOT_PERIOD_110k				30//28   // minimum slot period in ms for system with 4 anchors
//#endif

#define SF_PERIOD 						NUM_SLOT_6M81*SLOT_PERIOD_6M81 // superframe period in ms
#define POLL_SLEEP_DLY					SF_PERIOD // tag sleep delay after poll

#define TOA_DEFAULT_SYNC_PERIOD			19

#define GATEWAY_ANCHOR_ADDR				(0x8000)
#define A0_ANCHOR_ADDR					(0x8000)
#define A1_ANCHOR_ADDR					(0x8001)
#define A2_ANCHOR_ADDR					(0x8002)
#define A3_ANCHOR_ADDR					(0x8003)
#define A4_ANCHOR_ADDR					(0x8004)
#define A5_ANCHOR_ADDR					(0x8005)
#define A6_ANCHOR_ADDR					(0x8006)
#define A7_ANCHOR_ADDR					(0x8007)
#define LAST_ANCHOR_ADDR				(0x8000+MAX_ANCHOR_LIST_SIZE-1)


//#define TAG_POLL_MSG_LEN                    2+3				// FunctionCode(1), Range Num (1), StatusIO1 (1), StatusIO2 (1), StatIOseqnum (1)
//#define TAG_POLL_MSG_LEN                    2				// FunctionCode(1), Range Num (1)
#if VERSION == VER_1p0										// add 3 timestamps in each message
//#define TAG_POLL_MSG_LEN                    3+15				// FunctionCode(1), Range Num (1), WhoIsNextGateway(1)
#define TAG_POLL_MSG_LEN                    3				// FunctionCode(1), Range Num (1), WhoIsNextGateway(1)
#define ANCH_RESPONSE_MSG_LEN               8+15               // FunctionCode(1), Sleep Correction Time (2), Measured_TOF_Time(4), Range Num (1) (previous)
#else
#define TAG_POLL_MSG_LEN                    3//+2				// FunctionCode(1), Range Num (1), WhoIsNextGateway(1), getReport (1), CommandSeqNum(1), CommandOpMode(1)
//#define ANCH_RESPONSE_MSG_LEN               8//+2               // FunctionCode(1), Sleep Correction Time (2), Measured_TOF_Time(4), Range Num (1) (previous), CommandSeqNum(1), CommandOpMode(1)
#define ANCH_RESPONSE_MSG_LEN               14//+2               // FunctionCode(1), Sleep Correction Time (2), Measured_TOF_Time(4), Range Num (1) (previous), Immediate report ID (1), Measured_TOF_Time rpt(4), Range Num rpt (1)
#endif
//#define TAG_FINAL_MSG_LEN 	    	1+1+5+(5*MAX_ANCHOR_LIST_SIZE)+5+1
//															// FunctionCode(1), Range Num (1), Poll_TxTime(5),
//															// RespAnc0_RxTime(5),... RespAncN-1_RxTime(5),
//															// Final_TxTime(5), Valid Response Mask (1)
//#define TAG_FINAL_MSG_LEN 	    	1+1+5+(5*MAX_ANCHOR_LIST_SIZE)+5+2+1+1+1
															// FunctionCode(1), Range Num (1), Poll_TxTime(5),
															// RespAnc0_RxTime(5),... RespAncN-1_RxTime(5),
															// Final_TxTime(5), Valid Response Mask (2), StatusIO1 (1), StatusIO2 (1), StatIOseqnum (1)
#if VERSION == VER_1p0
#define TAG_FINAL_MSG_LEN 			ANCH_RESPONSE_MSG_LEN
#else
#define TAG_FINAL_MSG_LEN 	    	1+1+5+(5*MAX_ANCHOR_LIST_SIZE)+5+2//+2
															// FunctionCode(1), Range Num (1), Poll_TxTime(5),
															// RespAnc0_RxTime(5),... RespAncN-1_RxTime(5),
															// Final_TxTime(5), Valid Response Mask (2), CommandSeqNum(1), CommandOpMode(1)
#endif

// TOA messaging lengths
#define ANCH_SYNC_MSG_LEN			1+1+2+1+2+12+1//+2		// FunctionCode(1), TagId(1), SequenceNum(2), PrevTagIdPosReport(1), PosReportSeqNum(2),
															// PosReportX(4), PosReportY(4), PosReportZ(4), PosReportValid(1), CommandSeqNum(1), CommandOpMode(1)

#define TAG_BLINK_MSG_LEN			1+2+5//+2					// FunctionCode(1), SequenceNum(2), DiffTimestamp(5), CommandSeqNum(1), CommandOpMode(1)

//#define MAX_MAC_MSG_DATA_LEN            (ANCH_RESPONSE_MSG_LEN) //max message len of the above

#define MAX_MAC_MSG_DATA_LEN            (TAG_FINAL_MSG_LEN) //max message len of the above


//application data message byte offsets, N anchors
#define FCODE                               0               // Function code is 1st byte of messageData
#define PTXT                                2				// Poll TX time
#define RRXT0                               7				// A0 Response RX time, value: 7, 5 bytes
#define FTXT              RRXT0+(5*MAX_ANCHOR_LIST_SIZE)	// Final TX time
//#define VRESP								FTXT+5			// Mask of valid response times (e.g. if bit 1 = A0's response time is valid)
#define VRESP1                              FTXT+5			// Mask of valid response times LSB (e.g. if bit 1 = A0's response time is valid)
#define VRESP2                              FTXT+6			// Mask of valid response times MSB (e.g. if bit 1 = A8's response time is valid)
#define RES_TAG_SLP0                        1               // Response tag sleep correction LSB
#define RES_TAG_SLP1                        2               // Response tag sleep correction MSB
#define TOFR                                3				// ToF (n-1) 4 bytes
#define TOFRN								7				// range number 1 byte
#define TOFRPT_ID							TOFRN+1			// tag Id for the TOF report. The report is embedded in the resp of the next tag
#define TOFRPT								TOFRPT_ID+1		// TOF report value (4 bytes)
#define TOFRPT_N							TOFRPT+4		// range number of TOF report (1 byte)
#define POLL_RNUM                           1               // Poll message range number
#define WHOISGATEWAY						2				// Poll message assign which anchor be the gateway
#define POLL_TIMESTAMP1						WHOISGATEWAY+1					// Recorded TimestampTx1, include in Poll message from tag to anchor
#define POLL_TIMESTAMP2						WHOISGATEWAY+1+5				// Recorded TimestampRx2, include in Poll message from tag to anchor
#define POLL_TIMESTAMP3						WHOISGATEWAY+1+5+5				// Recorded TimestampTx3, include in Poll message from tag to anchor
#define RESP_TIMESTAMP1						TOFRN+1							// Recorded TimestampRx1, include in Resp message from anchor to tag
#define RESP_TIMESTAMP2						TOFRN+1+5						// Recorded TimestampTx2, include in Resp message from anchor to tag
#define RESP_TIMESTAMP3						TOFRN+1+5+5						// Recorded TimestampRx3, include in Resp message from anchor to tag
#define STATUSIO1FINAL						FTXT+7			// Status IO coming from USB, embed in Final message
#define STATUSIO2FINAL						FTXT+7+1		// Status IO coming from USB, embed in Final message
#define STATIOSEQFINAL						FTXT+7+1+1		// Status IO sequence number, coming from USB
#define STATUSIO1POLL						POLL_RNUM +2	// Status IO coming from USB, embed in Poll message
#define STATUSIO2POLL						POLL_RNUM +3	// Status IO coming from USB, embed in Poll message
#define STATIOSEQPOLL						POLL_RNUM +4	// Status IO sequence number, coming from USB

#define TWR_REPORT_TAGID                  	1               // Tag ID for ranges report after 1 set of TWR (1 byte)
#define TWR_REPORT_SEQNUM                  	2               // Sequence number for ranges report after 1 set of TWR (1 byte)
#define POLL_CMDSEQNUM						WHOISGATEWAY+1
#define POLL_CMDOPMODE						WHOISGATEWAY+2
#define RESP_CMDSEQNUM						TOFRN+1
#define RESP_CMDOPMODE						TOFRN+2
#define FINAL_CMDSEQNUM						VRESP2+1
#define FINAL_CMDOPMODE						VRESP2+2

#define SYNC_BLINK_TAGID                  	1               // Sync message tag id to start blinking (1 byte)
#define SYNC_BLINK_SEQNUM                   2               // Sync message sequence number for that tag (2 bytes)
#define SYNC_REP_TAGID                      4               // Sync message with tag id of reported position (1 byte)
#define SYNC_REP_SEQNUM                     5               // Sync message with sequence number of reported position (2 bytes)
#define SYNC_REP_POS_X						7				// Sync message tag's X position (4 bytes)
#define SYNC_REP_POS_Y						11				// Sync message tag's Y position (4 bytes)
#define SYNC_REP_POS_Z						15				// Sync message tag's Z position (4 bytes)
#define SYNC_REP_VALID						19				// Sync message tag's position is valid (1 byte)
#define TAG_BLINK_SEQNUM                 	1               // Tag message blink sequence number (2 bytes)
#define TAG_BLINK_DIFFTS                 	3               // Tag message blink diff timestamp (reply delay) (5 bytes)

#define SYNC_CMDSEQNUM						SYNC_REP_VALID+1
#define SYNC_CMDOPMODE						SYNC_REP_VALID+2
#define BLINK_CMDSEQNUM						TAG_BLINK_DIFFTS+5
#define BLINK_CMDOPMODE						TAG_BLINK_DIFFTS+5+1


#define TOF_REPORT_NUL 0
#define TOF_REPORT_T2A 1
#define TOF_REPORT_A2A 2

#define INVALID_TOF (0xABCDFFFF)

#define STANDARD_FRAME_SIZE         127
//#define STANDARD_FRAME_SIZE         80

#define ADDR_BYTE_SIZE_L            (8)
#define ADDR_BYTE_SIZE_S            (2)

#define FRAME_CONTROL_BYTES         2
#define FRAME_SEQ_NUM_BYTES         1
#define FRAME_PANID                 2
#define FRAME_CRC					2
#define FRAME_SOURCE_ADDRESS_S        (ADDR_BYTE_SIZE_S)
#define FRAME_DEST_ADDRESS_S          (ADDR_BYTE_SIZE_S)
#define FRAME_SOURCE_ADDRESS_L        (ADDR_BYTE_SIZE_L)
#define FRAME_DEST_ADDRESS_L          (ADDR_BYTE_SIZE_L)
#define FRAME_CTRLP					(FRAME_CONTROL_BYTES + FRAME_SEQ_NUM_BYTES + FRAME_PANID) //5
#define FRAME_CRTL_AND_ADDRESS_L    (FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_L + FRAME_CTRLP) //21 bytes for 64-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_S    (FRAME_DEST_ADDRESS_S + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //9 bytes for 16-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_LS	(FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //15 bytes for 1 16-bit address and 1 64-bit address)
//#define MAX_USER_PAYLOAD_STRING_LL     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_L-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 21 - 16 - 2 = 88
//#define MAX_USER_PAYLOAD_STRING_SS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_S-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 9 - 16 - 2 = 100
//#define MAX_USER_PAYLOAD_STRING_LS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_LS-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 15 - 16 - 2 = 94
#define MAX_USER_PAYLOAD_STRING_LL     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_L-FRAME_CRC) //127 - 21 - 2   this payload length includes everything (poll, resp, final)
#define MAX_USER_PAYLOAD_STRING_SS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_S-FRAME_CRC) //127 - 9 - 2	 this payload length includes everything (poll, resp, final)
#define MAX_USER_PAYLOAD_STRING_LS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_LS-FRAME_CRC) //127 - 15 - 2	 this payload length includes everything (poll, resp, final)

//NOTE: the user payload assumes that there are only 88 "free" bytes to be used for the user message (it does not scale according to the addressing modes)
#define MAX_USER_PAYLOAD_STRING	MAX_USER_PAYLOAD_STRING_LL

typedef enum inst_states
{
    TA_INIT, //0

    TA_TXE_WAIT,                //1 - state in which the instance will enter sleep (if ranging finished) or proceed to transmit a message
    TA_TXPOLL_WAIT_SEND,        //2 - configuration and sending of Poll message
    TA_TXFINAL_WAIT_SEND,       //3 - configuration and sending of Final message
    TA_TXRESPONSE_WAIT_SEND,    //4 - a place holder - response is sent from call back
    TA_TX_WAIT_CONF,            //5 - confirmation of TX done message

    TA_RXE_WAIT,                //6
    TA_RX_WAIT_DATA,            //7

    TA_SLEEP_DONE,               	//8
    TA_TXRESPONSE_SENT_POLLRX,    	//9
    TA_TXRESPONSE_SENT_RESPRX,    	//10
    TA_TXRESPONSE_SENT_TORX,		//11

	TOA_INIT,						//12
	TOA_SYNC_BCAST,					//13
	TOA_TAG_BLINK,					//14

	TWR_REPORT_BCAST				//15

} INST_STATES;

//ERWIN COPIED DWT_SIG from deca_device_api.h version 2.0.xx
//! callback events
#define DWT_SIG_RX_NOERR			0
#define DWT_SIG_TX_DONE             1		// Frame has been sent
#define DWT_SIG_RX_OKAY             2       // Frame Received with Good CRC
#define DWT_SIG_RX_ERROR            3       // Frame Received but CRC is wrong
#define DWT_SIG_RX_TIMEOUT          4       // Timeout on receive has elapsed
#define DWT_SIG_TX_AA_DONE			6		// ACK frame has been sent (as a result of auto-ACK)

#define DWT_SIG_RX_PHR_ERROR        8       // Error found in PHY Header
#define DWT_SIG_RX_SYNCLOSS         9       // Un-recoverable error in Reed Solomon Decoder
#define DWT_SIG_RX_SFDTIMEOUT       10      // Saw preamble but got no SFD within configured time
#define DWT_SIG_RX_PTOTIMEOUT		11		// Got preamble detection timeout (no preamble detected)

#define DWT_SIG_TX_PENDING			12		// TX is pending
#define DWT_SIG_TX_ERROR			13      // TX failed
#define DWT_SIG_RX_PENDING 			14 		// RX has been re-enabled
#define DWT_SIG_DW_IDLE				15      // DW radio is in IDLE (no TX or RX pending)

// This file defines data and functions for access to Parameters in the Device
//message structure for Poll, Response and Final message

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_L];             	//  05-12 using 64 bit addresses
    uint8 sourceAddr[ADDR_BYTE_SIZE_L];           	//  13-20 using 64 bit addresses
    uint8 messageData[MAX_USER_PAYLOAD_STRING_LL] ; //  22-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dlsl ;

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_S];             	//  05-06
    uint8 sourceAddr[ADDR_BYTE_SIZE_S];           	//  07-08
    uint8 messageData[MAX_USER_PAYLOAD_STRING_SS] ; //  09-124 (application data and any user payload)
//    uint8 messageData[TAG_FINAL_MSG_LEN] ; //  09-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dsss ;

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_L];             	//  05-12 using 64 bit addresses
    uint8 sourceAddr[ADDR_BYTE_SIZE_S];           	//  13-14
    uint8 messageData[MAX_USER_PAYLOAD_STRING_LS] ; //  15-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dlss ;

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_S];             	//  05-06
    uint8 sourceAddr[ADDR_BYTE_SIZE_L];           	//  07-14 using 64 bit addresses
    uint8 messageData[MAX_USER_PAYLOAD_STRING_LS] ; //  15-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dssl ;

typedef struct
{
    uint8 channelNumber ;       // valid range is 1 to 11
    uint8 preambleCode ;        // 00 = use NS code, 1 to 24 selects code
    uint8 pulseRepFreq ;        // NOMINAL_4M, NOMINAL_16M, or NOMINAL_64M
    uint8 dataRate ;            // DATA_RATE_1 (110K), DATA_RATE_2 (850K), DATA_RATE_3 (6M81)
    uint8 preambleLen ;         // values expected are 64, (128), (256), (512), 1024, (2048), and 4096
    uint8 pacSize ;
    uint8 nsSFD ;
    uint16 sfdTO;  //!< SFD timeout value (in symbols) e.g. preamble length (128) + SFD(8) - PAC + some margin ~ 135us... DWT_SFDTOC_DEF; //default value
} uwbPhyConfig_t ;

typedef struct
{
	uint16 maxTags ; 	// max number of tags (initiators) in 1 cell
	uint16 maxAnchor ;	// max number of anchors (responders) in 1 cell

    uint32 slotPeriod ; //slot period (time for 1 tag to range to 4 anchors)
    uint16 numSlots ; // number of slots in one superframe (number of tags supported)
    uint32 sfPeriod ; // superframe period in ms

    uint32 pollSleepDly ; // the minimum SLEEP time should be FRAME PERIOD so that tags don't interfere
    uint32 replyDly ; //response delay time (Tag or Anchor when sending Final/Response messages respectively)
//    uint16 replyDlyToa ; //response delay time in TOA mode
} sfConfig_t ;

/******************************************************************************************************************
*******************************************************************************************************************
*******************************************************************************************************************/

//size of the event queue, in this application there should be at most 2 unprocessed events,
//i.e. if there is a transmission with wait for response then the TX callback followed by RX callback could be executed
//in turn and the event queued up before the instance processed the TX event.
#define MAX_EVENT_NUMBER (6)

typedef struct
{
	uint8  type;			// event type - if 0 there is no event in the queue
	uint8  type_save;		// holds the event type - does not clear (used to show what event has been processed)
	uint8  type_pend;	    // set if there is a pending event
	uint16 rxLength ;		// length of RX data (does not apply to TX events)

	uint64 timeStamp ;		// last timestamp (Tx or Rx) - 40 bit DW1000 time

	uint32 timeStamp32l ;		   // last tx/rx timestamp - low 32 bits of the 40 bit DW1000 time
	uint32 timeStamp32h ;		   // last tx/rx timestamp - high 32 bits of the 40 bit DW1000 time

	uint32 uTimeStamp ;			  //32 bit system counter (ms) - STM32 tick time (at time of IRQ)

	union {
			//holds received frame (after a good RX frame event)
			uint8   frame[STANDARD_FRAME_SIZE];
    		srd_msg_dlsl rxmsg_ll ; //64 bit addresses
			srd_msg_dssl rxmsg_sl ;
			srd_msg_dlss rxmsg_ls ;
			srd_msg_dsss rxmsg_ss ; //16 bit addresses
	}msgu;

	//uint32 eventtime ;
	//uint32 eventtimeclr ;
	uint8 gotit;			//stores the instance function which processed the event (used for debug)
}event_data_t ;

// TX power and PG delay configuration structure
typedef struct {
                uint8 PGdelay;

                //TX POWER
                //31:24     BOOST_0.125ms_PWR
                //23:16     BOOST_0.25ms_PWR-TX_SHR_PWR
                //15:8      BOOST_0.5ms_PWR-TX_PHR_PWR
                //7:0       DEFAULT_PWR-TX_DATA_PWR
                uint32 txPwr[2]; //
}tx_struct;

// Antenna delay configuration structure
typedef struct {
                uint16 antDelay[2]; // antDelay[0]: PRF16,   antDelay[1]: PRF64
}antdelay_struct;

// STM32 device ID code
typedef struct {
	uint16_t off0;
	uint16_t off2;
	uint32_t off4;
	uint32_t off8;
}u_id;

typedef struct
{
	OPR_MODE opMode;			// operation mode: TWR or TOA
    INST_MODE mode;				//instance mode (tag or anchor)
//    INST_MODE_TOA mode_toa;				//instance mode (tag or anchor)

    INST_STATES testAppState ;			//state machine - current state
    INST_STATES nextState ;				//state machine - next state
    INST_STATES previousState ;			//state machine - previous state
    int done ;					//done with the current event/wait for next event to arrive

	//configuration structures
	dwt_config_t    configData ;	//DW1000 channel configuration
	dwt_txconfig_t  configTX ;		//DW1000 TX power configuration
        uint8	smartPowerEn ;  //ERWIN //!< Smart Power enable / disable
	uint16			txAntennaDelay ; //DW1000 TX antenna delay
	uint16			rxAntennaDelay ; //DW1000 RX antenna delay
	uint32 			txPower ;		 //DW1000 TX power
	uint8 txPowerChanged ;			//power has been changed - update the register on next TWR exchange
	uint8 antennaDelayChanged;		//antenna delay has been changed - update the register on next TWR exchange

	uint16 instanceAddress16; //contains tag/anchor address

	u_id mcuId;

	//timeouts and delays
	int32 tagSleepTime_ms; //in milliseconds - defines the nominal Tag sleep time period
	int32 tagSleepRnd; //add an extra slot duration to sleep time to avoid collision before getting synced by anchor 0

	//this is the delay used for the delayed transmit
	uint64 pollTx2FinalTxDelay ; //this is delay from Poll Tx time to Final Tx time in DW1000 units (40-bit)
	uint64 pollTx2FinalTxDelayAnc ; //this is delay from Poll Tx time to Final Tx time in DW1000 units (40-bit) for Anchor to Anchor ranging
	uint64 fixedReplyDelayAnc ;
	uint32 fixedReplyDelayAncP ;
	int ancRespRxDelay ;

//	uint16 tagFinalMessageLength ;	//this is the max of the message length = tag's final message length
	uint16 maxMessageLength ;	//in TWR mode this is the max of the message length = tag's final message length

	int fwtoTime_sy ;	//this is final message duration (longest out of ranging messages)
	int fwtoTimeAnc_sy ;
	uint32 delayedReplyTime;		// delayed reply time of ranging-init/response/final message

    uint32 rxTimeouts ;

    //message structure used for holding the data of the frame to transmit before it is written to the DW1000
	srd_msg_dsss msg_f ; // ranging message frame with 16-bit addresses

	//Tag function address/message configuration
	uint8   shortAdd_idx ;				// device's 16-bit address low byte (used as index into arrays [0 - 3])
	uint8   eui64[8];				// device's EUI 64-bit address
	uint16  psduLength ;			// used for storing the TX frame length
    uint8   frameSN;				// modulo 256 frame sequence number - it is incremented for each new frame transmission
	uint16  panID ;					// panid used in the frames

	antdelay_struct antDelaysTx[8];
	antdelay_struct antDelaysRx[8];
	uint8 	antennaType;			// 0: TREK1000, 1: Taoglass, 2: Camco (no cable), 3: Camco + 1m cable

	//64 bit timestamps
	//union of TX timestamps
	union {
		uint64 txTimeStamp ;		   // last tx timestamp
		uint64 tagPollTxTime ;		   // tag's poll tx timestamp
	    uint64 anchorRespTxTime ;	   // anchor's reponse tx timestamp
	}txu;
	uint64 tagPollRxTime ;          // receive time of poll message

	uint64 tagCalculatedFinalTxTime; // calculated final TX time


	//application control parameters
	uint8	wait4ack ;				// if this is set to DWT_RESPONSE_EXPECTED, then the receiver will turn on automatically after TX completion

    int16   responseTO ;
	uint8   instToSleep;			// if set the instance will go to sleep before sending the blink/poll message
	uint8	stopTimer;				// stop/disable an active timer
    uint8	instanceTimerEn;		// enable/start a timer
    uint32	instanceWakeTime;		//
    uint32  nextSleepPeriod;
    int32  	nextReportPeriod;		// time to check for immediate report
    uint32 	timeForceWakeUp;		// time of force wake up periodically (1 superframe slot)
    uint32 	periodForceWakeUp;

	uint8	gotTO;					// got timeout event

	uint16  rxResponseMaskAnc;
	uint16  rxResponseMask;			// bit mask - bit 0 = received response from anchor ID = 0, bit 1 from anchor ID = 1 etc...
	uint16  rxResponseMaskImRpt;	// rxResponseMask for immediate reporting after 1 TWR exchange
	uint16  rxResponseMaskReport;	// the one used in checking validity of ranges
	uint16  rxResponseMaskReportImRpt;	// the one used in checking validity of ranges
	uint8	rangeNum;				// incremented for each sequence of ranges (each slot)
//	uint8	rangeNumA[MAX_TAG_LIST_SIZE];				// array which holds last range number from each tag
	uint8	rangeNumA[MAX_BUFF_SIZE];				// array which holds last range number from each tag
	uint8	rangeNumAnc;			// incremented for each sequence of ranges (each slot) - anchor to anchor ranging
//	uint8	rangeNumAAnc[MAX_ANCHOR_LIST_SIZE]; //array which holds last range number for each anchor
	uint8	rangeNumAAnc[MAX_BUFF_SIZE]; //array which holds last range number for each anchor
	uint16  sframePeriod;
	uint16  slotPeriod;
	uint16  numSlots;
	uint32  a0SlotTime;
	uint32  a1SlotTime;
	int32   tagSleepCorrection;
	int32   tagSleepCorrection2;

    //diagnostic counters/data, results and logging
//    uint32 tof[MAX_TAG_LIST_SIZE]; //this is an array which holds last ToF from particular tag (ID 0-7)
    uint32 tof[MAX_BUFF_SIZE]; //this is an array which holds last ToF from particular tag (ID 0-7)

    //this is an array which holds last ToF to each anchor it should
//    uint32 tofArray[MAX_ANCHOR_LIST_SIZE]; //contain 4 ToF to 4 anchors all relating to same range number sequence
    uint32 tofArray[MAX_BUFF_SIZE]; //contain ToF to all anchors all relating to same range number sequence
    uint32 tofArrayImRpt[MAX_BUFF_SIZE]; //ToF Array used for immediate reporting

//    uint32 tofAnc[MAX_ANCHOR_LIST_SIZE]; //this is an array which holds last ToFs from particular anchors (0, 0-1, 0-2, 1-2)
    uint32 tofAnc[MAX_BUFF_SIZE]; //this is an array which holds last ToFs from particular anchors (0, 0-1, 0-2, 1-2)

    //this is an array which holds last ToFs of the Anchor to Anchor ranging
//    uint32 tofArrayAnc[MAX_ANCHOR_LIST_SIZE]; //it contains 3 ToFs relating to same range number sequence (0, 0-1, 0-2, 1-2)
#if (ANCTOANCTWR == 1) //allow anchor to anchor ranging
    uint32 tofArrayAnc[MAX_BUFF_SIZE]; //it contains 3 ToFs relating to same range number sequence (0, 0-1, 0-2, 1-2)
#endif

    int txMsgCount; //number of transmitted messages
	int	rxMsgCount; //number of received messages
	int lateTX; //number of "LATE" TX events
	int lateRX; //number of "LATE" RX events

    uint16 longTermRangeCount ; //total number of ranges

    int newRange;			//flag set when there is a new range to report TOF_REPORT_A2A or TOF_REPORT_T2A
    int newRangeImRpt;		//flag set when there is a new range to report TOF_REPORT_T2A (immediate reporting)
    int newRangeAncAddress; //last 4 bytes of anchor address - used for printing/range output display
    int newRangeTagAddress; //last 4 bytes of tag address - used for printing/range output display
    int newRangeTime;
    int newRangeTimeImRpt;

    uint8 gatewayAnchor ; //set to TRUE = 1 if anchor address == GATEWAY_ANCHOR_ADDR
    uint16 gatewayAnchorAddress ;
    uint8 enableReport ; //set to TRUE = 1 if TWR report is desired after TWR exchange (after Final from tag is received)
    uint8 getReportReqs ; //set to TRUE = 1 when TWR report is expected in the next TWR  cycle
    uint8 getReportResp ; //set to TRUE = 1 when TWR report is expected in the next TWR  cycle

	//event queue - used to store DW1000 events as they are processed by the dw_isr/callback functions
    event_data_t dwevent[MAX_EVENT_NUMBER]; //this holds any TX/RX events and associated message data
	event_data_t saved_dwevent; //holds an RX event while the ACK is being sent
    uint8 dweventIdxOut;
    uint8 dweventIdxIn;
	uint8 dweventPeek;
	uint8 monitor;
	uint32 timeofTx ;

	uint8 finalTxTimeIdx;	// index in the message buffer points to Final Tx Time in the Final message of TWR
	uint16 twrFinalMsgLen;	// length of Final message in TWR
	uint16 maxTagsTwr;
	uint16 maxAnchorsTwr;

	uint8 rxRespsIdx; //index into the array below (current tag (4bits)/seq number(4bits))
	int8 rxResps[128];
	int8 rxRespsImRpt[128];

	uint8 RangeBiasCorrection;
	uint8 UseUpdatedBiasTable;
	uint16 BiasCorrFactor[6];
	uint8 BiasTablePrf16[6][BIAS_TABLE_SIZE];
	uint8 BiasTablePrf64[6][BIAS_TABLE_SIZE];
	int16 BiasStartPrf16[6];
	int16 BiasStartPrf64[6];

	uint8 updateLcdOn;

//#if VERSION == VER_1p0
	// all timestamps used in DS-TWR
	uint64 TimestampTx1[4];
	uint64 TimestampTx2[4];
	uint64 TimestampTx3[4];
	uint64 TimestampRx1[4];
	uint64 TimestampRx2[4];
	uint64 TimestampRx3[4];

	// temporary buffers
//	uint64 tempTimestampTx1;
//	uint64 tempTimestampTx2;
//	uint64 tempTimestampTx3;
//	uint64 tempTimestampRx1;
//	uint64 tempTimestampRx2;
//	uint64 tempTimestampRx3;
//#endif

	int dwIDLE; //set to 1 when the RST goes high after wake up (it is set in process_dwRSTn_irq)
#if (READ_DIAGNOSTICS) == 1
//	dwt_rxdiag_t Tdiags_data[MAX_TAG_LIST_SIZE];
	dwt_rxdiag_t Tdiags_data[MAX_BUFF_SIZE];
//	uint8 Trssi[MAX_TAG_LIST_SIZE];				// tags' received signal strength indicator (RSSI)
//	uint8 Tfppl[MAX_TAG_LIST_SIZE];				// tags' first path power level (FPPL)
//	dwt_rxdiag_t Adiags_data[MAX_ANCHOR_LIST_SIZE];
	dwt_rxdiag_t Adiags_data[MAX_BUFF_SIZE];
//	uint8 Arssi[MAX_TAG_LIST_SIZE];				// anchors' received signal strength indicator (RSSI)
//	uint8 Afppl[MAX_TAG_LIST_SIZE];				// anchors' first path power level (FPPL)
#endif

	uint8 idxTagInBuff;							// index in buffer with information related to tag
	uint8 idxAnchorInBuff;						// index in buffer with information related to anchor

	uint8 enableCirData;						// set to 1 when requesting CIR data
#if (ENABLE_CIR_DATA) == 1
	uint8 CIRdata[2050];
#endif
	uint8 StatusIO1;								// status I/O coming from USB message, byte1
	uint8 StatusIO2;								// status I/O coming from USB message, byte2
	uint8 StatIOseqnum;								// status I/O coming from USB message, sequence number

	uint16 trxResponseTurnaround;
	uint16 fixedRplyDly_us;
	uint16 maxMessageDuration;

	int32 SyncedTime;
	int32 OffsetTime;

//	uint16 maxSupportedAnchorTWR;
//	uint16 maxSupportedTagTWR;

	// command related variable to select operation mode either in TWR or TOA mode
	uint8 commandSeqNum;

	// TOA related variables
	uint32 PeriodSync;
	uint16 maxSupportedTagTOA;
	uint16 SeqNumBlinkGlobal;
	uint8 idxBlinkInBuff;						// index in buffer to store Tag Blink information
	uint8 idxReportInBuff;						// index in buffer to store Tag Report information
	uint64 syncRxTimestamp[MAX_BUFF_SIZE];		// received timestamp of the SYNC message
	uint64 blinkRxTimestamp[MAX_BUFF_SIZE];		// received timestamp of the tag BLINK message
	int64 diffRxTimestamp[MAX_BUFF_SIZE];		// blinkRxTimestamp - syncRxTimestamp
	int64 diffTimestampTag[MAX_BUFF_SIZE];		// computed reply delay at tag (blink Tx Timestamp - sync RxTimestamp at tag)
	uint8 TagIdBlinkArray[MAX_BUFF_SIZE];
	uint8 TagIdBlink;
	uint16 SeqNumBlink[MAX_BUFF_SIZE];
	uint8 TagIdReportArray[MAX_BUFF_SIZE];
	uint8 TagIdReport;
	uint16 SeqNumReport[MAX_BUFF_SIZE];
	float ReportPosX[MAX_BUFF_SIZE];
	float ReportPosY[MAX_BUFF_SIZE];
	float ReportPosZ[MAX_BUFF_SIZE];
	uint8 ReportValid[MAX_BUFF_SIZE];
	uint8 NewBlinkData[MAX_BUFF_SIZE];
	uint16 toaEvtTime[MAX_BUFF_SIZE];								// TOA event time (CPU timestamp)
	uint32 CurrentTime;

} instance_data_t ;

//-------------------------------------------------------------------------------------------------------------
//
//	Functions used in logging/displaying range and status data
//
//-------------------------------------------------------------------------------------------------------------

// function to calculate and report the Time of Flight to the GUI/display
int reportTOF(int idx, uint32 tofx, uint8 isImRpt);
void clearDistTable(int idx);
void clearDistTableImRpt(int idx);
void setTagDist(int tidx, int aidx);
double getTagDist(int idx);

// clear the status/ranging data 
void instanceclearcounts(void) ;

//-------------------------------------------------------------------------------------------------------------
//
//	Functions used in driving/controlling the ranging application
//
//-------------------------------------------------------------------------------------------------------------

void instance_close(void);
// Call init, then call config, then call run. call close when finished
// initialise the instance (application) structures and DW1000 device
int instance_init(void);
// configure the instance and DW1000 device
void instance_config(uwbPhyConfig_t *config, sfConfig_t *sfconfig) ;

// configure the MAC address
void instancesetaddresses(uint16 address) ;

void inst_processrxtimeout(instance_data_t *inst);

// called (periodically or from and interrupt) to process any outstanding TX/RX events and to drive the ranging application
// TWR operation mode
int instance_run_TWR(void) ;       // returns indication of status report change
int testapprun_TWR(instance_data_t *inst, int message);
// TOA operation mode
int instance_run_TOA(void) ;       // returns indication of status report change
int testapprun_TOA(instance_data_t *inst, int message);

int instance_calcranges(uint32 *array, uint16 size, int reportRange, uint16* mask, uint8 isImRpt);

// calls the DW1000 interrupt handler
#define instance_process_irq(x) 	dwt_isr()  //call device interrupt handler
// configure TX/RX callback functions that are called from DW1000 ISR
//ERWIN void instance_rxcallback(const dwt_callback_data_t *rxd);
//ERWIN void instance_txcallback(const dwt_callback_data_t *txd);

// sets the Tag sleep delay time (the time Tag "sleeps" between each ranging attempt)
void instancesettagsleepdelay(int rangingsleep);
//void instancesetreplydelay(int delayus);
void instancesetreplydelay(int delayus, int maxAnchor);

// set/get the instance roles e.g. Tag/Anchor/Listener
void instancesetrole(int mode) ;                // 
int instancegetrole(void) ;
// get the DW1000 device ID (e.g. 0xDECA0130 for DW1000)
uint32 instancereaddeviceid(void) ;                                 // Return Device ID reg, enables validation of physical device presence

double instancegetidist(int idx);
double instancegetidistraw(int idx);
int instancegetidist_mm(int idx);
int instancegetidistraw_mm(int idx);
uint16 instancevalidranges(void);
void instancecleardisttableall();
void instancecleardisttableall_ImRpt();

void instance_backtoanchor(instance_data_t *inst);
int instancesenddlypacket(instance_data_t *inst, int delayedTx);

int instancegetrnum(void);
int instancegetrnuma(int idx);
int instancegetrnumanc(int idx);
int instancegetlcount(void);

int instancenewrangeancadd(void);
int instancenewrangetagadd(void);
int instancenewrangepolltim(void);
int instancenewrange(void);
int instancenewrangetim(void);

uint64 convertmicrosectodevicetimeu (double microsecu);
double convertdevicetimetosec(int32 dt);

#define DWT_PRF_64M_RFDLY   (514.462f)
#define DWT_PRF_16M_RFDLY   (513.9067f)
extern const uint16 rfDelays[2];
extern const uint16 rfDelaysTREK[2];
extern const tx_struct txSpectrumConfig[8];

extern const antdelay_struct rfDelaysAntOrig[8];
extern const antdelay_struct rfDelaysAntTaoglas[8];

extern instance_data_t instance_data[NUM_INST] ;

int instance_peekevent(void);

void instance_saveevent(event_data_t newevent, uint8 etype);

event_data_t instance_getsavedevent(void);

void instance_putevent(event_data_t newevent, uint8 etype);

event_data_t* instance_getevent(int x);

void instance_clearevents(void);

void instance_notify_DW1000_inIDLE(int idle);

// configure TX power
void instanceconfigtxpower(uint32 txpower);
void instancesettxpower(void);

// configure the antenna delays
void instanceconfigantennadelays(uint16 tx, uint16 rx);
void instancesetantennadelays(void);
uint16 instancetxantdly(void);
uint16 instancerxantdly(void);

void copyRangeBiasTable(void);

int instance_starttxtest(int framePeriod);
#ifdef __cplusplus
}
#endif

#endif
