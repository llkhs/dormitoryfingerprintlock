#include<reg52.h>
#include<intrins.h>

sbit chen=P2^0;    //指纹成功灯
sbit shi=P2^1;     //指纹失败灯
sbit Irfet_led=P2^3;	//红外指示灯
sbit led=P2^5;     //初始化灯
sbit pressed=P2^6;  //感应检测
sbit PWM = P2^7;  //设定PWM输出的I/O端口
sbit K1=P3^1; 
sbit GND=P1^1;	   //舵机接地
sbit IRIN=P3^2;	//红外定义
unsigned char count = 0;
unsigned char timer1;
unsigned int Irfet;	//红外接收正确逻辑状态,0为false/1为true;
typedef unsigned int u16;	  //对数据类型进行声明定义 typedef
typedef unsigned char u8;
u8 IrValue[6];
u8 Time;


//////////////////////////////////////////////////////////////////////////
volatile unsigned char FPM10A_RECEICE_BUFFER[32];        //定义接收缓存区
code unsigned char FPM10A_Pack_Head[6] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF};  //协议包头
code unsigned char FPM10A_Get_Img[6] = {0x01,0x00,0x03,0x01,0x00,0x05};    //获得指纹图像
code unsigned char FPM10A_Img_To_Buffer1[7]={0x01,0x00,0x04,0x02,0x01,0x00,0x08}; //将图像放入到BUFFER1
code unsigned char FPM10A_Search[11]={0x01,0x00,0x08,0x04,0x01,0x00,0x00,0x00,0x64,0x00,0x72}; //搜索指纹搜索范围0 - 999,使用BUFFER1中的特征码搜索
//////////////////////////////////////////////////////////////////////////

/************/

/////////////////////////////////////////////
//                 定时                    //               
/////////////////////////////////////////////
void delay1s(void)   //定时1S,晶振11.0592MHZ
{
    unsigned char a,b,c;
    for(c=13;c>0;c--)
        for(b=247;b>0;b--)
            for(a=142;a>0;a--);
    _nop_();  //if Keil,require use intrins.h
}

void delay100ms(void)   //定时100MS,晶振11.0592MHZ
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

void delay1_6_f(void)   //误差 0us
{
    unsigned char a,b,c;
    for(c=218;c>0;c--)
        for(b=131;b>0;b--)
            for(a=23;a>0;a--);
    _nop_();  //if Keil,require use intrins.h
}

void delay50ms(void)   //误差 0us
{
    unsigned char a,b;
    for(b=173;b>0;b--)
        for(a=143;a>0;a--);
}

void delay10ms(void)   //误差 0us
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

/********红外延时**********/
void delay_Ir(u16 i)	//红外延时i=1约10us
{
	while(i--);	
}

///////////////////////////////////
//             舵机转动          //
///////////////////////////////////
/*定时器T0初始化*/
void Timer0_Init()           
{
    TMOD &= 0x00;
    TMOD |= 0x01; //定时器T0设置成方式1
 
    TH0 = 0xff;   //定时常数 0.1ms 晶振为11.0592MHz
    TL0 = 0xa4;
 
    ET0 = 1;      
    TR0 = 1; 
	EA=1; 
}
	
/*T0中断初始化*/
void Time0_Init() interrupt 1 
{
	TR0 = 0; 
	TH0 = 0xff; // 0.1ms
	TL0 = 0xa4;
	
	if(count <= timer1) //timer1：5==0° 15==90°
	{ 
		PWM = 1; 
	}
	else 
	{ 
		PWM = 0; 
	}
	count++;
	if (count >= 200	) //T = 20ms清零
	{ 
		count = 0; 
	}
	TR0 = 1; //开启T0
}

///////////////////////////////////
/*指纹初始化*/
void Uart_Init(void) //初始化
{
    SCON=0x50;   //UART方式1:8位UART;   REN=1:允许接收
    PCON=0x00;   //SMOD=0:波特率不加倍
    TMOD=0x20;   //T1方式2,用于UART波特率
    TH1=0xFD;          //UART波特率设置:FDFD(9600)
    TL1=0xFD;   //UART波特率设置:FDFD(9600)
    TR1=1;         //允许T1计数
    EA=1;         //
}
///////////////////////////////////

/*红外初始化*/
void IrInit()
{
	IT0=1;	//下降沿触发
	EX0=1;	//打开中断0允许
	EA=1;	//打开总中断
	IRIN=1;	//初始化端口
}

/*
*UART发送和接收部分
*/

void Uart_Send_Byte(unsigned char c)//uart发送一个字节
{
        SBUF = c;
        while(!TI);                //发送完为1
        TI = 0;
}

unsigned char Uart_Receive_Byte()//UART接受一个字节
{       
        unsigned char dat;
        while(!RI);         //接收完为1
        RI = 0;
        dat = SBUF;
        return (dat);
}



////////////////////////////////////////////
//         AS608/FPM10A指纹模块命令       //
////////////////////////////////////////////

void FPM10A_Cmd_Send_Pack_Head(void)   //发送通讯协议包头
{
        int i;       
        for(i=0;i<6;i++)
   {
                Uart_Send_Byte(FPM10A_Pack_Head[i]);   
   }               
}

void FPM10A_Receive_Data(unsigned char ucLength) //接收指纹模块反馈数据缓冲
{
  unsigned char i;

  for (i=0;i<ucLength;i++)
     FPM10A_RECEICE_BUFFER[i] = Uart_Receive_Byte();

}

void FPM10A_Cmd_Get_Img(void)                ////FINGERPRINT_获得指纹图像命令(检测是否有指纹)
{
    unsigned char i;
    FPM10A_Cmd_Send_Pack_Head(); //发送通信协议包头
    for(i=0;i<6;i++)
        {
       Uart_Send_Byte(FPM10A_Get_Img[i]);
        }
}
//讲图像转换成特征码存放在Buffer1中
void FINGERPRINT_Cmd_Img_To_Buffer1(void)
{
        unsigned char i;
        FPM10A_Cmd_Send_Pack_Head(); //发送通信协议包头      
           for(i=0;i<7;i++)   //发送命令 将图像转换成 特征码 存放在 CHAR_buffer1
     {
             Uart_Send_Byte(FPM10A_Img_To_Buffer1[i]);                  
            }
}

//搜索指纹库前100枚(可以自己改DATA区的数字 最高999个)
void FPM10A_Cmd_Search_Finger(void)
{
       unsigned char i;                       
       FPM10A_Cmd_Send_Pack_Head(); //发送通信协议包头
       for(i=0;i<11;i++)
           {
                  Uart_Send_Byte(FPM10A_Search[i]);    //接收指纹模块发回的数据
                      }
}

//搜索是否有指纹,若有则认证
void FPM10A_Find_Fingerprint()
{
  FPM10A_Cmd_Get_Img();                                         //发送获得指纹图像命令
  FPM10A_Receive_Data(12);                                  //接收反馈数据缓冲
  if(FPM10A_RECEICE_BUFFER[9]==0)                 //根据反馈回来的第9位数据来判断模块上有无指纹，有则继续执行无则退出
  {
    delay100ms();
    FINGERPRINT_Cmd_Img_To_Buffer1();          //讲图像转换成特征码存放在Buffer1中
        FPM10A_Receive_Data(12);               
        FPM10A_Cmd_Search_Finger();                                //搜索全部用户100枚
        FPM10A_Receive_Data(16);
        if(FPM10A_RECEICE_BUFFER[9] == 0) //如果搜索到有相应的指纹  
        {
          chen=0;         //开锁灯亮
		  timer1=5;      //舵机转动开锁 
		  count=0;	
          delay1ms(2500);//延迟5s
		  timer1=27;	 //舵机归位
		  count=0;
		  chen=1;
        }
        else
        {
		 shi=0;
		 delay1s();
		 shi=1;
         delay100ms(); //延迟100ms,跳出
        }
  }
}


/**********读取红外*********/
void ReadIr() interrupt 0
{
	u8 j,k;
	u16 err;
	Time=0;
	timer1=27;	//舵机归位
	count=0;				 
	delay_Ir(700);	//7ms
	if(IRIN==0)		//确认是否真的接收到正确的信号
	{	 
		err=1000;				//1000*10us=10ms,超过说明接收到错误的信号
		/*当两个条件都为真是循环，如果有一个条件为假的时候跳出循环，免得程序出错的时
		侯，程序死在这里*/	
		while((IRIN==0)&&(err>0))	//等待前面9ms的低电平过去  		
		{			
			delay_Ir(1);
			err--;
		} 
		if(IRIN==1)			//如果正确等到9ms低电平
		{
			err=500;
			while((IRIN==1)&&(err>0))		 //等待4.5ms的起始高电平过去
			{
				delay_Ir(1);
				err--;
			}
			for(k=0;k<4;k++)		//共有4组数据
			{				
				for(j=0;j<8;j++)	//接收一组数据
				{
					err=60;		
					while((IRIN==0)&&(err>0))//等待信号前面的560us低电平过去
					{
						delay_Ir(1);
						err--;
					}
					err=500;
					while((IRIN==1)&&(err>0))	 //计算高电平的时间长度。
					{
						delay_Ir(10);	 //0.1ms
						Time++;
						err--;
						if(Time>30)
						{
							return;
						}
					}
					IrValue[k]>>=1;	 //k表示第几组数据
					if(Time>=8)			//如果高电平出现大于565us，那么是1
					{
						IrValue[k]|=0x80;
					}
					Time=0;		//用完时间要重新赋值							
				}
			}
		}
		//反码取反后与原码相同则说明信号接受正确
		if(IrValue[2]!=~IrValue[3])
		{	
			return;
		}
		else{
			Irfet = 1;	//红外状态 1 为真
			Irfet_led = 0;	//红外状态灯灯开
			delay500ms();
			//舵机归位
			timer1=27;		
			count=0;
		}
	}			
}



//////////////////////////////////////////////
//               主程序                     //
//////////////////////////////////////////////
void main()         
{
delay500ms();//单片机上电,等待1s稳定
Uart_Init();   //初始化串口
Timer0_Init(); //舵机初始化
IrInit();	//红外初始化
Irfet_led=1;	//红外灯
led=1;
chen=1;
shi=1;
GND=1;
timer1=27;	//舵机归位
count=0;
PWM=1;
pressed=0;
Irfet=0;	//红外接收状态置0
led=0;           //工作指示灯亮,提醒系统已经完成初始化  (测试为0)

        while(1)
        {
                if(pressed==1)         //指纹模块是被按下? 按下为1 否则为0
                {
	                Uart_Init();	//指纹初始化串口	
					timer1=27;		//舵机归位
					count=0;
	                delay500ms();//延迟0.5s//等待SOC初始化完成
	                Uart_Init();	//指纹初始化串口
					timer1=27;
					count=0;
	                FPM10A_Find_Fingerprint(); //查找,对比指纹
                }

				else if(Irfet==1){
					Irfet=0;		//红外信号重置
					chen=0;         //开锁灯亮
					timer1=5;      //舵机转动开锁 
					count=0;	
				    delay1ms(2500);//延迟5s
					timer1=27;	   //舵机归位
					count=0;
					chen=1;
					Irfet_led = 1;	//红外灯灭
					}
				else{
             		delay100ms();         //指纹模块没被按下 延迟100ms               
					}
			}
}