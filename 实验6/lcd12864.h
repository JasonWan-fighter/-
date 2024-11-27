//lcd12864.h
#ifndef   _LCD12864_H_
#define   _LCD12864_H_
/**************************************************************
               iO口宏定义区
***************************************************************/
sbit CS = P0 ^ 0;//对应实验箱上IO1 RS
sbit SID = P0 ^ 1;//对应实验箱上IO2 RW
sbit SCK = P0 ^ 2;//对应实验箱上IO3 E

void write_command(unsigned char Cbyte);   //写入指令函数
void write_data(unsigned char Dbyte);  //写入指令数据
void lcd_init(void);   //显示屏初始化
void lcd_clear_txt(void);  //显示屏清屏
void location_xy_12864(unsigned char x, unsigned char y);
void put_str(unsigned char row, unsigned char col, unsigned char* puts);
void put_char(unsigned char row, unsigned char col, unsigned char put);
#endif