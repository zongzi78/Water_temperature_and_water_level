/***************************************************
 * 程序说明：本程序为“50L自动电加热开水桶水温水位控制系统”的代码。
 * 硬件：采用的电路为普中51单核-V5.5开发板。
 * 接线：
 * 1. J22(P00~P07)接J6(A~Dp)
 * 2. J20(P12~P17)接交通灯模块(D3~D8)
 * 3. P10接电阻丝控制加热
 * 4. P11接电磁阀控制加水
 * 5. P20/P21接K4/K8控制水位
 * 6. P22~P24接J9(A~C)
 * 7. P37接DS18B20
 * 注意事项：
 **************************************************/
#include "temp.h"//自定义头文件，用""不用<>

void systeminit();//系统初始化函数头声明
void timerinit(void);//定时器计数器中断初始化函数头声明
void lowwaterlevel();//低水位处理子程序函数头声明
void mediumwaterlevel();//中水位处理子程序函数头声明
void highwaterlevel();//高水位处理子程序函数头声明
void tempcontrol();//温度控制子程序函数头声明
void tempdatadisplay(int temp);//温度显示函数头声明
void systemfault(uchar);//故障后处理函数头声明
void choose(unsigned char m);//38译码器进行数码管位码选择函数头声明

sbit decoder1 = P2^2;
sbit decoder2 = P2^3;
sbit decoder3 = P2^4;

sbit P10 = P1^0; sbit P11 = P1^1; sbit P12 = P1^2; sbit P13 = P1^3; 
sbit P14 = P1^4; sbit P15 = P1^5; sbit P16 = P1^6; sbit P17 = P1^7; 
sbit P21 = P2^1; sbit P20 = P2^0;

bit uptemp;//加热标志位，1表示正在加热0表示未加热
bit hlevel;//高水位标志位，1表示处于高水位
bit llevel;//低水位标志位
bit holdtemp;//保温标志位
bit fault;//故障标志位

uint i = 0; j = 0;//由于定时器最大只能65536us的定时，所以需要定义全局变量i,j进行多轮中断才能到设定时间
int nowtemp = 0;
int threeminsagotemp = 0;
uchar samplingnumber;//采样次数
long tempcache;//8次温度缓冲区，加和后存储在此变量，循环8次后除以8得平均温度（会舍弃小数点后的位数）

/*共阴数码管表0~9+A~F+无显示*/
unsigned char code nixietubemoon[17] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x00};
unsigned char code false1[] = {0x71, 0x77, 0x38, 0x6D, 0x79, 0x06};//水位检测故障
unsigned char code false2[] = {0x71, 0x77, 0x38, 0x6D, 0x79, 0x5B};//加热电阻丝故障

void main()
{
    systeminit();
    timerinit();
    while (1) {
        if (P21 == 1 && P20 == 1) {
            lowwaterlevel();
        }
        else if (P21 == 0 && P20 == 1) {
            mediumwaterlevel();
        }
        else if (P21 == 0 && P20 == 0) {
            highwaterlevel();
        }
        else if (P21 == 1 && P20 == 0) {
            systemfault(1);
        }
        tempcontrol();
        tempdatadisplay(nowtemp);
    }
}

/*********************************************************
 * 函数名：systeminit
 * 函数功能：系统初始化
 * 输入：void
 * 输出：void
 ********************************************************/
void systeminit()
{
    P1 = 0x03;//系统初始化，点亮全部指示灯，点亮全部数码管，查看显示是否正常（关闭电磁阀和电阻丝）
    for (i = 600; i > 0; i--) {
        for (j = 0; j < 8; j++) {
            choose(j);
            P0 = 0xFF;
            delay522us();
            P0 = 0x00;
        }
    }
    i = 0; j = 0;
    P1 = 0xFF;//确认显示无误，熄灭全部指示灯，熄灭全部数码管
    uptemp = 0; hlevel = 0; llevel = 0; holdtemp = 0; fault = 0;//各个标志位清0
    /*在初始化过程中，先采集一次温度用来进行第一次显示*/
    tempcache = 0;
    choose(7);
    P0 = nixietubemoon[0xC];//数码管显示"C"字样，表示系统处于采集温度状态
    for (samplingnumber = 0; samplingnumber < 8; samplingnumber++) {
        tempcache += ds18b20tempreadstart();
    }
    nowtemp = tempcache/8;
    /*以下程序解决开机中高水位不显示有水的问题*/
    if ((P21 == 0 && P20 == 0) || (P21 == 0 && P20 == 1)) {
        P15 = 0;
    }
}

/*********************************************************
 * 函数名：tempdatadisplay
 * 函数功能：对ds18b20读取到的温度进行处理并显示再动态数码管上
 * 输入：ds18b20tempreadstart()
 * 输出：void
 ********************************************************/
void tempdatadisplay(int temp)
{
    float tp;
    long templong;
    uchar tempdisplay[8];
    uchar m = 0;
    int n = 300;
    if (temp < 0) {
        tempdisplay[0] = 0x40;
        temp -= 1;
        temp = ~temp;
    }
    else {
        tempdisplay[0] = 0x00;
    }
    tp = temp;
    /*从这里往下注释掉的三段程序为显示小数点后两位版本的程序，现在采用的是显示小数点后四位的版本*/
    // temp = tp*0.0625*100+0.5;	
    // for (m = 5; m > 0; m--) {
    //     tempdisplay[i] = nixietubemoon[(temp % 10)];
    //     temp /= 10;
    // }
    // tempdisplay[3] |= 0x80;

    // while (n != 0) {
    //     for(m = 0; m < 6; m++) {
    //         choose(m);
    //         P0 = tempdisplay[m];
    //         delay522us();
    //         P0 = 0x00;//注意这一步对P0口的清零操作很重要，否则会出现重影模糊
    //     }
    //     n--;
    // }

    // for(m = 0; m < 6; m++) {
    //     choose(m);
    //     P0 = tempdisplay[m];
    //     delay522us();
    //     P0 = 0x00;//注意这一步对P0口的清零操作很重要，否则会出现重影模糊
    // }

    templong = (tp * 0.0625 * 10000);	
    for (m = 7; m > 0; m--) {
        tempdisplay[m] = nixietubemoon[(templong % 10)];
        templong /= 10;
    }
    tempdisplay[3] |= 0x80;

    /*从这里往下注释掉的一段程序，定义了循环次数n，使动态数码管显示程序重复循环n次，从而令数码管更亮，但程序对故障的反应速度变得不灵敏*/
    // while (n != 0) {
    //     for(m = 0; m < 8; m++) {
    //         choose(m);
    //         P0 = tempdisplay[m];
    //         delay522us();//延时一段时间可以使数码管更亮，但过长的延时会造成闪烁
    //         P0 = 0x00;//注意这一步对P0口的清零操作很重要，否则会出现重影模糊
    //     }
    //     n--;
    // }

    for(m = 0; m < 8; m++) {
        choose(m);
        P0 = tempdisplay[m];
        delay522us();//延时一段时间可以使数码管更亮，但过长的延时会造成闪烁
        P0 = 0x00;//注意这一步对P0口的清零操作很重要，否则会出现重影模糊
    }
}

/*********************************************************
 * 函数名：choose
 * 函数功能：38译码器输入端，用以进行动态数码管位选
 * 输入：unsigned int m
 * 输出：void
 ********************************************************/
void choose(unsigned char m)
{
    switch(m) {
        case 0:
        decoder1 = 0; decoder2 = 0; decoder3 = 0; break;
        case 1:
        decoder1 = 1; decoder2 = 0; decoder3 = 0; break;
        case 2:
        decoder1 = 0; decoder2 = 1; decoder3 = 0; break;
        case 3:
        decoder1 = 1; decoder2 = 1; decoder3 = 0; break;
        case 4:
        decoder1 = 0; decoder2 = 0; decoder3 = 1; break;
        case 5:
        decoder1 = 1; decoder2 = 0; decoder3 = 1; break;
        case 6:
        decoder1 = 0; decoder2 = 1; decoder3 = 1; break;
        case 7:
        decoder1 = 1; decoder2 = 1; decoder3 = 1; break;

    }
}

/*********************************************************
 * 函数名：timerinit
 * 函数功能：定时器计数器0&1中断初始化，12MHz晶振下，中断时间为50000us(50ms)
 * 输入：void
 * 输出：void
 ********************************************************/
void timerinit(void)
{
    TMOD = 0x11;
    TH0 = 0x3C;
    TL0 = 0xB0;
    TH1 = 0x3C;
    TL1 = 0xB0;
    EA = 1;
    ET0 = 1;ET1 = 1;
    TR0 = 1;TR1 = 1;
}

/*********************************************************
 * 函数名：timer0
 * 函数功能：定时器计数器0中断服务子程序，用来判断电阻丝故障，在电阻丝处于加热状态下，若大约30s温度未变化或温度降低则报警
 * 注意：由于同级中断不可嵌套，因此在采集温度（定时器中断1）时故障计时暂停，因此实际的故障计时约为50s
 * 输入：void
 * 输出：void
 ********************************************************/
void timer0() interrupt 1
{
    TH0 = 0x3C;
    TL0 = 0xB0;
    if (uptemp == 1) {
        if (i == 0) {
            threeminsagotemp = nowtemp;
        }
        i++;
    }
    else {
        i = 0;
    }
    if (i == 600) {
        if (nowtemp <= threeminsagotemp) {
            systemfault(2);
        }
        i = 0;
    }
}

/*********************************************************
 * 函数名：timer1
 * 函数功能：定时器计数器1中断服务子程序，用来设定采样间隔，每大约20s采样一次
 * 输入：void
 * 输出：void
 ********************************************************/
void timer1() interrupt 3
{
    TH1 = 0x3C;
    TL1 = 0xB0;
    j++;
    if (j == 240) {
        tempcache = 0;
        choose(7);
        P0 = nixietubemoon[0xC];
        for (samplingnumber = 0; samplingnumber < 8; samplingnumber++) {
            tempcache += ds18b20tempreadstart();
        }
        nowtemp = tempcache/8;
        j = 0;
    }
}

/*********************************************************
 * 函数名：highwaterlevel
 * 函数功能：高水位处理子程序
 * 输入：void
 * 输出：void
 ********************************************************/
void highwaterlevel()
{
    if (hlevel != 1) {
        hlevel = 1;
        P13 = 1;
        P11 = 1;
    }
}

/*********************************************************
 * 函数名：mediumwaterlevel
 * 函数功能：中水位处理子程序
 * 输入：void
 * 输出：void
 ********************************************************/
void mediumwaterlevel()
{
    if (llevel == 1) {
        llevel = 0;//低水位标志位清0
        P14 = 1;//熄灭缺水指示灯
        P15 = 0;//点亮有水指示灯
        
        P10 = 0;//开始加热（点亮表示电阻丝的指示灯）
        P12 = 0;//点亮加热指示灯
        uptemp = 1;//加热标志位置1
        P17 = 1;//熄灭保温指示灯
        holdtemp = 0;//保温标志位清0
    }
    else if (hlevel == 1) {
        hlevel = 0;//高水位标志位清0
    }
}

/*********************************************************
 * 函数名：lowwaterlevel
 * 函数功能：低水位处理子程序
 * 输入：void
 * 输出：void
 ********************************************************/
void lowwaterlevel()
{
    if (llevel != 1) {
        P15 = 1;//熄灭有水指示灯
        P14 = 0;//点亮缺水指示灯
        P11 = 0;//开始加水（点亮表示电磁阀的指示灯）
        P13 = 0;//点亮加水指示灯

        P10 = 1;//停止加热（熄灭表示电阻丝的指示灯）
        P12 = 1;//熄灭加热指示灯
        P17 = 1;//熄灭保温指示灯

        holdtemp = 0;//保温标志位清0
        llevel = 1;//低水位标志置1
        uptemp = 0;//加热标志清0
    }
}

/*********************************************************
 * 函数名：systemfault
 * 函数功能：故障显示，可判断是水位检测计故障还是电阻丝故障
 * 输入：uchar m
 * 输出：void
 ********************************************************/
void systemfault(uchar n)
{
    P1 = 0xBF;//点亮故障指示灯，熄灭其他指示灯，关闭加热，关闭加水
    fault = 1;//故障标志位置1
    llevel = 0; uptemp = 0; hlevel = 0; holdtemp = 0;//其他标志位置0
    while (1) {
        uchar m = 0;
        for (m = 2; m < 8; m++) {
            choose(m);
            if (n == 1) {
                P0 = false1[m-2];
            }
            else if (n == 2) {
                P0 = false2[m-2];
            }
            delay522us();
            P0 = 0x00;
        }
    }
}

/*********************************************************
 * 函数名：tempcontrol
 * 函数功能：温度控制子程序
 * 输入：void
 * 输出：void
 ********************************************************/
void tempcontrol()
{
    if (llevel != 1) {
        if (nowtemp <= 0x0500) {
            if (uptemp != 1) {
                P10 = 0;//开始加热（点亮表示电阻丝的指示灯）
                P12 = 0;//点亮加热指示灯
                uptemp = 1;//加热标志位置1
                P17 = 1;//熄灭保温指示灯
                holdtemp = 0;//保温标志位清0
            }
        }
        else if (nowtemp >= 0x0640) {
            P10 = 1;//停止加热（熄灭表示电阻丝的指示灯）
            P12 = 1;//熄灭加热指示灯
            uptemp = 0;//加热标志位清0
            P17 = 0;//点亮保温指示灯
            holdtemp = 1;//保温标志位置1
        }
        else if (holdtemp != 1) {
            if (uptemp != 1) {
                P10 = 0;//开始加热（点亮表示电阻丝的指示灯）
                P12 = 0;//点亮加热指示灯
                uptemp = 1;//加热标志位置1
                P17 = 1;//熄灭保温指示灯
                holdtemp = 0;//保温标志位清0
            }
        }
    }
}