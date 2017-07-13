/****************************************Copyright (c)****************************************************
**                                     
**--------------File Info---------------------------------------------------------------------------------
** File Name:               IOOperation.h
** Last modified Date:      2014.12
** Last Version:            1.0
** Description:             XMC4500 ͨ����������ӿ�����
** 
**--------------------------------------------------------------------------------------------------------
** Created By:              David
** Created date:            2014.12
** Version:                 1.0
** Descriptions:            The original version
*********************************************************************************************************/
#include "includes.h"

#ifndef __MIDDLEDRIVER_H
#define __MIDDLEDRIVER_H

// Input and output ports definition


extern void Delay_us(uint16_t us);
extern void Delay_ms(uint16_t ms);
extern void led(void);
extern void HIB_InitHibernat(void);


#endif /* __MIDDLEDRIVER_H */
