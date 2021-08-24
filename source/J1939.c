#include <stdlib.h>

#include "J1939.h"
#include "J1939_Config.h"

#define ADDRESS_CLAIM_TX   1    /**< 进入地址竞争发送处理模式*/
#define ADDRESS_CLAIM_RX   2    /**< 进入地址竞争接受处理模式*/

/** 设备的标称符
 *
 *  我们需要在"J1939_Config.h"中配置
 *  @note 在初始化中赋值，赋值参考参考J1939-81文档
 */
j1939_uint8_t CA_Name[J1939_DATA_LENGTH];
j1939_uint8_t CommandedAddress;

j1939_uint8_t J1939_Address;
J1939_FLAG_t J1939_Flags;
J1939_MESSAGE_t OneMessage;
CAN_NODE_t Can_Node;

/* 节点地址 */
j1939_uint8_t NodeAddress[J1939_NODE_NUM];

/* 接受队列全局变量(CAN_NODE) */
j1939_uint8_t RXHead            [J1939_NODE_NUM];
j1939_uint8_t RXTail            [J1939_NODE_NUM];
j1939_uint8_t RXQueueCount      [J1939_NODE_NUM];
J1939_MESSAGE_t RXQueue         [J1939_NODE_NUM][J1939_RX_QUEUE_SIZE];

/* 发送队列全局变量 (CAN_NODE) */
j1939_uint8_t TXHead            [J1939_NODE_NUM];
j1939_uint8_t TXTail            [J1939_NODE_NUM];
j1939_uint8_t TXQueueCount      [J1939_NODE_NUM];
J1939_MESSAGE_t TXQueue         [J1939_NODE_NUM][J1939_TX_QUEUE_SIZE];

struct RequestList REQUEST_LIST;

#if J1939_TP_RX_TX
/* TP协议全局变量 */
J1939_TP_FLAGS_t J1939_TP_Flags_t;
J1939_TRANSPORT_RX_INFO_t TP_RX_MSG;
J1939_TRANSPORT_TX_INFO_t TP_TX_MSG;


/* 声明TP轮询函数 */
static void J1939_TP_Poll(void);
#endif /* J1939_TP_RX_TX */

static void J1939_ReceiveMessages(void);
static J1939_Status_t J1939_TransmitMessages(void);
static void J1939_Response(const j1939_uint32_t PGN);

/**
 * @note  硬件滤波器2 或 软件滤波器  滤波配置（设置PS段）\n
 */
void SetAddressFilter(j1939_uint8_t Address)
{
    /* 软件滤波 */
#if J1939SoftwareFilterEn == J1939_TRUE
    NodeAddress[Can_Node] = Address;
#endif /* J1939SoftwareFilterEn */

    /* 硬件滤波 */
    Port_SetAddressFilter(Address);
}

/**
 * @param[in]  J1939_MESSAGE *
 * @note 发送*MsgPtr的信息，所有的数据字段（比如数据长度、优先级、和源地址）必须已经设置。\n
 */
void SendOneMessage(J1939_MESSAGE_t *MsgPtr)
{
    /* 设置消息的最后部分,确保DataLength规范。（参考CAN 2.0B） */
    MsgPtr->Mxe.Res = 0; /* 参考J1939的数据链路层（SAE J1939-21） */
    MsgPtr->Mxe.RTR = 0;

    if (MsgPtr->Mxe.DataLength > 8) {
        MsgPtr->Mxe.DataLength = 8;
    }

    /* 发送一帧消息，将 J1939_MESSAGE 中的所有消息加载道can模块自有的结构中 */
    Port_CAN_Transmit(MsgPtr);
}

/**
 * @param[in]  MsgPtr             用户要出队的消息
 * @param[in]  _Can_Node          要出队的CAN硬件编号
 * @return    RC_SUCCESS          消息出队成功
 * @return    RC_QUEUEEMPTY       没有消息返回
 * @note      从接受队列中读取一个信息到*MsgPtr。如果我们用的是中断，需要将中断失能，在获取接受队列数据时
 */
J1939_Status_t J1939_DequeueMessage(J1939_MESSAGE_t *MsgPtr, CAN_NODE_t _Can_Node)
{
    J1939_Status_t _rc = RC_SUCCESS;

    /*************************关接受中断*************************/
    Port_RXinterruptDisable();

    if (_Can_Node < J1939_NODE_NUM) {
        if (RXQueueCount[_Can_Node] == 0) {
            _rc = RC_QUEUEEMPTY;
        } else {
            *MsgPtr = RXQueue[_Can_Node][RXHead[_Can_Node]];
            RXHead[_Can_Node]++;

            if (RXHead[_Can_Node] >= J1939_RX_QUEUE_SIZE) {
                RXHead[_Can_Node] = 0;
            }

            RXQueueCount[_Can_Node]--;
        }
    } else {
        /* 若 _Can_Node 索引超过当前协议栈支持节点数，则返回 RC_CANNOTRECEIVE */
        _rc = RC_CANNOTRECEIVE;
    }

    /*************************开接受中断*************************/
    Port_RXinterruptEnable();

    return _rc;
}

/**
 * @param[in] MsgPtr              存储读取消息的缓存
 * @param[in] _Can_Node           读取消息的CAN硬件编号（从哪一路CAN读取数据）
 * @return    RC_SUCCESS          读取消息成功，
 * @return    RC_QUEUEEMPTY       读取消息不成功，没有消息。
 * @note      从接受队列中读取一个信息到*MsgPtr。
 */
J1939_Status_t J1939_Read_Message(J1939_MESSAGE_t *MsgPtr, CAN_NODE_t _Can_Node)
{
    return J1939_DequeueMessage(MsgPtr, _Can_Node);
}

/**
 * @param[in]  MsgPtr     用户要入队的消息
 * @param[in]  _Can_Node  要入队的CAN硬件编号（要选择的使用的CAN硬件编号）
 * @return     RC_SUCCESS          消息入队成功
 * @return     RC_QUEUEFULL        发送队列满，消息入队失败
 * @return     RC_CANNOTTRANSMIT   系统目前不能发送消息
 * @note     这段程序，将*MsgPtr放入发送消息队列中\n
 如果信息不能入队或者发送，将有一个相应的返回提示，\n
 如果发送中断被设置（可用），当消息队列后，发送中断被使能
 */
J1939_Status_t J1939_EnqueueMessage(J1939_MESSAGE_t *MsgPtr, CAN_NODE_t _Can_Node)
{
    J1939_Status_t _rc = RC_SUCCESS;

    Port_TXinterruptDisable();

    if (0) {
        _rc = RC_CANNOTTRANSMIT;
    } else {
        if ((J1939_OVERWRITE_TX_QUEUE == J1939_TRUE) || (TXQueueCount[_Can_Node] < J1939_TX_QUEUE_SIZE)) {
            if (TXQueueCount[_Can_Node] < J1939_TX_QUEUE_SIZE) {
                TXQueueCount[_Can_Node]++;
                TXTail[_Can_Node]++;

                if (TXTail[_Can_Node] >= J1939_TX_QUEUE_SIZE) {
                    TXTail[_Can_Node] = 0;
                }
            } else {
                J1939_Flags.TransmitMessagesdCover = 1; /* 发送数据被覆盖，上一帧数据被覆盖 */
            }

            TXQueue[_Can_Node][TXTail[_Can_Node]] = *MsgPtr;
        } else {
            _rc = RC_QUEUEFULL;
        }
    }

    Port_TXinterruptEnable();

    /* 触发发送中断 */
    Port_TXinterruptOk();

    return _rc;
}

/**
 * @param[in]  MsgPtr              存储发送消息的缓存
 * @param[in]  _Can_Node           发送消息的CAN硬件编号（从哪一路CAN发送数据）
 * @return     RC_SUCCESS          发送消息成功
 * @return     RC_QUEUEFULL        发送消息不成功，发送队列满，消息入队失败
 * @return     RC_CANNOTTRANSMIT   发送消息不成功，系统目前不能发送消息
 * @note       如果信息不能入队或者发送，将有一个相应的返回提示，\n
 */
J1939_Status_t J1939_Send_Message(J1939_MESSAGE_t *MsgPtr, CAN_NODE_t _Can_Node)
{
    return J1939_EnqueueMessage(MsgPtr, _Can_Node);
}

/**
 *
 * @note 这段代码在系统初始化中被调用,（放在CAN设备初始化之后）\n
 初始化J1939全局变量\n
 */
void J1939_Initialization(void)
{
    /* 初始化全局变量 */
    J1939_Flags.FlagVal = 0; /* 没有声明地址，其他的标识位将被设置为0（复位） */

    /* 初始化接受和发送队列 */
    j1939_uint8_t i;
    for (i = 0; i < J1939_NODE_NUM; i++)
    {
        TXHead[i] = 0;
        TXTail[i] = 0xFF;
        RXHead[i] = 0;
        RXTail[i] = 0xFF;
        TXQueueCount[i] = 0;
        RXQueueCount[i] = 0;

        /* 初始化节点地址 */
        NodeAddress[i] = J1939_STARTING_ADDRESS[i];
    }

    /* 初始化CAN节点的选择 */
    Can_Node = Select_CAN_NODE_1;

    /* 初始化请求链表 */
    REQUEST_LIST.PGN = 0;
    REQUEST_LIST.data = J1939_NULL;
    REQUEST_LIST.update = J1939_NULL;
    REQUEST_LIST.length = 0;
    REQUEST_LIST.Can_Node = Select_CAN_NODE_Null;
    REQUEST_LIST.next = J1939_NULL;

    /* 将TP协议置为空闲 */
#if J1939_TP_RX_TX
    J1939_TP_Flags_t.state = J1939_TP_NULL;
    J1939_TP_Flags_t.TP_RX_CAN_NODE = Select_CAN_NODE_Null;
    J1939_TP_Flags_t.TP_TX_CAN_NODE = Select_CAN_NODE_Null;

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
}

/**
 * @note 这个函数被调用，当设备产生CAN中断（可能是接受中断，也可能是发送中断）\n
 首先我们要清除中断标识位\n
 然后调用接受或者发送函数。
 */
#if J1939_POLL_ECAN == J1939_FALSE
void J1939_ISR(void)
{
    /* 判断相关标识位,是接受还是发送 */
    /* 清除标识位 */
    Port_CAN_identifier_clc();
    /* 调用相关的处理函数 */
    J1939_ReceiveMessages();
    J1939_TransmitMessages();
#if J1939_TP_RX_TX
    J1939_TP_Poll();
#endif /* J1939_TP_RX_TX */
    /* 可能存在因为错误产生中断，直接清除相关的标识位 */
}
#endif

/**
 * @param[in]  ElapsedTime   一个大概的毫秒数，通常设置 5 或 3
 * @note 如果我们采用轮询的方式获取信息，这个函数每几个毫秒将被调用一次。\n
 不断的接受消息和发送消息从消息队列中\n
 此外，如果我们正在等待一个地址竞争反应。\n
 如果超时，我们只接收特定的消息（目标地址 = J1939_Address）\n

 如果设备使用中断，此函数被调用，在调用J1939_Initialization（）函数后，因为\n
 J1939_Initialization（）可能初始化WaitingForAddressClaimContention标识位为1.\n

 如果接受到命令地址消息，这个函数也必须被调用，以防万一总线要求我们改变地址\n

 如果使用中断模式，本程序将不会处理接受和发送消息，只处理地址竞争超时。\n
 */
void J1939_Poll(void)
{
    /* 我们必须调用J1939_ReceiveMessages接受函数，在时间被重置为0之前。 */
#if J1939_POLL_ECAN == J1939_TRUE
    Can_Node = Select_CAN_NODE_1;
    J1939_Address = NodeAddress[0];
    J1939_ReceiveMessages();
    J1939_TransmitMessages();
    Can_Node = Select_CAN_NODE_2;
    J1939_Address = NodeAddress[1];
    J1939_ReceiveMessages();
    J1939_TransmitMessages();
    Can_Node = Select_CAN_NODE_3;
    J1939_Address = NodeAddress[2];
    J1939_ReceiveMessages();
    J1939_TransmitMessages();
    Can_Node = Select_CAN_NODE_4;
    J1939_Address = NodeAddress[3];
    J1939_ReceiveMessages();
    J1939_TransmitMessages();
#if J1939_TP_RX_TX
    J1939_TP_Poll();
#endif /* J1939_TP_RX_TX */
#endif /* J1939_POLL_ECAN == J1939_TRUE */
}

#if J1939SoftwareFilterEn == J1939_TRUE

/**
 * @return    RC_SUCCESS         消息是可以接受
 * @return    RC_CANNOTTRANSMIT  消息是不可以接受
 * @note 软件滤波器\n
 * @note 基于SAE J1939协议，我们需要CAN控制器提供至少3个滤波器给J1939协议代码。三个滤波器分别配置如下：
 1. 设置滤波器0，只接受广播信息（PF = 240 -255）。
 2. 设置设置滤波器1，2只接受全局地址（J1939_GLOBAL_ADDRESS）
 3. 随着程序的运行，将改变滤波器2，来适应程序逻辑。
 */
J1939_Status_t J1939_Messages_Filter(J1939_MESSAGE_t *MsgPtr)
{
    /* 滤波器0 */
    if ((MsgPtr->Mxe.PDUFormat) >= 240) {
        return RC_SUCCESS;
    }

    /* 滤波器1 */
    if (((MsgPtr->Mxe.PDUFormat) < 240) && (MsgPtr->Mxe.PDUSpecific == J1939_PS_DA_GLOBAL)) {
        return RC_SUCCESS;
    }

    /* 滤波器2 */
    if (((MsgPtr->Mxe.PDUFormat) < 240) && (MsgPtr->Mxe.PDUSpecific == NodeAddress[Can_Node])) {
        return RC_SUCCESS;
    }

    return RC_CANNOTTRANSMIT;
}

#endif /* J1939SoftwareFilterEn */

/**
 * @note 这段程序被调用，当CAN收发器接受数据（中断 或者 轮询）。\n
 如果一个信息被接受, 它将被调用\n
 如果信息是一个网络管理信息或长帧传输（TP），接受的信息将被加工处理，在这个函数中。\n
 否则, 信息将放置在用户的接收队列。\n
 注意：在这段程序运行期间中断是失能的。\n
 */
void J1939_ReceiveMessages(void)
{
#if J1939_TP_RX_TX
    j1939_uint32_t _pgn = 0;
#endif /* J1939_TP_RX_TX */

    /* 从接收缓存中读取信息到OneMessage中，OneMessage是一个全局变量 */
    /* Port_CAN_Receive函数读取到数据返回1，没有数据则返回0 */
    if (Port_CAN_Receive(&OneMessage)) {
#if J1939SoftwareFilterEn == J1939_TRUE

        if (J1939_Messages_Filter(&OneMessage) != RC_SUCCESS) {
            return;
        }

#endif /* J1939SoftwareFilterEn */

        switch (OneMessage.Mxe.PDUFormat) {
#if J1939_TP_RX_TX

            case J1939_PF_TP_CM: /* 参考J1939-21 TP多帧传输协议 */
                _pgn = (j1939_uint32_t) ((OneMessage.Mxe.Data[7] << 16) & 0xFF0000)
                       + (j1939_uint32_t) ((OneMessage.Mxe.Data[6] << 8) & 0xFF00)
                       + (j1939_uint32_t) ((OneMessage.Mxe.Data[5]) & 0xFF);

                if ((J1939_TP_Flags_t.state == J1939_TP_NULL) && (TP_RX_MSG.state == J1939_TP_RX_WAIT)) {
                    if (OneMessage.Mxe.Data[0] == 16) {
                        J1939_TP_Flags_t.state = J1939_TP_RX;
                        J1939_TP_Flags_t.TP_RX_CAN_NODE = Can_Node;

                        TP_RX_MSG.tp_rx_msg.SA = OneMessage.Mxe.SourceAddress;
                        TP_RX_MSG.tp_rx_msg.PGN = (j1939_uint32_t) ((OneMessage.Mxe.Data[7] << 16) & 0xFF0000)
                                                  + (j1939_uint32_t) ((OneMessage.Mxe.Data[6] << 8) & 0xFF00)
                                                  + (j1939_uint32_t) ((OneMessage.Mxe.Data[5]) & 0xFF);

                        /* 如果系统繁忙 */
                        if (TP_RX_MSG.osbusy) {
                            TP_RX_MSG.state = J1939_TP_RX_ERROR;
                            break;
                        }

                        /* 判断是否有足够的内存接收数据，如果没有直接，断开连接 */
                        if (((j1939_uint32_t) ((OneMessage.Mxe.Data[2] << 8) & 0xFF00)
                                + (j1939_uint32_t) ((OneMessage.Mxe.Data[1]) & 0xFF)) > J1939_TP_MAX_MESSAGE_LENGTH) {
                            TP_RX_MSG.state = J1939_TP_RX_ERROR;
                            break;
                        }

                        TP_RX_MSG.tp_rx_msg.byte_count = ((j1939_uint32_t) ((OneMessage.Mxe.Data[2] << 8) & 0xFF00)
                                                          + (j1939_uint32_t) ((OneMessage.Mxe.Data[1]) & 0xFF));
                        TP_RX_MSG.packets_total = OneMessage.Mxe.Data[3];
                        TP_RX_MSG.time = J1939_TP_T2;
                        TP_RX_MSG.state = J1939_TP_RX_READ_DATA;
                        break;
                    }

                    goto PutInReceiveQueue;
                    break;
                }

                if (J1939_TP_Flags_t.state == J1939_TP_TX) {
                    /* 校验PGN */
                    if (_pgn == TP_TX_MSG.tp_tx_msg.PGN) {
                        switch (OneMessage.Mxe.Data[0]) {
                            case J1939_CONTROL_BYTE_RTS:
                                /* 程序运行到这里，说明已经与网络中设备1建立虚拟链接（作为发送端），但是收到设备2的链接请求，并且同一个PGN消息请求*/
                                /* 根据J1939-21数据链路层的规定，我们要保持原有的链接，不做任何事，设备2会应为超时自动放弃链接*/
                                break;

                            case J1939_CONTROL_BYTE_CTS:
                                if ((J1939_TP_TX_CM_WAIT == TP_TX_MSG.state) || (J1939_TP_WAIT_ACK == TP_TX_MSG.state)) {
                                    /* 发送等待保持 */
                                    if (0x00u == OneMessage.Mxe.Data[1]) {
                                        /* 刷新等待计数器 */
                                        TP_TX_MSG.time = J1939_TP_T4;
                                    } else {
                                        if ((OneMessage.Mxe.Data[2] + OneMessage.Mxe.Data[1])
                                                > (TP_TX_MSG.packets_total + 1)) {
                                            /* 请求超出数据包范围 */
                                            TP_TX_MSG.state = J1939_TP_TX_ERROR;
                                        } else {
                                            /* response parameter OK */
                                            TP_TX_MSG.packets_request_num = OneMessage.Mxe.Data[1];
                                            TP_TX_MSG.packet_offset_p = (j1939_uint8_t) (OneMessage.Mxe.Data[2] - 1);
                                            TP_TX_MSG.state = J1939_TP_TX_DT;
                                        }

                                    }
                                }

                                break;

                            case J1939_CONTROL_BYTE_EOMACK:
                                if (J1939_TP_WAIT_ACK == TP_TX_MSG.state) {
                                    TP_TX_MSG.state = J1939_TX_DONE;
                                }

                                /* 这里可以增加一个对数据的校验 */
                                break;

                            case J1939_CONTROL_BYTE_CONNABORT:
                                /* 收到一个放弃连接，什么都不做，协议会在一段延时时间后主动放弃链接 */
                                break;

                            default:
                                break;
                        }
                    }
                }

                goto PutInReceiveQueue;
                break;
#endif /* J1939_TP_RX_TX */

#if J1939_TP_RX_TX

            case J1939_PF_TP_DT:
                if ((TP_RX_MSG.state == J1939_TP_RX_DATA_WAIT)
                        && (TP_RX_MSG.tp_rx_msg.SA == OneMessage.Mxe.SourceAddress)) {
                    TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0] - 1) * 7u] = OneMessage.Mxe.Data[1];
                    TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0] - 1) * 7u + 1] = OneMessage.Mxe.Data[2];
                    TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0] - 1) * 7u + 2] = OneMessage.Mxe.Data[3];
                    TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0] - 1) * 7u + 3] = OneMessage.Mxe.Data[4];
                    TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0] - 1) * 7u + 4] = OneMessage.Mxe.Data[5];
                    TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0] - 1) * 7u + 5] = OneMessage.Mxe.Data[6];
                    TP_RX_MSG.tp_rx_msg.data[(OneMessage.Mxe.Data[0] - 1) * 7u + 6] = OneMessage.Mxe.Data[7];

                    /* 特殊处理重新接受已接受过的数据包 */
                    if ((OneMessage.Mxe.Data[0]) > TP_RX_MSG.packets_ok_num) {
                        TP_RX_MSG.packets_ok_num++;
                    }

                    TP_RX_MSG.time = J1939_TP_T1;

                    /* 判断是否收到偶数个数据包或者读取到最后一个数据包 */
                    if ((TP_RX_MSG.packets_ok_num % 2 == 0) || (TP_RX_MSG.packets_ok_num == TP_RX_MSG.packets_total)) {
                        TP_RX_MSG.state = J1939_TP_RX_READ_DATA;
                        break;
                    }

                    break;
                }

                /* 程序不可能运行到这，但是我们不能放弃接受的数据包 */
                goto PutInReceiveQueue;
#endif /* J1939_TP_RX_TX */

            case J1939_PF_REQUEST:

                /* 用OneMessage.Mxe.PGN 来存下被请求的PGN */
                if (OneMessage.Mxe.Data[1] < 240) {
                    OneMessage.Mxe.PGN = (j1939_uint32_t) ((OneMessage.Mxe.Data[2] << 16) & 0x030000)
                                         + (j1939_uint32_t) ((OneMessage.Mxe.Data[1] << 8) & 0xFF00)
                                         + 0x00;
                } else {
                    OneMessage.Mxe.PGN = (j1939_uint32_t) ((OneMessage.Mxe.Data[2] << 16) & 0x030000)
                                         + (j1939_uint32_t) ((OneMessage.Mxe.Data[1] << 8) & 0xFF00)
                                         + (j1939_uint32_t) ((OneMessage.Mxe.Data[0]) & 0xFF);
                }

                J1939_Response(OneMessage.Mxe.PGN);
                break;

            default:
#if J1939_TP_RX_TX
PutInReceiveQueue: {
                    /*
                    if (OneMessage.Mxe.PDUFormat < 240) {
                        OneMessage.Mxe.PGN = (j1939_uint32_t)((OneMessage.Array[0] << 16) & 0x030000)
                                             + (j1939_uint32_t)((OneMessage.Array[1] << 8) & 0xFF00)
                                             + 0x00;
                    } else {
                        OneMessage.Mxe.PGN = (j1939_uint32_t)((OneMessage.Array[0] << 16) & 0x030000)
                                             + (j1939_uint32_t)((OneMessage.Array[1] << 8) & 0xFF00)
                                             + (j1939_uint32_t)((OneMessage.Array[2]) & 0xFF);
                    } */
                    if (OneMessage.Mxe.PDUFormat < 240) {
                        OneMessage.Mxe.PGN = (OneMessage.Mxe.Res << 17)
                                             + (OneMessage.Mxe.DataPage << 16)
                                             + (OneMessage.Mxe.PDUFormat << 8);
                    } else {
                        OneMessage.Mxe.PGN = (OneMessage.Mxe.Res << 17)
                                             + (OneMessage.Mxe.DataPage << 16)
                                             + (OneMessage.Mxe.PDUFormat << 8)
                                             + OneMessage.Mxe.PDUSpecific;
                    }

                    if ((J1939_OVERWRITE_RX_QUEUE == J1939_TRUE) || (RXQueueCount[Can_Node] < J1939_RX_QUEUE_SIZE)) {
                        if (RXQueueCount[Can_Node] < J1939_RX_QUEUE_SIZE) {
                            RXQueueCount[Can_Node]++;
                            RXTail[Can_Node]++;

                            if (RXTail[Can_Node] >= J1939_RX_QUEUE_SIZE) {
                                RXTail[Can_Node] = 0;
                            }
                        } else {
                            J1939_Flags.ReceivedMessagesdCover = 1; /* 产生数据覆盖 */
                            J1939_Flags.ReceivedMessagesdCoverOrDroppedNode = Can_Node;
                        }

                        RXQueue[Can_Node][RXTail[Can_Node]] = OneMessage;
                    } else {
                        J1939_Flags.ReceivedMessagesDropped = 1;    /* 产生数据溢出 */
                    }
                }

#endif /* J1939_TP_RX_TX */
                break;
        }
    }
}

/**
 * @return    RC_SUCCESS          信息发送成功
 * @return    RC_CANNOTTRANSMIT   系统没有发送消息,没有要发送的消息,或错误的CAN设备
 * @note      调用这个函数后，如果发送消息队列中有消息就位，则会发送消息 ，如果不能发送消息，相关的错误代码将返回。\n
 程序运行期间，中断是被失能的。
 */
static J1939_Status_t J1939_TransmitMessages(void)
{
    if (TXQueueCount[Can_Node] == 0) {
        /* 如果没有要发送的消息从发送消息队列中，恢复中断(清空发送标识位) */
        Port_TXinterruptEnable();

        return RC_CANNOTTRANSMIT;
    } else {
        while (TXQueueCount[Can_Node] > 0) {
            /* 确保上次数据发送成功 */
            /**************可增加一个判断函数**************************/

            TXQueue[Can_Node][TXHead[Can_Node]].Mxe.SourceAddress = NodeAddress[Can_Node];

            SendOneMessage((J1939_MESSAGE_t *) & (TXQueue[Can_Node][TXHead[Can_Node]]));
            TXHead[Can_Node]++;

            if (TXHead[Can_Node] >= J1939_TX_QUEUE_SIZE) {
                TXHead[Can_Node] = 0;
            }

            TXQueueCount[Can_Node]--;
        }

        /* 配置了一些标识位，使能中断 */
        Port_TXinterruptEnable();
    }

    return RC_SUCCESS;
}

#if J1939_TP_RX_TX
/**
 * @note 发送TP.DT，参考J1939-21
 */
void J1939_TP_DT_Packet_send(void)
{
    J1939_MESSAGE_t _msg;
    j1939_uint16_t _packet_offset_p;
    j1939_int32_t _i = 0;
    _msg.Mxe.Priority = J1939_PRIORITY_TP_DT;
    _msg.Mxe.DataPage = 0;
    _msg.Mxe.PDUFormat = J1939_PF_TP_DT;
    _msg.Mxe.DestinationAddress = TP_TX_MSG.tp_tx_msg.SA;
    _msg.Mxe.DataLength = 8;

    /* 获取请求发送的数据包数量 */
    if (TP_TX_MSG.packets_request_num > 0) {
        TP_TX_MSG.packets_request_num--;
        /* 获取数据偏移指针 */
        _packet_offset_p = (j1939_uint16_t) (TP_TX_MSG.packet_offset_p * 7u);
        /* 加载数据包编号 */
        _msg.Mxe.Data[0] = (j1939_uint8_t) (1u + TP_TX_MSG.packet_offset_p);

        for (_i = 0; _i < 7; _i++) {
            _msg.Mxe.Data[_i + 1] = TP_TX_MSG.tp_tx_msg.data[_packet_offset_p + _i];
        }

        /* 是否是最后一包数据消息 */
        if (TP_TX_MSG.packet_offset_p == (TP_TX_MSG.packets_total - 1u)) {
            /* 参数群是否能被填满，是否需要填充 */
            if (_packet_offset_p > TP_TX_MSG.tp_tx_msg.byte_count - 7) {
                /* 计算需要填充的数据数 */
                _i = TP_TX_MSG.tp_tx_msg.byte_count - _packet_offset_p - 7;

                for (; _i < 0; _i++) {
                    /* 我们默认J1939的参数群大小为8 */
                    _msg.Mxe.Data[_i + 8] = J1939_CONTROL_BYTE_RESERVED;
                }
            }

            TP_TX_MSG.packets_request_num = 0;
            TP_TX_MSG.packet_offset_p = 0;
            TP_TX_MSG.time = J1939_TP_T3;
            /* 跳转步骤，等待结束确认或则重新发送数据请求 */
            TP_TX_MSG.state = J1939_TP_WAIT_ACK;
        } else {
            /* 为下一个数据发送做准备 */
            TP_TX_MSG.packet_offset_p++;
            TP_TX_MSG.state = J1939_TP_TX_DT;
        }

        /* 可能队列已满，发不出去，但是这里不能靠返回值进行无限的死等 */
        J1939_EnqueueMessage(&_msg, Can_Node);
    } else {

        TP_TX_MSG.packets_request_num = 0;
        TP_TX_MSG.packet_offset_p = 0;
        TP_TX_MSG.time = J1939_TP_T3;
        TP_TX_MSG.state = J1939_TP_WAIT_ACK;
    }

}

/**
 * @note 发送TP。CM-RTS,16,23,4,255,PGN消息，参考J1939-21，
 */
void J1939_CM_Start(void)
{
    j1939_uint32_t pgn_num;
    J1939_MESSAGE_t _msg;

    pgn_num = TP_TX_MSG.tp_tx_msg.PGN;

    _msg.Mxe.Priority = J1939_PRIORITY_TP_CM;
    _msg.Mxe.DataPage = 0;
    _msg.Mxe.PDUFormat = J1939_PF_TP_CM;
    _msg.Mxe.DestinationAddress = TP_TX_MSG.tp_tx_msg.SA;
    _msg.Mxe.DataLength = 8;
    _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_RTS;
    _msg.Mxe.Data[1] = (j1939_uint8_t) TP_TX_MSG.tp_tx_msg.byte_count;
    _msg.Mxe.Data[2] = (j1939_uint8_t) ((TP_TX_MSG.tp_tx_msg.byte_count) >> 8);
    _msg.Mxe.Data[3] = TP_TX_MSG.packets_total;
    _msg.Mxe.Data[4] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[7] = (j1939_uint8_t) ((pgn_num >> 16) & 0xff);
    _msg.Mxe.Data[6] = (j1939_uint8_t) (pgn_num >> 8 & 0xff);
    _msg.Mxe.Data[5] = (j1939_uint8_t) (pgn_num & 0xff);

    /* 可能队列已满，发不出去，但是这里不能靠返回值进行无限的死等 */
    J1939_EnqueueMessage(&_msg, Can_Node);

    /* 刷新等待时间，触发下一个步骤 */
    TP_TX_MSG.time = J1939_TP_T3;
    TP_TX_MSG.state = J1939_TP_TX_CM_WAIT;
}

/**
 * @note 中断TP链接
 */
void J1939_TP_TX_Abort(void)
{
    J1939_MESSAGE_t _msg;
    j1939_uint32_t pgn_num;

    pgn_num = TP_TX_MSG.tp_tx_msg.PGN;

    _msg.Mxe.Priority = J1939_PRIORITY_TP_CM;
    _msg.Mxe.DataPage = 0;
    _msg.Mxe.PDUFormat = J1939_PF_TP_CM;
    _msg.Mxe.DestinationAddress = TP_TX_MSG.tp_tx_msg.SA;
    _msg.Mxe.DataLength = 8;
    _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_CONNABORT;
    _msg.Mxe.Data[1] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[2] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[3] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[4] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[7] = (j1939_uint8_t) ((pgn_num >> 16) & 0xff);
    _msg.Mxe.Data[6] = (j1939_uint8_t) (pgn_num >> 8 & 0xff);
    _msg.Mxe.Data[5] = (j1939_uint8_t) (pgn_num & 0xff);

    /* 可能队列已满，发不出去，但是这里不能靠返回值进行无限的死等 */
    J1939_EnqueueMessage(&_msg, Can_Node);
    /* 结束发送 */
    TP_TX_MSG.state = J1939_TX_DONE;
}

/**
 * @note 中断TP链接
 */
void J1939_TP_RX_Abort(void)
{
    J1939_MESSAGE_t _msg;
    j1939_uint32_t pgn_num;

    pgn_num = TP_RX_MSG.tp_rx_msg.PGN;

    _msg.Mxe.Priority = J1939_PRIORITY_TP_CM;
    _msg.Mxe.DataPage = 0;
    _msg.Mxe.PDUFormat = J1939_PF_TP_CM;
    _msg.Mxe.DestinationAddress = TP_RX_MSG.tp_rx_msg.SA;
    _msg.Mxe.DataLength = 8;
    _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_CONNABORT;
    _msg.Mxe.Data[1] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[2] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[3] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[4] = J1939_CONTROL_BYTE_RESERVED;
    _msg.Mxe.Data[7] = (j1939_uint8_t) ((pgn_num >> 16) & 0xff);
    _msg.Mxe.Data[6] = (j1939_uint8_t) (pgn_num >> 8 & 0xff);
    _msg.Mxe.Data[5] = (j1939_uint8_t) (pgn_num & 0xff);

    /* 可能队列已满，发不出去，但是这里不能靠返回值进行无限的死等 */
    J1939_EnqueueMessage(&_msg, Can_Node);
    /* 结束发送 */
    TP_RX_MSG.state = J1939_RX_DONE;
}

/**
 * @note TP的计时器
 */
j1939_uint8_t J1939_TP_TX_RefreshCMTimer(j1939_uint16_t dt_ms)
{
    if ((J1939_TP_TX_CM_WAIT == TP_TX_MSG.state) || (J1939_TP_WAIT_ACK == TP_TX_MSG.state)) {
        if (TP_TX_MSG.time > dt_ms) {
            TP_TX_MSG.time = TP_TX_MSG.time - dt_ms;
            return J1939_TP_TIMEOUT_NORMAL;
        } else {
            /* 超时  */
            TP_TX_MSG.time = 0u;
            return J1939_TP_TIMEOUT_ABNORMAL;
        }

    } else {
        return J1939_TP_TIMEOUT_NORMAL;
    }
}

/**
 * @note TP的计时器
 */
j1939_uint8_t J1939_TP_RX_RefreshCMTimer(j1939_uint16_t dt_ms)
{
    if ((J1939_TP_RX_DATA_WAIT == TP_RX_MSG.state)) {
        if (TP_RX_MSG.time > dt_ms) {
            TP_RX_MSG.time = TP_RX_MSG.time - dt_ms;
            return J1939_TP_TIMEOUT_NORMAL;
        } else {
            /* 超时 */
            TP_RX_MSG.time = 0u;
            return J1939_TP_TIMEOUT_ABNORMAL;
        }

    } else {
        return J1939_TP_TIMEOUT_NORMAL;
    }
}
/**
 * @note 发送读取数据 TP.CM_CTS 和 EndofMsgAck消息。
 */
void J1939_read_DT_Packet(void)
{
    J1939_MESSAGE_t _msg;
    j1939_uint32_t pgn_num;
    pgn_num = TP_RX_MSG.tp_rx_msg.PGN;

    _msg.Mxe.Priority = J1939_PRIORITY_TP_CM;
    _msg.Mxe.DataPage = 0;
    _msg.Mxe.PDUFormat = J1939_PF_TP_CM;
    _msg.Mxe.DestinationAddress = TP_RX_MSG.tp_rx_msg.SA;
    _msg.Mxe.DataLength = 8;

    /* 如果系统繁忙,保持链接但是不传送消息 */
    if (TP_RX_MSG.osbusy) {
        _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_CTS;
        _msg.Mxe.Data[1] = 0;
        _msg.Mxe.Data[2] = J1939_CONTROL_BYTE_RESERVED;
        _msg.Mxe.Data[3] = J1939_CONTROL_BYTE_RESERVED;
        _msg.Mxe.Data[4] = J1939_CONTROL_BYTE_RESERVED;
        _msg.Mxe.Data[7] = (j1939_uint8_t) ((pgn_num >> 16) & 0xff);
        _msg.Mxe.Data[6] = (j1939_uint8_t) (pgn_num >> 8 & 0xff);
        _msg.Mxe.Data[5] = (j1939_uint8_t) (pgn_num & 0xff);
        /* 可能队列已满，发不出去，但是这里不能靠返回值进行无限的死等 */
        J1939_EnqueueMessage(&_msg, Can_Node);
        return;
    }

    if (TP_RX_MSG.packets_total > TP_RX_MSG.packets_ok_num) {
        /* 最后一次响应，如果不足2包数据 */
        if ((TP_RX_MSG.packets_total - TP_RX_MSG.packets_ok_num) == 1) {
            _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_CTS;
            _msg.Mxe.Data[1] = 1;
            _msg.Mxe.Data[2] = TP_RX_MSG.packets_total;
            _msg.Mxe.Data[3] = J1939_CONTROL_BYTE_RESERVED;
            _msg.Mxe.Data[4] = J1939_CONTROL_BYTE_RESERVED;
            _msg.Mxe.Data[7] = (j1939_uint8_t) ((pgn_num >> 16) & 0xff);
            _msg.Mxe.Data[6] = (j1939_uint8_t) (pgn_num >> 8 & 0xff);
            _msg.Mxe.Data[5] = (j1939_uint8_t) (pgn_num & 0xff);
            /* 可能队列已满，发不出去，但是这里不能靠返回值进行无限的死等 */
            J1939_EnqueueMessage(&_msg, Can_Node);
            TP_RX_MSG.state = J1939_TP_RX_DATA_WAIT;
            return;
        }

        _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_CTS;
        _msg.Mxe.Data[1] = 2;
        _msg.Mxe.Data[2] = (TP_RX_MSG.packets_ok_num + 1);
        _msg.Mxe.Data[3] = J1939_CONTROL_BYTE_RESERVED;
        _msg.Mxe.Data[4] = J1939_CONTROL_BYTE_RESERVED;
        _msg.Mxe.Data[7] = (j1939_uint8_t) ((pgn_num >> 16) & 0xff);
        _msg.Mxe.Data[6] = (j1939_uint8_t) (pgn_num >> 8 & 0xff);
        _msg.Mxe.Data[5] = (j1939_uint8_t) (pgn_num & 0xff);

        /* 可能队列已满，发不出去，但是这里不能靠返回值进行无限的死等 */
        J1939_EnqueueMessage(&_msg, Can_Node);
        TP_RX_MSG.state = J1939_TP_RX_DATA_WAIT;
        return;
    } else {
        /* 发送传输正常结束消息，EndofMsgAck */
        _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_EOMACK;
        _msg.Mxe.Data[1] = (TP_RX_MSG.tp_rx_msg.byte_count & 0x00ff);
        _msg.Mxe.Data[2] = ((TP_RX_MSG.tp_rx_msg.byte_count >> 8) & 0x00ff);
        _msg.Mxe.Data[3] = TP_RX_MSG.packets_total;
        _msg.Mxe.Data[4] = J1939_CONTROL_BYTE_RESERVED;
        _msg.Mxe.Data[7] = (j1939_uint8_t) ((pgn_num >> 16) & 0xff);
        _msg.Mxe.Data[6] = (j1939_uint8_t) (pgn_num >> 8 & 0xff);
        _msg.Mxe.Data[5] = (j1939_uint8_t) (pgn_num & 0xff);
        /* 可能队列已满，发不出去，但是这里不能靠返回值进行无限的死等 */
        J1939_EnqueueMessage(&_msg, Can_Node);
        TP_RX_MSG.state = J1939_RX_DONE;
        return;
    }
}

/**
 * @note TP协议的心跳，为了满足在总线的计时准确，10ms轮询一次   J1939_TP_TX_RefreshCMTimer(10)\n
 如果想要更高的分辨率，1ms轮询一次，但是要改下面计时函数  J1939_TP_TX_RefreshCMTimer(1)
 */
void J1939_TP_Poll(void)
{
    if (J1939_TP_Flags_t.state == J1939_TP_NULL || J1939_TP_Flags_t.state == J1939_TP_OSBUSY) {
        return;
    }

    if (J1939_TP_Flags_t.state == J1939_TP_RX) {
        Can_Node = J1939_TP_Flags_t.TP_RX_CAN_NODE;

        switch (TP_RX_MSG.state) {
            case J1939_TP_RX_WAIT:
                ;
                break;

            case J1939_TP_RX_READ_DATA:
                /* 发送读取数据 TP.CM_CTS 和 EndofMsgAck消息 */
                J1939_read_DT_Packet();
                break;

            case J1939_TP_RX_DATA_WAIT:

                /* 等待TP.DT帧传输的消息 */
                if (J1939_TP_TIMEOUT_ABNORMAL == J1939_TP_RX_RefreshCMTimer(10)) {
                    /* 等待超时，发生连接异常，跳转到异常步骤 */
                    TP_RX_MSG.state = J1939_TP_RX_ERROR;
                }

                break;

            case J1939_TP_RX_ERROR:
                J1939_TP_RX_Abort();
                J1939_TP_Flags_t.TP_RX_CAN_NODE = Select_CAN_NODE_Null;
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

        return;
    }

    if (J1939_TP_Flags_t.state == J1939_TP_TX) {
        Can_Node = J1939_TP_Flags_t.TP_TX_CAN_NODE;

        switch (TP_TX_MSG.state) {
            case J1939_TP_TX_WAIT:
                /* 没有要发送的数据 */
                break;

            case J1939_TP_TX_CM_START:
                /* 发送TP.CM_RTS帧传输的消息(参考J1939-21) */
                J1939_CM_Start();
                break;

            case J1939_TP_TX_CM_WAIT:

                /* 等待TP.CM_CTS帧传输的消息 */
                if (J1939_TP_TIMEOUT_ABNORMAL == J1939_TP_TX_RefreshCMTimer(10)) {
                    /* 等待超时，发生连接异常，跳转到异常步骤 */
                    TP_TX_MSG.state = J1939_TP_TX_ERROR;
                }

                break;

            case J1939_TP_TX_DT:
                J1939_TP_DT_Packet_send();
                break;

            case J1939_TP_WAIT_ACK:

                /* 等待TP.EndofMsgACK帧传输的消息 */
                if (J1939_TP_TIMEOUT_ABNORMAL == J1939_TP_TX_RefreshCMTimer(10)) {
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
                /* 程序不会运行到这里来，可以增加一个调试输出 */
                break;
        }

        return;
    }
}

/**这是一个非阻塞io接口
 *
 * @param[in] PGN   TP会话的参数群编号
 * @param[in] SA        TP会话的目标地址
 * @param[in] *data TP会话的数据缓存地址
 * @param[in] data_num TP会话的数据大小
 * @param[in]  _Can_Node  要入队的CAN硬件编号（要选择的使用的CAN硬件编号）
 * @return    RC_SUCCESS        成功打开TP链接，开始进入发送流程
 * @return    RC_CANNOTTRANSMIT 不能发送，因为TP协议已经建立虚拟链接，并且未断开
 * @note      TP协议的发送函数
 */
J1939_Status_t J1939_TP_TX_Message(j1939_uint32_t PGN, j1939_uint8_t DA, j1939_uint8_t *data, j1939_uint16_t data_num,
                                   CAN_NODE_t _Can_Node)
{
    j1939_uint16_t _byte_count = 0;

    /* 取得发送权限 */
    if (J1939_TP_Flags_t.state == J1939_TP_NULL) {
        J1939_TP_Flags_t.state = J1939_TP_TX;
        J1939_TP_Flags_t.TP_TX_CAN_NODE = _Can_Node;
    } else {
        return RC_CANNOTTRANSMIT; /* 不能发送，因为TP协议已经建立虚拟链接，并且未断开 */
    }

    TP_TX_MSG.tp_tx_msg.PGN = PGN;
    TP_TX_MSG.tp_tx_msg.SA = DA;
    TP_TX_MSG.tp_tx_msg.byte_count = data_num;

    for (_byte_count = 0; _byte_count < data_num; _byte_count++) {
        TP_TX_MSG.tp_tx_msg.data[_byte_count] = data[_byte_count];
    }

    TP_TX_MSG.packet_offset_p = 0;
    TP_TX_MSG.packets_request_num = 0;
    TP_TX_MSG.packets_total = data_num / 7;

    if ((data_num % 7) != 0) {
        TP_TX_MSG.packets_total++;
    }

    TP_TX_MSG.time = J1939_TP_T3;
    /* 触发开始CM_START */
    TP_TX_MSG.state = J1939_TP_TX_CM_START;

    return RC_SUCCESS;
}

/**
 * @param[in]  msg.data      读取数据的缓存
 * @param[in]  msg.data_num   读取数据的缓存大小
 * @param[in]  _Can_Node      要入队的CAN硬件编号（要选择的使用的CAN硬件编号）
 * @param[out] msg.SA         数据源地址
 * @param[out] msg.byte_count 数据大小
 * @param[out] msg.PGN        数据参数群编号
 * @return  RC_CANNOTRECEIVE 不能接受，TP协议正在接受数据中
 * @return  RC_SUCCESS      读取数据成功
 * @note TP的接受函数 , 接受缓存的大小必须大于接受数据的大小，建议初始化缓存大小用  J1939_TP_MAX_MESSAGE_LENGTH\n
 请正确带入 缓存区的大小，参数错误程序运行有风险
 */
J1939_Status_t J1939_TP_RX_Message(TP_RX_MESSAGE_t *msg, CAN_NODE_t _Can_Node)
{
    j1939_uint16_t _a = 0;

    /* 判断是否能读取数据 */
    if (J1939_TP_Flags_t.state == J1939_TP_NULL && TP_RX_MSG.tp_rx_msg.PGN != 0) {
        J1939_TP_Flags_t.state = J1939_TP_OSBUSY;
    } else {
        return RC_CANNOTRECEIVE; /* 不能接受，TP协议正在接受数据中,或没有数据 */
    }

    /* 判断是不是要读取那一路CAN数据 */
    if (_Can_Node != J1939_TP_Flags_t.TP_RX_CAN_NODE) {
        /* 释放TP接管权限 */
        if (J1939_TP_Flags_t.state == J1939_TP_OSBUSY) {
            J1939_TP_Flags_t.state = J1939_TP_NULL;
        }

        return RC_CANNOTRECEIVE;
    }

    /* 判断数据缓存够不够 */
    if ((msg->data_num) < TP_RX_MSG.tp_rx_msg.byte_count) {
        return RC_CANNOTRECEIVE; /* 不能接受，缓存区太小 */
    }

    /* 获取数据 */
    for (_a = 0; _a < msg->data_num; _a++) {
        msg->data[_a] = TP_RX_MSG.tp_rx_msg.data[_a];
    }

    /* 获取数据 源地址 */
    msg->SA = TP_RX_MSG.tp_rx_msg.SA;
    /* 获取数据的大小 */
    msg->byte_count = TP_RX_MSG.tp_rx_msg.byte_count;
    /* 获取数据PGN */
    msg->PGN = TP_RX_MSG.tp_rx_msg.PGN;

    /* 丢弃读取过的数据 */
    TP_RX_MSG.tp_rx_msg.byte_count = 0u;
    TP_RX_MSG.tp_rx_msg.PGN = 0;

    /* 释放TP接管权限 */
    if (J1939_TP_Flags_t.state == J1939_TP_OSBUSY) {
        J1939_TP_Flags_t.state = J1939_TP_NULL;
    }

    return RC_SUCCESS;
}

/**
 * @param[in] pgn  被请求的参数群
 * @param[in] DA   目标地址（DestinationAddress） 当DA = 0xff表示是全局请求
 * @param[in] _Can_Node  要入队的CAN硬件编号（要选择的使用的CAN硬件编号）
 * @note      请求（从全局范围或则特定目的地的）参数群，请求规则J1939-21的16-17页，有明确的说明
 */
void J1939_Request_PGN(j1939_uint32_t PGN, j1939_uint8_t DA, CAN_NODE_t _Can_Node)
{
    J1939_MESSAGE_t _msg;

    _msg.Mxe.DataPage = 0;
    _msg.Mxe.Priority = J1939_PRIORITY_REQUEST;
    _msg.Mxe.DestinationAddress = DA;
    _msg.Mxe.DataLength = 3;
    _msg.Mxe.PDUFormat = J1939_PF_REQUEST;
    _msg.Mxe.Data[0] = (j1939_uint8_t) (PGN & 0x000000FF);
    _msg.Mxe.Data[1] = (j1939_uint8_t) ((PGN & 0x0000FF00) >> 8);
    _msg.Mxe.Data[2] = (j1939_uint8_t) ((PGN & 0x00FF0000) >> 16);

    while (J1939_EnqueueMessage(&_msg, _Can_Node) != RC_SUCCESS);
}

/**
 * @param[in]  data       需要发送数据的缓存
 * @param[in]  dataLength  发送数据的缓存大小
 * @param[in]  PGN         需要发送数据的PGN(参数群编号)
 * @param[in]  void (*dataUPFun)()  用于更新缓存data 的函数地址指针
 * @param[in]  _Can_Node      要入队的CAN硬件编号（要选择的使用的CAN硬件编号）
 * @note 创建一个PGN 的 请求 对应的 响应\n 如果收到改请求则先运行 REQUEST_LIST.dataUPFun(),在将数据REQUEST_LIST.data发送出去
 * @warning  本函数只能被串行调用，（多线程）并行调用请在函数外加互斥操作
 */
void J1939_Create_Response(j1939_uint8_t data[], j1939_uint16_t dataLength, j1939_uint32_t PGN, void (*dataUPFun)(),
                           CAN_NODE_t _Can_Node)
{
    /* 查找可用的链表项 */
    struct RequestList *_requestList = &REQUEST_LIST;

    while (J1939_NULL != _requestList->next) {
        _requestList = _requestList->next;
    }

    _requestList->next = (struct RequestList *) malloc(sizeof(struct RequestList));
    _requestList = _requestList->next;

    /* 对新的链表项赋值 */
    _requestList->data = data;
    _requestList->length = dataLength;
    _requestList->PGN = PGN;
    _requestList->update = dataUPFun;
    _requestList->Can_Node = _Can_Node;
    _requestList->next = J1939_NULL;

}

/**
 * @note 当收到一个PGN请求后，如果有REQUEST_LIST中有相应的PGN，则会自动发送REQUEST_LIST中的PGN。\n
 如果没有则会发送一个NACK; 本函数的响应逻辑，参考J1939-21 17页表4
 */
static void J1939_Response(const j1939_uint32_t PGN)
{
    J1939_MESSAGE_t _msg;

    /* 查找可用的链表项 */
    struct RequestList *_requestList = &REQUEST_LIST;

    while ((PGN != _requestList->PGN) || (Can_Node != _requestList->Can_Node)) {
        if (_requestList->next == J1939_NULL) {
            /* 原文档规定 全局请求不被支持时不能响应 NACK */
            if (OneMessage.Mxe.PDUSpecific == J1939_PS_DA_GLOBAL) {
                return;
            }

            if ((PGN & 0xFF00) >= 0xF000) {
                return;
            }

            /* 没有相应的PGN响应被创建，向总线发送一个NACK */
            _msg.Mxe.Priority = J1939_PRIORITY_ACK;
            _msg.Mxe.DataPage = 0;
            _msg.Mxe.PDUFormat = J1939_PF_ACKNOWLEDGMENT;
            _msg.Mxe.DestinationAddress = OneMessage.Mxe.SourceAddress;
            _msg.Mxe.DataLength = 8;
            _msg.Mxe.SourceAddress = J1939_Address;
            _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_NACK;
            _msg.Mxe.Data[1] = 0xFF;
            _msg.Mxe.Data[2] = 0xFF;
            _msg.Mxe.Data[3] = 0xFF;
            _msg.Mxe.Data[4] = 0xFF;
            _msg.Mxe.Data[5] = (PGN & 0x0000FF);
            _msg.Mxe.Data[6] = ((PGN >> 8) & 0x0000FF);
            _msg.Mxe.Data[7] = ((PGN >> 16) & 0x0000FF);

            SendOneMessage((J1939_MESSAGE_t *) &_msg);
            return;
        } else {
            _requestList = _requestList->next;
        }
    }

    /* 调用dataUPFun()函数，主要用于参数群数据更新 */
    if (J1939_NULL != _requestList->update) {
        _requestList->update();
    }

    /* 响应请求 */
    if (_requestList->length > 8) {
        /* 回一个确认响应多帧(非广播多帧) */
        if (RC_SUCCESS
                != J1939_TP_TX_Message(_requestList->PGN, OneMessage.Mxe.SourceAddress, _requestList->data,
                                       _requestList->length, Can_Node)) {
            /* 原文档规定 全局请求不被支持时不能响应 NACK */
            if (OneMessage.Mxe.PDUSpecific == J1939_PS_DA_GLOBAL) {
                return;
            }

            /* 如果长帧发送不成功 */
            _msg.Mxe.Priority = J1939_PRIORITY_ACK;
            _msg.Mxe.DataPage = 0;
            _msg.Mxe.PDUFormat = J1939_PF_ACKNOWLEDGMENT;
            _msg.Mxe.DestinationAddress = OneMessage.Mxe.SourceAddress;
            _msg.Mxe.DataLength = 8;
            _msg.Mxe.SourceAddress = J1939_Address;
            _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_ACCESS_DENIED;
            _msg.Mxe.Data[1] = 0xFF;
            _msg.Mxe.Data[2] = 0xFF;
            _msg.Mxe.Data[3] = 0xFF;
            _msg.Mxe.Data[4] = 0xFF;
            _msg.Mxe.Data[5] = (PGN & 0x0000FF);
            _msg.Mxe.Data[6] = ((PGN >> 8) & 0x0000FF);
            _msg.Mxe.Data[7] = ((PGN >> 16) & 0x0000FF);

            SendOneMessage((J1939_MESSAGE_t *) &_msg);
            return;
        }

        /* 回一个确认响应 */
        _msg.Mxe.Priority = J1939_PRIORITY_ACK;
        _msg.Mxe.DataPage = 0;
        _msg.Mxe.PDUFormat = J1939_PF_ACKNOWLEDGMENT;

        /* 原文档规定 全局请求响应到全局 */
        if (OneMessage.Mxe.PDUSpecific == J1939_PS_DA_GLOBAL) {
            _msg.Mxe.DestinationAddress = 0XFF;
        } else {
            _msg.Mxe.DestinationAddress = OneMessage.Mxe.SourceAddress;
        }

        _msg.Mxe.DataLength = 8;
        _msg.Mxe.SourceAddress = J1939_Address;
        _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_ACK;
        _msg.Mxe.Data[1] = 0xFF;
        _msg.Mxe.Data[2] = 0xFF;
        _msg.Mxe.Data[3] = 0xFF;
        _msg.Mxe.Data[4] = 0xFF;
        _msg.Mxe.Data[5] = (PGN & 0x0000FF);
        _msg.Mxe.Data[6] = ((PGN >> 8) & 0x0000FF);
        _msg.Mxe.Data[7] = ((PGN >> 16) & 0x0000FF);
        SendOneMessage((J1939_MESSAGE_t *) &_msg);
    } else {

        /* 回一个确认响应 */
        _msg.Mxe.Priority = J1939_PRIORITY_ACK;
        _msg.Mxe.DataPage = 0;
        _msg.Mxe.PDUFormat = J1939_PF_ACKNOWLEDGMENT;
        _msg.Mxe.SourceAddress = J1939_Address;

        /* 原文档规定 全局请求响应到全局 */
        if ((OneMessage.Mxe.PDUSpecific == J1939_PS_DA_GLOBAL) || ((PGN & 0xFF00) >= 0xF000)) {
            _msg.Mxe.DestinationAddress = 0XFF;
        } else {
            _msg.Mxe.DestinationAddress = OneMessage.Mxe.SourceAddress;
        }

        _msg.Mxe.DataLength = 8;
        _msg.Mxe.SourceAddress = J1939_Address;
        _msg.Mxe.Data[0] = J1939_CONTROL_BYTE_ACK;
        _msg.Mxe.Data[1] = 0xFF;
        _msg.Mxe.Data[2] = 0xFF;
        _msg.Mxe.Data[3] = 0xFF;
        _msg.Mxe.Data[4] = 0xFF;
        _msg.Mxe.Data[5] = (PGN & 0x0000FF);
        _msg.Mxe.Data[6] = ((PGN >> 8) & 0x0000FF);
        _msg.Mxe.Data[7] = ((PGN >> 16) & 0x0000FF);
        SendOneMessage((J1939_MESSAGE_t *) &_msg);

        /* 回一个确认响应单帧 */
        _msg.Mxe.Priority = J1939_PRIORITY_ACK;
        _msg.Mxe.DataPage = (((_requestList->PGN) >> 16) & 0x1);
        _msg.Mxe.PDUFormat = ((_requestList->PGN) >> 8) & 0xFF;
        _msg.Mxe.SourceAddress = J1939_Address;

        /* 原文档规定 全局请求响应到全局 */
        if (OneMessage.Mxe.PDUSpecific == J1939_PS_DA_GLOBAL) {
            _msg.Mxe.DestinationAddress = 0XFF;
        } else {
            _msg.Mxe.DestinationAddress = OneMessage.Mxe.SourceAddress;
        }

        _msg.Mxe.DataLength = _requestList->length;
        {
            j1939_uint8_t _i = 0;

            for (_i = 0; _i < (_requestList->length); _i++) {
                _msg.Mxe.Data[_i] = _requestList->data[_i];
            }

            for (; _i < 8; _i++) {
                _msg.Mxe.Data[_i] = 0xFF;
            }
        }
        SendOneMessage((J1939_MESSAGE_t *) &_msg);
    }
}
#endif
