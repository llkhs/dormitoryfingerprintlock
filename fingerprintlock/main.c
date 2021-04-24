#include<reg52.h>
#include<intrins.h>

sbit chen=P2^0;    //ָ�Ƴɹ���
sbit shi=P2^1;     //ָ��ʧ�ܵ�
sbit Irfet_led=P2^3;	//����ָʾ��
sbit led=P2^5;     //��ʼ����
sbit pressed=P2^6;  //��Ӧ���
sbit PWM = P2^7;  //�趨PWM�����I/O�˿�
sbit K1=P3^1; 
sbit GND=P1^1;	   //����ӵ�
sbit IRIN=P3^2;	//���ⶨ��
unsigned char count = 0;
unsigned char timer1;
unsigned int Irfet;	//���������ȷ�߼�״̬,0Ϊfalse/1Ϊtrue;
typedef unsigned int u16;	  //���������ͽ����������� typedef
typedef unsigned char u8;
u8 IrValue[6];
u8 Time;


//////////////////////////////////////////////////////////////////////////
volatile unsigned char FPM10A_RECEICE_BUFFER[32];        //������ջ�����
code unsigned char FPM10A_Pack_Head[6] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF};  //Э���ͷ
code unsigned char FPM10A_Get_Img[6] = {0x01,0x00,0x03,0x01,0x00,0x05};    //���ָ��ͼ��
code unsigned char FPM10A_Img_To_Buffer1[7]={0x01,0x00,0x04,0x02,0x01,0x00,0x08}; //��ͼ����뵽BUFFER1
code unsigned char FPM10A_Search[11]={0x01,0x00,0x08,0x04,0x01,0x00,0x00,0x00,0x64,0x00,0x72}; //����ָ��������Χ0 - 999,ʹ��BUFFER1�е�����������
//////////////////////////////////////////////////////////////////////////

/************/

/////////////////////////////////////////////
//                 ��ʱ                    //               
/////////////////////////////////////////////
void delay1s(void)   //��ʱ1S,����11.0592MHZ
{
    unsigned char a,b,c;
    for(c=13;c>0;c--)
        for(b=247;b>0;b--)
            for(a=142;a>0;a--);
    _nop_();  //if Keil,require use intrins.h
}

void delay100ms(void)   //��ʱ100MS,����11.0592MHZ
{
    unsigned char a,b;
    for(b=221;b>0;b--)
        for(a=207;a>0;a--);
}


void delay500ms(void)   //500ms
{
    unsigned char a,b,c;
    for(c=98;c>0;c--)
        for(b=127;b>0;b--)
            for(a=17;a>0;a--);
    _nop_();
}

void delay1_6_f(void)   //��� 0us
{
    unsigned char a,b,c;
    for(c=218;c>0;c--)
        for(b=131;b>0;b--)
            for(a=23;a>0;a--);
    _nop_();  //if Keil,require use intrins.h
}

void delay50ms(void)   //��� 0us
{
    unsigned char a,b;
    for(b=173;b>0;b--)
        for(a=143;a>0;a--);
}

void delay10ms(void)   //��� 0us
{
    unsigned char a,b,c;
    for(c=1;c>0;c--)
        for(b=38;b>0;b--)
            for(a=130;a>0;a--);
}

void delay1ms(unsigned int i)
{
   unsigned int k,j;
   for(k=0;k<i;k++)
      for(j=0;j<121;j++);
}

/********������ʱ**********/
void delay_Ir(u16 i)	//������ʱi=1Լ10us
{
	while(i--);	
}

///////////////////////////////////
//             ���ת��          //
///////////////////////////////////
/*��ʱ��T0��ʼ��*/
void Timer0_Init()           
{
    TMOD &= 0x00;
    TMOD |= 0x01; //��ʱ��T0���óɷ�ʽ1
 
    TH0 = 0xff;   //��ʱ���� 0.1ms ����Ϊ11.0592MHz
    TL0 = 0xa4;
 
    ET0 = 1;      
    TR0 = 1; 
	EA=1; 
}
	
/*T0�жϳ�ʼ��*/
void Time0_Init() interrupt 1 
{
	TR0 = 0; 
	TH0 = 0xff; // 0.1ms
	TL0 = 0xa4;
	
	if(count <= timer1) //timer1��5==0�� 15==90��
	{ 
		PWM = 1; 
	}
	else 
	{ 
		PWM = 0; 
	}
	count++;
	if (count >= 200	) //T = 20ms����
	{ 
		count = 0; 
	}
	TR0 = 1; //����T0
}

///////////////////////////////////
/*ָ�Ƴ�ʼ��*/
void Uart_Init(void) //��ʼ��
{
    SCON=0x50;   //UART��ʽ1:8λUART;   REN=1:�������
    PCON=0x00;   //SMOD=0:�����ʲ��ӱ�
    TMOD=0x20;   //T1��ʽ2,����UART������
    TH1=0xFD;          //UART����������:FDFD(9600)
    TL1=0xFD;   //UART����������:FDFD(9600)
    TR1=1;         //����T1����
    EA=1;         //
}
///////////////////////////////////

/*�����ʼ��*/
void IrInit()
{
	IT0=1;	//�½��ش���
	EX0=1;	//���ж�0����
	EA=1;	//�����ж�
	IRIN=1;	//��ʼ���˿�
}

/*
*UART���ͺͽ��ղ���
*/

void Uart_Send_Byte(unsigned char c)//uart����һ���ֽ�
{
        SBUF = c;
        while(!TI);                //������Ϊ1
        TI = 0;
}

unsigned char Uart_Receive_Byte()//UART����һ���ֽ�
{       
        unsigned char dat;
        while(!RI);         //������Ϊ1
        RI = 0;
        dat = SBUF;
        return (dat);
}



////////////////////////////////////////////
//         AS608/FPM10Aָ��ģ������       //
////////////////////////////////////////////

void FPM10A_Cmd_Send_Pack_Head(void)   //����ͨѶЭ���ͷ
{
        int i;       
        for(i=0;i<6;i++)
   {
                Uart_Send_Byte(FPM10A_Pack_Head[i]);   
   }               
}

void FPM10A_Receive_Data(unsigned char ucLength) //����ָ��ģ�鷴�����ݻ���
{
  unsigned char i;

  for (i=0;i<ucLength;i++)
     FPM10A_RECEICE_BUFFER[i] = Uart_Receive_Byte();

}

void FPM10A_Cmd_Get_Img(void)                ////FINGERPRINT_���ָ��ͼ������(����Ƿ���ָ��)
{
    unsigned char i;
    FPM10A_Cmd_Send_Pack_Head(); //����ͨ��Э���ͷ
    for(i=0;i<6;i++)
        {
       Uart_Send_Byte(FPM10A_Get_Img[i]);
        }
}
//��ͼ��ת��������������Buffer1��
void FINGERPRINT_Cmd_Img_To_Buffer1(void)
{
        unsigned char i;
        FPM10A_Cmd_Send_Pack_Head(); //����ͨ��Э���ͷ      
           for(i=0;i<7;i++)   //�������� ��ͼ��ת���� ������ ����� CHAR_buffer1
     {
             Uart_Send_Byte(FPM10A_Img_To_Buffer1[i]);                  
            }
}

//����ָ�ƿ�ǰ100ö(�����Լ���DATA�������� ���999��)
void FPM10A_Cmd_Search_Finger(void)
{
       unsigned char i;                       
       FPM10A_Cmd_Send_Pack_Head(); //����ͨ��Э���ͷ
       for(i=0;i<11;i++)
           {
                  Uart_Send_Byte(FPM10A_Search[i]);    //����ָ��ģ�鷢�ص�����
                      }
}

//�����Ƿ���ָ��,��������֤
void FPM10A_Find_Fingerprint()
{
  FPM10A_Cmd_Get_Img();                                         //���ͻ��ָ��ͼ������
  FPM10A_Receive_Data(12);                                  //���շ������ݻ���
  if(FPM10A_RECEICE_BUFFER[9]==0)                 //���ݷ��������ĵ�9λ�������ж�ģ��������ָ�ƣ��������ִ�������˳�
  {
    delay100ms();
    FINGERPRINT_Cmd_Img_To_Buffer1();          //��ͼ��ת��������������Buffer1��
        FPM10A_Receive_Data(12);               
        FPM10A_Cmd_Search_Finger();                                //����ȫ���û�100ö
        FPM10A_Receive_Data(16);
        if(FPM10A_RECEICE_BUFFER[9] == 0) //�������������Ӧ��ָ��  
        {
          chen=0;         //��������
		  timer1=5;      //���ת������ 
		  count=0;	
          delay1ms(2500);//�ӳ�5s
		  timer1=27;	 //�����λ
		  count=0;
		  chen=1;
        }
        else
        {
		 shi=0;
		 delay1s();
		 shi=1;
         delay100ms(); //�ӳ�100ms,����
        }
  }
}


/**********��ȡ����*********/
void ReadIr() interrupt 0
{
	u8 j,k;
	u16 err;
	Time=0;
	timer1=27;	//�����λ
	count=0;				 
	delay_Ir(700);	//7ms
	if(IRIN==0)		//ȷ���Ƿ���Ľ��յ���ȷ���ź�
	{	 
		err=1000;				//1000*10us=10ms,����˵�����յ�������ź�
		/*������������Ϊ����ѭ���������һ������Ϊ�ٵ�ʱ������ѭ������ó�������ʱ
		�������������*/	
		while((IRIN==0)&&(err>0))	//�ȴ�ǰ��9ms�ĵ͵�ƽ��ȥ  		
		{			
			delay_Ir(1);
			err--;
		} 
		if(IRIN==1)			//�����ȷ�ȵ�9ms�͵�ƽ
		{
			err=500;
			while((IRIN==1)&&(err>0))		 //�ȴ�4.5ms����ʼ�ߵ�ƽ��ȥ
			{
				delay_Ir(1);
				err--;
			}
			for(k=0;k<4;k++)		//����4������
			{				
				for(j=0;j<8;j++)	//����һ������
				{
					err=60;		
					while((IRIN==0)&&(err>0))//�ȴ��ź�ǰ���560us�͵�ƽ��ȥ
					{
						delay_Ir(1);
						err--;
					}
					err=500;
					while((IRIN==1)&&(err>0))	 //����ߵ�ƽ��ʱ�䳤�ȡ�
					{
						delay_Ir(10);	 //0.1ms
						Time++;
						err--;
						if(Time>30)
						{
							return;
						}
					}
					IrValue[k]>>=1;	 //k��ʾ�ڼ�������
					if(Time>=8)			//����ߵ�ƽ���ִ���565us����ô��1
					{
						IrValue[k]|=0x80;
					}
					Time=0;		//����ʱ��Ҫ���¸�ֵ							
				}
			}
		}
		//����ȡ������ԭ����ͬ��˵���źŽ�����ȷ
		if(IrValue[2]!=~IrValue[3])
		{	
			return;
		}
		else{
			Irfet = 1;	//����״̬ 1 Ϊ��
			Irfet_led = 0;	//����״̬�Ƶƿ�
			delay500ms();
			//�����λ
			timer1=27;		
			count=0;
		}
	}			
}



//////////////////////////////////////////////
//               ������                     //
//////////////////////////////////////////////
void main()         
{
delay500ms();//��Ƭ���ϵ�,�ȴ�1s�ȶ�
Uart_Init();   //��ʼ������
Timer0_Init(); //�����ʼ��
IrInit();	//�����ʼ��
Irfet_led=1;	//�����
led=1;
chen=1;
shi=1;
GND=1;
timer1=27;	//�����λ
count=0;
PWM=1;
pressed=0;
Irfet=0;	//�������״̬��0
led=0;           //����ָʾ����,����ϵͳ�Ѿ���ɳ�ʼ��  (����Ϊ0)

        while(1)
        {
                if(pressed==1)         //ָ��ģ���Ǳ�����? ����Ϊ1 ����Ϊ0
                {
	                Uart_Init();	//ָ�Ƴ�ʼ������	
					timer1=27;		//�����λ
					count=0;
	                delay500ms();//�ӳ�0.5s//�ȴ�SOC��ʼ�����
	                Uart_Init();	//ָ�Ƴ�ʼ������
					timer1=27;
					count=0;
	                FPM10A_Find_Fingerprint(); //����,�Ա�ָ��
                }

				else if(Irfet==1){
					Irfet=0;		//�����ź�����
					chen=0;         //��������
					timer1=5;      //���ת������ 
					count=0;	
				    delay1ms(2500);//�ӳ�5s
					timer1=27;	   //�����λ
					count=0;
					chen=1;
					Irfet_led = 1;	//�������
					}
				else{
             		delay100ms();         //ָ��ģ��û������ �ӳ�100ms               
					}
			}
}