#include <intrins.h>
#include <reg51.h>

// CH451显示模块命令定义
#define CH451_RESET 0x0201     // 复位命令
#define CH451_LEFTMOV 0x0300   // 设置移动模式 - 向左滚动
#define CH451_LEFTCYC 0x0301   // 设置移动模式 - 向左循环
#define CH451_RIGHTMOV 0x0302  // 设置移动模式 - 向右滚动
#define CH451_RIGHTCYC 0x0303  // 设置移动模式 - 向右循环
#define CH451_SYSOFF 0x0400    // 系统关闭 - 禁用所有功能
#define CH451_SYSON1 0x0401    // 系统开启 - 模式1
#define CH451_SYSON2 0x0403    // 系统开启 - 模式2
#define CH451_SYSON3 0x0407    // 系统开启 - 模式3，附加功能
#define CH451_DSP 0x0500       // 设置默认显示模式
#define CH451_BCD 0x0580       // 设置BCD编码模式
#define CH451_TWINKLE 0x0600   // 设置闪烁控制
#define CH451_DIG0 0x0800      // 显示数字0
#define CH451_DIG1 0x0900      // 显示数字1
#define CH451_DIG2 0x0A00      // 显示数字2
#define CH451_DIG3 0x0B00      // 显示数字3
#define CH451_DIG4 0x0C00      // 显示数字4
#define CH451_DIG5 0x0D00      // 显示数字5
#define CH451_DIG6 0x0E00      // 显示数字6
#define CH451_DIG7 0x0F00      // 显示数字7

// CH451控制引脚定义
sbit ch451_load = P1 ^ 3;  // LOAD引脚，控制命令的锁存
sbit ch451_din = P1 ^ 1;   // DIN引脚，数据输入
sbit ch451_dclk = P1 ^ 2;  // DCLK引脚，时钟输入
sbit ch451_dout = P3 ^ 2;  // DOUT引脚，数据输出，连接到P3.2（INT0引脚）
sbit beep_out = P1 ^ 5;    // 蜂鸣器输出引脚，控制蜂鸣器的鸣叫

unsigned char ch451_key;  // 存储按键值的变量

// DS18B20温度传感器控制引脚
sbit DQ = P0 ^ 0;    // 数据引脚DQ，连接到P0.0
#define DQ_H DQ = 1  // 宏定义，将DQ置高电平
#define DQ_L DQ = 0  // 宏定义，将DQ置低电平

// 计时相关的全局变量
unsigned int cnt = 0;        // 定时器计数器，cnt=1000对应1秒
unsigned char second_i = 0;  // 秒的个位
unsigned char second_t = 0;  // 秒的十位
unsigned char min_i = 0;     // 分钟的个位
unsigned char min_t = 0;     // 分钟的十位

// 备份变量（用于重置计时器）
unsigned char second_i_memo = 0;
unsigned char second_t_memo = 0;
unsigned char min_i_memo = 0;
unsigned char min_t_memo = 0;

unsigned char first_start = 1;   // 初始启动标志，1表示第一次启动
unsigned char display_mode = 1;  // 显示模式：0 = 倒计时，1 = 温度

// 倒计时器状态枚举，表示计时器当前的状态
enum COUNT_DOWN_TIMER_STATE
{
    INITIAL,  // 初始状态
    INPUT1,   // 输入分钟十位
    INPUT2,   // 输入分钟个位
    INPUT3,   // 输入秒钟十位
    INPUT4,   // 输入秒钟个位
    RUN,      // 运行状态
    PAUSE     // 暂停状态
};
enum COUNT_DOWN_TIMER_STATE timer_state = INITIAL;  // 当前倒计时器的状态

unsigned char beepflag = 0;  // 蜂鸣器标志，1表示蜂鸣器正在响
unsigned int beepcount = 0;  // 蜂鸣器计数器，用于控制蜂鸣器鸣叫时间

// 函数原型声明
void ch451_init();                               // 初始化CH451
void ch451_write(unsigned int command);          // 向CH451写入命令
void delay_10us(unsigned char n);                // 延时函数，每单位约10微秒
void reset_ds18b20(void);                        // 重置DS18B20
void write_byte(unsigned char dat);              // 向DS18B20写入一个字节
unsigned char read_byte(void);                   // 从DS18B20读取一个字节
void count_down_timer(unsigned char key_value);  // 处理倒计时器的状态机
unsigned int read_temperature();                 // 读取温度值
void display_temperature();                      // 显示温度值
unsigned char tran(unsigned char key);           // 转换按键编码为实际按键值
void reset_timer_to_initial();                   // 重置计时器到初始状态

// 初始化CH451显示模块
void ch451_init()
{
    ch451_din = 0;  // 初始化DIN为低电平
    ch451_din = 1;  // 设置DIN为高电平，准备开始通信
    IT1 = 0;        // 配置INT1为低电平触发方式
    IE1 = 0;        // 清除INT1中断标志
    PX1 = 0;        // 设置INT1为低优先级
    EX1 = 1;        // 启用外部中断1（INT1）
}

// 向CH451写入命令
void ch451_write(unsigned int command)
{
    unsigned char i;
    EX1 = 0;         // 写入过程中禁用外部中断1，防止干扰
    ch451_load = 0;  // 开始传输，LOAD置低
    for (i = 0; i < 12; i++)
    {                             // 发送12位命令
        ch451_din = command & 1;  // 发送最低位的数据位
        ch451_dclk = 0;           // 产生一个时钟下降沿
        command >>= 1;            // 右移一位，准备发送下一位
        ch451_dclk = 1;           // 产生一个时钟上升沿，数据被CH451读取
    }
    ch451_load = 1;  // 结束传输，LOAD置高，命令被执行
    EX1 = 1;         // 重新启用外部中断1
}

// 外部中断0服务程序，用于按键检测
void ch451_inter() interrupt 0
{
    unsigned char i;
    unsigned char command, keycode;
    command = 0x07;  // 读取命令，命令码为0111
    ch451_load = 0;  // 开始传输
    for (i = 0; i < 4; i++)
    {
        ch451_din = command & 1;  // 发送读取命令的每一位
        ch451_dclk = 0;           // 时钟下降沿
        command >>= 1;            // 右移一位，准备发送下一位
        ch451_dclk = 1;           // 时钟上升沿
    }
    ch451_load = 1;  // 结束命令传输
    keycode = 0;     // 初始化按键代码
    for (i = 0; i < 7; i++)
    {                           // 读取7位按键代码
        keycode <<= 1;          // 左移一位，为下一位腾出空间
        keycode |= ch451_dout;  // 读取DOUT引脚的值
        ch451_dclk = 0;         // 时钟下降沿
        ch451_dclk = 1;         // 时钟上升沿
    }
    ch451_key = keycode;  // 存储按键代码，供主程序使用
    IE0 = 0;              // 清除中断标志
}

// 延时函数，每单位约10微秒
void delay_10us(unsigned char n)
{
    unsigned char i;
    for (i = n; i > 0; --i)
    {
        _nop_();  // 空操作，产生延时
        //_nop_(); // 如果在2MHz下运行，取消注释，增加延时
    }
}

// 重置DS18B20温度传感器
void reset_ds18b20(void)
{
    DQ_H;            // 将DQ引脚置高电平
    delay_10us(10);  // 延时100微秒，等待稳定
    DQ_L;            // 将DQ引脚拉低，发送复位脉冲
    delay_10us(60);  // 延时600微秒，保持复位脉冲
    DQ_H;            // 释放DQ引脚，等待DS18B20响应
    delay_10us(24);  // 延时240微秒，等待存在脉冲
}

// 向DS18B20写入一个字节
void write_byte(unsigned char dat)
{
    unsigned char i = 0;
    for (i = 8; i > 0; i--)
    {
        if (dat & 0x01)
        {          // 如果当前位为1
            DQ_L;  // 拉低DQ引脚
            _nop_();
            _nop_();        // 短暂延时（约2微秒）
            DQ_H;           // 释放DQ引脚
            delay_10us(6);  // 延时60微秒，等待时间槽结束
        }
        else
        {                   // 如果当前位为0
            DQ_L;           // 拉低DQ引脚
            delay_10us(6);  // 延时60微秒，保持低电平
            DQ_H;           // 释放DQ引脚
            _nop_();
            _nop_();  // 短暂延时
        }
        dat >>= 1;  // 右移一位，处理下一位
    }
    delay_10us(3);  // 延时30微秒，准备下一个操作
}

// 从DS18B20读取一个字节
unsigned char read_byte(void)
{
    unsigned char i = 0;
    unsigned char dat = 0;
    for (i = 8; i > 0; i--)
    {          // 读取8位数据
        DQ_L;  // 拉低DQ引脚，开始读取时间槽
        _nop_();
        _nop_();    // 短暂延时（约2微秒）
        DQ_H;       // 释放DQ引脚
        dat >>= 1;  // 右移一位，为下一位腾出空间
        if (DQ)     // 读取DQ引脚的电平
        {
            dat |= 0x80;  // 如果DQ为高电平，设置dat的最高位为1
        }
        delay_10us(6);  // 延时60微秒，等待时间槽结束
    }
    return (dat);  // 返回读取的字节
}

// 重置计时器到初始状态
void reset_timer_to_initial()
{
    ch451_write(CH451_DIG0 | 0);  // 清零显示位0
    ch451_write(CH451_DIG1 | 0);  // 清零显示位1
    ch451_write(CH451_DIG2 | 0);  // 清零显示位2
    ch451_write(CH451_DIG3 | 0);  // 清零显示位3
    // 清空其他显示位
    ch451_write(CH451_DIG4 | 0x10);  // 关闭显示位4
    ch451_write(CH451_DIG5 | 0x10);  // 关闭显示位5
    ch451_write(CH451_DIG6 | 0x10);  // 关闭显示位6
    ch451_write(CH451_DIG7 | 0x10);  // 关闭显示位7
}

// 倒计时表状态机，处理按键输入
void count_down_timer(unsigned char key_value)
{
    if (key_value == 15)
    {             // F3键，取消计时，重置
        ET0 = 0;  // 禁用定时器0中断
        if (display_mode == 1)
        {
            display_mode = 0;  // 切换到计时器显示
        }
        reset_timer_to_initial();  // 重置计时器显示
        timer_state = INPUT1;      // 进入输入第一个数字的状态
        return;                    // 退出函数，等待下一个按键
    }
    switch (timer_state)
    {
        case INITIAL:                  // 初始状态，直接进入输入状态
            reset_timer_to_initial();  // 重置计时器显示
            timer_state = INPUT1;      // 转换到输入第一个数字
            break;
        case INPUT1:
            if (key_value >= 0 && key_value <= 9)
            {
                min_t = key_value;   // 存储输入的分钟十位
                min_t_memo = min_t;  // 备份分钟十位
                if (display_mode == 0)
                {                                     // 如果是倒计时模式
                    ch451_write(CH451_DIG3 | min_t);  // 显示分钟的十位
                }
                timer_state = INPUT2;  // 转换到输入第二个数字
            }
            break;
        case INPUT2:
            if (key_value >= 0 && key_value <= 9)
            {
                min_i = key_value;   // 存储输入的分钟个位
                min_i_memo = min_i;  // 备份分钟个位
                if (display_mode == 0)
                {
                    ch451_write(CH451_DIG2 | min_i | 0x80);  // 显示分钟的个位，带小数点
                }
                timer_state = INPUT3;  // 转换到输入第三个数字
            }
            break;
        case INPUT3:
            if (key_value >= 0 && key_value <= 5)
            {
                second_t = key_value;      // 存储输入的秒钟十位
                second_t_memo = second_t;  // 备份秒钟十位
                if (display_mode == 0)
                {
                    ch451_write(CH451_DIG1 | second_t);  // 显示秒的十位
                }
                timer_state = INPUT4;  // 转换到输入第四个数字
            }
            break;
        case INPUT4:
            if (key_value >= 0 && key_value <= 9)
            {
                second_i = key_value;      // 存储输入的秒钟个位
                second_i_memo = second_i;  // 备份秒钟个位
                if (display_mode == 0)
                {
                    ch451_write(CH451_DIG0 | second_i);  // 显示秒的个位
                }
                first_start = 1;      // 设置首次启动标志
                timer_state = PAUSE;  // 转换到暂停状态，等待启动
            }
            break;
        case PAUSE:
            if (key_value == 14)
            {                 // F1键，开始/暂停
                TMOD = 0x01;  // 使用定时器0，模式1
                if (first_start)
                {                // 如果是首次启动，初始化定时器
                    TH0 = 0xFC;  // 设置定时器初值，高位
                    TL0 = 0x64;  // 设置定时器初值，低位
                    first_start = 0;
                }
                ET0 = 1;            // 允许定时器0中断
                TR0 = 1;            // 启动定时器0
                PT0 = 0;            // 设置定时器0中断优先级
                timer_state = RUN;  // 转换到运行状态
            }
            break;
        case RUN:
            if (key_value == 14)
            {                         // F1键，暂停
                TR0 = 0;              // 停止定时器0
                timer_state = PAUSE;  // 转换到暂停状态
            }
            break;
    }
    if (key_value == 13)
    {  // F2键，切换显示模式
        if (display_mode == 0)
        {
            display_mode = 1;  // 切换到温度显示模式
            // 清空计时器显示
        }
        else
        {
            display_mode = 0;  // 切换到倒计时模式
            // 恢复计时器显示
            ch451_write(CH451_DIG3 | min_t);         // 显示分钟的十位
            ch451_write(CH451_DIG2 | min_i | 0x80);  // 显示分钟的个位，带小数点
            ch451_write(CH451_DIG1 | second_t);      // 显示秒的十位
            ch451_write(CH451_DIG0 | second_i);      // 显示秒的个位
        }
    }
}

// 定时器0中断服务程序，每1ms触发一次
void InterruptTimer0() interrupt 1
{
    TH0 = 0xFC;           // 重新加载定时器初值，高位
    TL0 = 0x64;           // 重新加载定时器初值，低位
    cnt++;                // 1ms计数器自增
    beep_out = beepflag;  // 控制蜂鸣器输出

    if (beepflag == 1)
    {
        beepcount++;  // 蜂鸣器计数器自增
        if (beepcount >= 3000)
        {                   // 蜂鸣器响3秒后关闭
            beepflag = 0;   // 关闭蜂鸣器标志
            beepcount = 0;  // 计数器清零
            beep_out = 0;   // 关闭蜂鸣器输出
        }
    }

    if (cnt >= 1000)
    {             // 每1000次中断（1秒）
        cnt = 0;  // 重置计数
        if (second_i > 0)
        {
            second_i--;  // 秒的个位减1
        }
        else
        {
            if (second_t > 0)
            {
                second_t--;    // 秒的十位减1
                second_i = 9;  // 秒的个位重置为9
            }
            else
            {
                if (min_i > 0)
                {
                    min_i--;       // 分钟的个位减1
                    second_t = 5;  // 秒的十位重置为5
                    second_i = 9;  // 秒的个位重置为9
                }
                else
                {
                    if (min_t > 0)
                    {
                        min_t--;       // 分钟的十位减1
                        min_i = 9;     // 分钟的个位重置为9
                        second_t = 5;  // 秒的十位重置为5
                        second_i = 9;  // 秒的个位重置为9
                    }
                    else
                    {
                        // 倒计时结束
                        // 还原初始时间
                        min_t = min_t_memo;
                        min_i = min_i_memo;
                        second_t = second_t_memo;
                        second_i = second_i_memo;
                        beepflag = 1;   // 启动蜂鸣器
                        beepcount = 0;  // 蜂鸣器计数器清零
                    }
                }
            }
        }
    }
}

// 读取温度函数
unsigned int read_temperature()
{
    unsigned int a = 0, b = 0, t = 0;
    unsigned int tt = 0;
    reset_ds18b20();    // 复位DS18B20
    write_byte(0xCC);   // 跳过ROM命令，直接对单个设备操作
    write_byte(0x44);   // 启动温度转换命令
    delay_10us(50000);  // 等待温度转换完成（约500ms）

    reset_ds18b20();    // 再次复位DS18B20
    write_byte(0xCC);   // 跳过ROM命令
    write_byte(0xBE);   // 读取暂存器中的温度值
    delay_10us(50000);  // 等待读取

    a = read_byte();  // 读取温度低字节
    b = read_byte();  // 读取温度高字节
    t = b;
    t <<= 8;               // 将高字节移至高8位
    t = t | a;             // 合并低字节和高字节，得到完整的温度值
    tt = t * 10 * 0.0625;  // 将温度转换为实际值（带一位小数）
    // tt = (unsigned int)(tt * 10 + 0.5); // 四舍五入，未使用
    return (tt);  // 返回温度值
}

// 显示温度
void display_temperature()
{
    unsigned char temperature_h;
    unsigned char temperature_t;
    unsigned char temperature_i;
    unsigned int f;
    f = read_temperature() - 10;  // 获取温度值，减去偏移量10（校准）
    temperature_i = f % 10;       // 提取温度的小数位
    f = f / 10;
    temperature_t = f % 10;  // 提取温度的个位
    f = f / 10;
    temperature_h = f % 10;  // 提取温度的十位
    if (display_mode == 1)
    {
        ch451_write(CH451_DIG0 | temperature_i);         // 显示小数位
        ch451_write(CH451_DIG1 | temperature_t | 0x80);  // 显示个位，带小数点
        ch451_write(CH451_DIG2 | temperature_h);         // 显示十位
        // 清空其他显示位
        ch451_write(CH451_DIG3 | 0x10);
        ch451_write(CH451_DIG4 | 0x10);
        ch451_write(CH451_DIG5 | 0x10);
        ch451_write(CH451_DIG6 | 0x10);
        ch451_write(CH451_DIG7 | 0x10);
    }
    else
    {
        // 如果不是温度显示模式，不做处理
    }
}

// 转换按键编码
unsigned char tran(unsigned char key)
{
    key &= 0x3F;  // 清除高两位，只保留低6位
    if (key < 4)
        return key;
    else if (key < 12)
        return key - 4;
    else if (key < 20)
        return key - 8;
    else if (key < 28)
        return key - 12;  // 根据按键编码的模式转换，映射到实际按键值
    else
        return 0xFF;  // 无效按键
}

// 主函数
int main()
{
    unsigned char key_value;
    unsigned int i;
    ch451_init();        // 初始化CH451显示模块
    ch451_write(0x403);  // 设置CH451的工作模式
    ch451_write(0x580);  // 设置CH451为BCD码显示模式
    // 初始化所有显示位为空
    ch451_write(CH451_DIG0 | 0x10);
    ch451_write(CH451_DIG1 | 0x10);
    ch451_write(CH451_DIG2 | 0x10);
    ch451_write(CH451_DIG3 | 0x10);
    ch451_write(CH451_DIG4 | 0x10);
    ch451_write(CH451_DIG5 | 0x10);
    ch451_write(CH451_DIG6 | 0x10);
    ch451_write(CH451_DIG7 | 0x10);

    while (1)
    {
        // DS18B20温度采集并显示
        EA = 0;                 // 禁用全局中断，防止干扰
        display_temperature();  // 显示温度
        EA = 1;                 // 启用全局中断

        if (timer_state == RUN)
        {
            if (display_mode == 0)
            {
                ch451_write(CH451_DIG3 | min_t);         // 显示分钟十位
                ch451_write(CH451_DIG2 | min_i | 0x80);  // 显示分钟个位，带小数点
                ch451_write(CH451_DIG1 | second_t);      // 显示秒的十位
                ch451_write(CH451_DIG0 | second_i);      // 显示秒的个位
                // 清空其他显示位
                ch451_write(CH451_DIG4 | 0x10);
                ch451_write(CH451_DIG5 | 0x10);
                ch451_write(CH451_DIG6 | 0x10);
                ch451_write(CH451_DIG7 | 0x10);
            }
            else
            {
                // 如果不在倒计时显示模式，清空相关显示位
                ch451_write(CH451_DIG3 | 0x10);
                ch451_write(CH451_DIG4 | 0x10);
                ch451_write(CH451_DIG5 | 0x10);
                ch451_write(CH451_DIG6 | 0x10);
                ch451_write(CH451_DIG7 | 0x10);
            }
        }

        EX0 = 1;            // 启用外部中断0（按键中断）
        PX0 = 1;            // 设置外部中断0为高优先级
        ch451_key = 0x0FF;  // 初始化按键值
        for (i = 10000; i > 0; --i)
        {  // 持续检测按键，循环等待
            if (ch451_key != 0xFF)
            {
                key_value = ch451_key;        // 获取按键值
                key_value = tran(key_value);  // 转换为实际按键值
                ch451_key = 0x0FF;            // 重置按键值
                count_down_timer(key_value);  // 处理按键事件
                break;                        // 退出循环，处理完一次按键
            }
        }
        EX0 = 0;  // 禁用外部中断0，防止干扰
    }
}
