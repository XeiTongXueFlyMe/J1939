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
 *	
 *
 * Version     Date        Description
 * ----------------------------------------------------------------------
 * v1.00     2017/06/04    首个版本
 * v1.01     2017/08/04    完善功能
 * v1.10     2017/11/22    Version 1 稳定发布版
 * v2.01     2017/11/24    Version 2 测试版发布
 * Author               Date         changes
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *XieTongXueFlyMe       7/06/04      首个版本
 *XieTongXueFlyMe       7/08/04      增加对TP的支持
 *XieTongXueFlyMe       7/11/24      增加对多路CAN硬件的收发，和报文处理
 **********************************************************************/
#ifndef __J1939_Config_H 
#define __J1939_Config_H 

#include "CAN.h"//使用者，自己工程必要的声明,主要是CAN驱动
#include "J1939.H"
extern CAN_NODE   Can_Node;   //CAN硬件选择


/***************************J1939 标识符和地址配置*************************************/
//设备默认的地址（地址命名是有规定的，参考J1939的附录B 地址和标识符的分配）
#define J1939_STARTING_ADDRESS_1 248
#define J1939_STARTING_ADDRESS_2 244
#define J1939_STARTING_ADDRESS_3 247
#define J1939_STARTING_ADDRESS_4 0

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
//是否使用接受协议TP对设备的地址进行远程配置（参考J1939-21，J1939-81）
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
/*不使用中断模式，不对下面的函数进行移植*/
#if J1939_POLL_ECAN == J1939_FALSE
	#define Port_RXinterruptEnable() J1939_RXinterruptEnable() 
	#define Port_RXinterruptDisable() J1939_RXinterruptDisable() 
	#define Port_TXinterruptEnable() J1939_TXinterruptEnable() 
	#define Port_TXinterruptDisable() J1939_TXinterruptDisable() 
	#define Port_TXinterruptOk() J1939_TXinterruptOk()
	#define Port_CAN_identifier_clc()  CAN_identifier_clc()
#endif 

/***************************************************************************************/

/*
*输入：
*输出： 
*说明：基于SAE J1939协议，我们需要CAN控制器提供至少3个滤波器给J1939协议代码。三个滤波器分别配置如下： 
		1. 设置滤波器0，只接受广播信息（PF = 240 -255）。 
		2. 设置设置滤波器1，2只接受全局地址（J1939_GLOBAL_ADDRESS） 
		3. 随着程序的运行，将改变滤波器1，来适应程序逻辑。
	J1939_SetAddressFilter() 是用来设置滤波器2的， 函数主要设置PS位（目标地址），其目的是，让控制器
	只接受发送给本设备的消息。
*/
void J1939_SetAddressFilter(unsigned char Ps_Address)
{
//	switch (Can_Node)
//	{
//		case  Select_CAN_NODE_1:
//		{
//			break;
//		}
//		case  Select_CAN_NODE_2:
//		{
//			break;
//		}
//		case  Select_CAN_NODE_3:
//		{
//			break;
//		}
//		case  Select_CAN_NODE_4:
//		{
//			break;
//		}
//		default  :
//		{
//			break;
//		}
//	}
	/*上面屏蔽的是标准的流程*/
	/*地址滤波和动态地址分配出现冲突（暂时优先考虑地址滤波）*/
	/*暂时在Select_CAN_NODE_1中 把每一路的滤波都设置了*/
	switch (Can_Node)
	{
		case  Select_CAN_NODE_1:
		{
			/*设置每一路的ID号*/
		

			/*设置每一路的遮掩码*/
	

			break;
		}
		default  :
		{
			break;
		}
	}
}
/*
*输入： CAN自带的结构体（包含ID） 协议本身的结构体 IDJ1939_MESSAGE
*输出： 
*说明：配置can的远程帧ID,     函数ChangeGroupIDofLMO（）提供这样的配置。
		配置方法和移植，和J1939_CAN_Transmit函数有关，建议一起配置：
*/
void ChangeGroupIDofLMO()
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
	
	switch (Can_Node)
	{
		case  Select_CAN_NODE_1:
		{
			
			/*加载第一路CAN硬件的29位ID*/
			ChangeGroupIDofLMO();
			/*CAN硬件加载数据长度*/
	
			/*CAN硬件加载数据*/

			/*CAN硬件加载RTR*/

			//CAN硬件开始发送数据
			
			break;
		}
		case  Select_CAN_NODE_2:
		{
	
			/*加载第二路CAN硬件的29位ID*/
			ChangeGroupIDofLMO();
			/*CAN硬件加载数据长度*/
	
			/*CAN硬件加载数据*/

			/*CAN硬件加载RTR*/

			//CAN硬件开始发送数据
			break;
		}
		case  Select_CAN_NODE_3:
		{
			
			/*加载第三路CAN硬件的29位ID*/
			ChangeGroupIDofLMO();
			/*CAN硬件加载数据长度*/
	
			/*CAN硬件加载数据*/

			/*CAN硬件加载RTR*/

			//CAN硬件开始发送数据
			break;
		}
		case  Select_CAN_NODE_4:
		{
			/*加载第四路CAN硬件的29位ID*/
			ChangeGroupIDofLMO();
			/*CAN硬件加载数据长度*/
	
			/*CAN硬件加载数据*/

			/*CAN硬件加载RTR*/

			//CAN硬件开始发送数据
			break;
		}
		default  :
		{
			break;
		}
	}
}
/*
*输入：J1939_MESSAGE *MsgPtr 数据要存入的内存的指针
*输出：1 | 0
*说明：读取CAN驱动的数据，如果没有数据，返回0
		将设备CAN中的数据取出，存入J1939_MESSAGE结构体中
*/

int J1939_CAN_Receive(J1939_MESSAGE *MsgPtr)
{

	switch (Can_Node)
	{
		case  Select_CAN_NODE_1:
		{
			if(如果有新的数据)
			{
				//你的代码，从CAN硬件1 中将数据读取后，存入 MsgPtr
				return 1;
			}
			return 0;
			break;
		}
		case  Select_CAN_NODE_2:
		{
			if(如果有新的数据)
			{
				//你的代码，从CAN硬件2 中将数据读取后，存入 MsgPtr
				return 1;
			}
			return 0;
			break;

		}
		case  Select_CAN_NODE_3:
		{
			if(如果有新的数据)
			{
				//你的代码，从CAN硬件3 中将数据读取后，存入 MsgPtr
				return 1;
			}
			return 0;
			break;

		}
		case  Select_CAN_NODE_4:
		{
			if(如果有新的数据)
			{
				//你的代码，从CAN硬件4 中将数据读取后，存入 MsgPtr
				return 1;
			}
			return 0;
			break;
		}
		default  :
		{
			return 0;//没有消息
			break;
		}
	}
	return 0;//没有消息
}
/*
*输入：
*输出： 
*说明：地址竞争重新估算，设备在总线上要重新申请的地址。
	  设备是固定地址的，不需要移植此函数。
	  函数为各个厂家想申请的地址，如果默认地址没有申请成功
	  则调用此函数，重新申请一个希望得到的地址，当然可能再一次
	  申请不成功，所以函数可以返回多个不同的地址，一次返回一个
*/
j1939_int8_t  ECU_RecalculateAddress( unsigned char * Address)
{
	return 0;//没有生成新的地址，返回0
}
/*不使用中断模式，不对下面的函数进行移植*/
#if J1939_POLL_ECAN == J1939_FALSE
/*
*输入：
*输出：
*说明：使能接受中断
*/
	void J1939_RXinterruptEnable()
	{
		;
	}
/*
*输入：
*输出：
*说明：失能接受中断
*/
	void J1939_RXinterruptDisable()
	{
		;
	}
/*
*输入：
*输出：
*说明：使能发送中断
*/
	void J1939_TXinterruptEnable()
	{
		;
	}
/*
*输入：
*输出：
*说明：失能发送中断
*/
	void J1939_TXinterruptDisable()
	{
		;
	}
/*
*输入：
*输出：
*说明：触发发送中断标致位，当协议栈在中断模式下，要发送消息，将调用此函数
	CAN驱动函数，就将直接把消息发送出去，不需要协议在调用任何can驱动函数
*/
	void J1939_TXinterruptOk()
	{
		;
	}
/*
*输入：
*输出：
*说明：清除CAN驱动相关的中断产生标识位，包括（发送中断标志位，接受中断标
	志位，can总线错误标识位）
*/
	void CAN_identifier_clc()
	{
		;
	}
#endif

#endif
