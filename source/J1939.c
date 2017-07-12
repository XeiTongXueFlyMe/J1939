/*********************************************************************
 *
 *            J1939 Main Source Code
 *
 *********************************************************************
 * 文件名:        J1939.c
 *
 *	本程序是由XieTongXueFlyMe对现有的J1939协议文档分析，和对前辈的贡献总结和封装，写出
 *的一套开源的J1939驱动。
 *	本协议特点：
 *		1.易移植（不针对特定的CAN硬件，只要满足CAN2.0B即可）
 *		2.轻量级（可适应低端的MCU）
 *		3.支持多任务调用接口（可用于嵌入式系统）
 *		4.双模式（轮询或者中断，逻辑更加简单明了）
 *		5.不掉帧（数据采用收发列队缓存）
 *	协议参考文献：
 *		1.SAE J1939 J1939概述
 *		2.SAE J1939-01 卡车，大客车控制通信文档（大概的浏览J1939协议的用法）
 *		3.SAE J1939-11 物理层文档
 *		4.SAE J1939-13 物理层文档
 *		5.SAE J1939-15 物理层文档
 *		6.SAE J1939-21 数据链路层文档（定义信息帧的数据结构，编码规则）
 *		7.SAE J1939-31 网络层文档（定义网络层的链接协议）
 *		8.SAE J1939-71 应用层文档（定义常用物理参数格式）
 *		9.SAE J1939-73 应用层文档（用于故障诊断）
 *		10.SAE J1939-74 应用层文档（可配置信息）
 *		11.SAE J1939-75 应用层文档（发电机组和工业设备）
 *		12.SAE J1939-81 网络管理协议
 *
 *  源代码分析网址：
 *		http://blog.csdn.net/xietongxueflyme/article/details/74908563
 *
 *
 * Version     Date        Description
 * ----------------------------------------------------------------------
 * v1.00     2017/06/04    首个版本
 *
 *
 * Author               Date         changes
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *XieTongXueFlyMe       7/06/04      首个版本
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
unsigned char                   CA_Name[J1939_DATA_LENGTH];//设备的标称符（参考1939-81）
unsigned char                   CommandedAddress;   
#if J1939_ACCEPT_CMDADD == J1939_TRUE   
    unsigned char               CommandedAddressSource;   
    unsigned char               CommandedAddressName[J1939_DATA_LENGTH];   
#endif   
unsigned long                   ContentionWaitTime;   
unsigned char                   J1939_Address;   
J1939_FLAG                      J1939_Flags;   
J1939_MESSAGE                   OneMessage;   
//接受列队全局变量   
unsigned char                   RXHead;   
unsigned char                   RXTail;   
unsigned char                   RXQueueCount;   
J1939_MESSAGE                   RXQueue[J1939_RX_QUEUE_SIZE];   
//发送列队全局变量  
unsigned char                   TXHead;   
unsigned char                   TXTail;   
unsigned char                   TXQueueCount;   
J1939_MESSAGE                   TXQueue[J1939_TX_QUEUE_SIZE];   
      
// 函数声明   
#if J1939_ACCEPT_CMDADD == J1939_TRUE   
    BOOL CA_AcceptCommandedAddress( void );   
#endif   
   
#if J1939_ARBITRARY_ADDRESS != 0x00   
//地址竞争重新估算，设备在总线上要申请的地址
extern  BOOL ECU_RecalculateAddress( unsigned char * Address);   
#endif   

/*
*输入：unsigned char *     Array of NAME bytes 
*输出： -1 - CA_Name 是小于 OtherName
*       0  - CA_Name与OtherName 相等
*       1  - CA_Name 是大于 OtherName
*说明：比较传入的数组数据名称与设备当前名称存储在CA_Name。
*/
signed char CompareName( unsigned char *OtherName )   
{   
    unsigned char   i;   
   
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
    unsigned char i;   
   
    for (i=0; i<J1939_DATA_LENGTH; i++)   
    	OneMessage.Mxe.Data[i]= CA_Name[i];
}   

/*
*输入：
*输出：
*说明：设置过滤器为指定值，起到过滤地址作用。(参考CAN2.0B的滤波器)
       滤波函数的设置段为PS段
*/    
void SetAddressFilter( unsigned char Address )   
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
static void J1939_AddressClaimHandling( unsigned char Mode )   
{   
    OneMessage.Mxe.Priority = J1939_CONTROL_PRIORITY;
    OneMessage.Mxe.PDUFormat = J1939_PF_ADDRESS_CLAIMED;
    OneMessage.Mxe.DestinationAddress = J1939_GLOBAL_ADDRESS;
    OneMessage.Mxe.DataLength = J1939_DATA_LENGTH;

    if (Mode == ADDRESS_CLAIM_TX)   
        goto SendAddressClaim;   
   
    if (OneMessage.Mxe.SourceAddress != J1939_Address)
        return;   
   
    if (CompareName( OneMessage.Mxe.Data ) != -1) // Our CA_Name is not less
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
unsigned char J1939_DequeueMessage( J1939_MESSAGE *MsgPtr )   
{   
    unsigned char   rc = RC_SUCCESS;   

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
unsigned char J1939_EnqueueMessage( J1939_MESSAGE *MsgPtr )   
{   
    unsigned char   rc = RC_SUCCESS;   

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

void J1939_Initialization( BOOL InitNAMEandAddress )   
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
    //可能存在因为错误产生中断，直接清除相关的标识位 
}   
#endif   
/*
*输入： unsigned char   一个大概的毫秒数，通常设置 5 或 3 
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
void J1939_Poll( unsigned long ElapsedTime )   
{
    //更新的竞争等待时间
    ContentionWaitTime += ElapsedTime;   
    //我们必须调用J1939_ReceiveMessages接受函数，在时间被重置为0之前。
    #if J1939_POLL_ECAN == J1939_TRUE   
        J1939_ReceiveMessages();
        J1939_TransmitMessages();   
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
        如果信息是一个网络管理信息，接受的信息将被加工处理，在这个函数中。
        否则, 信息将放置在用户的接收队列。
        注意：在这段程序运行期间中断是失能的。
        注意：为了节省空间，函数J1939_CommandedAddressHandling（）采用内联的函数
*/   
static void J1939_ReceiveMessages( void )   
{   
    /*从接收缓存中读取信息到OneMessage中，OneMessage是一个全局变量*/
    /*Port_CAN_Receive函数读取到数据返回1，没有数据则返回0*/
    while(Port_CAN_Receive(&OneMessage))
    {
        switch( OneMessage.Mxe.PDUFormat)
        {   
#if J1939_ACCEPT_CMDADD == J1939_TRUE   
            case J1939_PF_TP_CM:   
                if ((OneMessage.Mxe.Data[0] == J1939_BAM_CONTROL_BYTE) &&
                    (OneMessage.Mxe.Data[5] == J1939_PGN0_COMMANDED_ADDRESS) &&
                    (OneMessage.Mxe.Data[6] == J1939_PGN1_COMMANDED_ADDRESS) &&
                    (OneMessage.Mxe.Data[7] == J1939_PGN2_COMMANDED_ADDRESS))
                {   
                    J1939_Flags.GettingCommandedAddress = 1;   
                    CommandedAddressSource = OneMessage.Mxe.SourceAddress;
                }   
                break;   
            case J1939_PF_DT:   
                if ((J1939_Flags.GettingCommandedAddress == 1) &&   
                    (CommandedAddressSource == OneMessage.Mxe.SourceAddress))
                {   // Commanded Address Handling   
                    if ((!J1939_Flags.GotFirstDataPacket) &&   
                        (OneMessage.Mxe.Data[0] == 1))
                    {   
                        for (Loop=0; Loop<7; Loop++)   
                            CommandedAddressName[Loop] = OneMessage.Mxe.Data[Loop+1];
                        J1939_Flags.GotFirstDataPacket = 1;   
                    }   
                    else if ((J1939_Flags.GotFirstDataPacket) &&   
                        (OneMessage.Mxe.Data[0] == 2))
                    {   
                        CommandedAddressName[7] = OneMessage.Mxe.Data[1];
                        CommandedAddress =OneMessage.Mxe.Data[2];
                        if ((CompareName( CommandedAddressName ) == 0) &&   // Make sure the message is for us.   
                            CA_AcceptCommandedAddress())                    // and we can change the address.   
                            J1939_AddressClaimHandling( ADDRESS_CLAIM_TX );   
                        J1939_Flags.GotFirstDataPacket = 0;   
                        J1939_Flags.GettingCommandedAddress = 0;   
                    }   
                    else    // This really shouldn't happen, but just so we don't drop the data packet   
                        goto PutInReceiveQueue;   
                }   
                else   
                    goto PutInReceiveQueue;   
                break;   
#endif   
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
static unsigned char J1939_TransmitMessages( void )   
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
