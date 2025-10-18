IN
POPR  S0  # R

PUSH  20  
POPR  S1  # width

PUSHR S1
PUSH  2
DIV
POPR  S3  # center_x

PUSH  20  
POPR  S2  # height

PUSHR S2
PUSH  2
DIV
POPR  S4  # center_y

PUSH  0   
POPR  T0  # x

PUSH  0   
POPR  T1  # y

:loop_y
PUSH  0
POPR  T0      # x = 0

:loop_x
CALL  :draw
PUSHR T0
PUSH  1
ADD
POPR  T0      # x += 1
PUSHR T0
PUSHR S1      # stk: x, width
JB    :loop_x # if x < width

PUSHR T1
PUSH  1
ADD
POPR  T1
PUSHR T1
PUSHR S2
JB    :loop_y
DRAW
HLT

:draw
PUSHR T0
PUSHR S3
SUB
POPR  T3

PUSHR T1
PUSHR S4
SUB
POPR  T4

PUSHR T3
PUSHR T3
MUL
PUSHR T4
PUSHR T4
MUL
ADD
PUSHR S0
PUSHR S0
MUL
PUSH  1
ADD   
JA    :draw_ret

PUSHR T1
PUSHR S1
MUL
PUSHR T0
ADD      # stk: y * width + x
POPR  T3
PUSH  35 # ascii '#'
POPM  [T3]
JMP   :draw_ret
:draw_ret
RET
