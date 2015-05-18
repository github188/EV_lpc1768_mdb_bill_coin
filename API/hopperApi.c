#include "..\config.h"


#define DEBUG_HOPPER

#ifdef 	DEBUG_HOPPER
#define print_hopper(...)	Trace(__VA_ARGS__)
#else
#define print_hopper(...)
#endif



ST_HOPPER stHopper[HOPPER_NUMS];//��������hopper�� �ṹ������

//����hopper���� �ṹ������ ���������λ��ּ��� 1 > 2 > 3
//���� hopper1 hopper2 ͨ����ֵ 1Ԫ hopper3 ͨ����ֵΪ 5ë �� hopper1 hopper2
//�����ڼ���1��,hopper3 �����ڼ���2��  ����3Ϊ��  ��
ST_HOPPER_LEVEL stHopperLevel[HOPPER_NUMS];

static uint8  sn=0;
static volatile uint8 sendbuf[64] = {0};
static volatile uint8 recvbuf[64] = {0};






/*********************************************************************************************************
** Function name:     	HP_send
** Descriptions:	    EVBͨѶ
** input parameters:    Chl:ѡ��ͨ����Head:��ͷ��Sn�����кţ�Type:��Ϣ���Addr:�豸��ַ;Data�����ݣ�
** output parameters:   *ACK��Ӧ���
** Returned value:      1���յ�Ӧ��0��δ�յ�Ӧ��ͨѶʧ��
*********************************************************************************************************/
uint8 HP_send(uint8 addr,uint8 cmd,uint16 data)
{
	uint8 index = 0,i,ch,len,crc;
	memset((void *)recvbuf,0,sizeof(recvbuf));
	
	sn = (cmd == HP_PAYOUT) ? sn + 1: sn; //����sn
	
	sendbuf[index++] = 0xED;
	sendbuf[index++] = 0x08; //len
	sendbuf[index++] = sn;
	sendbuf[index++] = cmd;
	sendbuf[index++] = addr;
	sendbuf[index++] = LUINT16(data);
	sendbuf[index++] = HUINT16(data);
	sendbuf[index++] = XorCheck((uint8 *)sendbuf,7);
	
	
	//�������ݽ�ֹ�л�����
	OSSchedLock();
	Uart3PutStr((uint8 *)sendbuf,8);
	OSSchedUnlock();
	
	#ifdef DEBUG_HOPPER
	print_hopper("HP-SEND[%d]:",index);
	for(i = 0;i < index;i++){
		print_hopper("%02x ",sendbuf[i]);
	}
	print_hopper("\r\n");
	#endif
	
	index = 0;
	Timer.hopper_recv_timeout = 100;
	while(Timer.hopper_recv_timeout){ //1000ms���յ�ACK,����ʱ
		if(Uart3BuffIsNotEmpty() == 1){
			ch = Uart3GetCh();
			if(index == 0){
				if(ch == 0xFD){
					recvbuf[index++] = ch;
				}
			}
			else if(index == 1){
				len = ch;
				recvbuf[index++] = ch;
			}
			else if(index >= (len - 1)){
				recvbuf[index++] = ch;
				crc = XorCheck((uint8 *)recvbuf,len - 1);
				if(crc == ch){
#ifdef DEBUG_HOPPER
					print_hopper("HP-RECV[%d]:",recvbuf[1]);
					for(i = 0;i < recvbuf[1];i++){
						print_hopper("%02x ",recvbuf[i]);
					}
					print_hopper("\r\n");
#endif
					return 1;
				}
			}
			else{
				recvbuf[index++] = ch;
			}
		}
		else{
			msleep(20);
		}
	}
	return 0;
}



/*********************************************************************************************************
** Function name:     	HP_check
** Descriptions:	    hopper�豸�������ƺ���
** input parameters:    addr:�豸��Ӳ�������ַ				
** output parameters:   ��
** Returned value:      1���յ�Ӧ��0��Ӧ��ʱ  2 æ
*********************************************************************************************************/
uint8 HP_send_check(ST_HOPPER *hopper)
{
	uint8 res,s;
	res = HP_send(hopper->addr,HP_QUERY,0x00);
	if(res == 1){
		if(recvbuf[1] == 0x08 && hopper->addr == recvbuf[4]) {
			s = recvbuf[3];
			print_hopper("HP_state=%x\r\n",s);
			
			if(s & (0x01U << 1)){ //hopperæ
				return 3;
			}
			
			if((s & (0x01U << 2)) || (s & (0x01U << 7))){ //ָ�����
				return 3;
			}
			
			if(s == 0){ //hopper ����
				hopper->state = HP_STATE_NORMAL;
			}
			else{
				if((s & (0x01U << 3)) || (s & (0x01U << 5))){ //hopper ȱ��
					hopper->state = HP_STATE_QUEBI;
				}
				
				if((s & (0x01U << 4)) || (s & (0x01U << 6))){ //hopper ����
					hopper->state = HP_STATE_FAULT;
				}
			}
			hopper->lastCount = INTEG16(recvbuf[6], recvbuf[5]);
			return 1;
		}
		
		return 0;			
	}
	
	hopper->state = HP_STATE_COM; //ͨ��ʧ��
	return 0;	
}


/*********************************************************************************************************
** Function name:     	HP_send_output
** Descriptions:	    EVBhopper�豸�������ƺ���
** input parameters:    addr:�豸��Ӳ�������ַ		
						num:Hopper���Ҳ����ĳ�������
** output parameters:   ��
** Returned value:      1���յ�Ӧ��0��δ�յ�Ӧ��ͨѶʧ��
*********************************************************************************************************/
uint8 HP_send_output(ST_HOPPER *hopper,uint16 num)
{
	uint8 i,res;
	for(i = 0;i < 3;i++){
		res = HP_send(hopper->addr,HP_PAYOUT,num);
		if(res == 1 && recvbuf[1] == 0x06){
			return 1;
		}
		else{
			msleep(100);
		}
	}
	return 0;
}



/*********************************************************************************************************
** Function name:     	HP_init
** Descriptions:	    hopper�豸��ʼ������
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void HP_init(void)
{
	uint8 i,j,levelNum = 0;
	uint32 value;
	for(i = 0; i < HOPPER_NUMS;i++){
		value = stHopper[i].ch;
		memset((void *)&stHopper[i],0,sizeof(ST_HOPPER));
		memset((void *)&stHopperLevel[i],0,sizeof(ST_HOPPER_LEVEL));
		
		stHopper[i].ch = value;
		stHopper[i].addr = i;
		
	}
	
	//�������
	for(i = 0;i < HOPPER_NUMS;i++){
		value = 0;
		//��ѯδ���伶���е����
		for(j = 0;j < HOPPER_NUMS;j++){
			if(!stHopper[j].level && value < stHopper[j].ch){
				value = stHopper[j].ch;
			}
		}
		stHopperLevel[i].ch = value;
		if(value){
			for(j = 0;j < HOPPER_NUMS;j++){   //��ѯ��ͬ ��ֵ�Ķ� ���з���
				if(value == stHopper[j].ch){
					levelNum = stHopperLevel[i].num++;
					stHopperLevel[i].hopper[levelNum] = &stHopper[j];
					stHopper[j].level = 1;//��������־
				}
			}
		}
		
		print_hopper("levelNo=%d num =%d value = %d\r\n",
			i,stHopperLevel[i].num,stHopperLevel[i].ch);
	}

	print_hopper("hopperInit over..\r\n");
}




uint8 HP_setCh(uint8 no,uint32 value)
{
	if(no > HOPPER_NUMS || no == 0){
		return 0;
	}
	stHopper[no - 1].ch = value;
	return 1;
	
}

/*********************************************************************************************************
** Function name:     	HP_payout_by_addr
** Descriptions:	    hopper����ַ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
uint8 HP_payout_by_addr(ST_HOPPER *hopper,uint16 num)
{
	uint8 res;
	res = HP_send_output(hopper,num);
	if(res != 1){
		return 0;
	}
	
	msleep(1000); //�ȴ�1s
	Timer.hopper_payout_timeout = 5000 + num * 250;
	while(Timer.hopper_payout_timeout){
		res = HP_send_check(hopper);
		if(res == 1){ //�������
			if(num == hopper->lastCount){
				return 1;
			}
			else{
				return 0;
			}
		}
		msleep(100);
	}
	return 0;
	
}


/*********************************************************************************************************
** Function name:     	HP_payout_by_level
** Descriptions:	    hopper�豸�����������
** input parameters:    changeCount������Ҫ���������
** output parameters:   remainCount��������ʧ�ܺ�ʣ�������
						levelNo :ѡ���ĸ����� ����
** Returned value:      1���ұҳɹ���0��ʧ��
*********************************************************************************************************/
uint8  HP_payout_by_level(uint8 level,uint16 payCount,uint16 *remainCount)
{
	uint8 i,cycleFlag = 0,res;
	uint16 changedCount = 0;//��������
	ST_HOPPER *hopper;
	if(payCount == 0){
		*remainCount = 0;
		return 1;
	}
	
	//���� hopper�����Ƿ����
	if(level > HOPPER_NUMS){
		*remainCount = payCount;
		print_hopper("The level = %d > %d(max) \r\n",level,HOPPER_NUMS);
		return 0;
	}
	
	if(stHopperLevel[level - 1].num  == 0) { //û�п��õĶ�
	
		print_hopper("The level = %d useableNum = is 0 \r\n",level);
		*remainCount = payCount;
		return 0;
	}
	
	//��ѯ�Ƿ���ѭ����
	for(i = 0; i < stHopperLevel[level - 1].num;i++){
		hopper = stHopperLevel[level - 1].hopper[i];
		if(hopper->isCycle && hopper->state == HP_STATE_NORMAL){
			cycleFlag = 1;
			break;
		}		
	}
	
	if(cycleFlag){	//��ѭ���� ������ѭ��������
		hopper = stHopperLevel[level - 1].hopper[i];
		res = HP_payout_by_addr(hopper,payCount);
		if(res == 1){
			*remainCount  = 0;
			return 1;
		}
		else{
			changedCount += hopper->lastCount;	//ѭ��������ʧ�� ����������	
		}
	}
	
	//û��ѭ���� �����ѡ��һ�����õĶ� ����
	for(i = 0; i < stHopperLevel[level - 1].num;i++)
	{	
		hopper = stHopperLevel[level - 1].hopper[i];
		if(hopper->fault >= HOPPER_FAULT_NUM)//����豸����������
			continue;
		res = HP_payout_by_addr(hopper,payCount - changedCount);
		print_hopper("Select--level = %d addr= %d res =%d\r\n",level,hopper->addr,res);
		if(res == 1){
			*remainCount  = 0;
			hopper->fault =  0;//�������
			return 1;
		}
		else{
			changedCount +=	hopper->lastCount;
			print_hopper("PayCount = %d,hopper[%d]->lastCount =%d,changedCount =%d\r\n",
					payCount,i,hopper->lastCount,changedCount);
			//���˱�ʾ�ö� �Ѿ��޷��ұ� ��������ϲ����		
			hopper->fault++;//����+1
		}				
	}
	//��ѯ���������п��õĶ� ���Һ�����յ�ʧ�����
	if(payCount >= changedCount){
		*remainCount = payCount - changedCount;
	}
	else{
		*remainCount = 0;
	}
	
	return 0;
		
	
	
}



/*********************************************************************************************************
** Function name:     	HP_payout
** Descriptions:	    Ӳ�Ҷ����㷨 ��ֵ�Ӵ�С�ұ�
** input parameters:   
** output parameters:   
** Returned value:      1���ұҳɹ���0��ʧ��
*********************************************************************************************************/
uint32 HP_payout(uint32 payAmount)
{
	uint8 i;
	uint16 remainCount = 0,payCount = 0;
	uint32 remainAmount;
	
	remainAmount = payAmount;
	//Ӳ���������㷨
	for(i = HOPPER_NUMS;i > 0;i--){
		if(stHopperLevel[i - 1].ch > 0){
			remainAmount  = payAmount % stHopperLevel[i - 1].ch;
			 payCount = payAmount / stHopperLevel[i - 1].ch;
			 print_hopper("Coin level[%d].payCount = %d \r\n",i-1,payCount);
			
			 if(HP_payout_by_level(i,payCount,&remainCount) == 1){
				if(remainAmount == 0)
					return payAmount;		
			 }
			 else{
				 payAmount = remainAmount  + remainCount * stHopperLevel[i - 1].ch;
				 remainAmount = payAmount;
			 }
		}
	}
	
	if(payAmount >= remainAmount){
		payAmount -= remainAmount;
	}
	else{
		payAmount = 0;
	}
	
	return payAmount;	
}



uint8 HP_allHopperCheck(void){
	uint8 i = 0;
	if(Timer.hopper_check_timeout == 0){
		for(i = 0;i < HOPPER_NUMS;i++){
			if(stHopper[i].ch > 0){
				HP_send_check(&stHopper[i]);
			}
		}
		Timer.hopper_check_timeout = 500;//3����һ����
	}
	return 1;
}




void HP_task(void)
{
	HP_allHopperCheck();
}




/**************************************End Of File*******************************************************/
