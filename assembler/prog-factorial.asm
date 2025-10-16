PUSH  3
POPR  S0
CALL  :factorial
PUSHR S1
OUT
HLT

:factorial
PUSHR S0
PUSH  1
JE    :n_is_one
PUSHR S0 # s0: n, s: ret, n
PUSHR S0 # s0: n, s: ret, n, n
PUSH  1
SUB
POPR  S0 # s0: n-1, s: ret, n
CALL  :factorial 
PUSHR S1 # s1: (n-1)!, s: ret, n, (n-1)!
MUL
POPR  S1 # s1: n!
RET
:n_is_one
PUSH  1
POPR  S1
RET
