/*********************************************************************
 *
 *            J1939 Main Source Code
 *
 *********************************************************************
 *
 *	本程序是由XieTongXueFlyMe对现有的J1939协议文档分析，和对前辈的贡献总结,
 * 写出的一套开源的J1939驱动。
 *	本协议特点：
 *		1.易移植（不针对特定的CAN硬件，只要满足CAN2.0B即可）
 *		2.轻量级（可适应低端的MCU）
 *		3.支持多任务调用接口（可用于嵌入式系统）
 *		4.双模式（轮询或者中断，逻辑更加简单明了）
 *		5.不掉帧（数据采用收发列队缓存）
 *
 *  源代码分析网址：
 *	http://blog.csdn.net/xietongxueflyme/article/details/74908563
 *
 * Version     Date        Description
 * ----------------------------------------------------------------------
 * v1.00     2017/06/04    首个版本
 * v1.01     2017/08/04    完善功能
 *
 * Author               Date         changes
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *XieTongXueFlyMe       7/06/04      首个版本
 *XieTongXueFlyMe       7/08/04      增加对TP的支持
 **********************************************************************/
#ifndef         __J1939_SOURCE
#define         __J1939_SOURCE
#endif

#include "J1939.H"   
#include "J1939_config.H"

//特定声明
#define J1939_TRUE         1
#define J1939_FALSE        0
#define ADDRESS_CLAIM_TX   1
#define ADDRESS_CLAIM_RX   2

//全局变量。
j1939_uint8_t                   CA_Name[J1939_DATA_LENGTH];//设备的标称符（参考1939-81）
j1939_uint8_t                   CommandedAddress;   
#if J1939_ACCEPT_CMDADD == J1939_TRUE   
    j1939_uint8_t               CommandedAddressSource;   
    j1939_uint8_t               CommandedAddressName[J1939_DATA_LENGTH];   
#endif   
j1939_uint32_t                  ContentionWaitTime;   
j1939_uint8_t                   J1939_Address;   
J1939_FLAG                      J1939_Flags;   
J1939_MESSAGE                   OneMessage;   
//接受列队全局变量   
j1939_uint8_t                   RXHead;   
j1939_uint8_t                   RXTail;   
j1939_uint8_t                   RXQueueCount;   
J1939_MESSAGE                   RXQueue[J1939_RX_QUEUE_SIZE];   
//发送列队全局变量  
j1939_uint8_t                   TXHead;   
j1939_uint8_t                   TXTail;   
j1939_uint8_t                   TXQueueCount;   
J1939_MESSAGE                   TXQueue[J1939_TX_QUEUE_SIZE];
#if J1939_TP_RX_TX
//TP协议全局变量  
J1939_TP_Flags                  J1939_TP_Flags_t;   
J1939_TRANSPORT_RX_INFO         TP_RX_MSG;    
J1939_TRANSPORT_TX_INFO         TP_TX_MSG;
#endif //J1939_TP_RX_TX
// 函数声明   
#if J1939_ACCEPT_CMDADD == J1939_TRUE   
    j1939_int8_t CA_AcceptCommandedAddress( void );   
#endif   
   
#if J1939_ARBITRARY_ADDRESS != 0x00   
//地址竞争重新估算，设备在总线上要申请的地址
extern  j1939_int8_t ECU_RecalculateAddress( j1939_uint8_t * Address);   
#endif   

/*
*输入：uint8_t *     Array of NAME bytes 
*输出： -1 - CA_Name 是小于 OtherName
*       0  - CA_Name与OtherName 相等
*       1  - CA_Name 是大于 OtherName
*说明：比较传入的数组数据名称与设备当前名称存储在CA_Name。
*/
j1939_int8_t CompareName( j1939_uint8_t *OtherName )   
{   
    j1939_uint8_t   i;   
   
    for (i = 0; (i<J1939_DATA_LENGTH) && (OtherName[i] == CA_Name[i]); i++);   
   
    if (i == J1939_DATA_LENGTH)   
        return 0;   
    else if (CA_Name[i] < OtherName[i] )   
        return -1;   
    else   
        return 1;   
}   
/*
*输入：
*输出：
*说明：设备名字复制到消息缓冲区的数据数组。我们可以使用这个函数在其他的函数,不会使用任何额外的堆栈空间。
*/  
void CopyName(void)   
{   
    j1939_uint8_t i;   
   
    for (i=0; i<J1939_DATA_LENGTH; i++)   
    	OneMessage.Mxe.Data[i]= CA_Name[i];
}   

/*
*输入：
*输出：
*说明：设置过滤器为指定值，起到过滤地址作用。(参考CAN2.0B的滤波器)
       滤波函数的设置段为PS段
*/    
void SetAddressFilter( j1939_uint8_t Address )   
{   
   Port_SetAddressFilter(Address);
}   
/*
*输入： J1939_MESSAGE far *
*输出：
*说明： 发送*MsgPtr的信息，
        所有的数据字段（比如数据长度、优先级、和源地址）必须已经设置。
        在调用这个函数之前，设备在总线上声明的地址是正确的。
*/   
void SendOneMessage( J1939_MESSAGE *MsgPtr )   
{    
    //设置消息的最后部分,确保DataLength规范。（参考CAN B2.0）
	MsgPtr->Mxe.Res = 0;//参考J1939的数据链路层（SAE J1939-21）
	MsgPtr->Mxe.RTR = 0;
    if (MsgPtr->Mxe.DataLength > 8)
    	MsgPtr->Mxe.DataLength = 8;
    //发送一帧消息，将 J1939_MESSAGE 中的所有消息加载道can模块自有的结构中     
     Port_CAN_Transmit(MsgPtr);
}   
/*
*输入： ADDRESS_CLAIM_RX 或 ADDRESS_CLAIM_TX
*输出：
*说明：//J1939地址请求处理  （参考J1939的网络层）
    这段程序被调用，当CA必须要求其地址在总线上或另一个CA是试图声称相同的地址在总线上,
    我们必须捍卫自己或放弃的地址。
    如果CA的私有范围有一个地址0 - 127或248 - 248,它可以立即解决。
*补充：
    ADDRESS_CLAIM_RX表示一个地址声明消息已经收到,这个CA必须保卫或放弃其地址。
    ADDRESS_CLAIM_TX表明CA是初始化一个声明其地址。
*/      
static void J1939_AddressClaimHandling( j1939_uint8_t Mode )   
{   
    OneMessage.Mxe.Priority = J1939_CONTROL_PRIORITY;
    OneMessage.Mxe.PDUFormat = J1939_PF_ADDRESS_CLAIMED;
    OneMessage.Mxe.DestinationAddress = J1939_GLOBAL_ADDRESS;
    OneMessage.Mxe.DataLength = J1939_DATA_LENGTH;

    if (Mode == ADDRESS_CLAIM_TX)   
        goto SendAddressClaim;   
   
    if (OneMessage.Mxe.SourceAddress != J1939_Address)
        return;   
    /*如果我们的设备名，比网络中竞争的小*/
    if (CompareName( OneMessage.Mxe.Data ) != -1)
    {
        //配置ECU为，可以是随机地址
        #if J1939_ARBITRARY_ADDRESS != 0x00   
            if (ECU_RecalculateAddress( &CommandedAddress ))   
                goto SendAddressClaim;   
        #endif   
   
        // 发送一个地址申请帧
        CopyName();   
        OneMessage.Mxe.SourceAddress = J1939_NULL_ADDRESS;
        SendOneMessage( (J1939_MESSAGE *) &OneMessage );   
   
        //设置地址过滤器为J1939_GLOBAL_ADDRESS。  
        SetAddressFilter( J1939_GLOBAL_ADDRESS );   
   
        J1939_Flags.CannotClaimAddress = 1;   
        J1939_Flags.WaitingForAddressClaimContention = 0;   
        return;   
    }   
   
SendAddressClaim:
    //发送一个请求地址消息。（申请本地的地址为CommandedAddress）
    CopyName();   
    OneMessage.Mxe.SourceAddress = CommandedAddress;

    SendOneMessage( (J1939_MESSAGE *) &OneMessage );   
   
    if (((CommandedAddress & 0x80) == 0) ||         // Addresses 0-127   
        ((CommandedAddress & 0xF8) == 0xF8))        // Addresses 248-253 (254,255 illegal)   
    {   
        J1939_Flags.CannotClaimAddress = 0;   
        J1939_Address = CommandedAddress;   

        //设置地址过滤器为J1939_Address。
        SetAddressFilter( J1939_Address );   
    }   
    else   
    {   
        //我们没有一个专有的地址,所以我们需要等待。  
        J1939_Flags.WaitingForAddressClaimContention = 1;   
        ContentionWaitTime = 0;   
    }   
}
 /*
*输入：
*输出: RC_SUCCESS          消息列中移除成功
       RC_QUEUEEMPTY       没有消息返回 
       RC_CANNOTRECEIVE    目前系统没有接受到消息，应为不能在网络中申请到地址
*说明：从接受队列中读取一个信息到*MsgPtr。如果我们用的是中断，需要将中断失能，在获取接受队列数据时
*/    
j1939_uint8_t J1939_DequeueMessage( J1939_MESSAGE *MsgPtr )   
{   
    j1939_uint8_t   rc = RC_SUCCESS;   

 //***************************关接受中断********************************  
 #if J1939_POLL_ECAN == J1939_FALSE
    Port_RXinterruptDisable();
 #endif  
    if (RXQueueCount == 0)   
    {   
        if (J1939_Flags.CannotClaimAddress)   
            rc = RC_CANNOTRECEIVE;   
        else   
            rc = RC_QUEUEEMPTY;   
    }   
    else   
    {   
        *MsgPtr = RXQueue[RXHead];   
        RXHead ++;   
        if (RXHead >= J1939_RX_QUEUE_SIZE)   
            RXHead = 0;   
        RXQueueCount --;   
    }   
  //***************************开接受中断********************************   
#if J1939_POLL_ECAN == J1939_FALSE
   Port_RXinterruptEnable();
#endif

   return rc;   
}   
/*
*输入： J1939_MESSAGE *     用户要入队的消息 
*输出:  RC_SUCCESS          消息入队成功 
        RC_QUEUEFULL        发送列队满，消息入队失败 
        RC_CANNOTTRANSMIT   系统目前不能发送消息  
*说明：这段程序，将*MsgPtr放入发送消息列队中
       如果信息不能入队或者发送，将有一个相应的返回提示，
        如果发送中断被设置（可用），当消息列队后，发送中断被使能
*/   
j1939_uint8_t J1939_EnqueueMessage( J1939_MESSAGE *MsgPtr )   
{   
    j1939_uint8_t   rc = RC_SUCCESS;   

#if J1939_POLL_ECAN == J1939_FALSE  
    Port_TXinterruptDisable();
#endif 
   
    if (J1939_Flags.CannotClaimAddress)   
        rc = RC_CANNOTTRANSMIT;   
    else   
    {   
        if ((J1939_OVERWRITE_TX_QUEUE == J1939_TRUE) ||   
             (TXQueueCount < J1939_TX_QUEUE_SIZE))   
        {   
            if (TXQueueCount < J1939_TX_QUEUE_SIZE)   
            {   
                TXQueueCount ++;   
                TXTail ++;   
                if (TXTail >= J1939_TX_QUEUE_SIZE)   
                    TXTail = 0;   
            }   
            TXQueue[TXTail] = *MsgPtr;   
        }   
        else   
            rc = RC_QUEUEFULL;   
    }   

#if J1939_POLL_ECAN == J1939_FALSE   
    Port_TXinterruptEnable();
    //触发发送中断
    Port_TXinterruptOk();
#endif   
    return rc;   
}   
/*
*输入： InitNAMEandAddress  是否需要初始化标识符
*输出:   
*说明： 这段代码被调用，在系统初始化中。（放在CAN设备初始化之后）
        初始化J1939全局变量
        然后在总线上，声明设备自己的地址
        如果设备需要初始化自己的标识符和地址，将InitNAMEandAddress置位。
*/   

void J1939_Initialization( j1939_uint8_t InitNAMEandAddress )   
{
    /*初始化全局变量*/   
    J1939_Flags.FlagVal = 1; // 没有声明地址，同事其他的标识位将被设置为0（复位）
    ContentionWaitTime = 0l; //初始化地址竞争等待时间

    /*初始化接受和发送列队*/
    TXQueueCount = 0; 
    TXHead = 0;   
    TXTail = 0xFF;     
    RXHead = 0;   
    RXTail = 0xFF;   
    RXQueueCount = 0;
    /*将TP协议置为空闲*/
#if J1939_TP_RX_TX
    J1939_TP_Flags_t.state = J1939_TP_NULL;

    TP_TX_MSG.packets_request_num = 0;
    TP_TX_MSG.packets_total = 0;
	TP_TX_MSG.packet_offset_p = 0;
	TP_TX_MSG.time = 0;
	TP_TX_MSG.state = J1939_TP_TX_WAIT;

    TP_RX_MSG.packets_ok_num = 0;
	TP_RX_MSG.packets_total = 0;
	TP_RX_MSG.time = 0;
	TP_RX_MSG.state = J1939_TP_RX_WAIT;
#endif
    if (InitNAMEandAddress)   
    {   
        J1939_Address = J1939_STARTING_ADDRESS;   
        CA_Name[7] = J1939_CA_NAME7;   
        CA_Name[6] = J1939_CA_NAME6;   
        CA_Name[5] = J1939_CA_NAME5;   
        CA_Name[4] = J1939_CA_NAME4;   
        CA_Name[3] = J1939_CA_NAME3;   
        CA_Name[2] = J1939_CA_NAME2;   
        CA_Name[1] = J1939_CA_NAME1;   
        CA_Name[0] = J1939_CA_NAME0;   
    }   
    CommandedAddress = J1939_Address;   
       
    J1939_AddressClaimHandling( ADDRESS_CLAIM_TX );
}   
/*
*输入： 
*输出:   
*说明： 这个函数被调用，当设备产生CAN中断（可能是接受中断，也可能是发送中断）
        首先我们要清除中断标识位
        然后调用接受或者发送函数。
*/     
#if J1939_POLL_ECAN == J1939_FALSE   
void J1939_ISR( void )   
{   
    //判断相关标识位,是接受还是发送
    //清除标识位
    Port_CAN_identifier_clc();
    //调用相关的处理函数    
    J1939_ReceiveMessages();   
    J1939_TransmitMessages();   
    #if J1939_TP_RX_TX
        J1939_TP_Poll();
    #endif //J1939_TP_RX_TX
    //可能存在因为错误产生中断，直接清除相关的标识位 
}   
#endif   
/*
*输入： j1939_uint8_t   一个大概的毫秒数，通常设置 5 或 3 
*输出:   
*说明：如果我们采用轮询的方式获取信息，这个函数每几个毫秒将被调用一次。
        不断的接受消息和发送消息从消息队列中
        此外，如果我们正在等待一个地址竞争反应。 
        如果超时，我们只接收特定的消息（目标地址 = J1939_Address）
  
        如果设备使用中断，此函数被调用，在调用J1939_Initialization（）函数后，因为
        J1939_Initialization（）可能初始化WaitingForAddressClaimContention标识位为1.

        如果接受到命令地址消息，这个函数也必须被调用，以防万一总线要求我们改变地址

        如果使用中断模式，本程序将不会处理接受和发送消息，只处理地址竞争超时。
*/     
void J1939_Poll( j1939_uint32_t ElapsedTime )   
{
    //更新的竞争等待时间
    ContentionWaitTime += ElapsedTime;   
    //我们必须调用J1939_ReceiveMessages接受函数，在时间被重置为0之前。
    #if J1939_POLL_ECAN == J1939_TRUE
        J1939_ReceiveMessages();
        J1939_TransmitMessages();
	#if J1939_TP_RX_TX
        J1939_TP_Poll();
	#endif //J1939_TP_RX_TX
    #endif   

//当ECU需要竞争地址时，WaitingForAddressClaimContention在初始化中置位，并运行下面语句
//如果我们正在等待一个地址竞争反应。 并且超时，我们只接收特定的消息（目标地址 = J1939_Address）
    if (J1939_Flags.WaitingForAddressClaimContention &&   
        (ContentionWaitTime >= 250000l))   
    {   
        J1939_Flags.CannotClaimAddress = 0;   
        J1939_Flags.WaitingForAddressClaimContention = 0;   
        J1939_Address = CommandedAddress;   

        //如果我们使用中断,确保中断是禁用的,因为它会打乱我们程序逻辑
#if J1939_POLL_ECAN == J1939_FALSE
        Port_RXinterruptDisable();
        Port_TXinterruptDisable();
#endif
        //设置接收滤波器地址为设备地址。
        SetAddressFilter( J1939_Address );   
        //开启中断
#if J1939_POLL_ECAN == J1939_FALSE
        Port_TXinterruptEnable();
        Port_RXinterruptEnable();
#endif

    }   
}   
/*
*输入： 
*输出:   
*说明： 这段程序被调用，当CAN收发器接受数据后从中断 或者 轮询。
        如果一个信息被接受, 它将被调用
        如果信息是一个网络管理信息或长帧传输（TP），接受的信息将被加工处理，在这个函数中。
        否则, 信息将放置在用户的接收队列。
        注意：在这段程序运行期间中断是失能的。
        注意：为了节省空间，函数J1939_CommandedAddressHandling（）采用内联的函数
*/   
static void J1939_ReceiveMessages( void )   
{
#if J1939_TP_RX_TX
	j1939_uint32_t _pgn = 0;
#endif //J1939_TP_RX_T
    /*从接收缓存中读取信息到OneMessage中，OneMessage是一个全局变量*/
    /*Port_CAN_Receive函数读取到数据返回1，没有数据则返回0*/
    if(Port_CAN_Receive(&OneMessage))
    {
        switch( OneMessage.Mxe.PDUFormat)
        { 
#if (J1939_TP_RX_TX || J1939_ACCEPT_CMDADD)
			case J1939_PF_TP_CM:       //参考J1939-21 TP多帧传输协议
#if J1939_ACCEPT_CMDADD == J1939_TRUE
                //参考J1939-81 地址命令配置
                if ((OneMessage.Mxe.Data[0] == J1939_BAM_CONTROL_BYTE) &&
                    (OneMessage.Mxe.Data[5] == J1939_PGN0_COMMANDED_ADDRESS) &&
                    (OneMessage.Mxe.Data[6] == J1939_PGN1_COMMANDED_ADDRESS) &&
                    (OneMessage.Mxe.Data[7] == J1939_PGN2_COMMANDED_ADDRESS))
                {   
                    J1939_Flags.GettingCommandedAddress = 1;   
                    CommandedAddressSource = OneMessage.Mxe.SourceAddress;
                    break;
                }
#endif//J1939_ACCEPT_CMDADD
#if J1939_TP_RX_TX
                _pgn = (j1939_uint32_t)((OneMessage.Mxe.Data[7]<<16)&0xFF0000)
                						+(j1939_uint32_t)((OneMessage.Mxe.Data[6]<<8)&0xFF00)
                						+(j1939_uint32_t)((OneMessage.Mxe.Data[5])&0xFF);
                if((J1939_TP_Flags_t.state == J1939_TP_NULL) && (TP_RX_MSG.state == J1939_TP_RX_WAIT))
				{

                    if(OneMessage.Mxe.Data[0] == 16)
                    {
                    	J1939_TP_Flags_t.state = J1939_TP_RX;

                    	TP_RX_MSG.tp_rx_msg.SA = OneMessage.Mxe.SourceAddress;
                    	TP_RX_MSG.tp_rx_msg.PGN = (j1939_uint32_t)((OneMessage.Mxe.Data[7]<<16)&0xFF0000)
                            						+(j1939_uint32_t)((OneMessage.Mxe.Data[6]<<8)&0xFF00)
                            						+(j1939_uint32_t)((OneMessage.Mxe.Data[5])&0xFF);
                    	/*如果系统繁忙*/
                    	if(TP_RX_MSG.osbusy)
                    	{
                    		TP_RX_MSG.state = J1939_TP_RX_ERROR;
                    		break;
                    	}
                    	/*判断是否有足够的内存接收数据，如果没有直接，断开连接*/
                    	if(((j1939_uint32_t)((OneMessage.Mxe.Data[2]<<8)&0xFF00)
                    			+(j1939_uint32_t)((OneMessage.Mxe.Data[1])&0xFF)) > J1939_TP_MAX_MESSAGE_LENGTH)
                    	{
                    		TP_RX_MSG.state = J1939_TP_RX_ERROR;
                    		break;
                    	}
                    	TP_RX_MSG.tp_rx_msg.byte_count = ((j1939_uint32_t)((OneMessage.Mxe.Data[2]<<8)&0xFF00)
                    									 +(j1939_uint32_t)((OneMessage.Mxe.Data[1])&0xFF));
                    	TP_RX_MSG.packets_total = OneMessage.Mxe.Data[3];
                    	TP_RX_MSG.time = J1939_TP_T2;
                    	TP_RX_MSG.state = J1939_TP_RX_READ_DATA;
                    	break;
                    }
                    goto PutInReceiveQueue;
                    break;
				}
                if(J1939_TP_Flags_t.state == J1939_TP_TX)
				{
					/*校验PGN*/
					if (_pgn == TP_TX_MSG.tp_tx_msg.PGN)
					{
		    		    switch(OneMessage.Mxe.Data[0])
		        		{
		        			case J1939_RTS_CONTROL_BYTE:
		            			/* 程序运行到这里，说明已经与网络中设备1建立虚拟链接（作为发送端），但是收到设备2的链接请求，并且同一个PGN消息请求*/
		            			/* 根据J1939-21数据链路层的规定，我们要保持原有的链接，不做任何事，设备2会应为超时自动放弃链接*/
		            			break;
		        			case J1939_CTS_CONTROL_BYTE:
								if((J1939_TP_TX_CM_WAIT == TP_TX_MSG.state) || (J1939_TP_WAIT_ACK == TP_TX_MSG.state))
								{
									/* 发送等待保持 */
									if(0x00u == OneMessage.Mxe.Data[1])
									{
										/* 刷新等待计数器 */
										TP_TX_MSG.time = J1939_TP_T4;
									}
									else
									{
										if((OneMessage.Mxe.Data[2]+OneMessage.Mxe.Data[1]) > (TP_TX_MSG.packets_total+1))
										{
											/*请求超出数据包范围*/
											TP_TX_MSG.state = J1939_TP_TX_ERROR;
										}
										else
										{ /* response parameter OK */
											TP_TX_MSG.packets_request_num = OneMessage.Mxe.Data[1];
											TP_TX_MSG.packet_offset_p = (j1939_uint8_t)(OneMessage.Mxe.Data[2] - 1);
											TP_TX_MSG.state = J1939_TP_TX_DT;
										}

									}
								}
		            			break;
		        			case J1939_EOMACK_CONTROL_BYTE:
		        				if(J1939_TP_WAIT_ACK == TP_TX_MSG.state)
								{
		        					TP_TX_MSG.state = J1939_TX_DONE;
								}
		        				//这里可以增加一个对数据的校验
		            			break;
		        			case J1939_CONNABORT_CONTROL_BYTE:
		        				//收到一个放弃连接，什么都不做，协议会在一段延时时间后主动放弃链接
		            			break;
		        			default:
		            			break;
		        		}
					}
				}
#endif//J1939_TP_RX_TX
                goto PutInReceiveQueue;
				break;
#endif //(J1939_TP_RX_TX || J1939_ACCEPT_CMDADD)
            /*远程对ECU的地址配置*/
#if J1939_TP_RX_TX || J1939_ACCEPT_CMDADD
            case J1939_PF_DT:   
#if J1939_ACCEPT_CMDADD
                if ((J1939_Flags.GettingCommandedAddress == 1) &&   
                    (CommandedAddressSource == OneMessage.Mxe.SourceAddress))
                {     
                    if ((!J1939_Flags.GotFirstDataPacket) &&   
                        (OneMessage.Mxe.Data[0] == 1))
                    {   
                        for (Loop=0; Loop<7; Loop++)   
                            CommandedAddressName[Loop] = OneMessage.Mxe.Data[Loop+1];
                        J1939_Flags.GotFirstDataPacket = 1;
                        break;
                    }   
                    else if ((J1939_Flags.GotFirstDataPacket) &&   
                        (OneMessage.Mxe.Data[0] == 2))
                    {   
                        CommandedAddressName[7] = OneMessage.Mxe.Data[1];
                        CommandedAddress =OneMessage.Mxe.Data[2];
                        // 确保消息是针对与我们的。然后可以改变地址。
                        if ((CompareName( CommandedAddressName ) == 0) && CA_AcceptCommandedAddress())   
                            J1939_AddressClaimHandling( ADDRESS_CLAIM_TX );   
                        J1939_Flags.GotFirstDataPacket = 0;   
                        J1939_Flags.GettingCommandedAddress = 0;
                        break;
                    }
                }
#endif//J1939_ACCEPT_CMDADD
#if J1939_TP_RX_TX
                if((TP_RX_MSG.state == J1939_TP_RX_DATA_WAIT)&&(TP_RX_MSG.tp_rx_msg.SA == OneMessage.Mxe.SourceAddress))
                {
                	TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0]-1)*7u]=OneMessage.Mxe.Data[1];
                	TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0]-1)*7u+1]=OneMessage.Mxe.Data[2];
                	TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0]-1)*7u+2]=OneMessage.Mxe.Data[3];
                	TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0]-1)*7u+3]=OneMessage.Mxe.Data[4];
                	TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0]-1)*7u+4]=OneMessage.Mxe.Data[5];
                	TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0]-1)*7u+5]=OneMessage.Mxe.Data[6];
                	TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0]-1)*7u+6]=OneMessage.Mxe.Data[7];
					/*特殊处理重新接受已接受过的数据包*/
                	if((OneMessage.Mxe.Data[0]) > TP_RX_MSG.packets_ok_num)
					{
                		TP_RX_MSG.packets_ok_num++;
					}
					TP_RX_MSG.time = J1939_TP_T1;
					/*判断是否收到偶数个数据包或者读取到最后一个数据包*/
					if((TP_RX_MSG.packets_ok_num%2 == 0) ||(TP_RX_MSG.packets_ok_num == TP_RX_MSG.packets_total))
					{
						TP_RX_MSG.state = J1939_TP_RX_READ_DATA;
						break ;
					}
					break ;
                }
                //程序不可能运行到这，但是我们不能放弃接受的数据包
                goto PutInReceiveQueue;
#endif//J1939_TP_RX_TX
#endif//(J1939_TP_RX_TX || J1939_ACCEPT_CMDADD)
            case J1939_PF_REQUEST:   
                if ((OneMessage.Mxe.Data[0] == J1939_PGN0_REQ_ADDRESS_CLAIM) &&
                    (OneMessage.Mxe.Data[1] == J1939_PGN1_REQ_ADDRESS_CLAIM) &&
                    (OneMessage.Mxe.Data[2] == J1939_PGN2_REQ_ADDRESS_CLAIM))
                    J1939_RequestForAddressClaimHandling();   
                else   
                    goto PutInReceiveQueue;   
                break;   
            case J1939_PF_ADDRESS_CLAIMED:   
                J1939_AddressClaimHandling( ADDRESS_CLAIM_RX );   
                break;   
            default:   
PutInReceiveQueue:   
                if ( (J1939_OVERWRITE_RX_QUEUE == J1939_TRUE) ||   
                    (RXQueueCount < J1939_RX_QUEUE_SIZE))   
                {   
                    if (RXQueueCount < J1939_RX_QUEUE_SIZE)   
                    {   
                        RXQueueCount ++;   
                        RXTail ++;   
                        if (RXTail >= J1939_RX_QUEUE_SIZE)   
                            RXTail = 0;   
                    }   
                    RXQueue[RXTail] = OneMessage;   
                }   
                else   
                    J1939_Flags.ReceivedMessagesDropped = 1;   
        }   
    }

}   
/*
*输入： 
*输出:   
*说明： 如果接收到地址声明请求，这个函数将被调用
        如果我们的设备不需要发送一个地址声明，我们将不发送地址声明消息，否则，反之。
        为了减少代码大小。两条消息之间只有源地址的变化。
*/     
  
static void J1939_RequestForAddressClaimHandling( void )   
{   
    if (J1939_Flags.CannotClaimAddress)   
    	OneMessage.Mxe.SourceAddress = J1939_NULL_ADDRESS;  //发送一个不能声明的地址消息
    else   
    	OneMessage.Mxe.SourceAddress = J1939_Address;       //发送一个当前地址的声明
   
    OneMessage.Mxe.Priority = J1939_CONTROL_PRIORITY;
    OneMessage.Mxe.PDUFormat = J1939_PF_ADDRESS_CLAIMED;    // 同 J1939_PF_CANNOT_CLAIM_ADDRESS值 一样
    OneMessage.Mxe.DestinationAddress = J1939_GLOBAL_ADDRESS;
    OneMessage.Mxe.DataLength = J1939_DATA_LENGTH;
    CopyName();   

    SendOneMessage( (J1939_MESSAGE *) &OneMessage );   
}   
/*
*输入： 
*输出:  RC_SUCCESS          信息发送成功
        RC_CANNOTTRANSMIT   系统没有发送消息，因为设备没有申请到地址，或者没有要发送的消息
*说明：  调用这个函数后，如果发送消息列队中有消息就位，则会发送消息 ，如果不能发送消息，相关的错误代码将返回。
        程序运行期间，中断是被失能的。 
*/    
static j1939_uint8_t J1939_TransmitMessages( void )   
{   
    if (TXQueueCount == 0)   
    {   
        //如果没有要发送的消息从发送消息列队中，恢复中断(清空发送标识位)
#if J1939_POLL_ECAN == J1939_FALSE
        Port_TXinterruptEnable();
#endif
        return RC_CANNOTTRANSMIT;
    }   
    else   
    {  
        //设备没有正确的声明到地址 
        if (J1939_Flags.CannotClaimAddress)   
            return RC_CANNOTTRANSMIT;   

        while(TXQueueCount > 0)
        {
            /*确保上次数据发送成功*/
            /**************可增加一个判断函数**************************/

            TXQueue[TXHead].Mxe.SourceAddress = J1939_Address;
            SendOneMessage( (J1939_MESSAGE *) &(TXQueue[TXHead]) );   
            TXHead ++;   
            if (TXHead >= J1939_TX_QUEUE_SIZE)   
                TXHead = 0;   
            TXQueueCount --;  
        }
     
       /*配置了一些标识位，使能中断*/
#if J1939_POLL_ECAN == J1939_FALSE
        Port_TXinterruptEnable();
#endif
    }   
    return RC_SUCCESS;   
}
#if J1939_TP_RX_TX
/*
*输入：
*输出:
*说明：  发送TP.DT，参考J1939-21，
*/
void J1939_TP_DT_Packet_send(void)
{
	J1939_MESSAGE _msg;
	j1939_uint16_t _packet_offset_p;
	j1939_int32_t _i=0;
	_msg.Mxe.Priority = J1939_TP_DT_PRIORITY;
	_msg.Mxe.DataPage =0;
	_msg.Mxe.PDUFormat = J1939_PF_DT;
	_msg.Mxe.DestinationAddress = TP_TX_MSG.tp_tx_msg.SA;
	_msg.Mxe.DataLength = 8;


    /*获取请求发送的数据包数量*/
    if(TP_TX_MSG.packets_request_num > 0)
    {
    	TP_TX_MSG.packets_request_num--;
    	/*获取数据偏移指针*/
    	_packet_offset_p = (j1939_uint16_t)(TP_TX_MSG.packet_offset_p*7u);
    	/*加载数据包编号*/
    	_msg.Mxe.Data[0] = (j1939_uint8_t)(1u + TP_TX_MSG.packet_offset_p);

        for(_i = 0; _i<7; _i++)
        {
        	_msg.Mxe.Data[_i+1] = TP_TX_MSG.tp_tx_msg.data[_packet_offset_p + _i];
        }
        /*是否是最后一包数据消息*/
        if(TP_TX_MSG.packet_offset_p == (TP_TX_MSG.packets_total - 1u))
        {
        	/*参数群是否能被填满，是否需要填充，*/
            if ( _packet_offset_p > TP_TX_MSG.tp_tx_msg.byte_count - 7 )
            {
            	/*计算需要填充的数据数*/
            	_i = TP_TX_MSG.tp_tx_msg.byte_count- _packet_offset_p - 7 ;

                for (    ; _i < 0  ; _i++ )
                {
                	/*我们默认J1939的参数群大小为8*/
                	_msg.Mxe.Data[_i+8] = J1939_RESERVED_BYTE ;
                }
            }


            TP_TX_MSG.packets_request_num = 0;
            TP_TX_MSG.packet_offset_p = 0;
            TP_TX_MSG.time = J1939_TP_T3;
            /* 跳转步骤，等待结束确认或则重新发送数据请求*/
            TP_TX_MSG.state = J1939_TP_WAIT_ACK;
        }
        else
        {
        	/*为下一个数据发送做准备*/
        	TP_TX_MSG.packet_offset_p++;
        	TP_TX_MSG.state = J1939_TP_TX_DT;
        }

        while (J1939_EnqueueMessage( &_msg ) != RC_SUCCESS)
        	J1939_Poll(5);
    }
    else
    {

    	TP_TX_MSG.packets_request_num = 0;
    	TP_TX_MSG.packet_offset_p = 0;
    	TP_TX_MSG.time = J1939_TP_T3;
    	TP_TX_MSG.state = J1939_TP_WAIT_ACK;
    }

}
/*
*输入：
*输出:
*说明：  发送TP。CM-RTS,16,23,4,255,PGN消息，参考J1939-21，
*/
void J1939_CM_Start(void)
{
	j1939_uint32_t pgn_num;
	J1939_MESSAGE _msg;

    pgn_num = TP_TX_MSG.tp_tx_msg.PGN;

    _msg.Mxe.Priority = J1939_TP_CM_PRIORITY;
    _msg.Mxe.DataPage =0;
    _msg.Mxe.PDUFormat = J1939_PF_TP_CM;
    _msg.Mxe.DestinationAddress = TP_TX_MSG.tp_tx_msg.SA;
    _msg.Mxe.DataLength = 8;
    _msg.Mxe.Data[0] = J1939_RTS_CONTROL_BYTE;
    _msg.Mxe.Data[1] = (j1939_uint8_t) TP_TX_MSG.tp_tx_msg.byte_count ;
    _msg.Mxe.Data[2] = (j1939_uint8_t) ((TP_TX_MSG.tp_tx_msg.byte_count)>>8);
    _msg.Mxe.Data[3] = TP_TX_MSG.packets_total;
    _msg.Mxe.Data[4] = J1939_RESERVED_BYTE;
    _msg.Mxe.Data[7] = (j1939_uint8_t)((pgn_num>>16) & 0xff);
    _msg.Mxe.Data[6] = (j1939_uint8_t)(pgn_num>>8 & 0xff);
    _msg.Mxe.Data[5] = (j1939_uint8_t)(pgn_num & 0xff);

	while (J1939_EnqueueMessage( &_msg ) != RC_SUCCESS)
		J1939_Poll(5);

	/*刷新等待时间，触发下一个步骤（）*/
    TP_TX_MSG.time = J1939_TP_T3;
    TP_TX_MSG.state = J1939_TP_TX_CM_WAIT;

}
/*
*输入：
*输出:
*说明：  中断TP链接
*/
void J1939_TP_TX_Abort(void)
{
	J1939_MESSAGE _msg;
	j1939_uint32_t pgn_num;

	pgn_num = TP_TX_MSG.tp_tx_msg.PGN;

	_msg.Mxe.Priority = J1939_TP_CM_PRIORITY;
	_msg.Mxe.DataPage =0;
	_msg.Mxe.PDUFormat = J1939_PF_TP_CM;
	_msg.Mxe.DestinationAddress = TP_TX_MSG.tp_tx_msg.SA;
	_msg.Mxe.DataLength = 8;
	_msg.Mxe.Data[0] = J1939_CONNABORT_CONTROL_BYTE;
	_msg.Mxe.Data[1] = J1939_RESERVED_BYTE;
	_msg.Mxe.Data[2] = J1939_RESERVED_BYTE;
	_msg.Mxe.Data[3] = J1939_RESERVED_BYTE;
	_msg.Mxe.Data[4] = J1939_RESERVED_BYTE;
	_msg.Mxe.Data[7] = (j1939_uint8_t)((pgn_num>>16) & 0xff);
	_msg.Mxe.Data[6] = (j1939_uint8_t)(pgn_num>>8 & 0xff);
	_msg.Mxe.Data[5] = (j1939_uint8_t)(pgn_num & 0xff);

	while (J1939_EnqueueMessage( &_msg ) != RC_SUCCESS)
		J1939_Poll(5);
	/*结束发送*/
    TP_TX_MSG.state = J1939_TX_DONE;

}
/*
*输入：
*输出:
*说明：  中断TP链接
*/
void J1939_TP_RX_Abort(void)
{
	J1939_MESSAGE _msg;
	j1939_uint32_t pgn_num;

	pgn_num = TP_RX_MSG.tp_rx_msg.PGN;

	_msg.Mxe.Priority = J1939_TP_CM_PRIORITY;
	_msg.Mxe.DataPage =0;
	_msg.Mxe.PDUFormat = J1939_PF_TP_CM;
	_msg.Mxe.DestinationAddress = TP_RX_MSG.tp_rx_msg.SA;
	_msg.Mxe.DataLength = 8;
	_msg.Mxe.Data[0] = J1939_CONNABORT_CONTROL_BYTE;
	_msg.Mxe.Data[1] = J1939_RESERVED_BYTE;
	_msg.Mxe.Data[2] = J1939_RESERVED_BYTE;
	_msg.Mxe.Data[3] = J1939_RESERVED_BYTE;
	_msg.Mxe.Data[4] = J1939_RESERVED_BYTE;
	_msg.Mxe.Data[7] = (j1939_uint8_t)((pgn_num>>16) & 0xff);
	_msg.Mxe.Data[6] = (j1939_uint8_t)(pgn_num>>8 & 0xff);
	_msg.Mxe.Data[5] = (j1939_uint8_t)(pgn_num & 0xff);

	while (J1939_EnqueueMessage( &_msg ) != RC_SUCCESS)
		J1939_Poll(5);
	/*结束发送*/
    TP_RX_MSG.state = J1939_RX_DONE;

}
/*
*输入：
*输出:
*说明：  TP的计时器
*/
j1939_uint8_t J1939_TP_TX_RefreshCMTimer(j1939_uint16_t dt_ms)
{
	if((J1939_TP_TX_CM_WAIT == TP_TX_MSG.state)||(J1939_TP_WAIT_ACK == TP_TX_MSG.state))
	{
		if(TP_TX_MSG.time > dt_ms)
		{
			TP_TX_MSG.time = TP_TX_MSG.time - dt_ms;
			return J1939_TP_TIMEOUT_NORMAL;
		}
		else
		{
			/*超时 */
			TP_TX_MSG.time = 0u;
			return  J1939_TP_IMEOUT_ABNORMAL;
		}

	}
	else
	{
		return  J1939_TP_TIMEOUT_NORMAL;
	}
}
/*
*输入：
*输出:
*说明：  TP的计时器
*/
j1939_uint8_t J1939_TP_RX_RefreshCMTimer(j1939_uint16_t dt_ms)
{
	if((J1939_TP_RX_DATA_WAIT == TP_RX_MSG.state))
	{
		if(TP_RX_MSG.time > dt_ms)
		{
			TP_RX_MSG.time = TP_RX_MSG.time - dt_ms;
			return J1939_TP_TIMEOUT_NORMAL;
		}
		else
		{
			/*超时 */
			TP_RX_MSG.time = 0u;
			return  J1939_TP_IMEOUT_ABNORMAL;
		}

	}
	else
	{
		return  J1939_TP_TIMEOUT_NORMAL;
	}
}
/*
*输入：
*输出:
*说明：  发送读取数据 TP.CM_CTS 和 EndofMsgAck消息。
*/
void J1939_read_DT_Packet()
{
	J1939_MESSAGE _msg;
	j1939_uint32_t pgn_num;
	pgn_num = TP_RX_MSG.tp_rx_msg.PGN;

	_msg.Mxe.Priority = J1939_TP_CM_PRIORITY;
	_msg.Mxe.DataPage =0;
	_msg.Mxe.PDUFormat = J1939_PF_TP_CM;
	_msg.Mxe.DestinationAddress = TP_RX_MSG.tp_rx_msg.SA;
	_msg.Mxe.DataLength = 8;

	/*如果系统繁忙,保持链接但是不传送消息*/
	if(TP_RX_MSG.osbusy)
	{
		_msg.Mxe.Data[0] = J1939_CTS_CONTROL_BYTE;
		_msg.Mxe.Data[1] = 0;
		_msg.Mxe.Data[2] = J1939_RESERVED_BYTE;
		_msg.Mxe.Data[3] = J1939_RESERVED_BYTE;
		_msg.Mxe.Data[4] = J1939_RESERVED_BYTE;
		_msg.Mxe.Data[7] = (j1939_uint8_t)((pgn_num>>16) & 0xff);
		_msg.Mxe.Data[6] = (j1939_uint8_t)(pgn_num>>8 & 0xff);
		_msg.Mxe.Data[5] = (j1939_uint8_t)(pgn_num & 0xff);
		while (J1939_EnqueueMessage( &_msg ) != RC_SUCCESS)
				J1939_Poll(5);
		return ;
	}
	if(TP_RX_MSG.packets_total > TP_RX_MSG.packets_ok_num)
	{
		/*最后一次响应，如果不足2包数据*/
		if((TP_RX_MSG.packets_total - TP_RX_MSG.packets_ok_num)==1)
		{
			_msg.Mxe.Data[0] = J1939_CTS_CONTROL_BYTE;
			_msg.Mxe.Data[1] = 1;
			_msg.Mxe.Data[2] = TP_RX_MSG.packets_total;
			_msg.Mxe.Data[3] = J1939_RESERVED_BYTE;
			_msg.Mxe.Data[4] = J1939_RESERVED_BYTE;
			_msg.Mxe.Data[7] = (j1939_uint8_t)((pgn_num>>16) & 0xff);
			_msg.Mxe.Data[6] = (j1939_uint8_t)(pgn_num>>8 & 0xff);
			_msg.Mxe.Data[5] = (j1939_uint8_t)(pgn_num & 0xff);
			while (J1939_EnqueueMessage( &_msg ) != RC_SUCCESS)
					J1939_Poll(5);
			TP_RX_MSG.state = J1939_TP_RX_DATA_WAIT;
			return ;
		}
		_msg.Mxe.Data[0] = J1939_CTS_CONTROL_BYTE;
		_msg.Mxe.Data[1] = 2;
		_msg.Mxe.Data[2] = (TP_RX_MSG.packets_ok_num + 1);
		_msg.Mxe.Data[3] = J1939_RESERVED_BYTE;
		_msg.Mxe.Data[4] = J1939_RESERVED_BYTE;
		_msg.Mxe.Data[7] = (j1939_uint8_t)((pgn_num>>16) & 0xff);
		_msg.Mxe.Data[6] = (j1939_uint8_t)(pgn_num>>8 & 0xff);
		_msg.Mxe.Data[5] = (j1939_uint8_t)(pgn_num & 0xff);

		while (J1939_EnqueueMessage( &_msg ) != RC_SUCCESS)
				J1939_Poll(5);
		TP_RX_MSG.state = J1939_TP_RX_DATA_WAIT;
		return ;
	}else
	{
		/*发送传输正常结束消息，EndofMsgAck*/
		_msg.Mxe.Data[0] = J1939_EOMACK_CONTROL_BYTE;
		_msg.Mxe.Data[1] = (TP_RX_MSG.tp_rx_msg.byte_count & 0x00ff);
		_msg.Mxe.Data[2] = ((TP_RX_MSG.tp_rx_msg.byte_count >> 8) & 0x00ff);
		_msg.Mxe.Data[3] = TP_RX_MSG.packets_total;
		_msg.Mxe.Data[4] = J1939_RESERVED_BYTE;
		_msg.Mxe.Data[7] = (j1939_uint8_t)((pgn_num>>16) & 0xff);
		_msg.Mxe.Data[6] = (j1939_uint8_t)(pgn_num>>8 & 0xff);
		_msg.Mxe.Data[5] = (j1939_uint8_t)(pgn_num & 0xff);
		while (J1939_EnqueueMessage( &_msg ) != RC_SUCCESS)
				J1939_Poll(5);
		TP_RX_MSG.state = J1939_RX_DONE;
		return ;
	}
}
/*
*输入：
*输出:
*说明：  TP协议的心跳，为了满足在总线的计时准确，10ms轮询一次   J1939_TP_TX_RefreshCMTimer(10)
*说明：  如果想要更高的分辨率，1ms轮询一次，但是要改下面计时函数  J1939_TP_TX_RefreshCMTimer(1)
*/
void J1939_TP_Poll()
{
	if(J1939_TP_Flags_t.state == J1939_TP_NULL || J1939_TP_Flags_t.state == J1939_TP_OSBUSY)
	{
		return ;
	}
	if(J1939_TP_Flags_t.state == J1939_TP_RX)
	{
		switch(TP_RX_MSG.state)
		{
		case J1939_TP_RX_WAIT:
			;
    		break;
		case J1939_TP_RX_READ_DATA:
			/*发送读取数据 TP.CM_CTS 和 EndofMsgAck消息*/
			J1939_read_DT_Packet();
			break;
		case J1939_TP_RX_DATA_WAIT:
			/*等待TP.DT帧传输的消息*/
			if(J1939_TP_IMEOUT_ABNORMAL == J1939_TP_RX_RefreshCMTimer(10))
			{
				/* 等待超时，发生连接异常，跳转到异常步骤 */
				TP_RX_MSG.state = J1939_TP_RX_ERROR;
			}
			break;
		case J1939_TP_RX_ERROR:
			J1939_TP_RX_Abort();
			break;
		case J1939_RX_DONE:
			TP_RX_MSG.packets_ok_num = 0;
			TP_RX_MSG.packets_total = 0;
			TP_RX_MSG.time = J1939_TP_T3;
			TP_RX_MSG.state = J1939_TP_RX_WAIT;
			J1939_TP_Flags_t.state = J1939_TP_NULL;
			break;
        default:
            break;
		}
		return ;
	}
	if(J1939_TP_Flags_t.state == J1939_TP_TX)
	{
		switch (TP_TX_MSG.state)
		{
			case J1939_TP_TX_WAIT:
				/*没有要发送的数据*/
	    		break;
			case J1939_TP_TX_CM_START:
				/*发送TP.CM_RTS帧传输的消息(参考j1939-21)*/
				J1939_CM_Start();
				break;
			case J1939_TP_TX_CM_WAIT:
	    		/*等待TP.CM_CTS帧传输的消息*/
				if(J1939_TP_IMEOUT_ABNORMAL == J1939_TP_TX_RefreshCMTimer(10))
				{
					/* 等待超时，发生连接异常，跳转到异常步骤 */
					TP_TX_MSG.state = J1939_TP_TX_ERROR;
				}
			break;
			case J1939_TP_TX_DT:
				J1939_TP_DT_Packet_send();
	    		break;
	        case J1939_TP_WAIT_ACK:
	        	/*等待TP.EndofMsgACK帧传输的消息*/
				if(J1939_TP_IMEOUT_ABNORMAL == J1939_TP_TX_RefreshCMTimer(10))
				{
					/* 等待超时，发生连接异常，跳转到异常步骤 */
					TP_TX_MSG.state = J1939_TP_TX_ERROR;
				}
	            break;
			case J1939_TP_TX_ERROR:
				J1939_TP_TX_Abort();
	    		break;
			case J1939_TX_DONE:
				TP_TX_MSG.packets_request_num = 0;
				TP_TX_MSG.packet_offset_p = 0;
				TP_TX_MSG.time = J1939_TP_T3;
				TP_TX_MSG.state = J1939_TP_TX_WAIT;
				J1939_TP_Flags_t.state = J1939_TP_NULL;
	    		break;
	        default:
	        	//程序不会运行到这里来，可以增加一个调试输出
	            break;
		}
		return ;
	}
}
/*
*输入： PGN				TP会话的参数群编号
*输入： SA					TP会话的目标地址
*输入： *data				TP会话的数据缓存地址
*输入： data_num		    TP会话的数据大小
*输出: RC_SUCCESS        成功打开TP链接，开始进入发送流程
*输出: RC_CANNOTTRANSMIT 不能发送，因为TP协议已经建立虚拟链接，并且未断开
*说明：  TP协议的发送函数
*/
j1939_int8_t J1939_TP_TX_Message(j1939_uint32_t PGN,j1939_uint8_t SA,j1939_int8_t *data,j1939_uint16_t data_num)
{
	j1939_uint16_t _byte_count =0;
	/*取得发送权限*/
	if(J1939_TP_Flags_t.state == J1939_TP_NULL)
	{
		J1939_TP_Flags_t.state = J1939_TP_TX;
	}else
	{
		return RC_CANNOTTRANSMIT;//不能发送，因为TP协议已经建立虚拟链接，并且未断开
	}

	TP_TX_MSG.tp_tx_msg.PGN = PGN;
	TP_TX_MSG.tp_tx_msg.SA = SA;
	TP_TX_MSG.tp_tx_msg.byte_count = data_num;
	for(_byte_count = 0;_byte_count < data_num;_byte_count++)
	{
		TP_TX_MSG.tp_tx_msg.data[_byte_count] = data[_byte_count];
	}
	TP_TX_MSG.packet_offset_p = 0;
	TP_TX_MSG.packets_request_num = 0;
	TP_TX_MSG.packets_total = data_num/7 +1;
	TP_TX_MSG.time = J1939_TP_T3;
	//触发开始CM_START
	TP_TX_MSG.state = J1939_TP_TX_CM_START;

	return RC_SUCCESS;
}
/*
*输入： data		读取数据的缓存
*输入： data_num  读取数据的缓存大小
*输出: RC_CANNOTRECEIVE 不能接受，TP协议正在接受数据中
*输出: RC_SUCCESS		读取数据成功
*说明：  TP的接受函数 , 接受缓存的大小必须大于接受数据的大小，建议初始化缓存大小用  J1939_TP_MAX_MESSAGE_LENGTH
*说明：  请正确带入 缓存区的大小，参数错误程序运行有风险
*应用实例：
		j1939_uint32_t data[J1939_TP_MAX_MESSAGE_LENGTH]
		while(J1939_TP_RX_Message( data，sizeof(data))==RC_SUCCESS)
		  J1939_Poll(5);
*/
j1939_int8_t J1939_TP_RX_Message(j1939_int8_t *data,j1939_uint16_t data_num)
{
	j1939_uint16_t _a = 0;
	/*判断是否能读取数据*/
	if(J1939_TP_Flags_t.state == J1939_TP_NULL)
	{
		J1939_TP_Flags_t.state = J1939_TP_OSBUSY;
	}else
	{
		return RC_CANNOTRECEIVE;//不能接受，TP协议正在接受数据中
	}
    //判断数据缓存够不够
	if(data_num < TP_RX_MSG.tp_rx_msg.byte_count)
	{
		return RC_CANNOTRECEIVE;//不能接受，缓存区太小
	}

	for(_a = 0;_a < data_num;_a++)
	{
		data[_a] = TP_RX_MSG.tp_rx_msg.data[_a];
	}

    /*丢弃读取过的数据*/
	TP_RX_MSG.tp_rx_msg.byte_count= 0u;
	TP_RX_MSG.tp_rx_msg.PGN = 0;

	/*释放TP接管权限*/
	if(J1939_TP_Flags_t.state == J1939_TP_OSBUSY)
	{
		J1939_TP_Flags_t.state = J1939_TP_NULL;
	}

	return RC_SUCCESS;
}
#endif
