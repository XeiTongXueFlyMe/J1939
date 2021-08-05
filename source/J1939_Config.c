#include "J1939_Config.h"

/******************************J1939 地址配置******************************/

/* 设备默认的地址（地址命名是有规定的，参考J1939的附录B 地址和标识符的分配） */
const j1939_uint8_t J1939_STARTING_ADDRESS[J1939_NODE_NUM] = {
    [Select_CAN_NODE_1] = 0x01,
    [Select_CAN_NODE_2] = 244,
    [Select_CAN_NODE_3] = 247,
    [Select_CAN_NODE_4] = 0,
};

/*
 * 输入：
 * 输出：
 * 说明：基于SAE J1939协议，我们需要CAN控制器提供至少3个滤波器给J1939协议代码。三个滤波器分别配置如下：
        1. 设置滤波器0，只接受广播信息（PF = 240 -255）。
        2. 设置滤波器1，2只接受全局地址（J1939_GLOBAL_ADDRESS）
        3. 随着程序的运行，将改变滤波器2，来适应程序逻辑。
    J1939_SetAddressFilter() 是用来设置滤波器2的， 函数主要设置PS位（目标地址），其目的是，让控制器
    只接受发送给本设备的消息。
 * 警告： 滤波器0，1是在CAN驱动里配置，如果对硬件滤波配置不是很熟练，可以使能软件滤波器，#define J1939SoftwareFilterEn
 * 则可跳过本函数的移植和CAN硬件滤波器的配置，为了J1939协议栈性能最优化，建议只是用硬件滤波。
 */
void J1939_SetAddressFilter(j1939_uint8_t Ps_Address)
{
    switch (Can_Node) {
        case Select_CAN_NODE_1: {
            break;
        }

        case Select_CAN_NODE_2: {
            break;
        }

        case Select_CAN_NODE_3: {
            break;
        }

        case Select_CAN_NODE_4: {
            break;
        }

        default: {
            break;
        }
    }
}

/*
 * 输入： *MsgPtr，协议要发送的消息，
 * 输出：
 * 说明： 将数据 从MsgPtr结构体赋值到CAN驱动自带的结构体中
        先将传入函数的MsgPtr中的数据写到CAN的结构体，再调用CAN驱动的发送函数
        默认支持4路CAN硬件的收发。如少于4路，只需配置相应的Can_Node开关代码区，
        其他（Select_CAN_NODE）保持不变。就直接返回（break）。
 */
void J1939_CAN_Transmit(J1939_MESSAGE_t *MsgPtr)
{
    switch (Can_Node) {
        case Select_CAN_NODE_1: {
            /* 加载第一路CAN硬件的29位ID */

            /* CAN硬件加载数据长度 */

            /* CAN硬件加载数据 */

            /* CAN硬件加载RTR */

            /* CAN硬件开始发送数据 */

            break;
        }

        case Select_CAN_NODE_2: {
            /* 加载第二路CAN硬件的29位ID */

            /* CAN硬件加载数据长度 */

            /* CAN硬件加载数据 */

            /* CAN硬件加载RTR */

            /* CAN硬件开始发送数据 */

            break;
        }

        case Select_CAN_NODE_3: {
            /* 加载第三路CAN硬件的29位ID */

            /* CAN硬件加载数据长度 */

            /* CAN硬件加载数据 */

            /* CAN硬件加载RTR */

            /* CAN硬件开始发送数据 */

            break;
        }

        case Select_CAN_NODE_4: {
            /* 加载第四路CAN硬件的29位ID */

            /* CAN硬件加载数据长度 */

            /* CAN硬件加载数据 */

            /* CAN硬件加载RTR */

            /* CAN硬件开始发送数据 */

            break;
        }

        default: {
            break;
        }
    }
}

/*
 * 输入：*MsgPtr 数据要存入的内存的指针
 * 输出： 1 | 0
 * 说明： 读取CAN驱动的数据，如果没有数据，返回0
        将CAN中的数据取出，存入J1939_MESSAGE结构体中
        默认支持4路CAN硬件的收发。如少于4路，只需配置相应的Can_Node开关代码区，
        其他（Select_CAN_NODE）保持不变。就直接返回（return 0）
 */
int J1939_CAN_Receive(J1939_MESSAGE_t *MsgPtr)
{
    switch (Can_Node) {
        case Select_CAN_NODE_1: {
            if ("你的代码")
            {
                /* 你的代码，从CAN硬件1 中将数据读取后，存入 MsgPtr */
                return 1;
            }

            return 0;
            break;
        }

        case Select_CAN_NODE_2: {
            break;
        }

        case Select_CAN_NODE_3: {
            break;
        }

        case Select_CAN_NODE_4: {
            break;
        }

        default: {
            return 0; /* 没有消息 */
            break;
        }
    }

    return 0; /* 没有消息 */
}

/*不使用中断模式，不对下面的函数进行移植*/
#if J1939_POLL_ECAN == J1939_FALSE
/*
 * 输入：
 * 输出：
 * 说明：使能接受中断
 */
void J1939_RXinterruptEnable()
{
    ;
}

/*
 * 输入：
 * 输出：
 * 说明：失能接受中断
 */
void J1939_RXinterruptDisable()
{
    ;
}

/*
 * 输入：
 * 输出：
 * 说明：使能发送中断
 */
void J1939_TXinterruptEnable()
{
    ;
}

/*
 * 输入：
 * 输出：
 * 说明：失能发送中断
 */
void J1939_TXinterruptDisable()
{
    ;
}

/*
 * 输入：
 * 输出：
 * 说明：触发发送中断标致位，当协议栈在中断模式下，要发送消息，将调用此函数，
        CAN驱动函数，就将直接把消息发送出去，不需要协议在调用任何can驱动函数
 */
void J1939_TXinterruptOk()
{
    ;
}

/*
 * 输入：
 * 输出：
 * 说明：清除CAN驱动相关的中断产生标识位，包括（发送中断标志位，接受中断标志位，can总线错误标识位）
 */
void CAN_identifier_clc()
{
    ;
}
#endif
