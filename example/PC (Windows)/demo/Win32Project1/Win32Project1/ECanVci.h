

#ifndef _V_ECANVCI_H_                                        
#define _V_ECANVCI_H_

#include "windows.h"
#include <iostream>
using namespace std;


//接口卡类型定义

#define USBCAN1		3
#define USBCAN2		4

//CAN错误码
#define	ERR_CAN_OVERFLOW			0x0001	//CAN控制器内部FIFO溢出
#define	ERR_CAN_ERRALARM			0x0002	//CAN控制器错误报警
#define	ERR_CAN_PASSIVE				0x0004	//CAN控制器消极错误
#define	ERR_CAN_LOSE				0x0008	//CAN控制器仲裁丢失
#define	ERR_CAN_BUSERR				0x0010	//CAN控制器总线错误
#define	ERR_CAN_REG_FULL			0x0020	//CAN接收寄存器满
#define	ERR_CAN_REG_OVER			0x0040	//CAN接收寄存器溢出
#define	ERR_CAN_ZHUDONG	    		0x0080	//CAN控制器主动错误

//通用错误码
#define	ERR_DEVICEOPENED			0x0100	//设备已经打开
#define	ERR_DEVICEOPEN				0x0200	//打开设备错误
#define	ERR_DEVICENOTOPEN			0x0400	//设备没有打开
#define	ERR_BUFFEROVERFLOW			0x0800	//缓冲区溢出
#define	ERR_DEVICENOTEXIST			0x1000	//此设备不存在
#define	ERR_LOADKERNELDLL			0x2000	//装载动态库失败
#define ERR_CMDFAILED				0x4000	//执行命令失败错误码
#define	ERR_BUFFERCREATE			0x8000	//内存不足


//函数调用返回状态值
#define	STATUS_OK					1
#define STATUS_ERR					0
	
#define CMD_DESIP			0
#define CMD_DESPORT			1
#define CMD_CHGDESIPANDPORT		2


//1.ZLGCAN系列接口卡信息的数据类型。
typedef  struct  _BOARD_INFO{
		USHORT	hw_Version;
		USHORT	fw_Version;
		USHORT	dr_Version;
		USHORT	in_Version;
		USHORT	irq_Num;
		BYTE	can_Num;
		CHAR	str_Serial_Num[20];
		CHAR	str_hw_Type[40];
		USHORT	Reserved[4];
} BOARD_INFO,*P_BOARD_INFO; 

//2.定义CAN信息帧的数据类型。
typedef  struct  _CAN_OBJ{
	UINT	ID;
	UINT	TimeStamp;
	BYTE	TimeFlag;
	BYTE	SendType;
	BYTE	RemoteFlag;//是否是远程帧
	BYTE	ExternFlag;//是否是扩展帧
	BYTE	DataLen;
	BYTE	Data[8];
	BYTE	Reserved[3];
}CAN_OBJ,*P_CAN_OBJ;

//3.定义CAN控制器状态的数据类型。
typedef struct _CAN_STATUS{
	UCHAR	ErrInterrupt;
	UCHAR	regMode;
	UCHAR	regStatus;
	UCHAR	regALCapture;
	UCHAR	regECCapture; 
	UCHAR	regEWLimit;
	UCHAR	regRECounter; 
	UCHAR	regTECounter;
	DWORD	Reserved;
}CAN_STATUS,*P_CAN_STATUS;

//4.定义错误信息的数据类型。
typedef struct _ERR_INFO{
		UINT	ErrCode;
		BYTE	Passive_ErrData[3];
		BYTE	ArLost_ErrData;
} ERR_INFO,*P_ERR_INFO;

//5.定义初始化CAN的数据类型
typedef struct _INIT_CONFIG{
	DWORD	AccCode;
	DWORD	AccMask;
	DWORD	Reserved;
	UCHAR	Filter;
	UCHAR	Timing0;	
	UCHAR	Timing1;	
	UCHAR	Mode;
}INIT_CONFIG,*P_INIT_CONFIG;

typedef struct _tagChgDesIPAndPort
{
	char szpwd[10];
	char szdesip[20];
	int desport;
}CHGDESIPANDPORT;

///////// new add struct for filter /////////
typedef struct _FILTER_RECORD{
	DWORD ExtFrame;	//是否为扩展帧
	DWORD Start;
	DWORD End;
}FILTER_RECORD,*P_FILTER_RECORD;

//#define Dll_EXPORTS

#ifdef Dll_EXPORTS
	#define DllAPI __declspec(dllexport)
#else
	#define DllAPI __declspec(dllimport)

#endif


#define EXTERNC	 extern "C"
#define CALL __stdcall//__cdecl

//extern "C"
//{

EXTERNC	DllAPI DWORD CALL OpenDevice(DWORD DeviceType,DWORD DeviceInd,DWORD Reserved);
EXTERNC DllAPI DWORD CALL CloseDevice(DWORD DeviceType,DWORD DeviceInd);

EXTERNC DllAPI DWORD CALL InitCAN(DWORD DeviceType, DWORD DeviceInd, DWORD CANInd, P_INIT_CONFIG pInitConfig);

EXTERNC DllAPI DWORD CALL ReadBoardInfo(DWORD DeviceType,DWORD DeviceInd,P_BOARD_INFO pInfo);
EXTERNC DllAPI DWORD CALL ReadErrInfo(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd,P_ERR_INFO pErrInfo);
EXTERNC DllAPI DWORD CALL ReadCANStatus(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd,P_CAN_STATUS pCANStatus);

EXTERNC DllAPI DWORD CALL GetReference(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd,DWORD RefType,PVOID pData);
EXTERNC DllAPI DWORD CALL SetReference(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd,DWORD RefType,PVOID pData);

EXTERNC DllAPI ULONG CALL GetReceiveNum(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd);
EXTERNC DllAPI DWORD CALL ClearBuffer(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd);

EXTERNC DllAPI DWORD CALL StartCAN(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd);
EXTERNC DllAPI DWORD CALL ResetCAN(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd);

EXTERNC DllAPI ULONG CALL Transmit(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd,P_CAN_OBJ pSend,ULONG Len);
EXTERNC DllAPI ULONG CALL Receive(DWORD DeviceType,DWORD DeviceInd,DWORD CANInd,P_CAN_OBJ pReceive,ULONG Len,INT WaitTime);

class ECan
{
public:
	/*设备号*/
	int nDeviceType; // USBCAN-II
	/*引索号*/
	int nDeviceInd;
	/*通常设为0*/
	int nReserved;
	/*can通道号^can1 = 1,can2 = 2^*/
	int nCANInd;
	/*配置初始化结构体*/
	INIT_CONFIG vic;
	/*USBCAN的信息结构体*/
	BOARD_INFO vbi;
	/*发送数据结构体*/
	CAN_OBJ nTransmitData;
	/*用于接受数据的结构体*/
	CAN_OBJ nReceiveData[100];  //暂时用100个报作为邮箱
	/*接受超时时间,以毫秒为单位*/
	BYTE nReceiveTime;
public:
	ECan()
	{
		nDeviceType = 4; // USBCAN-II 
		nDeviceInd = 0;  // 默认为0
		nReserved = 0;   // 默认为0
		nCANInd = 0;
		nReceiveTime = 250;

		ZeroMemory(&vic, sizeof(INIT_CONFIG));
		ZeroMemory(&vbi, sizeof(BOARD_INFO));
		ZeroMemory(&nTransmitData, sizeof(CAN_OBJ));
		vic.AccMask = 0xffffffff;
		vic.Filter = 0;//不使能滤波
		vic.Mode = 0; //0表示正常模式，1表示只听模式
		/*代表波特率为250Kbps*/
		vic.Timing0 = 0x01;
		vic.Timing1 = 0x1c;
		
		DWORD dwRel;
		Sleep(100);
		dwRel = ECanOpenDevice();
		if (dwRel != STATUS_OK)
		{
			cout << "打开USBCAN失败\n";
			return ;
		}
		else
		{
			cout << "打开USBCAN成功\n"; 
		}
		Sleep(100);
		dwRel = ECanInitCAN();
		Sleep(100);
		ClearBuffer(nDeviceType, nDeviceInd, nCANInd);
		if (dwRel != STATUS_OK)
		{
			cout << "初始化USBCAN失败\n";
			ECanCloseDevice();
			cout << "USBCAN设备已经关闭\n";
			return ;
		}
		else
		{
			cout << "初始化USBCAN成功\n";
		}
		Sleep(100);
		dwRel = ECanReadBoardInfo();
		if(dwRel != STATUS_OK)
		{
			cout << "读取USBCAN的基础信息失败\n";
			ZeroMemory(&vbi, sizeof(BOARD_INFO));
		}
		else
		{
			cout << "读取USBCAN的基础信息成功\n";
		}
		Sleep(100);
		dwRel = ECanStartCAN();
		if (dwRel != STATUS_OK)
		{
			cout << "USBCAN启动失败\n";
			ZeroMemory(&vbi, sizeof(BOARD_INFO));
			ECanCloseDevice();
			cout << "USBCAN设备已经关闭\n";
		}
		else
		{
			cout << "USBCAN启动成功\n";
		}
		Sleep(1000);
	}
	~ECan()
	{
		ECanCloseDevice();
	}
	void ECanReset()
	{
		nDeviceType = 4; // USBCAN-II 
		nDeviceInd = 0;  // 默认为0
		nReserved = 0;   // 默认为0
		nCANInd = 0;
		nReceiveTime = 250;

		ZeroMemory(&vic, sizeof(INIT_CONFIG));
		ZeroMemory(&vbi, sizeof(BOARD_INFO));
		ZeroMemory(&nTransmitData, sizeof(CAN_OBJ));
		vic.AccMask = 0xffffffff;
		vic.Filter = 0;//不使能滤波
		vic.Mode = 0; //0表示正常模式，1表示只听模式
					  /*代表波特率为250Kbps*/
		vic.Timing0 = 0x01;
		vic.Timing1 = 0x1c;

		DWORD dwRel;
		Sleep(100);
		dwRel = ECanOpenDevice();
		if (dwRel != STATUS_OK)
		{
			cout << "打开USBCAN失败\n";
			return;
		}
		else
		{
			cout << "打开USBCAN成功\n";
		}
		Sleep(100);
		dwRel = ECanInitCAN();
		Sleep(100);
		ClearBuffer(nDeviceType, nDeviceInd, nCANInd);
		if (dwRel != STATUS_OK)
		{
			cout << "初始化USBCAN失败\n";
			ECanCloseDevice();
			cout << "USBCAN设备已经关闭\n";
			return;
		}
		else
		{
			cout << "初始化USBCAN成功\n";
		}
		Sleep(100);
		dwRel = ECanReadBoardInfo();
		if (dwRel != STATUS_OK)
		{
			cout << "读取USBCAN的基础信息失败\n";
			ZeroMemory(&vbi, sizeof(BOARD_INFO));
		}
		else
		{
			cout << "读取USBCAN的基础信息成功\n";
		}
		Sleep(100);
		dwRel = ECanStartCAN();
		if (dwRel != STATUS_OK)
		{
			cout << "USBCAN启动失败\n";
			ZeroMemory(&vbi, sizeof(BOARD_INFO));
			ECanCloseDevice();
			cout << "USBCAN设备已经关闭\n";
		}
		else
		{
			cout << "USBCAN启动成功\n";
		}
		Sleep(1000);
	}
	DWORD ECanOpenDevice()
	{
		return OpenDevice(nDeviceType, nDeviceInd, nReserved);
	}
	DWORD ECanCloseDevice()
	{
		return CloseDevice(nDeviceType, nDeviceInd);
	}
	DWORD ECanInitCAN()
	{
		return InitCAN(nDeviceType, nDeviceInd, nCANInd,&vic);
	}
	DWORD ECanStartCAN()
	{
		return StartCAN(nDeviceType, nDeviceInd, nCANInd);
	}
	DWORD ECanTransmit(UINT id,BYTE sendtype, BYTE remoteflag, BYTE externflag, BYTE datalen,BYTE data[8])
	{
		nTransmitData.ID = id;
		nTransmitData.SendType = sendtype;        //发送帧类型      
		nTransmitData.RemoteFlag = remoteflag;    //是否是远程帧 =0时为数据帧，=1时为远程帧
		nTransmitData.ExternFlag = externflag;    //是否是扩展帧 =0时为标准帧（11位帧ID）=1时为扩展帧（29位帧ID）
		nTransmitData.DataLen = datalen;          //数据长度    <=8
		int i = 0;
		for (i = 0; i < datalen; i++)
		{
			nTransmitData.Data[i] = data[i];
		}
		for ( ; i < 8; i++)
		{
			nTransmitData.Data[i] =0xff;
		}
		return Transmit(nDeviceType, nDeviceInd, nCANInd,&nTransmitData, 1);
	}
	/*调用函数后可以去，数据将存入nReceiveData[]缓存*/
	DWORD ECanReceive()
	{
		/*缓存清零*/
		Sleep(100);
	//	ECanStartCAN();
     	int DwRel = GetReceiveNum(nDeviceType, nDeviceInd, nCANInd);
		for (int i = 0; i < DwRel; i++)
		{
			ZeroMemory((nReceiveData + i), sizeof(CAN_OBJ));
		}
		if(DwRel != 0)
		{
			/*返回实际读取到的帧数。如果返回值为0xFFFFFFFF，则表示读取数据失败，有错误发生，请调用ReadErrInfo函数来获取错误码*/
			int DwRell = Receive(nDeviceType, nDeviceInd, nCANInd, nReceiveData, 100, nReceiveTime);
			ClearBuffer(nDeviceType, nDeviceInd, nCANInd);
		//	ResetCAN(nDeviceType, nDeviceInd, nCANInd);
			return  DwRell;
		}
		//ResetCAN(nDeviceType, nDeviceInd, nCANInd);
	
		return 0;
	}
	DWORD ECanReadBoardInfo()
	{
		return ReadBoardInfo(nDeviceType, nDeviceInd, &vbi);
	}
};

#endif