/****************************************Copyright (c)****************************************************
**                                        xxxx

**--------------File Info---------------------------------------------------------------------------------
** File Name:               IOOperation.c
** Last modified Date:      2014.12.11
** Last Version:            1.0
** Description:             XMC4500 ͨ����������ӿ�����
** 
**--------------------------------------------------------------------------------------------------------
** Created By:              David
** Created date:            2014.12.17
** Version:                 1.0
** Descriptions:            The original version ��ʼ�汾
**
*********************************************************************************************************/

#include "includes.h"
#include "math.h"


/*
*********************************************************************************************************
*                                            Delay_us()
*
* Description : 微秒延时函数
*
* @param[in]  ：us数
*
*
* Return(s)   : void
*
* Note(s)     : none.
*********************************************************************************************************
*/
void Delay_us(uint16_t us)
{
    uint32_t delaynum;

	//delaynum=(SystemCoreClock/1000000)*us;

    delaynum=15*us;    //更精确

	while(delaynum--);

}


 /*
*********************************************************************************************************
*                                            Delay_ms()
*
* Description : 毫秒延时函数
*
* @param[in]  ：ms数
*
*
* Return(s)   : void
*
* Note(s)     : none.
*********************************************************************************************************
*/
void Delay_ms(uint16_t ms)
{

	while(ms--)
   	   Delay_us(1000);

}




void led(void)
{
	Delay_ms(100);
    DIGITAL_IO_SetOutputLow(&O_SystemLight);
    Delay_ms(100);
    DIGITAL_IO_SetOutputHigh(&O_SystemLight);

}




 /*
*********************************************************************************************************
*                                           void HIB_InitHibernat(void)
*
* Description : 休眠控制初始化
*
* @param[in]  ：void
*
*
* Return(s)   : void
*
* Note(s)     : 注意与休眠相关的寄存器是不会被复位的，修改这些寄存器配置后需拔掉RTC的电池.
*********************************************************************************************************
*/

void HIB_InitHibernat(void)
{

//  Initialize Hibernate Domain;
  XMC_SCU_HIB_EnableHibernateDomain();   //  Initialize Hibernate Domain;Release reset of Hibernate Domain;
  XMC_SCU_HIB_EnableInternalSlowClock();   //  enable internal slow clock

// Configure Hibernate Control I/O
    XMC_SCU_HIB_SetPinMode(XMC_SCU_HIB_IO_0,XMC_SCU_HIB_PIN_MODE_OUTPUT_PUSH_PULL_HIBCTRL);
    XMC_SCU_HIB_SetPinOutputLevel(XMC_SCU_HIB_IO_0,XMC_SCU_HIB_IO_OUTPUT_LEVEL_LOW);

// Select Wake-up Triggers
  XMC_SCU_HIB_SetWakeupTriggerInput(XMC_SCU_HIB_IO_1);
  XMC_SCU_HIB_SetPinMode(XMC_SCU_HIB_IO_1,XMC_SCU_HIB_PIN_MODE_INPUT_PULL_UP);

// Enable hibernate wakeup event source
  XMC_SCU_HIB_EnableEvent(XMC_SCU_HIB_EVENT_WAKEUP_ON_NEG_EDGE);   //source1:XMC_SCU_HIB_IO_1

  XMC_SCU_HIB_EnableEvent(XMC_SCU_HIB_EVENT_WAKEUP_ON_RTC);             //source2:RTC EVENT
 // XMC_RTC_EnableHibernationWakeUp(XMC_RTC_WAKEUP_EVENT_ON_HOURS);

  // Restore Context Data to RAM
  // XMC_SCU_HIB_EnterHibernateState();       //Enter Hibernate

}




