
#include "includes.h"
#include "../J1939/J1939.H"

void thread_CANComm (void const *argument)
{
	J1939_MESSAGE Msg;
	int _b=1;
	J1939_Initialization( TRUE );

	// 等待地址声明超时
	while (J1939_Flags.WaitingForAddressClaimContention)
		J1939_Poll(5);
	//地址已经设置好了（设备已挂载到总线上）
	while(1)
	{

		Msg.Mxe.Priority				= J1939_INFO_PRIORITY;
		Msg.Mxe.Res						= 0;
		Msg.Mxe.DataPage				= 0;
		Msg.Mxe.PDUFormat				= _b;
		Msg.Mxe.DestinationAddress	    = 0XFF;
		Msg.Mxe.DataLength				= 8;
		while (J1939_EnqueueMessage( &Msg ) != RC_SUCCESS)
			  J1939_Poll(5);
		while (RXQueueCount > 0)
		{
			J1939_DequeueMessage( &Msg );
			if (Msg.Mxe.PDUFormat == 1)
				_b++;
		}
		J1939_Poll(20);
	}
}

void  CAN_rEVENT_BEBUG(void)
{
//    uint8_t rBuff_CAN[8]={0};
//    uint32_t receive_status=0;
//    // Check for Node error
//    if ( CAN_NODE_GetStatus(&CAN_NODE3_DEBUG) & XMC_CAN_NODE_STATUS_ALERT_WARNING )
//    {
//      //Clear the flag
//      CAN_NODE_DisableEvent(&CAN_NODE3_DEBUG,XMC_CAN_NODE_EVENT_ALERT);
//      return ;
//    }
}

void  CAN_tEVENT_BEBUG(void)
{
	;
}
