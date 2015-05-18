#ifndef _HOPPER_API_H_
#define _HOPPER_API_H_
#include "..\Drivers\board.h"

//��ǰֻ��Hopper��ָ��������л����Ĳ���ָ���
#define HP_QUERY				(0x51)				//Hopper�Ĳ�ѯ����ָ��
#define HP_PAYOUT				(0x50)				//Hopper�ĳ��Ҳ���ָ��


#define HOPPER_NUMS                   3  //����hopper����


#define HOPPER_FAULT_NUM         1   //����hopper�����ֵĹ��ϴ���


#define HP_STATE_NORMAL			0
#define HP_STATE_QUEBI			1
#define HP_STATE_FAULT			2
#define HP_STATE_COM			3 //hopper ͨ�Ź���








//����hopper�� �ṹ��
typedef struct __st_hopper_{
    uint8 addr;			//hopper �����ַ
    uint8 no;			//hopper���
    uint8 state;		//hopper״̬  0 ���� 1 ȱ�� 2 ����
	uint8 lastCount;	//�ϴβ�����������
	uint8 level;		//��������־
	uint8 isCycle;		//ѭ������־
	uint8 fault;	    //������ϱ�־
	uint32 ch;			//ͨ����ֵ
	
}ST_HOPPER;

extern ST_HOPPER stHopper[HOPPER_NUMS];//��������hopper�� �ṹ������

typedef struct _st_hopper_level_{
    uint8 num;//������hopper����
    ST_HOPPER *hopper[HOPPER_NUMS];//����hopper���ṹ��ָ������
    uint32 ch;//������ͨ����ֵ
}ST_HOPPER_LEVEL;


//����hopper���� �ṹ������ ���������λ��ּ��� 1 > 2 > 3
//���� hopper1 hopper2 ͨ����ֵ 1Ԫ hopper3 ͨ����ֵΪ 5ë �� hopper1 hopper2
//�����ڼ���1��,hopper3 �����ڼ���2��  ����3Ϊ��  ��

extern ST_HOPPER_LEVEL stHopperLevel[HOPPER_NUMS];
void HP_task(void);
uint32 HP_payout(uint32 payAmount);
void HP_init(void);
uint8 HP_setCh(uint8 no,uint32 value);
#endif


/**************************************End Of File*******************************************************/
