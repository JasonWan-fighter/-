$NOMOD51
$INCLUDE (8051.MCU)
 
OP1 EQU 66H
OPERATOR EQU 67H
OP2 EQU 68H
 
ORG 0000H
AJMP START;�����ж���ڵ�ַ
ORG 000BH
LJMP CTC0;����תָ���ʱ���жϳ���
ORG 0100H

START:
;--------------------------------------------------------
    CALL CLEAR
    MOV R0,#60H;R0����ʾ��������Ԫ��ַ
    MOV R7,#0FEH;������λ����ܵ�λѡ�ź�
    MOV TMOD, #01H
    MOV TL0, #0F0H
    MOV TH0, #0D8H ;��·ͼ�����þ���Ϊ12MHZ����ʱ���ļ���Ƶ��Ϊ 1MHz��
    ;��ʱ10ms, T0��ʼֵ�����55536��D8F0H    (65536-55536)/(1M)=0.01
    SETB TR0 ;�Ȳ�������ʱ��T0���ȵ����¡�=��ʱ������T0
    SETB ET0 ;����T0�ж�
    SETB EA ;CPU�����ж�
;---------------------------------------------------------------


;������--------------------------------------------------------------
MAIN:LJMP KEYJUDGE
SCAN:LCALL KEYSCAN
     LJMP MAIN 
;�ж��Ƿ��а�������    
KEYJUDGE:
    MOV P3,#0F0H
    MOV A,P3
    XRL A,#0F0H 
    JNZ DELAY 
    LJMP MAIN
    ;DELAY�������Ǳ���һ�ΰ����Ķ�μ��
    ;���ڲ��Թ�����0FFH*0FFH�ε�DELAY�������ظ����
    ;�����ڵ�һ��DELAY�ĺ����ּ���һ��DELAY
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
;��ɨ�跨���������ӳ���  ������������뻺����60H��61H  
;�õ��Ĵ���R0��R1��R2��R3
;R6���кţ�0��1��2��3��
;R1���к�*4��0��4��8��C��
;R2������ѡ
;R3����ѭ���������ܹ��ĴΣ�

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
    ;���ⰴ���ˡ�C������ջ�����
    CALL CLEAR
    RET
NEXT1:
    ;�����������ֵ�������
    MOV 65H,64H
    MOV 64H,63H
    MOV 63H,62H
    MOV 62H,61H
    MOV 61H,60H;��20h��Ԫ�е���������61h
    MOV 60H,A;���������������60H
    RET
 
;====================================================================

;��ʱ��0�ж��ӳ���------------------------------------------------------------------
CTC0: 
    MOV TL0, #0F0H
    MOV TH0, #0D8H
    MOV DPTR, #TABLE
    MOV P1,R7
    MOV A,@R0
    MOVC A,@A+DPTR
    MOV P0, A
    MOV A,R7
    RL A;λѡ�ź�����һλ
    CJNE A,#0BFH,NEXT2
    MOV A,#0FEH
NEXT2:
    MOV R7,A
    INC R0;��ʾ��������Ԫ���ƶ�һλ
    CJNE R0,#66H,EXIT
    MOV R0,#60H
EXIT:
    RETI
 ;-----------------------------------------------------------------------------------
    
;��ջ�����---------------------------------------------------------------------------
CLEAR:
    MOV 60H,#10H;��ӦTABLE���00H����������ϲ���ʾ
    MOV 61H,#10H;
    MOV 62H,#10H;
    MOV 63H,#10H;
    MOV 64H,#10H;
    MOV 65H,#10H;
    RET
 ;--------------------------------------------------------------------------------------
    
TABLE: DB 3FH, 06H, 5BH, 4FH, 66H, 6DH, 7DH, 07H, 7FH, 6FH, 77H, 7CH, 39H, 5EH, 79H, 71H  ,00H;���һ��00h��ʾ����ʾ
KEYBOARD:DB 07H,08H,09H,0DH;7,8,9,/(Devide)
         DB 04H,05H,06H,0FH;4,5,6,*(��F��ʾ��
	 DB 01H,02H,03H,0BH;1,2,3,-(suB)
	 DB 0CH,00H,0EH,0AH;C,0,=(Equal),+(Add)
     
END