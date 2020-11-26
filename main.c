/***************************************************
 * ����˵����������Ϊ��50L�Զ�����ȿ�ˮͰˮ��ˮλ����ϵͳ���Ĵ��롣
 * Ӳ�������õĵ�·Ϊ����51����-V5.5�����塣
 * ���ߣ�
 * 1. J22(P00~P07)��J6(A~Dp)
 * 2. J20(P12~P17)�ӽ�ͨ��ģ��(D3~D8)
 * 3. P10�ӵ���˿���Ƽ���
 * 4. P11�ӵ�ŷ����Ƽ�ˮ
 * 5. P20/P21��K4/K8����ˮλ
 * 6. P22~P24��J9(A~C)
 * 7. P37��DS18B20
 * ע�����
 **************************************************/
#include "temp.h"//�Զ���ͷ�ļ�����""����<>

void systeminit();//ϵͳ��ʼ������ͷ����
void timerinit(void);//��ʱ���������жϳ�ʼ������ͷ����
void lowwaterlevel();//��ˮλ�����ӳ�����ͷ����
void mediumwaterlevel();//��ˮλ�����ӳ�����ͷ����
void highwaterlevel();//��ˮλ�����ӳ�����ͷ����
void tempcontrol();//�¶ȿ����ӳ�����ͷ����
void tempdatadisplay(int temp);//�¶���ʾ����ͷ����
void systemfault(uchar);//���Ϻ�����ͷ����
void choose(unsigned char m);//38���������������λ��ѡ����ͷ����

sbit decoder1 = P2^2;
sbit decoder2 = P2^3;
sbit decoder3 = P2^4;

sbit P10 = P1^0; sbit P11 = P1^1; sbit P12 = P1^2; sbit P13 = P1^3; 
sbit P14 = P1^4; sbit P15 = P1^5; sbit P16 = P1^6; sbit P17 = P1^7; 
sbit P21 = P2^1; sbit P20 = P2^0;

bit uptemp;//���ȱ�־λ��1��ʾ���ڼ���0��ʾδ����
bit hlevel;//��ˮλ��־λ��1��ʾ���ڸ�ˮλ
bit llevel;//��ˮλ��־λ
bit holdtemp;//���±�־λ
bit fault;//���ϱ�־λ

uint i = 0; j = 0;//���ڶ�ʱ�����ֻ��65536us�Ķ�ʱ��������Ҫ����ȫ�ֱ���i,j���ж����жϲ��ܵ��趨ʱ��
int nowtemp = 0;
int threeminsagotemp = 0;
uchar samplingnumber;//��������
long tempcache;//8���¶Ȼ��������Ӻͺ�洢�ڴ˱�����ѭ��8�κ����8��ƽ���¶ȣ�������С������λ����

/*��������ܱ�0~9+A~F+����ʾ*/
unsigned char code nixietubemoon[17] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x00};
unsigned char code false1[] = {0x71, 0x77, 0x38, 0x6D, 0x79, 0x06};//ˮλ������
unsigned char code false2[] = {0x71, 0x77, 0x38, 0x6D, 0x79, 0x5B};//���ȵ���˿����

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
 * ��������systeminit
 * �������ܣ�ϵͳ��ʼ��
 * ���룺void
 * �����void
 ********************************************************/
void systeminit()
{
    P1 = 0x03;//ϵͳ��ʼ��������ȫ��ָʾ�ƣ�����ȫ������ܣ��鿴��ʾ�Ƿ��������رյ�ŷ��͵���˿��
    for (i = 600; i > 0; i--) {
        for (j = 0; j < 8; j++) {
            choose(j);
            P0 = 0xFF;
            delay522us();
            P0 = 0x00;
        }
    }
    i = 0; j = 0;
    P1 = 0xFF;//ȷ����ʾ����Ϩ��ȫ��ָʾ�ƣ�Ϩ��ȫ�������
    uptemp = 0; hlevel = 0; llevel = 0; holdtemp = 0; fault = 0;//������־λ��0
    /*�ڳ�ʼ�������У��Ȳɼ�һ���¶��������е�һ����ʾ*/
    tempcache = 0;
    choose(7);
    P0 = nixietubemoon[0xC];//�������ʾ"C"��������ʾϵͳ���ڲɼ��¶�״̬
    for (samplingnumber = 0; samplingnumber < 8; samplingnumber++) {
        tempcache += ds18b20tempreadstart();
    }
    nowtemp = tempcache/8;
    /*���³����������и�ˮλ����ʾ��ˮ������*/
    if ((P21 == 0 && P20 == 0) || (P21 == 0 && P20 == 1)) {
        P15 = 0;
    }
}

/*********************************************************
 * ��������tempdatadisplay
 * �������ܣ���ds18b20��ȡ�����¶Ƚ��д�����ʾ�ٶ�̬�������
 * ���룺ds18b20tempreadstart()
 * �����void
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
    /*����������ע�͵������γ���Ϊ��ʾС�������λ�汾�ĳ������ڲ��õ�����ʾС�������λ�İ汾*/
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
    //         P0 = 0x00;//ע����һ����P0�ڵ������������Ҫ������������Ӱģ��
    //     }
    //     n--;
    // }

    // for(m = 0; m < 6; m++) {
    //     choose(m);
    //     P0 = tempdisplay[m];
    //     delay522us();
    //     P0 = 0x00;//ע����һ����P0�ڵ������������Ҫ������������Ӱģ��
    // }

    templong = (tp * 0.0625 * 10000);	
    for (m = 7; m > 0; m--) {
        tempdisplay[m] = nixietubemoon[(templong % 10)];
        templong /= 10;
    }
    tempdisplay[3] |= 0x80;

    /*����������ע�͵���һ�γ��򣬶�����ѭ������n��ʹ��̬�������ʾ�����ظ�ѭ��n�Σ��Ӷ�������ܸ�����������Թ��ϵķ�Ӧ�ٶȱ�ò�����*/
    // while (n != 0) {
    //     for(m = 0; m < 8; m++) {
    //         choose(m);
    //         P0 = tempdisplay[m];
    //         delay522us();//��ʱһ��ʱ�����ʹ����ܸ���������������ʱ�������˸
    //         P0 = 0x00;//ע����һ����P0�ڵ������������Ҫ������������Ӱģ��
    //     }
    //     n--;
    // }

    for(m = 0; m < 8; m++) {
        choose(m);
        P0 = tempdisplay[m];
        delay522us();//��ʱһ��ʱ�����ʹ����ܸ���������������ʱ�������˸
        P0 = 0x00;//ע����һ����P0�ڵ������������Ҫ������������Ӱģ��
    }
}

/*********************************************************
 * ��������choose
 * �������ܣ�38����������ˣ����Խ��ж�̬�����λѡ
 * ���룺unsigned int m
 * �����void
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
 * ��������timerinit
 * �������ܣ���ʱ��������0&1�жϳ�ʼ����12MHz�����£��ж�ʱ��Ϊ50000us(50ms)
 * ���룺void
 * �����void
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
 * ��������timer0
 * �������ܣ���ʱ��������0�жϷ����ӳ��������жϵ���˿���ϣ��ڵ���˿���ڼ���״̬�£�����Լ30s�¶�δ�仯���¶Ƚ����򱨾�
 * ע�⣺����ͬ���жϲ���Ƕ�ף�����ڲɼ��¶ȣ���ʱ���ж�1��ʱ���ϼ�ʱ��ͣ�����ʵ�ʵĹ��ϼ�ʱԼΪ50s
 * ���룺void
 * �����void
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
 * ��������timer1
 * �������ܣ���ʱ��������1�жϷ����ӳ��������趨���������ÿ��Լ20s����һ��
 * ���룺void
 * �����void
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
 * ��������highwaterlevel
 * �������ܣ���ˮλ�����ӳ���
 * ���룺void
 * �����void
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
 * ��������mediumwaterlevel
 * �������ܣ���ˮλ�����ӳ���
 * ���룺void
 * �����void
 ********************************************************/
void mediumwaterlevel()
{
    if (llevel == 1) {
        llevel = 0;//��ˮλ��־λ��0
        P14 = 1;//Ϩ��ȱˮָʾ��
        P15 = 0;//������ˮָʾ��
        
        P10 = 0;//��ʼ���ȣ�������ʾ����˿��ָʾ�ƣ�
        P12 = 0;//��������ָʾ��
        uptemp = 1;//���ȱ�־λ��1
        P17 = 1;//Ϩ����ָʾ��
        holdtemp = 0;//���±�־λ��0
    }
    else if (hlevel == 1) {
        hlevel = 0;//��ˮλ��־λ��0
    }
}

/*********************************************************
 * ��������lowwaterlevel
 * �������ܣ���ˮλ�����ӳ���
 * ���룺void
 * �����void
 ********************************************************/
void lowwaterlevel()
{
    if (llevel != 1) {
        P15 = 1;//Ϩ����ˮָʾ��
        P14 = 0;//����ȱˮָʾ��
        P11 = 0;//��ʼ��ˮ��������ʾ��ŷ���ָʾ�ƣ�
        P13 = 0;//������ˮָʾ��

        P10 = 1;//ֹͣ���ȣ�Ϩ���ʾ����˿��ָʾ�ƣ�
        P12 = 1;//Ϩ�����ָʾ��
        P17 = 1;//Ϩ����ָʾ��

        holdtemp = 0;//���±�־λ��0
        llevel = 1;//��ˮλ��־��1
        uptemp = 0;//���ȱ�־��0
    }
}

/*********************************************************
 * ��������systemfault
 * �������ܣ�������ʾ�����ж���ˮλ���ƹ��ϻ��ǵ���˿����
 * ���룺uchar m
 * �����void
 ********************************************************/
void systemfault(uchar n)
{
    P1 = 0xBF;//��������ָʾ�ƣ�Ϩ������ָʾ�ƣ��رռ��ȣ��رռ�ˮ
    fault = 1;//���ϱ�־λ��1
    llevel = 0; uptemp = 0; hlevel = 0; holdtemp = 0;//������־λ��0
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
 * ��������tempcontrol
 * �������ܣ��¶ȿ����ӳ���
 * ���룺void
 * �����void
 ********************************************************/
void tempcontrol()
{
    if (llevel != 1) {
        if (nowtemp <= 0x0500) {
            if (uptemp != 1) {
                P10 = 0;//��ʼ���ȣ�������ʾ����˿��ָʾ�ƣ�
                P12 = 0;//��������ָʾ��
                uptemp = 1;//���ȱ�־λ��1
                P17 = 1;//Ϩ����ָʾ��
                holdtemp = 0;//���±�־λ��0
            }
        }
        else if (nowtemp >= 0x0640) {
            P10 = 1;//ֹͣ���ȣ�Ϩ���ʾ����˿��ָʾ�ƣ�
            P12 = 1;//Ϩ�����ָʾ��
            uptemp = 0;//���ȱ�־λ��0
            P17 = 0;//��������ָʾ��
            holdtemp = 1;//���±�־λ��1
        }
        else if (holdtemp != 1) {
            if (uptemp != 1) {
                P10 = 0;//��ʼ���ȣ�������ʾ����˿��ָʾ�ƣ�
                P12 = 0;//��������ָʾ��
                uptemp = 1;//���ȱ�־λ��1
                P17 = 1;//Ϩ����ָʾ��
                holdtemp = 0;//���±�־λ��0
            }
        }
    }
}