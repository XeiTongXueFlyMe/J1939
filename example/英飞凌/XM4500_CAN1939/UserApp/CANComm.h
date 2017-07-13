#include "includes.h"


#ifndef CANSYSCOMM_H_
#define CANSYSCOMM_H_


osThreadId thread_CANComm_ID;               // assigned ID for thread_Comm
#define   thread_Periodms_CANComm       500

struct  DebugCommand_Inquiry_Type
{

	//位段结构
		 struct
		 {
			uint8_t  SummaryInfo:1;
			uint8_t  CellsVoltage:1;
			uint8_t  PointsTemperature:1;
			uint8_t  CellsSOC:1;
			uint8_t  CellsSOH:1;
			uint8_t  CellsIb:1;
			uint8_t  CellsBr:1;
			uint8_t  SystemTime:1;

		}Inquiry1;

		 struct
		 {
			uint8_t  BordID:1;
			uint8_t  SystemInfo:1;
			uint8_t  reserved1:1;
			uint8_t  reserved2:1;
			uint8_t  reserved3:1;
			uint8_t  reserved4:1;
			uint8_t  reserved5:1;
			uint8_t  reserved6:1;

		}Inquiry2;

}DebugCommand_Inquiry;

//CAN开关型命令组
struct  DebugCommand_ONOFF_Type
{
	//位段结构
		 struct
		 {
			uint8_t  EnterHibernate:1;       //进入休眠
			uint8_t  AutoHibernate:1;        //自动休眠开关
			uint8_t  RecoveryRunParameter:1; //恢复运行参数
			uint8_t  RecoveryConfigParameter:1; //恢复出厂配置
			uint8_t  ReleaseDisChargeProtectFlag:1;  //解除过放保护
			uint8_t  reserved1:1;
			uint8_t  reserved2:1;
			uint8_t  reserved3:1;

		}ONOFF1;
//		 struct
//		 {
//            ;
//		}ONOFF2;

}DebugCommand_ONOFF;


 struct SystemStatus_Type
 {
	 //位段结构
	 struct
	 {
	    uint8_t  CellCharge_Over:1;
	    uint8_t  CellDischarge_Over:1;
	    uint8_t  Temp_Over:1;
	    uint8_t  DischargeCurrent_Over:1;
	    uint8_t  SOC_Below:1;
	    uint8_t  Insulativity_Low:1;                 //绝缘度低
	    uint8_t  Insulativity_Below:1;              //绝缘度过低
	    uint8_t  CANOutage:1 ;                     //通讯中断

	 }Status1;

	 struct
	 {
		uint8_t  TotalValtage_Over:1;
		uint8_t  TotalValtage_Below:1;
	    uint8_t  ChargeCurrent_Over:1;
	    uint8_t  Temp_Below:1;
	    uint8_t  TempDiff_Over:1;            //温差过大
	    uint8_t  ValtageDiff_Over:1;         //压差过大
	    uint8_t  TempRise_Over:1;           //温升过快
	    uint8_t  ShortCircuitProtect:1 ;     //短路保护

	 }Status2;

	 struct
	 {
		 uint8_t  RelayStatus_Discharge:1;
		 uint8_t  RelayStatus_Charge:1;
	     uint8_t  RelayStatus_PreCharge:1;
	     uint8_t  RelayStatus_Heat:1;
	     uint8_t  RelayStatus_Cool :1;
	     uint8_t  RelayStatus_AlarmBeep:1;
	     uint8_t  RelayStatus_Reserve1:1;
	     uint8_t  RelayStatus_Reserve2:1 ;

	}Status3;


}SystemStatus;


//充电机状态
STRUCT(ChargerState_Type)
{

	    uint8_t StateData[8];          //所有状态数据

  		uint16_t Voltage;              //输出电压:0.1V
  		uint16_t Current;              //输出电流:0.1A
  		uint8_t   WorkState;		 //0：正常充电；1：空载关闭；2：电池反接关闭；3：充满关闭
  		bool  CommStatus;        //通讯状态  0：正常；1：超时
  		bool  TempStatus;         //温度状态  0：正常；1：过温保护

} ChargerState;

extern void CAN_TransmitSingleFramePG ( const CAN_NODE_LMO_t *const lmo_ptr, uint8_t * PGArray);      //发送单帧 参数组
extern void thread_CANComm(void const *argument);
extern void  GetChargerState(ChargerState_Type * State);
extern void  ControlCharger(uint16_t voltage, uint16_t current, bool onoff);
extern void Message_toUper(void);
extern void RespondONOFFCommand(void);
extern void Message_Broadcast(void) ;
extern void Message_toDashBoard(void);

#endif /* CANSYSCOMM_H_ */
