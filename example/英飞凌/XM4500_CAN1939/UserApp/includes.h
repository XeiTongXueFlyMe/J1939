/****************************************Copyright (c)*********************
**                                     
**--------------File Info---------------------------------------------------------------------------------
** File Name:               includes.h
** Last modified Date:      2014.12
** Last Version:            1.0
** Description:             This file is used for including and gathering head files.
** 
**--------------------------------------------------------------------------------------------------------
** Created By:              David
** Created date:            2014.12
** Version:                 1.0
** Descriptions:            The original version
**************************************************************************/

#ifndef __INCLUDES_H
#define __INCLUDES_H

#include <DAVE.h>    		//Declarations from DAVE3 Code Generation (includes SFR declaration)
#include <stdio.h>
#include <string.h>
#include "math.h"


#define STRUCT(type) typedef struct tag##type type;\
struct tag##type

#include "MiddleDriver.h"
#include "CANComm.h"



extern bool OSLoadCompleteFlag;    //各项初始化完成后，操作系统加载完成标志。用于防止初始化时使用到系统函数而出错

#endif /* __INCLUDES_H */
