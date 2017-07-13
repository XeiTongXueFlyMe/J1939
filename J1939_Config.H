/*********************************************************************
 *
 *             J1939_config Include File
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
#ifndef __J1939_Config_H 
#define __J1939_Config_H 

#include "J1939.H"


/***************************J1939 标识符和地址配置*************************************/

//设备默认的地址（地址命名是有规定的，参考J1939的附录B 地址和标识符的分配）
#define J1939_STARTING_ADDRESS 243
//如果声明不为0，表示我们的ECU(电子控制单元)支持网络中申请的任意地址，（参考J1939的网络层）
#define J1939_ARBITRARY_ADDRESS 0x00
#define J1939_INDUSTRY_GROUP 0
#define J1939_VEHICLE_INSTANCE 0
#define J1939_CA_NAME7 (J1939_ARBITRARY_ADDRESS | (J1939_INDUSTRY_GROUP << 4) | J1939_VEHICLE_INSTANCE)
#define J1939_VEHICLE_SYSTEM 0
#define J1939_CA_NAME6 (J1939_VEHICLE_SYSTEM << 1)
#define J1939_FUNCTION 0
#define J1939_CA_NAME5 J1939_FUNCTION
#define J1939_FUNCTION_INSTANCE 0
#define J1939_ECU_INSTANCE 0
#define J1939_CA_NAME4 ((J1939_FUNCTION_INSTANCE << 3) | J1939_ECU_INSTANCE)
#define J1939_MANUFACTURER_CODE 0
#define J1939_IDENTITY_NUMBER 50
#define J1939_CA_NAME3 (J1939_MANUFACTURER_CODE >> 3)
#define J1939_CA_NAME2 (((J1939_MANUFACTURER_CODE & 0x07) << 5) | (J1939_IDENTITY_NUMBER >> 16))
#define J1939_CA_NAME1 ((J1939_IDENTITY_NUMBER >> 8) & 0xFF)
#define J1939_CA_NAME0 (J1939_IDENTITY_NUMBER & 0xFF)

/******************************J1939功能配置******************************************/

//是否使用接受协议（对TP协议的支持，参考J1939-21）
#define J1939_ACCEPT_CMDADD J1939_FALSE
#define J1939_RX_QUEUE_SIZE 3
//当mcu来不及处理消息，接收消息列队是否允许被新的消息覆盖
#define J1939_OVERWRITE_RX_QUEUE J1939_FALSE
#define J1939_TX_QUEUE_SIZE 3
//当mcu来不及处理消息，发送消息列队是否允许被新的消息覆盖
#define J1939_OVERWRITE_TX_QUEUE J1939_FALSE
//是否使用轮询模式（否则使用中断模式）
#define J1939_POLL_ECAN J1939_TRUE
#define J1939_PRIORITIZED_INT J1939_TRUE

/******************************J1939移植配置函数******************************************/

#define Port_CAN_Transmit(MsgPtr) J1939_CAN_Transmit(MsgPtr) 
#define Port_CAN_Receive(MsgPtr) J1939_CAN_Receive(MsgPtr)
#define Port_SetAddressFilter(Address) J1939_SetAddressFilter(Address)
#define Port_RXinterruptEnable() J1939_RXinterruptEnable() 
#define Port_RXinterruptDisable() J1939_RXinterruptDisable() 
#define Port_TXinterruptEnable() J1939_TXinterruptEnable() 
#define Port_TXinterruptDisable() J1939_TXinterruptDisable() 
#define Port_TXinterruptOk() J1939_TXinterruptOk()
#define Port_CAN_identifier_clc()  CAN_identifier_clc()

/***************************************************************************************/
/*
*输入：
*输出： 
*说明：基于SAE J1939协议，我们需要CAN控制器提供至少3个滤波器给J1939协议代码。三个滤波器分别配置如下： 
		1. 设置滤波器0，只接受广播信息（PF = 240 -255）。 
		2. 设置设置滤波器1，2只接受全局地址（J1939_GLOBAL_ADDRESS） 
		3. 随着程序的运行，将改变滤波器2，来适应程序逻辑。
	J1939_SetAddressFilter() 是用来设置滤波器2的， 函数主要设置PS位（目标地址），其目的是，让控制器
	只接受发送给本设备的消息。
*/
void J1939_SetAddressFilter(unsigned char Ps_Address)
{
	;
}
/*
*输入： CAN自带的结构体（包含ID） 协议本身的结构体 IDJ1939_MESSAGE
*输出： 
*说明：我们需要配置can的远程帧ID,函数ChangeGroupIDofLMO（）提供这样的配置。配置方法和移植，和
	J1939_CAN_Transmit函数有关，建议一起配置：
	我们需要配置can的远程帧ID,函数ChangeGroupIDofLMO（）提供这样的配置。配置方法和移植，和
	J1939_CAN_Transmit函数有关，建议一起配置：
*/
void ChangeGroupIDofLMO(const CAN_NODE_LMO_t *lmo_ptr,J1939_MESSAGE *MsgPtr)
{
	;
}
/*
*输入： J1939_MESSAGE *MsgPtr ，协议要发送的消息，
*输出： 
*说明：从MsgPtr加载到CAN驱动自带的结构体中
		将和ChangeGroupIDofLMO（）联合移植（可不这样设计）
		先将传入函数的MsgPtr中的数据写到CAN的结构体（加载），调用CAN驱动的发送函数
*/

void J1939_CAN_Transmit(J1939_MESSAGE *MsgPtr)
{
	/*加载29位ID*/
	ChangeGroupIDofLMO(can驱动的结构体，MsgPtr);
	/*加载数据长度*/
	;
	/*加载数据*/
	;
	/*加载RTR*/
	;
	/*开始发送数据，调用CAN驱动的发送函数*/
	;
}
/*
*输入：J1939_MESSAGE *MsgPtr 数据要存入的内存的指针
*输出： 
*说明：读取CAN驱动的数据，如果没有数据，返回0
		将设备CAN中的数据取出，存入J1939_MESSAGE结构体中
*/

int J1939_CAN_Receive(J1939_MESSAGE *MsgPtr)
{
	if ( ) //判断有没有新的数据
	{
	  // 清除接受标识位
	  ;
	  // 读取数据
	  ;
	  //将29位标志位（can_identifier）写入J1939的结构中
	  _id = XXX; //假如_id等于读取CAN驱动中的ID
	  MsgPtr->Array[0] = _id>>(8*3);
	  MsgPtr->Array[1] = _id>>(8*2);
	  MsgPtr->Array[2] = _id>>(8*1);
	  MsgPtr->Array[3] = _id>>(8*0);
	  //读取数据长度
	  MsgPtr->Mxe.DataLength = XXXXXXX;
	  if (MsgPtr->Mxe.DataLength > 8)
		  MsgPtr->Mxe.DataLength = 8;
	  //读取数据
	  MsgPtr->Mxe.Data[0] = XXXXXX;
	  MsgPtr->Mxe.Data[1] = XXXXXX;
	  MsgPtr->Mxe.Data[2] = XXXXXX;
	  MsgPtr->Mxe.Data[3] = XXXXXX;
	  MsgPtr->Mxe.Data[4] = XXXXXX;
	  MsgPtr->Mxe.Data[5] = XXXXXX;
	  MsgPtr->Mxe.Data[6] = XXXXXX;
	  MsgPtr->Mxe.Data[7] = XXXXXX;

	  return 1;
	}else
	{
		return 0;//没有消息
	}
}
#if J1939_POLL_ECAN == J1939_FALSE
	void J1939_RXinterruptEnable()
	{
		;
	}

	void J1939_RXinterruptDisable()
	{
		;
	}
	void J1939_TXinterruptEnable()
	{
		;
	}

	void J1939_TXinterruptDisable()
	{
		;
	}
	void J1939_TXinterruptOk()
	{
		;
	}
	void CAN_identifier_clc()
	{
		;
	}
#endif
//地址竞争重新估算，设备在总线上要申请的地址,设备是固定地址的，不需要移植此函数
BOOL ECU_RecalculateAddress( unsigned char * Address)
{
	return 0;//没有生成新的地址，返回0
}





#endif
