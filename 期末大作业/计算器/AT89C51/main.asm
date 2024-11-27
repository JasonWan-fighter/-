$NOMOD51
$INCLUDE (8051.MCU)
 
OP1 EQU 66H
OPERATOR EQU 67H
OP2 EQU 68H
 
ORG 0000H
AJMP START;跳过中断入口地址
ORG 000BH
LJMP CTC0;长跳转指令：定时器中断程序
ORG 0100H

START:
;--------------------------------------------------------
    CALL CLEAR
    MOV R0,#60H;R0存显示缓冲区单元地址
    MOV R7,#0FEH;用作四位数码管的位选信号
    MOV TMOD, #01H
    MOV TL0, #0F0H
    MOV TH0, #0D8H ;电路图中设置晶振为12MHZ，则定时器的计数频率为 1MHz。
    ;定时10ms, T0初始值计算得55536即D8F0H    (65536-55536)/(1M)=0.01
    SETB TR0 ;先不启动定时器T0，等到按下“=”时再启动T0
    SETB ET0 ;允许T0中断
    SETB EA ;CPU开放中断
;---------------------------------------------------------------


;主程序--------------------------------------------------------------
MAIN:LJMP KEYJUDGE
SCAN:LCALL KEYSCAN
     LJMP MAIN 
;判断是否有按键按下    
KEYJUDGE:
    MOV P3,#0F0H
    MOV A,P3
    XRL A,#0F0H 
    JNZ DELAY 
    LJMP MAIN
    ;DELAY的作用是避免一次按键的多次检测
    ;由于测试过程中0FFH*0FFH次的DELAY还会有重复检测
    ;所以在第一段DELAY的后面又加了一段DELAY
DELAY:MOV R4,#0FFH 
LOOP1:MOV R5,#0FFH 
LOOP2:DJNZ R5,LOOP2 
    DJNZ R4,LOOP1
    MOV R4,#0FFH 
LOOP10:MOV R5,#0A0H 
LOOP20:DJNZ R5,LOOP20 
    DJNZ R4,LOOP10
    MOV P3,#0F0H
    MOV A,P3
    XRL A,#0F0H
    JZ MAIN
    JMP SCAN
;-----------------------------------------------------------------------------------

;====================================================================    
;行扫描法键盘输入子程序  将输入的数存入缓冲区60H，61H  
;用到寄存器R0、R1、R2、R3
;R6存列号（0，1，2，3）
;R1存行号*4（0，4，8，C）
;R2用作列选
;R3控制循环次数（总共四次）

KEYSCAN:
    MOV R6,#00H 
    MOV R1,#00H 
    MOV R2,#0FEH 
    MOV R3,#04H 
KEY:
    MOV P3,R2
KEY0:JB P3.4,KEY1
    MOV R1,#00H
    LJMP NUM
KEY1:
    JB P3.5,KEY2 
    MOV R1,#04H 
    LJMP NUM 
KEY2:
    JB P3.6,KEY3 
    MOV R1,#08H 
    LJMP NUM 
KEY3:
    JB P3.7,NEXT 
    MOV R1,#0CH 
    LJMP NUM
NEXT:
    INC R6
    MOV A,R2
    RL A
    MOV R2,A
    DJNZ R3,KEY 
    LJMP NUM 
NUM:
    MOV A,R6
    ADD A,R1
    MOV DPTR,#KEYBOARD
    MOVC A,@A+DPTR 
    CJNE A,#0CH,NEXT1
    ;到这按下了“C”，清空缓冲区
    CALL CLEAR
    RET
NEXT1:
    ;到这输入数字到缓冲区
    MOV 65H,64H
    MOV 64H,63H
    MOV 63H,62H
    MOV 62H,61H
    MOV 61H,60H;把20h单元中的内容送往61h
    MOV 60H,A;把新输入的数送往60H
    RET
 
;====================================================================

;定时器0中断子程序------------------------------------------------------------------
CTC0: 
    MOV TL0, #0F0H
    MOV TH0, #0D8H
    MOV DPTR, #TABLE
    MOV P1,R7
    MOV A,@R0
    MOVC A,@A+DPTR
    MOV P0, A
    MOV A,R7
    RL A;位选信号左移一位
    CJNE A,#0BFH,NEXT2
    MOV A,#0FEH
NEXT2:
    MOV R7,A
    INC R0;显示缓冲区单元号移动一位
    CJNE R0,#66H,EXIT
    MOV R0,#60H
EXIT:
    RETI
 ;-----------------------------------------------------------------------------------
    
;清空缓冲区---------------------------------------------------------------------------
CLEAR:
    MOV 60H,#10H;对应TABLE表的00H，在数码管上不显示
    MOV 61H,#10H;
    MOV 62H,#10H;
    MOV 63H,#10H;
    MOV 64H,#10H;
    MOV 65H,#10H;
    RET
 ;--------------------------------------------------------------------------------------
    
TABLE: DB 3FH, 06H, 5BH, 4FH, 66H, 6DH, 7DH, 07H, 7FH, 6FH, 77H, 7CH, 39H, 5EH, 79H, 71H  ,00H;最后一个00h表示不显示
KEYBOARD:DB 07H,08H,09H,0DH;7,8,9,/(Devide)
         DB 04H,05H,06H,0FH;4,5,6,*(用F表示）
	 DB 01H,02H,03H,0BH;1,2,3,-(suB)
	 DB 0CH,00H,0EH,0AH;C,0,=(Equal),+(Add)
     
END