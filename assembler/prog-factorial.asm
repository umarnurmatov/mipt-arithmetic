PUSH  3
POPR  S0
CALL  :1
PUSHR S1
OUT
HLT

:1       # factorial

PUSHR S0
PUSH  1
JE    :2 # n == 1: return 1
PUSHR S0 # s0: n, s: ret, n
PUSHR S0 # s0: n, s: ret, n, n
PUSH  1
SUB
POPR  S0 # s0: n-1, s: ret, n
CALL  :1 
PUSHR S1 # s1: (n-1)!, s: ret, n, (n-1)!
MUL
POPR  S1 # s1: n!
RET
:2
PUSH  1
POPR  S1
RET
