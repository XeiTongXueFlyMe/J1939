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
#ifndef __J1939_H 
#define __J1939_H 
/**********************变量类型定义**************************/
#define FALSE 0
#define TRUE 1

typedef unsigned int   j1939_uint32_t;
typedef int   		   j1939_int32_t;
typedef unsigned short j1939_uint16_t;
typedef unsigned char  j1939_uint8_t;
typedef char           j1939_int8_t;


//函数返回代码
#define RC_SUCCESS			    0  //成功
#define RC_QUEUEEMPTY			1  //列队为空
#define RC_QUEUEFULL			1  //列队满
#define RC_CANNOTRECEIVE		2  //不能接收
#define RC_CANNOTTRANSMIT		2  //不能传输
#define RC_PARAMERROR			3  //参数错误
 
//内部常量
#define J1939_FALSE				0 
#define J1939_TRUE				1 

// J1939 默认的优先级（参考J1939文档）
#define J1939_CONTROL_PRIORITY			0x03 
#define J1939_INFO_PRIORITY			    0x06 
#define J1939_PROPRIETARY_PRIORITY		0x06 
#define J1939_REQUEST_PRIORITY			0x06 
#define J1939_ACK_PRIORITY			    0x06 
#define J1939_TP_CM_PRIORITY			0x07 
#define J1939_TP_DT_PRIORITY			0x07 
 
// J1939 定义的地址
#define J1939_GLOBAL_ADDRESS			255 
#define J1939_NULL_ADDRESS			    254 
 
//J1939协议栈的PNG请求响应，相关的定义
#define J1939_PF_REQUEST2				201  //请求
#define J1939_PF_TRANSFER				202  //转移

#define J1939_PF_REQUEST				234  //请求，用提供握手机制
#define J1939_PF_ACKNOWLEDGMENT			232  //确认请求，用提供握手机制

#define J1939_ACK_CONTROL_BYTE			0    //代表确认
#define J1939_NACK_CONTROL_BYTE			1    //PNG不被支持。否定消息
#define J1939_ACCESS_DENIED_CONTROL_BYTE	2//拒绝访问，但是信息是被支持
#define J1939_CANNOT_RESPOND_CONTROL_BYTE	3//不能做出反应，有空能是接受的缓存不够 

//TP协议的一些宏定义 
#define J1939_PF_DT				        235	    // 协议传输---数据传输
#define J1939_PF_TP_CM				    236		// 协议传输---链接管理

//TP的超时时间，单位（ms）
#define J1939_TP_Tr						200
#define J1939_TP_Th						500
#define J1939_TP_T1						750
#define J1939_TP_T2						1250
#define J1939_TP_T3						1250
#define J1939_TP_T4						1050
#define J1939_TP_TIMEOUT_NORMAL			0
#define J1939_TP_IMEOUT_ABNORMAL		1       //超时
#define J1939_RTS_CONTROL_BYTE			16		// TP.CM_RTS
#define J1939_CTS_CONTROL_BYTE			17		// TP.CM_CTS
#define J1939_EOMACK_CONTROL_BYTE		19		// 消息应答结束
#define J1939_BAM_CONTROL_BYTE			32		// 广播公告消息
#define J1939_CONNABORT_CONTROL_BYTE	255	// 连接中断控制字节（放弃连接）
#define J1939_RESERVED_BYTE             0xFF
 
//与J1939网络层有关的定义 
#define J1939_PGN2_REQ_ADDRESS_CLAIM	0x00 
#define J1939_PGN1_REQ_ADDRESS_CLAIM	0xEA   
#define J1939_PGN0_REQ_ADDRESS_CLAIM	0x00 
 
#define J1939_PGN2_COMMANDED_ADDRESS	0x00 
#define J1939_PGN1_COMMANDED_ADDRESS	0xFE	//命令地址消息 
#define J1939_PGN0_COMMANDED_ADDRESS	0xD8    //参考J1939-81 地址命令配置
 
#define J1939_PF_ADDRESS_CLAIMED		238
#define J1939_PF_CANNOT_CLAIM_ADDRESS	238
#define J1939_PF_PROPRIETARY_A			239         //专用A
#define J1939_PF_PROPRIETARY_B			255         //专用B

//是否对TP协议的支持（是否支持长帧（大于8字节的数据）的发送与接受）
#define J1939_TP_RX_TX J1939_TRUE
//TP协议的支持的最大接受发送消息长度(最大可配置为1785)
#define J1939_TP_MAX_MESSAGE_LENGTH 1785
#if J1939_TP_RX_TX
/*TP的状态描述*/
typedef enum
{
	J1939_TP_NULL,
	J1939_TP_RX,
	J1939_TP_TX,
	J1939_TP_OSBUSY,//系统繁忙
}J1939_TP_State;

typedef struct
{
	/*TP的连接状态*/
	J1939_TP_State state;
}J1939_TP_Flags;

typedef struct
{
	j1939_uint32_t PGN ;
	j1939_uint8_t  data[J1939_TP_MAX_MESSAGE_LENGTH] ;
	j1939_uint16_t byte_count;/*数据大小（字节）*/
	j1939_uint8_t  SA;/*目标地址（发送目的地  或  接受来源地）*/

} J1939_MESSAGE_T ;

typedef enum
{
	J1939_TP_TX_WAIT,
	J1939_TP_TX_CM_START,
	J1939_TP_TX_CM_WAIT,
	J1939_TP_TX_DT,
	J1939_TP_WAIT_ACK,
	J1939_TP_TX_ERROR,
	J1939_TX_DONE,
}J1939_TP_Tx_Step;//协议的发送步骤
typedef struct
{
	J1939_MESSAGE_T       tp_tx_msg;
	j1939_uint16_t        time;
	j1939_uint8_t         packet_offset_p;//数据包偏移指针
	j1939_uint8_t         packets_total;//总共有多少个数据包
	j1939_uint8_t         packets_request_num;//请求发送的数据包数（接受方准备接受的数据包数）
	J1939_TP_Tx_Step      state ;
} J1939_TRANSPORT_TX_INFO;

typedef enum
{
	J1939_TP_RX_WAIT,
	J1939_TP_RX_READ_DATA,
	J1939_TP_RX_DATA_WAIT,
	J1939_TP_RX_ERROR,
	J1939_RX_DONE,
}J1939_TP_Rx_Step;//协议的接收步骤
typedef struct
{
	J1939_MESSAGE_T    	 tp_rx_msg;
	j1939_uint8_t		 osbusy;//此位置1，代表系统繁忙，cpu需要处理其他的事物，直接拒绝一切的链接请求
								//如果正在接受中，此位置1，则会发出链接保持消息帧。
	j1939_uint16_t    	 time;
	j1939_uint8_t        packets_total;//总共有多少个数据包
	j1939_uint8_t        packets_ok_num;//已经接受的数据包数
	J1939_TP_Rx_Step     state ;
} J1939_TRANSPORT_RX_INFO;

#endif //J1939_TP_RX_TX
// J1939 Data Structures 
// J1939_MESSAGE_STRUCT旨在J1939消息块映射到设备的地址映射。 只有字段PDU格式不映射到设备寄存器。
// 结构应该简单地使用PDUFormat和忽视PDUFormat_Top。调整将立即接收和传输之前。
// 注:编译器创建结构从低一点的位置高一些位置，所以可能出现不匹配的设备寄存器。
#define J1939_MSG_LENGTH	5  //消息长度
#define J1939_DATA_LENGTH	8  //数据长度
 
union J1939_MESSAGE_UNION 
{ 
	struct   me 
	{ 
		j1939_uint8_t	DataPage			: 1;
		j1939_uint8_t	Res					: 1;
		j1939_uint8_t	Priority			: 3;
		j1939_uint8_t	PDUFormat_Top		: 3;
		j1939_uint8_t	PDUFormat;					
		j1939_uint8_t	PDUSpecific; 
		j1939_uint8_t	SourceAddress; 
		j1939_uint8_t	DataLength 			: 4;
		j1939_uint8_t	RTR					: 4;
		j1939_uint8_t	Data[J1939_DATA_LENGTH]; 
	};
	struct me Mxe;
	j1939_uint8_t		Array[J1939_MSG_LENGTH + J1939_DATA_LENGTH]; 
}; 

#define GroupExtension 		PDUSpecific 
#define DestinationAddress 	PDUSpecific 
typedef union J1939_MESSAGE_UNION J1939_MESSAGE; 

union J1939_FLAGS_UNION
{ 
	
	struct 
	{ 
		j1939_uint32_t	CannotClaimAddress				: 1; 
		j1939_uint32_t	WaitingForAddressClaimContention: 1; 
		j1939_uint32_t	GettingCommandedAddress			: 1; 
		j1939_uint32_t	GotFirstDataPacket				: 1; 
		j1939_uint32_t	ReceivedMessagesDropped			: 1; 
   }; 
	  j1939_uint8_t		FlagVal; 
}; 
typedef union J1939_FLAGS_UNION J1939_FLAG; 

#define Dll_EXPORTS //工程制作DLL

#ifdef Dll_EXPORTS
#define J1939DllAPI __declspec(dllexport)
#else
#define J1939DllAPI __declspec(dllimport)
#endif

#ifdef _cplusplus
#define  EXTERNC	 extern "C"
#else
#define  EXTERNC	 extern
#endif 
//#define CALL __stdcall//__cdecl

// 向外部声明的全局变量
EXTERNC J1939DllAPI j1939_uint8_t	CA_Name[J1939_DATA_LENGTH];
EXTERNC J1939DllAPI j1939_uint8_t 	J1939_Address;
EXTERNC J1939DllAPI J1939_FLAG    	J1939_Flags;
EXTERNC J1939DllAPI J1939_MESSAGE    OneMessage;
EXTERNC J1939DllAPI j1939_uint8_t	RXQueueCount; //RX 列队计数器
#if J1939_TP_RX_TX
//TP协议全局变量  
EXTERNC J1939DllAPI J1939_TP_Flags           	   J1939_TP_Flags_t;
EXTERNC J1939DllAPI J1939_TRANSPORT_RX_INFO         TP_RX_MSG;
EXTERNC J1939DllAPI J1939_TRANSPORT_TX_INFO         TP_TX_MSG;
#endif //J1939_TP_RX_TX


#ifdef J1939_ACCEPT_CMDADD
EXTERNC  DllAPI void  J1939_CommandedAddressHandling( void );
#endif
//出列 用于接受 *MsgPtr代表要存入的缓存
EXTERNC J1939DllAPI j1939_uint8_t	J1939_DequeueMessage( J1939_MESSAGE *MsgPtr );
//入列 用于发送 *MsgPtr代表要发送的缓存
EXTERNC J1939DllAPI j1939_uint8_t  	J1939_EnqueueMessage( J1939_MESSAGE *MsgPtr );
//初始化
EXTERNC J1939DllAPI void 			J1939_Initialization(j1939_uint8_t );
//中断入口
EXTERNC J1939DllAPI void			    J1939_ISR( void );
//扫描入口
EXTERNC J1939DllAPI void 			J1939_Poll( j1939_uint32_t ElapsedTime );
//TP协议的发送函数
EXTERNC J1939DllAPI j1939_int8_t J1939_TP_TX_Message(j1939_uint32_t PGN, j1939_uint8_t SA, j1939_int8_t *data, j1939_uint16_t data_num);
//TP协议的接受函数
EXTERNC J1939DllAPI j1939_int8_t J1939_TP_RX_Message(j1939_int8_t *data,j1939_uint16_t data_num);
//TP轮询函数
EXTERNC J1939DllAPI void J1939_TP_Poll();
#ifdef  __J1939_SOURCE 
static void 		    J1939_ReceiveMessages( void );
static void 		    J1939_RequestForAddressClaimHandling( void );
static j1939_uint8_t 	J1939_TransmitMessages( void );
static void 		    J1939_AddressClaimHandling( j1939_uint8_t Mode );
#endif 
 
#endif


 
