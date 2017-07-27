# 简述：
  文档分为两个独立的文件，source文件存放协议栈，example存放J1939协议栈的移植示例，每个示例可单独编译运行。将不断的更新移植示例。
# 协议特性：
* 易移植（不针对特定的CAN硬件，只要满足CAN2.0B即可）
* 轻量级（可适应低端的MCU）
* 支持多任务调用接口（可用于嵌入式系统）
* 双模式（轮询或者中断，逻辑更加简单明了）
* 不掉帧（数据采用收发列队缓存）
# 功能：
* 地址声明竞争
* 消息广播
* 消息请求
* 消息确认，响应
* 群功能
* 专用传输A
* 专用传输B
* 远程地址配
* 多帧传输协议TP（待续...）
# 协议格式：
* UTF-8	
# J1939协议栈接口
* J1939_Initialization(BOOL)
* J1939_ISR(void)
* J1939_Poll(unsigned long ElapsedTime)
* J1939_DequeueMessage(J1939_MESSAGE *MsgPtr)
* J1939_EnqueueMessage(J1939_MESSAGE *MsgPtr)
	   
# 源代码分析网址：
* <http://blog.csdn.net/xietongxueflyme/article/details/74908563>
  
# 源代码移植：
* <http://blog.csdn.net/xietongxueflyme/article/details/74923355>

# 协议中参考的资料：
* <http://download.csdn.net/detail/xietongxueflyme/9887994>
# 示例：
* 轮询模式简单使用示例
* 备注：接受处理，不是标准的接受处理。这里只是测试接受
```
void main( void )
{
    can_init();
    J1939_Initialization( TRUE );
    //等待地址超时
    while (J1939_Flags.WaitingForAddressClaimContention)
        J1939_Poll(5);
    //设备确认总线上没有，竞争地址的设备存在
    while (1)
    {
    /***********************发送数据***************************/
        Msg.DataPage                = 0;
        Msg.Priority                = J1939_CONTROL_PRIORITY;
        Msg.DestinationAddress      = OTHER_NODE;
        Msg.DataLength              = 8;
        Msg.PDUFormat               = 0xfe;
        Msg.Data[0]         = 0xFF;
        Msg.Data[1]         = 0xFF;
        Msg.Data[2]         = 0xFF;
        Msg.Data[3]         = 0xFF;
        Msg.Data[4]         = 0xFF;
        Msg.Data[5]         = 0xFF;
        Msg.Data[6]         = 0xFF;
        Msg.Data[7]         = 0xFF; 
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
* 中断模式
```
void main()
{
    can_init();
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
            if (Msg.PDUFormat == J1939_PF_REQUEST)
            {
                //判断参数群是否被本设备支持
                if ((Msg.Data[0] == J1939_PGN0_REQ_ENGINE_SPEED) &&
                     (Msg.Data[1] == J1939_PGN1_REQ_ENGINE_SPEED) &&
                     (Msg.Data[2] == J1939_PGN2_REQ_ENGINE_SPEED))
                {
                    if (某种原因不能响应)
                    {
                        /*********发送不能响应（参考J1939-21）*************/
                        Msg.Priority            = J1939_ACK_PRIORITY;
                        Msg.DataPage            = 0;
                        Msg.PDUFormat           = J1939_PF_ACKNOWLEDGMENT;
                        Msg.DestinationAddress  = Msg.SourceAddress;
                        Msg.DataLength          = 8;
                        Msg.Data[0]         = J1939_NACK_CONTROL_BYTE;
                        Msg.Data[1]         = 0xFF;
                        Msg.Data[2]         = 0xFF;
                        Msg.Data[3]         = 0xFF;
                        Msg.Data[4]         = 0xFF;
                        Msg.Data[5]         = J1939_PGN0_REQ_ENGINE_SPEED;
                        Msg.Data[6]         = J1939_PGN1_REQ_ENGINE_SPEED;
                        Msg.Data[7]         = J1939_PGN2_REQ_ENGINE_SPEED;
                    }
                    else
                    {
                    /*******************上传相关的参数群*****************/
                        Msg.Priority    = J1939_INFO_PRIORITY;
                        Msg.DataPage    = J1939_PGN2_REQ_ENGINE_SPEED & 0x01;
                        Msg.PDUFormat   = J1939_PGN1_REQ_ENGINE_SPEED;
                        Msg.GroupExtension = J1939_PGN0_REQ_ENGINE_SPEED;
                        Msg.DataLength  = 1;
                        Msg.Data[0]     = EngineSpeed;
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