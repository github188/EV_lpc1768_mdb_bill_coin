#include "deviceTask.h"
#include "..\config.h"

#define DEV_DEBUG
#ifdef DEV_DEBUG

#define print_dev(...)	Trace(__VA_ARGS__)
#else
#define print_dev(...)
#endif

//����������
OS_EVENT *g_KeyMsg;
void *KeyMsgArray[2]; // ������������

extern OS_EVENT *g_mdb_event; //����MDb�¼�




/*********************************************************************************************************
** Function name:       CreateMBox
** Descriptions:        Ϊ����֮��ͨ�Ŵ���������ź���
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void CreateMBox(void)
{
	//������������
	g_KeyMsg = OSQCreate(&KeyMsgArray[0],2);
}

/*********************************************************************************************************
** Function name:       SystemInit
** Descriptions:        ϵͳ��ʼ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void SystemInit()
{
	InitUart0();
	InitUart1();
	InitUart2();
	InitUart3();
    InitBuzzer();
	InitI2C0();
	InitTimer(0,240000);
	
}





void DEV_task(void *pdata)
{	
	uint8 temp;
	SystemInit();//ϵͳ�����ӿڳ�ʼ��
	CreateMBox();//�������䡢�ź���	
	memset((void *)&stBill,0,sizeof(stBill));
	memset((void *)&stCoin,0,sizeof(stCoin));
	memset((void *)&stMdb,0,sizeof(stMdb));
	FM_readFromFlash();
	MDB_billInit();
	MDB_coinInit();
	while(1){
		temp = MDB_getBillAcceptor();
		if(temp == BILL_ACCEPTOR_MDB){
			billTaskPoll();
		}

		temp = MDB_getCoinAcceptor();
		if(temp == COIN_ACCEPTOR_MDB){
			coinTaskPoll();
		}
		
		temp = MDB_getCoinDispenser();
		if(temp == COIN_DISPENSER_HOPPER){
			HP_task();
		}	
		DB_task();	
		msleep(100);
		
	}
}



