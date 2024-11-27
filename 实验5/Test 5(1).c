#include <intrins.h>
#include <reg51.h>

// CH451��ʾģ�������
#define CH451_RESET 0x0201     // ��λ����
#define CH451_LEFTMOV 0x0300   // �����ƶ�ģʽ - �������
#define CH451_LEFTCYC 0x0301   // �����ƶ�ģʽ - ����ѭ��
#define CH451_RIGHTMOV 0x0302  // �����ƶ�ģʽ - ���ҹ���
#define CH451_RIGHTCYC 0x0303  // �����ƶ�ģʽ - ����ѭ��
#define CH451_SYSOFF 0x0400    // ϵͳ�ر� - �������й���
#define CH451_SYSON1 0x0401    // ϵͳ���� - ģʽ1
#define CH451_SYSON2 0x0403    // ϵͳ���� - ģʽ2
#define CH451_SYSON3 0x0407    // ϵͳ���� - ģʽ3�����ӹ���
#define CH451_DSP 0x0500       // ����Ĭ����ʾģʽ
#define CH451_BCD 0x0580       // ����BCD����ģʽ
#define CH451_TWINKLE 0x0600   // ������˸����
#define CH451_DIG0 0x0800      // ��ʾ����0
#define CH451_DIG1 0x0900      // ��ʾ����1
#define CH451_DIG2 0x0A00      // ��ʾ����2
#define CH451_DIG3 0x0B00      // ��ʾ����3
#define CH451_DIG4 0x0C00      // ��ʾ����4
#define CH451_DIG5 0x0D00      // ��ʾ����5
#define CH451_DIG6 0x0E00      // ��ʾ����6
#define CH451_DIG7 0x0F00      // ��ʾ����7

// CH451�������Ŷ���
sbit ch451_load = P1 ^ 3;  // LOAD���ţ��������������
sbit ch451_din = P1 ^ 1;   // DIN���ţ���������
sbit ch451_dclk = P1 ^ 2;  // DCLK���ţ�ʱ������
sbit ch451_dout = P3 ^ 2;  // DOUT���ţ�������������ӵ�P3.2��INT0���ţ�
sbit beep_out = P1 ^ 5;    // ������������ţ����Ʒ�����������

unsigned char ch451_key;  // �洢����ֵ�ı���

// DS18B20�¶ȴ�������������
sbit DQ = P0 ^ 0;    // ��������DQ�����ӵ�P0.0
#define DQ_H DQ = 1  // �궨�壬��DQ�øߵ�ƽ
#define DQ_L DQ = 0  // �궨�壬��DQ�õ͵�ƽ

// ��ʱ��ص�ȫ�ֱ���
unsigned int cnt = 0;        // ��ʱ����������cnt=1000��Ӧ1��
unsigned char second_i = 0;  // ��ĸ�λ
unsigned char second_t = 0;  // ���ʮλ
unsigned char min_i = 0;     // ���ӵĸ�λ
unsigned char min_t = 0;     // ���ӵ�ʮλ

// ���ݱ������������ü�ʱ����
unsigned char second_i_memo = 0;
unsigned char second_t_memo = 0;
unsigned char min_i_memo = 0;
unsigned char min_t_memo = 0;

unsigned char first_start = 1;   // ��ʼ������־��1��ʾ��һ������
unsigned char display_mode = 1;  // ��ʾģʽ��0 = ����ʱ��1 = �¶�

// ����ʱ��״̬ö�٣���ʾ��ʱ����ǰ��״̬
enum COUNT_DOWN_TIMER_STATE
{
    INITIAL,  // ��ʼ״̬
    INPUT1,   // �������ʮλ
    INPUT2,   // ������Ӹ�λ
    INPUT3,   // ��������ʮλ
    INPUT4,   // �������Ӹ�λ
    RUN,      // ����״̬
    PAUSE     // ��ͣ״̬
};
enum COUNT_DOWN_TIMER_STATE timer_state = INITIAL;  // ��ǰ����ʱ����״̬

unsigned char beepflag = 0;  // ��������־��1��ʾ������������
unsigned int beepcount = 0;  // �����������������ڿ��Ʒ���������ʱ��

// ����ԭ������
void ch451_init();                               // ��ʼ��CH451
void ch451_write(unsigned int command);          // ��CH451д������
void delay_10us(unsigned char n);                // ��ʱ������ÿ��λԼ10΢��
void reset_ds18b20(void);                        // ����DS18B20
void write_byte(unsigned char dat);              // ��DS18B20д��һ���ֽ�
unsigned char read_byte(void);                   // ��DS18B20��ȡһ���ֽ�
void count_down_timer(unsigned char key_value);  // ������ʱ����״̬��
unsigned int read_temperature();                 // ��ȡ�¶�ֵ
void display_temperature();                      // ��ʾ�¶�ֵ
unsigned char tran(unsigned char key);           // ת����������Ϊʵ�ʰ���ֵ
void reset_timer_to_initial();                   // ���ü�ʱ������ʼ״̬

// ��ʼ��CH451��ʾģ��
void ch451_init()
{
    ch451_din = 0;  // ��ʼ��DINΪ�͵�ƽ
    ch451_din = 1;  // ����DINΪ�ߵ�ƽ��׼����ʼͨ��
    IT1 = 0;        // ����INT1Ϊ�͵�ƽ������ʽ
    IE1 = 0;        // ���INT1�жϱ�־
    PX1 = 0;        // ����INT1Ϊ�����ȼ�
    EX1 = 1;        // �����ⲿ�ж�1��INT1��
}

// ��CH451д������
void ch451_write(unsigned int command)
{
    unsigned char i;
    EX1 = 0;         // д������н����ⲿ�ж�1����ֹ����
    ch451_load = 0;  // ��ʼ���䣬LOAD�õ�
    for (i = 0; i < 12; i++)
    {                             // ����12λ����
        ch451_din = command & 1;  // �������λ������λ
        ch451_dclk = 0;           // ����һ��ʱ���½���
        command >>= 1;            // ����һλ��׼��������һλ
        ch451_dclk = 1;           // ����һ��ʱ�������أ����ݱ�CH451��ȡ
    }
    ch451_load = 1;  // �������䣬LOAD�øߣ����ִ��
    EX1 = 1;         // ���������ⲿ�ж�1
}

// �ⲿ�ж�0����������ڰ������
void ch451_inter() interrupt 0
{
    unsigned char i;
    unsigned char command, keycode;
    command = 0x07;  // ��ȡ���������Ϊ0111
    ch451_load = 0;  // ��ʼ����
    for (i = 0; i < 4; i++)
    {
        ch451_din = command & 1;  // ���Ͷ�ȡ�����ÿһλ
        ch451_dclk = 0;           // ʱ���½���
        command >>= 1;            // ����һλ��׼��������һλ
        ch451_dclk = 1;           // ʱ��������
    }
    ch451_load = 1;  // ���������
    keycode = 0;     // ��ʼ����������
    for (i = 0; i < 7; i++)
    {                           // ��ȡ7λ��������
        keycode <<= 1;          // ����һλ��Ϊ��һλ�ڳ��ռ�
        keycode |= ch451_dout;  // ��ȡDOUT���ŵ�ֵ
        ch451_dclk = 0;         // ʱ���½���
        ch451_dclk = 1;         // ʱ��������
    }
    ch451_key = keycode;  // �洢�������룬��������ʹ��
    IE0 = 0;              // ����жϱ�־
}

// ��ʱ������ÿ��λԼ10΢��
void delay_10us(unsigned char n)
{
    unsigned char i;
    for (i = n; i > 0; --i)
    {
        _nop_();  // �ղ�����������ʱ
        //_nop_(); // �����2MHz�����У�ȡ��ע�ͣ�������ʱ
    }
}

// ����DS18B20�¶ȴ�����
void reset_ds18b20(void)
{
    DQ_H;            // ��DQ�����øߵ�ƽ
    delay_10us(10);  // ��ʱ100΢�룬�ȴ��ȶ�
    DQ_L;            // ��DQ�������ͣ����͸�λ����
    delay_10us(60);  // ��ʱ600΢�룬���ָ�λ����
    DQ_H;            // �ͷ�DQ���ţ��ȴ�DS18B20��Ӧ
    delay_10us(24);  // ��ʱ240΢�룬�ȴ���������
}

// ��DS18B20д��һ���ֽ�
void write_byte(unsigned char dat)
{
    unsigned char i = 0;
    for (i = 8; i > 0; i--)
    {
        if (dat & 0x01)
        {          // �����ǰλΪ1
            DQ_L;  // ����DQ����
            _nop_();
            _nop_();        // ������ʱ��Լ2΢�룩
            DQ_H;           // �ͷ�DQ����
            delay_10us(6);  // ��ʱ60΢�룬�ȴ�ʱ��۽���
        }
        else
        {                   // �����ǰλΪ0
            DQ_L;           // ����DQ����
            delay_10us(6);  // ��ʱ60΢�룬���ֵ͵�ƽ
            DQ_H;           // �ͷ�DQ����
            _nop_();
            _nop_();  // ������ʱ
        }
        dat >>= 1;  // ����һλ��������һλ
    }
    delay_10us(3);  // ��ʱ30΢�룬׼����һ������
}

// ��DS18B20��ȡһ���ֽ�
unsigned char read_byte(void)
{
    unsigned char i = 0;
    unsigned char dat = 0;
    for (i = 8; i > 0; i--)
    {          // ��ȡ8λ����
        DQ_L;  // ����DQ���ţ���ʼ��ȡʱ���
        _nop_();
        _nop_();    // ������ʱ��Լ2΢�룩
        DQ_H;       // �ͷ�DQ����
        dat >>= 1;  // ����һλ��Ϊ��һλ�ڳ��ռ�
        if (DQ)     // ��ȡDQ���ŵĵ�ƽ
        {
            dat |= 0x80;  // ���DQΪ�ߵ�ƽ������dat�����λΪ1
        }
        delay_10us(6);  // ��ʱ60΢�룬�ȴ�ʱ��۽���
    }
    return (dat);  // ���ض�ȡ���ֽ�
}

// ���ü�ʱ������ʼ״̬
void reset_timer_to_initial()
{
    ch451_write(CH451_DIG0 | 0);  // ������ʾλ0
    ch451_write(CH451_DIG1 | 0);  // ������ʾλ1
    ch451_write(CH451_DIG2 | 0);  // ������ʾλ2
    ch451_write(CH451_DIG3 | 0);  // ������ʾλ3
    // ���������ʾλ
    ch451_write(CH451_DIG4 | 0x10);  // �ر���ʾλ4
    ch451_write(CH451_DIG5 | 0x10);  // �ر���ʾλ5
    ch451_write(CH451_DIG6 | 0x10);  // �ر���ʾλ6
    ch451_write(CH451_DIG7 | 0x10);  // �ر���ʾλ7
}

// ����ʱ��״̬��������������
void count_down_timer(unsigned char key_value)
{
    if (key_value == 15)
    {             // F3����ȡ����ʱ������
        ET0 = 0;  // ���ö�ʱ��0�ж�
        if (display_mode == 1)
        {
            display_mode = 0;  // �л�����ʱ����ʾ
        }
        reset_timer_to_initial();  // ���ü�ʱ����ʾ
        timer_state = INPUT1;      // ���������һ�����ֵ�״̬
        return;                    // �˳��������ȴ���һ������
    }
    switch (timer_state)
    {
        case INITIAL:                  // ��ʼ״̬��ֱ�ӽ�������״̬
            reset_timer_to_initial();  // ���ü�ʱ����ʾ
            timer_state = INPUT1;      // ת���������һ������
            break;
        case INPUT1:
            if (key_value >= 0 && key_value <= 9)
            {
                min_t = key_value;   // �洢����ķ���ʮλ
                min_t_memo = min_t;  // ���ݷ���ʮλ
                if (display_mode == 0)
                {                                     // ����ǵ���ʱģʽ
                    ch451_write(CH451_DIG3 | min_t);  // ��ʾ���ӵ�ʮλ
                }
                timer_state = INPUT2;  // ת��������ڶ�������
            }
            break;
        case INPUT2:
            if (key_value >= 0 && key_value <= 9)
            {
                min_i = key_value;   // �洢����ķ��Ӹ�λ
                min_i_memo = min_i;  // ���ݷ��Ӹ�λ
                if (display_mode == 0)
                {
                    ch451_write(CH451_DIG2 | min_i | 0x80);  // ��ʾ���ӵĸ�λ����С����
                }
                timer_state = INPUT3;  // ת�����������������
            }
            break;
        case INPUT3:
            if (key_value >= 0 && key_value <= 5)
            {
                second_t = key_value;      // �洢���������ʮλ
                second_t_memo = second_t;  // ��������ʮλ
                if (display_mode == 0)
                {
                    ch451_write(CH451_DIG1 | second_t);  // ��ʾ���ʮλ
                }
                timer_state = INPUT4;  // ת����������ĸ�����
            }
            break;
        case INPUT4:
            if (key_value >= 0 && key_value <= 9)
            {
                second_i = key_value;      // �洢��������Ӹ�λ
                second_i_memo = second_i;  // �������Ӹ�λ
                if (display_mode == 0)
                {
                    ch451_write(CH451_DIG0 | second_i);  // ��ʾ��ĸ�λ
                }
                first_start = 1;      // �����״�������־
                timer_state = PAUSE;  // ת������ͣ״̬���ȴ�����
            }
            break;
        case PAUSE:
            if (key_value == 14)
            {                 // F1������ʼ/��ͣ
                TMOD = 0x01;  // ʹ�ö�ʱ��0��ģʽ1
                if (first_start)
                {                // ������״���������ʼ����ʱ��
                    TH0 = 0xFC;  // ���ö�ʱ����ֵ����λ
                    TL0 = 0x64;  // ���ö�ʱ����ֵ����λ
                    first_start = 0;
                }
                ET0 = 1;            // ����ʱ��0�ж�
                TR0 = 1;            // ������ʱ��0
                PT0 = 0;            // ���ö�ʱ��0�ж����ȼ�
                timer_state = RUN;  // ת��������״̬
            }
            break;
        case RUN:
            if (key_value == 14)
            {                         // F1������ͣ
                TR0 = 0;              // ֹͣ��ʱ��0
                timer_state = PAUSE;  // ת������ͣ״̬
            }
            break;
    }
    if (key_value == 13)
    {  // F2�����л���ʾģʽ
        if (display_mode == 0)
        {
            display_mode = 1;  // �л����¶���ʾģʽ
            // ��ռ�ʱ����ʾ
        }
        else
        {
            display_mode = 0;  // �л�������ʱģʽ
            // �ָ���ʱ����ʾ
            ch451_write(CH451_DIG3 | min_t);         // ��ʾ���ӵ�ʮλ
            ch451_write(CH451_DIG2 | min_i | 0x80);  // ��ʾ���ӵĸ�λ����С����
            ch451_write(CH451_DIG1 | second_t);      // ��ʾ���ʮλ
            ch451_write(CH451_DIG0 | second_i);      // ��ʾ��ĸ�λ
        }
    }
}

// ��ʱ��0�жϷ������ÿ1ms����һ��
void InterruptTimer0() interrupt 1
{
    TH0 = 0xFC;           // ���¼��ض�ʱ����ֵ����λ
    TL0 = 0x64;           // ���¼��ض�ʱ����ֵ����λ
    cnt++;                // 1ms����������
    beep_out = beepflag;  // ���Ʒ��������

    if (beepflag == 1)
    {
        beepcount++;  // ����������������
        if (beepcount >= 3000)
        {                   // ��������3���ر�
            beepflag = 0;   // �رշ�������־
            beepcount = 0;  // ����������
            beep_out = 0;   // �رշ��������
        }
    }

    if (cnt >= 1000)
    {             // ÿ1000���жϣ�1�룩
        cnt = 0;  // ���ü���
        if (second_i > 0)
        {
            second_i--;  // ��ĸ�λ��1
        }
        else
        {
            if (second_t > 0)
            {
                second_t--;    // ���ʮλ��1
                second_i = 9;  // ��ĸ�λ����Ϊ9
            }
            else
            {
                if (min_i > 0)
                {
                    min_i--;       // ���ӵĸ�λ��1
                    second_t = 5;  // ���ʮλ����Ϊ5
                    second_i = 9;  // ��ĸ�λ����Ϊ9
                }
                else
                {
                    if (min_t > 0)
                    {
                        min_t--;       // ���ӵ�ʮλ��1
                        min_i = 9;     // ���ӵĸ�λ����Ϊ9
                        second_t = 5;  // ���ʮλ����Ϊ5
                        second_i = 9;  // ��ĸ�λ����Ϊ9
                    }
                    else
                    {
                        // ����ʱ����
                        // ��ԭ��ʼʱ��
                        min_t = min_t_memo;
                        min_i = min_i_memo;
                        second_t = second_t_memo;
                        second_i = second_i_memo;
                        beepflag = 1;   // ����������
                        beepcount = 0;  // ����������������
                    }
                }
            }
        }
    }
}

// ��ȡ�¶Ⱥ���
unsigned int read_temperature()
{
    unsigned int a = 0, b = 0, t = 0;
    unsigned int tt = 0;
    reset_ds18b20();    // ��λDS18B20
    write_byte(0xCC);   // ����ROM���ֱ�ӶԵ����豸����
    write_byte(0x44);   // �����¶�ת������
    delay_10us(50000);  // �ȴ��¶�ת����ɣ�Լ500ms��

    reset_ds18b20();    // �ٴθ�λDS18B20
    write_byte(0xCC);   // ����ROM����
    write_byte(0xBE);   // ��ȡ�ݴ����е��¶�ֵ
    delay_10us(50000);  // �ȴ���ȡ

    a = read_byte();  // ��ȡ�¶ȵ��ֽ�
    b = read_byte();  // ��ȡ�¶ȸ��ֽ�
    t = b;
    t <<= 8;               // �����ֽ�������8λ
    t = t | a;             // �ϲ����ֽں͸��ֽڣ��õ��������¶�ֵ
    tt = t * 10 * 0.0625;  // ���¶�ת��Ϊʵ��ֵ����һλС����
    // tt = (unsigned int)(tt * 10 + 0.5); // �������룬δʹ��
    return (tt);  // �����¶�ֵ
}

// ��ʾ�¶�
void display_temperature()
{
    unsigned char temperature_h;
    unsigned char temperature_t;
    unsigned char temperature_i;
    unsigned int f;
    f = read_temperature() - 10;  // ��ȡ�¶�ֵ����ȥƫ����10��У׼��
    temperature_i = f % 10;       // ��ȡ�¶ȵ�С��λ
    f = f / 10;
    temperature_t = f % 10;  // ��ȡ�¶ȵĸ�λ
    f = f / 10;
    temperature_h = f % 10;  // ��ȡ�¶ȵ�ʮλ
    if (display_mode == 1)
    {
        ch451_write(CH451_DIG0 | temperature_i);         // ��ʾС��λ
        ch451_write(CH451_DIG1 | temperature_t | 0x80);  // ��ʾ��λ����С����
        ch451_write(CH451_DIG2 | temperature_h);         // ��ʾʮλ
        // ���������ʾλ
        ch451_write(CH451_DIG3 | 0x10);
        ch451_write(CH451_DIG4 | 0x10);
        ch451_write(CH451_DIG5 | 0x10);
        ch451_write(CH451_DIG6 | 0x10);
        ch451_write(CH451_DIG7 | 0x10);
    }
    else
    {
        // ��������¶���ʾģʽ����������
    }
}

// ת����������
unsigned char tran(unsigned char key)
{
    key &= 0x3F;  // �������λ��ֻ������6λ
    if (key < 4)
        return key;
    else if (key < 12)
        return key - 4;
    else if (key < 20)
        return key - 8;
    else if (key < 28)
        return key - 12;  // ���ݰ��������ģʽת����ӳ�䵽ʵ�ʰ���ֵ
    else
        return 0xFF;  // ��Ч����
}

// ������
int main()
{
    unsigned char key_value;
    unsigned int i;
    ch451_init();        // ��ʼ��CH451��ʾģ��
    ch451_write(0x403);  // ����CH451�Ĺ���ģʽ
    ch451_write(0x580);  // ����CH451ΪBCD����ʾģʽ
    // ��ʼ��������ʾλΪ��
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
        // DS18B20�¶Ȳɼ�����ʾ
        EA = 0;                 // ����ȫ���жϣ���ֹ����
        display_temperature();  // ��ʾ�¶�
        EA = 1;                 // ����ȫ���ж�

        if (timer_state == RUN)
        {
            if (display_mode == 0)
            {
                ch451_write(CH451_DIG3 | min_t);         // ��ʾ����ʮλ
                ch451_write(CH451_DIG2 | min_i | 0x80);  // ��ʾ���Ӹ�λ����С����
                ch451_write(CH451_DIG1 | second_t);      // ��ʾ���ʮλ
                ch451_write(CH451_DIG0 | second_i);      // ��ʾ��ĸ�λ
                // ���������ʾλ
                ch451_write(CH451_DIG4 | 0x10);
                ch451_write(CH451_DIG5 | 0x10);
                ch451_write(CH451_DIG6 | 0x10);
                ch451_write(CH451_DIG7 | 0x10);
            }
            else
            {
                // ������ڵ���ʱ��ʾģʽ����������ʾλ
                ch451_write(CH451_DIG3 | 0x10);
                ch451_write(CH451_DIG4 | 0x10);
                ch451_write(CH451_DIG5 | 0x10);
                ch451_write(CH451_DIG6 | 0x10);
                ch451_write(CH451_DIG7 | 0x10);
            }
        }

        EX0 = 1;            // �����ⲿ�ж�0�������жϣ�
        PX0 = 1;            // �����ⲿ�ж�0Ϊ�����ȼ�
        ch451_key = 0x0FF;  // ��ʼ������ֵ
        for (i = 10000; i > 0; --i)
        {  // ������ⰴ����ѭ���ȴ�
            if (ch451_key != 0xFF)
            {
                key_value = ch451_key;        // ��ȡ����ֵ
                key_value = tran(key_value);  // ת��Ϊʵ�ʰ���ֵ
                ch451_key = 0x0FF;            // ���ð���ֵ
                count_down_timer(key_value);  // �������¼�
                break;                        // �˳�ѭ����������һ�ΰ���
            }
        }
        EX0 = 0;  // �����ⲿ�ж�0����ֹ����
    }
}
