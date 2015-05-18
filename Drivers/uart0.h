/****************************************Copyright (c)*************************************************
**                      Fujian Junpeng Communicaiton Technology Co.,Ltd.
**                               http://www.easivend.com.cn
**--------------File Info------------------------------------------------------------------------------
** File name:           uart0.h
** Last modified Date:  2013-01-04
** Last Version:         
** Descriptions:        uart0����������غ�������                      
**------------------------------------------------------------------------------------------------------
** Created by:          sunway
** Created date:        2013-01-04
** Version:             V0.1 
** Descriptions:        The original version
**------------------------------------------------------------------------------------------------------
** Modified by:         
** Modified date:       
** Version:             
** Descriptions:        
********************************************************************************************************/
#ifndef __UART0_H 
#define __UART0_H

#define MDB_UART_COM		1   //����MDBͨ�ſ���

#if MDB_UART_COM == 1
void InitUart0(void);
unsigned char uart0_getCmd(unsigned char *data);
void Uart0IsrHandler(void);
void Uart0PutChar(unsigned char ch);
void Uart0PutStr(unsigned char const *Str, unsigned int Len);
void Trace(unsigned char *format,...);
#else
void InitUart0(void);
void Uart0IsrHandler(void);
void Uart0PutChar(unsigned char ch);
void Uart0PutStr(unsigned char const *Str, unsigned int Len);
void Trace(unsigned char *format,...);
unsigned char Uart0BuffIsNotEmpty(void);
unsigned char Uart0GetCh(void);
void ClrUart0Buff(void) ;
#endif

#endif
/**************************************End Of File*******************************************************/
