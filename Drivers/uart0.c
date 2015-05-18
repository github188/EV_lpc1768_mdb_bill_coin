/****************************************Copyright (c)*************************************************
**                      Fujian Junpeng Communicaiton Technology Co.,Ltd.
**                               http://www.easivend.com.cn
**--------------File Info------------------------------------------------------------------------------
** File name:           uart0.c
** Last modified Date:  2013-01-04
** Last Version:         
** Descriptions:        Uart0��������                   
**------------------------------------------------------------------------------------------------------
** Created by:          sunway 
** Created date:        2013-01-04
** Version:             V0.1 
** Descriptions:        The original version       
********************************************************************************************************/
#include "..\config.h"
#include <stdarg.h>

//#define	_DEBUG_TRACE

#define	UART0_BPS			9600

#if MDB_UART_COM == 1
#define UART0_BUF_SIZE		128
static volatile unsigned char uart0_buf[UART0_BUF_SIZE];
static volatile unsigned char uart0_in = 0;
static volatile unsigned char uart0_len;
static volatile unsigned char read_buf[UART0_BUF_SIZE];
static volatile unsigned char read_in = 0;

static volatile unsigned char uart0_req = 0;
#else
#define	UART0BUFFERLEN		128
volatile unsigned char Uart0RevBuff[UART0BUFFERLEN];
volatile unsigned char Uart0RxdHead;
volatile unsigned char Uart0RxdTail;
#endif





#if MDB_UART_COM == 1
void InitUart0 (void)
{
    unsigned short int ulFdiv;   
    U0LCR  = 0x83;                                                  //�������ò�����
    ulFdiv = (FPCLK / 16) / UART0_BPS;                              //���ò�����
    U0DLM  = ulFdiv / 256;
    U0DLL  = ulFdiv % 256; 
    U0LCR  = 0x03;                                                  //����������
    U0FCR  = 0x87;                                                  //ʹ��FIFO������8���ֽڴ�����
    U0IER  = 0x01;                                                  //ʹ�ܽ����ж�
	zyIsrSet(NVIC_UART0,(unsigned long)Uart0IsrHandler,PRIO_TEN);	//ʹ�ܽ����ж�
}


void Uart0IsrHandler (void)
{
	unsigned char i,ch,crc;
	OSIntEnter();   
	read_in = 0;	
    while((U0IIR & 0x01) == 0) {                         			//�ж��Ƿ����жϹ���
        switch(U0IIR & 0x0E){ 										//�ж��жϱ�־                                     
            case 0x04 : for(i = 0;i < 8;i++){
							read_buf[read_in++] = U0RBR;
						}
					break;
            case 0x0C : while((U0LSR & 0x01) == 0x01){                         
		                	read_buf[read_in++] = U0RBR;
		                }
		                break;
            default	: break;
        }
    }
	
	if(uart0_req == 0){
		for(i = 0;i < read_in;i++){
			ch = read_buf[i];
			if(uart0_in == 0){
				if(ch == HEAD_PC){
					uart0_buf[uart0_in++] = ch;
					Timer.uart0_timeout = 50;
				}
			}
			else if(uart0_in == 1){ 
				uart0_len = ch;
				if(uart0_len >= UART0_BUF_SIZE){
					uart0_in = 0;
				}
				else{
					uart0_buf[uart0_in++] = ch;
				}
			}
			else if(uart0_in >= uart0_len){ 
				uart0_buf[uart0_in++] = ch; 
				crc = XorCheck((unsigned char *)uart0_buf,uart0_len);
				if(crc == ch){
					uart0_req = 1;
				}
				else{
					uart0_req = 0;
				}
				uart0_len = uart0_in;
				uart0_in = 0;
			}
			else{
				uart0_buf[uart0_in++] = ch;
			}
			
			if(uart0_in > 0 && Timer.uart0_timeout == 0){
				uart0_in = 0;
				//uart0_req = 0;
			}
		}
	}
	
    OSIntExit();

}


unsigned char uart0_getCmd(unsigned char *data)
{
	if(uart0_req == 1){
		memcpy(data,(void *)uart0_buf,uart0_len);
		uart0_req = 0;
		return 1;
	}
	else{
		return 0;
	}
	
}



#else


/*********************************************************************************************************
** Function name:     	InitUart0
** Descriptions:	    ���ڳ�ʼ��������Ϊ8λ����λ��1λֹͣλ������żУ�飬������Ϊ9600
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void InitUart0 (void)
{
    unsigned short int ulFdiv;   
    U0LCR  = 0x83;                                                  //�������ò�����
    ulFdiv = (FPCLK / 16) / UART0_BPS;                              //���ò�����
    U0DLM  = ulFdiv / 256;
    U0DLL  = ulFdiv % 256; 
    U0LCR  = 0x03;                                                  //����������
    U0FCR  = 0x87;                                                  //ʹ��FIFO������8���ֽڴ�����
    U0IER  = 0x01;                                                  //ʹ�ܽ����ж�
	Uart0RxdHead  = 0;
	Uart0RxdTail  = 0;
	memset((void *)Uart0RevBuff,0x00,UART0BUFFERLEN);				//��ʼ��������
	zyIsrSet(NVIC_UART0,(unsigned long)Uart0IsrHandler,PRIO_TEN);	//ʹ�ܽ����ж�
}



/*********************************************************************************************************
* Function Name:        Uart0IsrHandler
* Description:          UART0 �жϴ�����
* Input:                None
* Output:               None
* Return:               None
*********************************************************************************************************/
void Uart0IsrHandler (void)
{
	unsigned char Num;
	unsigned char rxd_head;
	unsigned char rxd_data;
	//OS_ENTER_CRITICAL();
	OSIntEnter();      
    while((U0IIR & 0x01) == 0)                          			//�ж��Ƿ����жϹ���
	{
        switch(U0IIR & 0x0E) 										//�ж��жϱ�־
		{                                         
            case 0x04 : for (Num = 0; Num < 8; Num++)				//���������ж�
						{
		                	rxd_data = U0RBR;
							rxd_head = (Uart0RxdHead + 1);
					        if( rxd_head >= UART0BUFFERLEN ) 
					           	rxd_head = 0;
					        if( rxd_head != Uart0RxdTail) 
					        {
					           	Uart0RevBuff[Uart0RxdHead] = rxd_data;
					           	Uart0RxdHead = rxd_head;
					        }
		                }
		                break;
            case 0x0C : while((U0LSR & 0x01) == 0x01)				//�ַ���ʱ�жϣ��ж������Ƿ�������
						{                         
		                	rxd_data = U0RBR;
							rxd_head = (Uart0RxdHead + 1);
					        if( rxd_head >= UART0BUFFERLEN ) 
					           	rxd_head = 0;
					        if( rxd_head != Uart0RxdTail) 
					        {
					           	Uart0RevBuff[Uart0RxdHead] = rxd_data;
					           	Uart0RxdHead = rxd_head;
					        }
		                }
		                break;
            default	: break;
        }
    }
    OSIntExit();
    //OS_EXIT_CRITICAL();
}



/*********************************************************************************************************
** Function name:	    Uart0BuffIsNotEmpty
** Descriptions:	    �жϻ������Ƿ�ǿ�
** input parameters:    ��
** output parameters:   ��
** Returned value:      1���ǿգ�0����
*********************************************************************************************************/
unsigned char Uart0BuffIsNotEmpty(void)
{
	if(Uart0RxdHead==Uart0RxdTail) 
		return 0;
	else
		return 1;
	
}


/*********************************************************************************************************
** Function name:     	Uart0_ClrBuf
** Descriptions:	    �������2���ջ�����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void ClrUart0Buff(void) 
{
	Uart0RxdTail = 0;
	Uart0RxdHead = 0;
}	



/*********************************************************************************************************
** Function name:	    Uart0GetCh
** Descriptions:	    �Ӵ��ڻ�������ȡһ���ֽڣ���֮ǰ��Ҫ�жϻ������ǿ�
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��ȡ��������
*********************************************************************************************************/
unsigned char Uart0GetCh(void)
{
    unsigned char ch;
    ch = Uart0RevBuff[Uart0RxdTail];
    Uart0RevBuff[Uart0RxdTail] = 0;
    Uart0RxdTail++;
    if(Uart0RxdTail >= UART0BUFFERLEN)
		Uart0RxdTail = 0;
	return ch;
}

#endif


/*********************************************************************************************************
** Function name:	    Uart0PutChar
** Descriptions:	    �򴮿ڷ����ӽ����ݣ����ȴ����ݷ�����ɣ�ʹ�ò�ѯ��ʽ
** input parameters:    ch:Ҫ���͵�����
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void Uart0PutChar(unsigned char ch)
{
    U0THR = ch;                                                      //д������
    while((U0LSR & 0x20) == 0);                                      //�ȴ����ݷ������
}

/*********************************************************************************************************
** Function name:	    Uart0PutStr
** Descriptions:	    �򴮿ڷ����ַ���
** input parameters:    Str:  Ҫ���͵��ַ���ָ��
**                      Len:   Ҫ���͵����ݸ���
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void Uart0PutStr(unsigned char const *Str, unsigned int Len)
{
    unsigned int i;  
    for (i=0;i<Len;i++) 
	{
        Uart0PutChar(*Str++);
    }
}



/*********************************************************************************************************
** Function name:	    Trace
** Descriptions:	    ����Trace
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void Trace(unsigned char *format,...)
{
 	#ifdef _DEBUG_TRACE
	va_list arg_ptr;
	unsigned char StringTemp[256];
	unsigned int i;
	unsigned int len;
	va_start(arg_ptr, format);
	len = vsprintf((char *)StringTemp,(const char *)format,arg_ptr);
	va_end(arg_ptr);
	for(i=0;i<len;i++)
	{
		Uart0PutChar(StringTemp[i]);
	}
	#endif	
}


/**************************************End Of File*******************************************************/
