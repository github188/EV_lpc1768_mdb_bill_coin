#include "..\config.h"

#include "coinApi.h"

#define MDB_COIN_RESET							0x08   		
#define MDB_COIN_SETUP							0x09   		
#define MDB_COIN_TUBE_STATUS					0x0A   	
#define MDB_COIN_POLL							0x0B				
#define MDB_COIN_COIN_TYPE						0x0C  
#define MDB_COIN_DISPENSE						0x0D  
#define MDB_COIN_EXPANSION						0x0F  
#define MDB_COIN_IDENTIFICATION 				0x00  
#define MDB_COIN_FEATURE_ENABLE 				0x01
#define MDB_COIN_PAYOUT							0x02
#define MDB_COIN_PAYOUT_STATUS					0x03
#define MDB_COIN_DIAGNOSTIC_STATUS				0x05 



static uint8 xdata recvbuf[36] = {0};
static uint8 xdata recvlen = 0;

ST_DEV_COIN  xdata stCoin;


/*********************************************************************************************************
** Function name:       coin_send
** Descriptions:         Ӳ��������� 
** input parameters:    Dev:�豸��(��5λ��ַ������λ����)��
**					    				*wdata:Ҫ���͵�����	wlen��Ҫ�������ݵĳ���
** output parameters:   
** Returned value:      0:ͨ�ų�ʱ 1ͨѶ�ɹ� 2���մ��� 
*********************************************************************************************************/
static uint8 coin_send(uint8 dev,uint8 *wdata,uint8 wlen)
{
	uint8 res;
	memset(recvbuf,0,sizeof(recvbuf));
	res = MDB_conversation(dev,wdata,wlen,recvbuf,&recvlen);
	return (res == 1) ?  1 : (res == 0 ? 0 : 2);
}



/*********************************************************************************************************
** Function name:       coin_reset
** Descriptions:        Ӳ�����豸��λ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      0��ʱ ,1�ɹ� 2���ݴ���
*********************************************************************************************************/
static uint8 coin_reset()
{
	uint8 res;
	res = coin_send(MDB_COIN_RESET,NULL,0x00); //Reset
	return res;
}






/*********************************************************************************************************
** Function name:       coin_setup
** Descriptions:        Ӳ������ʼ������
** input parameters:    ��
** output parameters:   ��
** Returned value:      0��ʱ ,1�ɹ� 2���ݴ���
*********************************************************************************************************/
static uint8 coin_setup(ST_DEV_COIN *coin)
{
	uint8 res,i;
	res = coin_send(MDB_COIN_SETUP,NULL,0x00); //setup
	if(res != 1)
		return res;
	coin->level = recvbuf[0];
	coin->codel = INTEG16(recvbuf[1],recvbuf[2]);
	coin->scale = recvbuf[3];
	coin->decimal = recvbuf[4];
	coin->rato = coin->scale * 100;//�Է�Ϊ��λ
	for(i = 0;i < coin->decimal;i++){
		coin->rato /= 10;
	}
	coin->tube.routing	= INTEG16(recvbuf[5],recvbuf[6]);
	for(i = 0;i < 16;i++){
		if(coin->tube.routing & (0x01 << i)){
			coin->tube.credit[i] = recvbuf[7+i];
		}
		else{
			coin->tube.credit[i] = 0;
		}
		//����ͨ����ֵ �Է�Ϊ��λ 0���ʾ��ͨ��������
		coin->tube.ch[i] = coin->tube.credit[i] * coin->rato;	
	}

	return 1;
}




/*********************************************************************************************************
** Function name:       coin_tube_status
** Descriptions:        Ӳ����Ǯ��״̬
** input parameters:    ��
** output parameters:   ��
** Returned value:      0��ʱ ,1�ɹ� 2���ݴ���
*********************************************************************************************************/
static uint8 coin_tube_status(COIN_TUBE *tube)
{
	uint8 res,i;
	res = coin_send(MDB_COIN_TUBE_STATUS,NULL,0x00); //tube_status
	if(res != 1)
		return res;
	tube->full = INTEG16(recvbuf[0], recvbuf[1]);
	stCoin.amount.remainAmount = 0;
	for(i = 0;i < 16;i++){
		if(tube->routing & (0x01 << i)){
			tube->num[i] = recvbuf[2 + i];
		}
		else{
			tube->num[i] = 0;
		}
		stCoin.amount.remainAmount += tube->num[i] * stCoin.rato;
	}
	return res;
}




/*********************************************************************************************************
** Function name:       coin_poll
** Descriptions:        Ӳ������ѵ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      0��ʱ ,1�ɹ� 2���ݴ���
*********************************************************************************************************/
static uint8 coin_poll(uint16 *errNo,uint8 *statusNo)
{
	uint8 res,i,z1,z2,temp,status = 0;
	*errNo = 0;
	res = coin_send(MDB_COIN_POLL,NULL,0x00); //poll
	if(res != 1)
		return res;
	if(recvlen == 0){	
		return 1;
	}
	for(i = 0;i < recvlen;){
		z1 = recvbuf[i++];
		if(z1 & (0x01 << 7)){
			z2 = recvbuf[i++];
		}
		else{
			if(z1 & (0x01 << 6)){
				z2 = recvbuf[i++];
				temp = (z1 >> 4) & 0x03;
				if(temp == 0x00 || temp == 0x01){
					stCoin.amount.oneAmount = stCoin.tube.ch[z1 & 0x0F];
					stCoin.amount.recvAmount += stCoin.amount.oneAmount;
				}
			}
			else{
				if(z1 & (0x01 << 5)){
						temp = temp;
				}
				else{//status 
					temp = z1 & 0x0F;
					switch(temp){
						case 0x01:
						status |= COIN_BIT_PAYBACK;					
						break;
						case 0x02:
							status |= COIN_BIT_BUSY;
							break;			                 //changer pay out busy					
					    case 0x03:	break;			                 //no credit //���ܽ����Ӧͨ�� 			   
					    case 0x04:			                 //defective tube sensor//����������
					    	*errNo |= COIN_ERR_SENSOR;				   	   	
							break;
					    case 0x05:	break;		                 //double arriver	   	   
					    case 0x06:	break;		                 //acceptor unplugged						   				   
					    case 0x07:			                 //tube jam	//���ҿ���
							*errNo |=COIN_ERR_TUBEJAM;				   
						    break;
					    case 0x08:			                 //rom chksum err  //ROM����				   
						  	*errNo |=COIN_ERR_ROM;
						    break;
					    case 0x09:			                 //coin routing err	//����ͨ������				   
						   	*errNo |=COIN_ERR_ROUTING;
						   	break;
					    case 0x0A:
							status |= COIN_BIT_BUSY;
							break;			                 //changer busy   
					    case 0x0B:
							break;			                 //changer was reset//�ո�λ				   		   
					   	case 0x0C:			                 //coin jam	//Ͷ�ҿ���
					   		*errNo |=COIN_ERR_JAM;				   
						   	break;
					   	case 0x0D:		              	     //possible credited coin removal  //��ͼ�Ƴ�Ӳ��		   
						   	//*errNo |= COIN_ERR_UNKNOW;
						   	break;
						default:break;
					}
				}
			}
		}
	}

	*statusNo = status;
	return 1;
	
}






/*********************************************************************************************************
** Function name:       coin_enable
** Descriptions:        ʹ��Ӳ����
** input parameters:    ��
** output parameters:   ��
** Returned value:     0 ��ʱ 1�ɹ� 2���ݴ���
*********************************************************************************************************/
uint8 coin_enable(uint8 flag)
{
	uint8 wbuf[4]={0x00,0x00,0xFF,0xFF},i,res;
	if(flag == 1){
		for(i = 0;i < 8;i++){
		if(stCoin.tube.credit[i + 8] != 0 && stCoin.tube.credit[i + 8] != 0xFF)
			wbuf[0] |= (0x01 << i);
		}
		for(i = 0;i < 8;i++){
			if(stCoin.tube.credit[i] != 0 && stCoin.tube.credit[i] != 0xFF)
				wbuf[1] |= (0x01 << i);			
		}
	}
	
	for(i = 0;i < 3;i++){
		res = coin_send(MDB_COIN_COIN_TYPE,wbuf,4);
		if(res == 1){
			if(flag == 1){
				stCoin.state.s &= ~COIN_BIT_DISABLE;
			}
			else{
				stCoin.state.s |= COIN_BIT_DISABLE;
			}
			return 1;
		}
		msleep(150);
	}
	return res;
}



/*********************************************************************************************************
** Function name:       coin_dispense
** Descriptions:        level2 �������
** input parameters:    type��������ͨ��
**                      num ������������
** output parameters:   						
** Returned value:      0 ʧ�� 1�ɹ�
*********************************************************************************************************/
#if 0
uint8 coin_dispense(uint8 type,uint8 num)
{
	uint8 res,sbuf;
	sbuf = ((num & 0xF0) << 4 ) | (type & 0x0F);
	res = coin_send(MDB_COIN_DISPENSE,&sbuf,1);
	return (res == 1);
}
#endif


/*********************************************************************************************************
** Function name:       coin_payout
** Descriptions:        level3�������
** input parameters:    payNums �����ö�� �Ǹ������ϵ������õ�ö��
** output parameters:   
                        ch--��ͨ��ʵ�ʳ�������						
** Returned value:      ����ɹ� 1��ʧ��0
*********************************************************************************************************/
uint8 coin_payout(uint8 payNums,uint8 *ch)
{
	uint8 res,sbuf[2]={0},i;
	sbuf[0] = MDB_COIN_PAYOUT;
	sbuf[1] = payNums;	//amount / stCoin.rato;
	res = coin_send(MDB_COIN_EXPANSION,sbuf,2);
	if(res == 1)
	{
		Timer.coin_payout_timeout = payNums * 100 + 200;
		while(Timer.coin_payout_timeout){					
			sbuf[0] = 0x04;
			res = coin_send(MDB_COIN_EXPANSION,sbuf,1);
			if(res == 1 && recvlen == 0){//�����������
				msleep(100);
				sbuf[0] = 0x03;
				res = coin_send(MDB_COIN_EXPANSION,sbuf,1);
				if(res == 1 && recvlen > 0){
					for(i = 0;i < 16;i++){
						if(stCoin.tube.routing & (0x01 << i)){
							ch[i] = recvbuf[i];
						}
						else{
							ch[i] = 0;
						}
					}
					return 1;
				}
			}
			msleep(100);
		}
	}				
	return 0;
}

/*********************************************************************************************************
** Function name:       coin_payout
** Descriptions:        level3�������
** input parameters:    amount������Ҫ�ҳ���Ӳ�ҽ�� ��Ϊ��λ
** output parameters:   
                        ch--��ͨ��ʵ�ʳ�������						
** Returned value:      ����ɹ� 1��ʧ��0
*********************************************************************************************************/
uint32 coinPayout(uint32 payMoney)
{
	uint32 xdata changedAmount = 0;
	uint32 xdata payAmout = 0;
	uint32 xdata oneAmount = 0;
	uint16 xdata rato = stCoin.rato;
	uint8 xdata ch[16] = {0};
	uint8 xdata i;
	
	if(payMoney == 0 || rato == 0) 
		return 0;
	
	payAmout = payMoney;
	while(1){
		oneAmount = 0;
		coin_payout(payAmout / rato,ch);
		for(i = 0; i < 16; i++){
			oneAmount +=  ch[i] * stCoin.tube.ch[i];		
		}
		changedAmount += oneAmount;
		if(oneAmount == 0){
			return changedAmount;
		}
		if(payMoney > changedAmount){ //û����
			payAmout = payMoney - changedAmount;
			msleep(100);
			continue;
		}
		else{
			return changedAmount; 
		}
	}
}


/*********************************************************************************************************
** Function name:       coin_expansion_status
** Descriptions:        ��չָ����Ӳ����״̬
** input parameters:    ��
** output parameters:   
** Returned value:      
*********************************************************************************************************/
uint8 coin_expansion_status(uint16 *errNo,uint8 *statusNo)
{
    uint8 res,sbuf = 0x05;
    uint8 manCode,subCode,i;
	*errNo = 0;
	*statusNo = 0;
    res = coin_send(MDB_COIN_EXPANSION,&sbuf,1);
	if(res == 1){
		if(recvlen < 2){
			return 1;
		}
		for(i = 0;i < recvlen;){
			manCode = recvbuf[i++];
			subCode = recvbuf[i++];
			if(manCode == 0x03){
				*statusNo |= COIN_BIT_OK;
			}
			if(	manCode <= 0x05){//����
				subCode = subCode;
			}
			else if(manCode == 0x06){	//��vmc����
				*statusNo |= COIN_BIT_DISABLE;
			}
			else if(manCode == 0x11){
				if(subCode == 0x10){
					subCode = subCode;
				}
			}
			else{//�������ֹ���
				if(manCode == 0x15){
					if(subCode == 0x02){ //Ӳ�Ҷ����Ƴ�
						*errNo |= COIN_ERR_REMOVETUBE;
						return 1;
					}
				}
				*errNo |= COIN_ERR_UNKNOW;
			}
		}
		return 1;
	} 
	return res;
}


static uint8 coin_identifict(COIN_IDENTIFICT *identifict)
{
	uint8 res,sbuf = 0x00,i,index = 0;
    res = coin_send(MDB_COIN_EXPANSION,&sbuf,1);
	if(res != 1)
		return res;

	identifict->manufacturerCode[0] = recvbuf[index++];
	identifict->manufacturerCode[1] = recvbuf[index++];
	
	for(i = 0;i < 12;i++){
		identifict->sn[i] = recvbuf[index++];
	}
	for(i = 0;i < 12;i++){
		identifict->model[i] = recvbuf[index++];
	}

	identifict->softwareVersion[0] = recvbuf[index++];
	identifict->softwareVersion[1] = recvbuf[index++];

	for(i = 0;i < 4;i++){
		identifict->feature[i] = recvbuf[index++];
	}

	return 1;

	
}


uint8 coin_feature_enable()
{
	uint8 res,sbuf[5] = {0x01,0xFF,0xFF,0xFF,0xFF};//���� ��չ����
    res = coin_send(MDB_COIN_EXPANSION,sbuf,5);
	return (res == 1);
}



/*********************************************************************************************************
** Function name:       coinInit
** Descriptions:        Ӳ�����豸��ʼ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      0��ʱ ,1�ɹ� 2���ݴ���
*********************************************************************************************************/
uint8 coinInit(void)
{
	uint8 res,statusNo;
	uint16 errNo;
	ST_DEV_COIN *coin = &stCoin;
	
	res = coin_reset();
	if(res != 1)
		return res;
	msleep(150);
	res = coin_poll(&errNo, &statusNo);
	msleep(150);
	res = coin_feature_enable();
	msleep(150);
	res = coin_setup(coin);//Setup
	msleep(150);	
	res = coin_identifict(&coin->identifict); //Identification
	msleep(150);
	res = coin_enable(0);
	return (res == 1);
}


/*********************************************************************************************************
** Function name:       coinTaskPoll
** Descriptions:        Ӳ����������ѵ
** input parameters:    ��
** output parameters:   
** Returned value:      
*********************************************************************************************************/
uint8 coinTaskPoll(void)
{
	static uint8 flush = 0,comErr = 5;
	uint8 res,s;
	uint16 err;
	COIN_STATE *coinState = &stCoin.state;
	if((coinState->s & COIN_BIT_FAULT) && (coinState->err & COIN_ERR_COM)){
		if(Timer.timer_coin_reset == 0){
			res = coinInit();
			if(res == 1){
				coinState->err = 0;
				coinState->s = COIN_BIT_DISABLE;
			}
			else{
				Timer.timer_coin_reset = 1000;
			}
		}
	}
	else{
		switch(flush){
			case 0:case 2:case 4:case 6:
				res = coin_poll(&err, &s);
				if(s & COIN_BIT_PAYBACK){
					coinState->s |= COIN_BIT_PAYBACK;
				}
				if(err){
					coinState->err |= err;
					coinState->s |= COIN_BIT_FAULT;
				}
				break;
			case 1:case 3:case 5:case 7:
				res = coin_expansion_status(&err,&s);
				if(err){
					coinState->err |= err;
					coinState->s |= COIN_BIT_FAULT;
				}
				if(s & COIN_BIT_OK){ //Ӳ�����ӹ��ϻָ�
					coinState->s &= ~COIN_BIT_FAULT;
					coinState->err = 0;
				}
				break;
			case 8:
				res = coin_tube_status(&stCoin.tube);
				break;
			default:
				flush = 0;
				res = 1;
				break;
		}
		
		comErr = (res == 0) ? (comErr >= 5 ? 5 : comErr + 1) : 0;//��ʱ
		if(comErr >= 5){
			coinState->s |= COIN_BIT_FAULT;
			coinState->err |= COIN_ERR_COM;
		}
		flush++;
	}	
	return 0;
}


