/****************************************Copyright (c)*************************************************
**                      Fujian Junpeng Communicaiton Technology Co.,Ltd.
**                               http://www.easivend.com.cn
**--------------File Info------------------------------------------------------------------------------
** File name:           config.h
** Last modified Date:  2013-01-06
** Last Version:        No
** Descriptions:        头文件配置，位于工程主目录                     
**------------------------------------------------------------------------------------------------------
** Created by:          chensunwei
** Created date:        2013-01-06
** Version:             V0.1
** Descriptions:        The original version        
********************************************************************************************************/
#ifndef __CONFIG_H 
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  通用库相关头文件
*********************************************************************************************************/
#include <stdint.h> 
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <setjmp.h>
#include <rt_misc.h>
#include <stdarg.h>
/*********************************************************************************************************
  固件库相关头文件
*********************************************************************************************************/
#include ".\firmware\firmware.h"
#include ".\firmware\zy_if.h"
#include ".\firmware\LPC1700.h"
#include ".\firmware\target_cfg.h"
#include ".\firmware\LPC1700PinCfg.h"

/*********************************************************************************************************
  外设驱动头文件
*********************************************************************************************************/
#include ".\Drivers\uart0.h"
#include ".\Drivers\uart1.h"
#include ".\Drivers\uart2.h"
#include ".\Drivers\uart3.h"
#include ".\Drivers\buzzer.h"
#include ".\Drivers\i2c.h"
#include ".\Drivers\timer.h"
#include ".\Drivers\key.h"
#include ".\Drivers\MDB.h"
#include ".\Drivers\board.h"


/*********************************************************************************************************
  应用层
*********************************************************************************************************/



/*********************************************************************************************************
  中间层
*********************************************************************************************************/

#include ".\API\logApi.h"
#include ".\API\flashApi.h"
#include ".\API\billApi.h"
#include ".\API\coinApi.h"
#include ".\API\hopperApi.h"
#include ".\API\PcoinApi.h"
#include ".\API\pcConApi.h"
#ifdef __cplusplus
	}
#endif 

#endif

/**************************************End Of File*******************************************************/
