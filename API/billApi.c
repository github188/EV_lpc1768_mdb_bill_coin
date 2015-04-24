#include "..\config.h"
#include "billApi.h"



#define MDB_BILL_RESET			0x30   		//��λ
#define MDB_BILL_SETUP			0x31   		//��ʼ��
#define MDB_BILL_SECURITY		0x32   		//
#define MDB_BILL_POLL				0x33				//��ѵ
#define MDB_BILL_BILL_TYPE	0x34   		//
#define MDB_BILL_ESCROW			0x35  		//
#define MDB_BILL_STACKER		0x36  
#define MDB_BILL_EXPANSION 	0x37  		//��չ����

static uint8 xdata recvbuf[36] = {0};
static uint8 xdata recvlen = 0;

ST_DEV_BILL xdata stBill;


static uint8 xdata isEscrow = 0;

/*********************************************************************************************************
** Function name:       bill_send
** Descriptions:          ֽ��������� 
** input parameters:    dev:�豸��(��5λ��ַ������λ����)��
**											*wdata:Ҫ���͵����� wdata��Ҫ�������ݵĳ���
** output parameters:   
** Returned value:      0:ͨ�ų�ʱ 1ͨѶ�ɹ� 2���մ��� 
*********************************************************************************************************/
static uint8 bill_send(uint8 dev,uint8 *wdata,uint8 wlen)
{
	uint8  res;
	memset(recvbuf,0,sizeof(recvbuf));
	res = MDB_conversation(dev,wdata,wlen,recvbuf,&recvlen);
	return (res == 1) ?  1 : (res == 0 ? 0 : 2);
}




/*********************************************************************************************************
** Function name:       bill_reset
** Descriptions:        ֽ�����豸��λ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      0��ʱ ,1�ɹ� 2���ݴ���
*********************************************************************************************************/
static uint8 bill_reset()
{
	uint8 res;
	
	res = bill_send(MDB_BILL_RESET,NULL,0x00); //Reset
	return res;
}


static uint8 bill_setup(BILL_SETUP *setup)
{
	uint8 res,i;
	res = bill_send(MDB_BILL_SETUP,NULL,0x00);
	if(res != 1)
		return res;
	setup->level = recvbuf[0];
	setup->codel = INTEG16(recvbuf[1],recvbuf[2]);
	setup->scale = INTEG16(recvbuf[3],recvbuf[4]);
	setup->decimal = recvbuf[5];
	
	setup->rato = setup->scale * 100;//�Է�Ϊ��λ
	for(i = 0;i < setup->decimal;i++){
		setup->rato /= 10;
	}
	setup->stkCapacity = INTEG16(recvbuf[6],recvbuf[7]);
	setup->security = INTEG16(recvbuf[8],recvbuf[9]);
	setup->escrowFun = (recvbuf[10] == 0) ?  0 : 1;

	for(i=0;i<16;i++){
		if(recvbuf[11+i] == 0xFF) 
			setup->channel[i] = 0;
		else
			setup->channel[i] = (uint32)recvbuf[11+i] * setup->rato;			
	}
	return 1;
}




/*********************************************************************************************************
** Function name:       bill_poll
** Descriptions:        ֽ������ѯ
** input parameters:    ��
** output parameters: 
** Returned value:      0��ʱ ,1�ɹ� 2���ݴ���
*********************************************************************************************************/
uint8 bill_poll(BILL_POLL *poll)
{
	uint8 res,i,temp,z1;
	poll->err = 0;
	poll->s = 0;
	poll->routing = BILL_POLL_N_A;
	res = bill_send(MDB_BILL_POLL,NULL,0x00);	
	if(res != 1)
		return res;	
	if(recvlen == 0){ //ack
		return 1;
	}
	for(i = 0;i < recvlen;){
		z1 = recvbuf[i++];
		if(z1 & 0x80){ //�ձ�
			temp = (z1 >> 4) & 0x07;
			if(temp == 0x00){ //BILL STACKED
				poll->routing = BILL_POLL_STACKED;
			}
			else if(temp == 0x01){ //�ݴ���
				poll->routing = BILL_POLL_ESCROW_POSITION;
				poll->type = z1 & 0x0F;
			}
			else if(temp == 0x02){ //ֽ�ұ��˻�
				poll->routing = BILL_POLL_RETURNED;
			}
			else if(temp == 0x04){
				poll->routing = BILL_POLL_DISABLED_REJECTED;
			}
			else{
				poll->routing = BILL_POLL_N_A;
			}
		}
		else{	//״̬
			temp = z1 & 0x0F;
			switch(temp)   //validator status
			{
	            case 0x01:			                 //defective motor  
	            	poll->err |= BILL_ERR_MOTO;
			        break;
		        case 0x02:			                 //sensor problem
					poll->err |= BILL_ERR_SENSOR;
			        break;
		        case 0x03:			                 //validator busy
					poll->s |= BILL_BIT_BUSY;
		        	break;
		        case 0x04:			                 //rom chksum err
					poll->err |= BILL_ERR_ROM;
		        	break;
		        case 0x05:			                 //validator jammed
					poll->err |= BILL_ERR_JAM;	       
			        break;
		        case 0x06:			                 //validator was reset
					break;
		        case 0x07:			                 //bill removed	
		        	break;
		        case 0x08:			                 //cash box out of position
					poll->err |= BILL_ERR_REMOVECASH;	 
			        break;
		        case 0x09:			                 //validator disabled	
		        	poll->s |= BILL_BIT_DISABLE;	 
					break;
		        case 0x0A:			                 //invalid escrow request
		       		break;
		        case 0x0B:			                 //bill rejected
		        	break;	
		        case 0x0C:			                 //possible credited bill removal
					poll->err |= BILL_ERR_CASH;
			        break;
		        default:	
					break;
	         }
		}
	}
	return 1;
}



/*********************************************************************************************************
** Function name:       bill_enable
** Descriptions:        ʹ��ֽ����
** input parameters:    flag 1:ʹ��  0:����
** output parameters:   ��
** Returned value:      0ʧ��  1�ɹ�
*********************************************************************************************************/
uint8 bill_enable(uint8 flag)
{
	uint8 i,wbuf[4] = {0x00,0x00,0xFF,0xFF},res;
	if(flag == 1){
		for(i = 0;i < 8;i++){
			if(stBill.setup.channel[i] != 0){
				wbuf[1] |= (0x01 << i);
			}
			if(stBill.setup.channel[i + 8] != 0){
				wbuf[0] |= (0x01 << i);
			}
		}
	}
	
	for(i = 0;i < 3;i++){
		res = bill_send(MDB_BILL_BILL_TYPE,wbuf,4); //Enable
		if(res == 1){
			if(flag == 1){
				stBill.s.status &=  ~BILL_BIT_DISABLE;
			}
			else{
				stBill.s.status |=  BILL_BIT_DISABLE;
			}
			return 1;
		}
		msleep(150);
	}
    return (res == 1);
}

/*********************************************************************************************************
** Function name:       bill_escrow
** Descriptions:         �ݴ�ֽ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
uint8 bill_escrow(void)
{
	uint8 wbuf = 0x01,res;
    res = bill_send(MDB_BILL_ESCROW,&wbuf,1); //Enable
    return (res == 1);
}



/*********************************************************************************************************
** Function name:       bill_reject
** Descriptions:        �˱�
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
uint8 bill_reject(void)
{
	uint8 wdata = 0,res;
   	res = bill_send(MDB_BILL_ESCROW,&wdata,1);	
	return (res == 1);	
}


/*********************************************************************************************************
** Function name:       bill_expansion_id
** Descriptions:        �˱�
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
uint8 bill_expansion_id(BILL_IDENTIFICT *identifict)
{
	uint8 wdata = 0,res,index = 0,i;
   	res = bill_send(MDB_BILL_EXPANSION,&wdata,1);	
	if(res != 1){
		return res;
	}
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
	
	return 1;
}







/*********************************************************************************************************
** Function name:       billInit
** Descriptions:          ֽ�����豸��ʼ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      0ʧ�� 1�ɹ�2��ʱ
*********************************************************************************************************/
uint8 billInit(void)
{
	uint8 res;
	res = bill_reset();
	if(res != 1)
		return res;
	msleep(150);
	bill_poll(&stBill.poll);
	msleep(150);
	bill_setup(&stBill.setup);
	msleep(150);
	bill_expansion_id(&stBill.identifict);
	msleep(150);
	res = bill_enable(0);
	return (res == 1);
}


/*********************************************************************************************************
** Function name:       billTaskPoll
** Descriptions:        ֽ�����豸��ѵ
** input parameters:    ��
** output parameters: 
** Returned value:      ��ֽ�����뷵��1���޷���0
*********************************************************************************************************/
uint8 billTaskPoll(void)
{
	BILL_STATE *state = &stBill.s;
	uint8 res,temp;
	static uint8 comErr = 5;
	
	if((state->status & BILL_BIT_FAULT) && (state->errNo & BILL_ERR_COM)){
		if(Timer.timer_bill_reset == 0){
			res = billInit();
			if(res == 1){
				state->status = 0;
				state->errNo = 0;
				state->status |= BILL_BIT_DISABLE;
			}
			else{
				Timer.timer_bill_reset = 1000;
			}
		}	
	}
	else{
		res = bill_poll(&stBill.poll);
		comErr = (res == 0) ? (comErr >= 5 ? 5 : comErr + 1) : 0;//��ʱ
		if(res == 1){
			if(stBill.poll.s & BILL_BIT_DISABLE){
				stBill.s.status |= BILL_BIT_DISABLE;
				if(stBill.poll.err){
					stBill.s.errNo |= stBill.poll.err;
					stBill.s.status |= BILL_BIT_FAULT;
				}
			}
			else{
				stBill.s.status &= ~BILL_BIT_DISABLE;
				if(stBill.poll.err == 0){
					stBill.s.errNo = 0;
					stBill.s.status &= ~BILL_BIT_FAULT;
				}
			}
			
			if(stBill.poll.routing == BILL_POLL_N_A){
				if(Timer.bill_escrow_timeout == 0 && isEscrow == 1){ //Ѻ����ʱ
					isEscrow = 0;
				}
			}
			else if(stBill.poll.routing == BILL_POLL_ESCROW_POSITION){
				msleep(100);
				if(bill_escrow() == 1){ //Ѻ��ָ��ͳɹ�
					isEscrow = 1;
					Timer.bill_escrow_timeout = 3100; //31���Ѻ����ʱ
				}
				else{
					msleep(100);
					bill_reject();
				}
			}
			else if(stBill.poll.routing == BILL_POLL_RETURNED){
				isEscrow = 0;
			}
			else if(stBill.poll.routing == BILL_POLL_STACKED){
				if(isEscrow == 1){
					isEscrow = 0;
					temp = stBill.poll.type % 16;
					stBill.amount.oneAmount = stBill.setup.channel[temp];
					stBill.amount.recvAmount += stBill.setup.channel[temp];
				}
			}
		}
		
		if(comErr >= 5){
			state->status |= BILL_BIT_FAULT;
			state->errNo |= BILL_ERR_COM;
		}
	}
	return 0;
}












