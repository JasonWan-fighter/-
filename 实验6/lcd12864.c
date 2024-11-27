//lcd12864.c
#include <reg51.h>
#include <intrins.h>
#include"lcd12864.h"
/**************************************************************


      //串行方式控制
/*******************************************************************
              常量声明区
********************************************************************/
unsigned char code AC_TABLE[] = {                   //坐标编码
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
};
/****************************************************************
              发送一个字节
*****************************************************************/
void send_byte(unsigned char dbyte)
{
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        SCK = 0;
        dbyte = dbyte << 1;
        SID = CY;
        SCK = 1;
        SCK = 0;
    }
}
/**********************************************************
              接收一个字节
***********************************************************/

unsigned char receive_byte(void)
{
    unsigned char i, temp1, temp2;
    temp1 = temp2 = 0;
    for (i = 0; i < 8; i++)
    {
        temp1 = temp1 << 1;
        SCK = 0;
        SCK = 1;
        SCK = 0;
        if (SID) temp1++;
    }
    for (i = 0; i < 8; i++)
    {
        temp2 = temp2 << 1;
        SCK = 0;
        SCK = 1;
        SCK = 0;
        if (SID) temp2++;
    }
    return ((0xf0 & temp1) + (0x0f & temp2));
}
/****************************************************************
                      检查忙状态
******************************************************************/
void check_busy(void)
{
    do send_byte(0xfc);     //11111,RW(1),RS(0),0
    while (0x80 & receive_byte());
}

/******************************************************************
           写一个字节的指令
*******************************************************************/
void write_command(unsigned char Cbyte)
{
    CS = 1;
    check_busy();
    send_byte(0xf8);          //11111,RW(0),RS(0),0
    send_byte(0xf0 & Cbyte);
    send_byte(0xf0 & Cbyte << 4);
    CS = 0;
}
/*************************************************************
                 写一个字节的数据
**************************************************************/
void write_data(unsigned char dbyte)
{
    CS = 1;
    check_busy();
    send_byte(0xfa);          //11111,RW(0),RS(1),0
    send_byte(0xf0 & dbyte);
    send_byte(0xf0 & dbyte << 4);
    CS = 0;
}

/******************************************************************
                         lcd初始化函数
*******************************************************************/
void lcd_init(void)
{
    write_command(0x30);
    write_command(0x03);
    write_command(0x0c);
    write_command(0x01);
    write_command(0x06);
}

/*******************************************************************************************************
                                 设定光标函数
********************************************************************************************************/
void location_xy_12864(unsigned char x, unsigned char y)
{
    switch (x)
    {
    case 0:
        x = 0x80; break;
    case 1:
        x = 0x90; break;
    case 2:
        x = 0x88; break;
    case 3:
        x = 0x98; break;
    default:
        x = 0x80;
    }
    y = y & 0x07;
    write_command(0x30);
    write_command(y + x);
    write_command(y + x);

}
/***********************************************************************************
                  清除文本
************************************************************************************/
void lcd_clear_txt(void)
{
    unsigned char i;
    write_command(0x30);
    write_command(0x80);
    for (i = 0; i < 64; i++)
        write_data(0x20);
    location_xy_12864(0, 0);
}
/****************************************************************************************
                      显示字符串
*****************************************************************************************/
void put_str(unsigned char row, unsigned char col, unsigned char* puts)
{
    write_command(0x30);
    write_command(AC_TABLE[8 * row + col]);
    while (*puts != '\0')
    {
        if (col == 8)
        {
            col = 0;
            row++;
        }
        if (row == 4) row = 0;
        write_command(AC_TABLE[8 * row + col]);
        write_data(*puts);
        puts++;
        if (*puts != '\0')
        {
            write_data(*puts);
            puts++;
            col++;
        }
    }
}

/****************************************************************************************
                      显示字符
*****************************************************************************************/
void put_char(unsigned char row, unsigned char col, unsigned char put)
{
    write_command(0x30);
    write_command(AC_TABLE[8 * row + col]);
    write_data(put);
}
