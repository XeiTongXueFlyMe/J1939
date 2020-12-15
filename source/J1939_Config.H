#ifndef __J1939_Config_H
#define __J1939_Config_H

#include "J1939.h"

/******************************J1939功能配置******************************/

#define J1939_NODE_NUM              4

#define J1939_RX_QUEUE_SIZE         3
/* 当MCU来不及处理消息，接收消息队列是否允许被新的消息覆盖 */
#define J1939_OVERWRITE_RX_QUEUE    J1939_FALSE

#define J1939_TX_QUEUE_SIZE         3
/* 当MCU来不及处理消息，发送消息队列是否允许被新的消息覆盖 */
#define J1939_OVERWRITE_TX_QUEUE    J1939_FALSE

/* 是否使用轮询模式（否则使用中断模式） */
#define J1939_POLL_ECAN             J1939_TRUE
/* 是否启用软件滤波器 */
#define J1939SoftwareFilterEn       J1939_TRUE

extern CAN_NODE_t Can_Node; /* CAN硬件选择 */
extern const j1939_uint8_t J1939_STARTING_ADDRESS[J1939_NODE_NUM];

/******************************J1939移植配置函数******************************/

void J1939_SetAddressFilter(j1939_uint8_t Ps_Address);
void J1939_CAN_Transmit(J1939_MESSAGE_t *MsgPtr);
int  J1939_CAN_Receive(J1939_MESSAGE_t *MsgPtr);

#define Port_CAN_Transmit(MsgPtr)       J1939_CAN_Transmit(MsgPtr)
#define Port_CAN_Receive(MsgPtr)        J1939_CAN_Receive(MsgPtr)
#define Port_SetAddressFilter(Address)  J1939_SetAddressFilter(Address)

/* 不使用中断模式，不对下面的函数进行移植 */
#if J1939_POLL_ECAN == J1939_FALSE
void J1939_RXinterruptEnable();
void J1939_RXinterruptDisable();
void J1939_TXinterruptEnable();
void J1939_TXinterruptDisable();
void J1939_TXinterruptOk();
void CAN_identifier_clc();
#define Port_RXinterruptEnable()    J1939_RXinterruptEnable()
#define Port_RXinterruptDisable()   J1939_RXinterruptDisable()
#define Port_TXinterruptEnable()    J1939_TXinterruptEnable()
#define Port_TXinterruptDisable()   J1939_TXinterruptDisable()
#define Port_TXinterruptOk()        J1939_TXinterruptOk()
#define Port_CAN_identifier_clc()   CAN_identifier_clc()
#else
#define Port_RXinterruptEnable()
#define Port_RXinterruptDisable()
#define Port_TXinterruptEnable()
#define Port_TXinterruptDisable()
#define Port_TXinterruptOk()
#define Port_CAN_identifier_clc()
#endif /* #if J1939_POLL_ECAN == J1939_FALSE */

#endif /* #ifndef __J1939_Config_H */
