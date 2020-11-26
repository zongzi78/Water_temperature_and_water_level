/*-------------------------------------------------------
temp.h

用户自定义的用于DS18B20温度检测模块的头文件。
 * 根据C Primer Plus 16.6.3条件编译一节，自定义头文件
   的标识符不应该使用_或者__作为前准，避免与标准头文件中
   的宏发生冲突。
-------------------------------------------------------*/

#ifndef TEMP_H_
#define TEMP_H_

#include <reg51.h>
#include <intrins.h>

//---重定义关键词---//
#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint 
#define uint unsigned int
#endif

/*定义IO口*/
sbit DSPORT = P3^7;

/*函数头声明*/
void delay10us();
void delay52us();
void delay522us();
void delay800ms();
uchar ds18b20init();
void ds18b20writebyte(uchar dat);
uchar ds18b20readbyte();
void ds18b20tempchange();
void ds18b20tempread();
int ds18b20tempreadstart();

#endif