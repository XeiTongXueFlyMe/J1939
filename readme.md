# 简述：
  文档分为两个独立的文件，source文件存放协议栈，example存放J1939协议栈的移植示例，每个示例可单独编译运行。将不断的更新移植示例，接受网友合并推送的移植示例。
# 协议特性：
* 易移植（不针对特定的CAN硬件，只要满足CAN2.0B即可）
* 轻量级（可适应低端的MCU）
* 支持多任务调用接口（可用于嵌入式系统）
* 双模式（轮询或者中断，逻辑更加简单明了）
* 不掉帧（数据采用收发列队缓存）
# 功能：
* 消息广播
* 消息请求
* 消息确认，响应
* 群功能
* 专用传输A
* 专用传输B
* 地址声明竞争
* 远程地址配置
* 自动分配地址
* 多帧传输协议TP
# 协议格式：
* UTF-8-BOM	
# J1939协议栈接口
* J1939_Initialization(BOOL)
* J1939_ISR(void)
* J1939_Poll(unsigned long ElapsedTime)
* J1939_DequeueMessage(J1939_MESSAGE *MsgPtr)
* J1939_EnqueueMessage(J1939_MESSAGE *MsgPtr)
* J1939_TP_TX_Message(unsigned int PGN,unsigned char SA,char *data,unsigned short data_num)
* J1939_TP_RX_Message(char *data,unsigned short data_num)

# 源代码移植：
* <http://blog.csdn.net/xietongxueflyme/article/details/74923355>

# 协议应用示例：
* <http://blog.csdn.net/XieTongXueFlyMe/article/category/7081225>
	   
# 源代码分析网址：
* <http://blog.csdn.net/xietongxueflyme/article/details/74908563>
  
# 协议参考的资料：
* <http://download.csdn.net/detail/xietongxueflyme/9887994>

# 示例1：
* 轮询模式
* 备注：消息的发送接受简单的示例
```
void main( void )
{
    J1939_MESSAGE Msg;
    /*在CAN驱动初始化中，请配置好滤波，说明参考移植函数（滤波函数）*/
    /* can_init(); */

    J1939_Initialization( TRUE );
    //等待地址超时
    while (J1939_Flags.WaitingForAddressClaimContention)
        J1939_Poll(5);
    //运行到这里，说明地址已经声明好（设备已挂载到总线上）
    while (1)
    {
    /********发送数据(参考J1939的ID组成填充下面)***********/
		Msg.Mxe.DataPage = 0;
		Msg.Mxe.Priority = J1939_CONTROL_PRIORITY;
		Msg.Mxe.DestinationAddress = 0x0f;
		Msg.Mxe.DataLength = 8;
		Msg.Mxe.PDUFormat = 0xfe;
		Msg.Mxe.Data[0] = 1;
		Msg.Mxe.Data[1] = 2;
		Msg.Mxe.Data[2] = 3;
		Msg.Mxe.Data[3] = 4;
		Msg.Mxe.Data[4] = 5;
		Msg.Mxe.Data[5] = 6;
		Msg.Mxe.Data[6] = 7;
		Msg.Mxe.Data[7] = 8;
        while (J1939_EnqueueMessage( &Msg ) != RC_SUCCESS)
            J1939_Poll(5);
     /***********************处理接受数据*************************/
        while (RXQueueCount > 0)
        {
            J1939_DequeueMessage( &Msg );
            if (Msg.PDUFormat == 0x01)
                //你的功能码;
            else if (Msg.PDUFormat == 0x02)
                //你的功能码;
        }

        J1939_Poll(20);
    }
}
```
# 示例2：
* 轮询模式
* 备注：长帧的发送，TP的发送示例
```
void main( void )
{
    char data[100] = {1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7};
	
	/*在CAN驱动初始化中，请配置好滤波，说明参考移植函数（滤波函数）*/
    /* can_init(); */

    J1939_Initialization( TRUE );
    // 等待地址声明超时
    while (J1939_Flags.WaitingForAddressClaimContention)
          J1939_Poll(5);
    //地址已经声明好（设备已挂载到总线上）
    while(1)
    {
        /*发送一个长帧 data*/
        while(J1939_TP_TX_Message(65259,0XF1,data,sizeof(data))==RC_SUCCESS)
              J1939_Poll(5);
        osDelay(1); //基本单位为10ms * 1;
        J1939_Poll(20);
    }
}
```
# 示例3：
* 轮询模式
* 备注：长帧的接受，TP的接受示例
```
void main( void )
{
	//建议初始化缓存大小用  J1939_TP_MAX_MESSAGE_LENGTH
	char data[J1939_TP_MAX_MESSAGE_LENGTH] = {0};
	
	/*在CAN驱动初始化中，请配置好滤波，说明参考移植函数（滤波函数）*/
    /* can_init(); */

	J1939_Initialization( TRUE );
	// 等待地址声明超时
	while (J1939_Flags.WaitingForAddressClaimContention)
	    J1939_Poll(5);
	//地址已经声明好（设备已挂载到总线上）
	//最简单的示例
	while(1)
	{
        /*读取TP接受数据*/
        while(J1939_TP_RX_Message( data，sizeof(data))==RC_SUCCESS)
            J1939_Poll(5);

        osDelay(1); //基本单位为10ms * 1;
        J1939_Poll(20);
	}

	//完整的接受逻辑示例
	while(1)
	{
        /*判断有没有接受的TP到来*/
        if(TP_RX_MSG.tp_rx_msg.byte_count >0)
        {
            /*判断数据是否是我们想要的PGN*/
            if(TP_RX_MSG.tp_rx_msg.PGN == 我们想要的PGN)
            {
                while(J1939_TP_RX_Message( data，sizeof(data))==RC_SUCCESS)
                J1939_Poll(5);
            }
        }
        osDelay(1); //基本单位为10ms * 1;
        J1939_Poll(20);
	}
}
```
# 示例4：
* 中断模式
* 备注：消息确认，响应
```
void main()
{
   /*在CAN驱动初始化中，请配置好滤波，说明参考移植函数（滤波函数）*/
    /* can_init(); */
	
    J1939_Initialization( TRUE );
    //等待地址超时
    while (J1939_Flags.WaitingForAddressClaimContention)
        J1939_Poll(5);
    //设备确认总线上没有，竞争地址的设备存在
    while (1)
    {
        //判断接受列队中，存在多少个接受消息（RXQueueCount ）
        while (RXQueueCount > 0)
        {
            //读取接受列队中的数据到Msg （出队）
            J1939_DequeueMessage( &Msg );
            /*判断是否是数据请求帧*/
            if (Msg.Mxe.PDUFormat == J1939_PF_REQUEST)
            {
                //判断参数群是否被本设备支持
                if ((Msg.Mxe.Data[0] == J1939_PGN0_REQ_ENGINE_SPEED) &&
                     (Msg.Mxe.Data[1] == J1939_PGN1_REQ_ENGINE_SPEED) &&
                     (Msg.Mxe.Data[2] == J1939_PGN2_REQ_ENGINE_SPEED))
                {
                    if (某种原因不能响应)
                    {
                        /*********发送不能响应（参考J1939-21）*************/
                        Msg.Mxe.Priority            = J1939_ACK_PRIORITY;
                        Msg.Mxe.DataPage            = 0;
                        Msg.Mxe.PDUFormat           = J1939_PF_ACKNOWLEDGMENT;
                        Msg.Mxe.DestinationAddress  = Msg.SourceAddress;
                        Msg.Mxe.DataLength          = 8;
                        Msg.Mxe.Data[0]         = J1939_NACK_CONTROL_BYTE;
                        Msg.Mxe.Data[1]         = 0xFF;
                        Msg.Mxe.Data[2]         = 0xFF;
                        Msg.Mxe.Data[3]         = 0xFF;
                        Msg.Mxe.Data[4]         = 0xFF;
                        Msg.Mxe.Data[5]         = J1939_PGN0_REQ_ENGINE_SPEED;
                        Msg.Mxe.Data[6]         = J1939_PGN1_REQ_ENGINE_SPEED;
                        Msg.Mxe.Data[7]         = J1939_PGN2_REQ_ENGINE_SPEED;
                    }
                    else//相关的PGN（参数群的响应）
                    {
                    /*******************上传相关的参数群*****************/
                        Msg.Mxe.Priority    = J1939_INFO_PRIORITY;
                        Msg.Mxe.DataPage    = J1939_PGN2_REQ_ENGINE_SPEED & 0x01;
                        Msg.Mxe.PDUFormat   = J1939_PGN1_REQ_ENGINE_SPEED;
                        Msg.Mxe.GroupExtension = J1939_PGN0_REQ_ENGINE_SPEED;
                        Msg.Mxe.DataLength  = 1;
                        Msg.Mxe.Data[0]     = EngineSpeed;
                    }
                    while (J1939_EnqueueMessage( &Msg ) != RC_SUCCESS);
                }
            }
        }
    }
}

```
# 协议参考文献：
	1. SAE J1939 J1939概述
	2. SAE J1939-01 卡车，大客车控制通信文档（大概的浏览J1939协议的用法）
	3. SAE J1939-11 物理层文档
	4. SAE J1939-13 物理层文档
	5. SAE J1939-15 物理层文档
	6. SAE J1939-21 数据链路层文档（定义信息帧的数据结构，编码规则）
	7. SAE J1939-31 网络层文档（定义网络层的链接协议）
	8. SAE J1939-71 应用层文档（定义常用物理参数格式）
	9. SAE J1939-73 应用层文档（用于故障诊断）
	10. SAE J1939-74 应用层文档（可配置信息）
	11. SAE J1939-75 应用层文档（发电机组和工业设备）
	12. SAE J1939-81 网络管理协议
