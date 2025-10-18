IN
POPR  A0
CALL  :factorial
PUSHR A1
OUT
HLT

:factorial
PUSHR A0
PUSH  1
JE    :n_is_one
PUSHR A0 # a0: n, s: ret, n
PUSHR A0 # a0: n, s: ret, n, n
PUSH  1
SUB
POPR  A0 # a0: n-1, s: ret, n
CALL  :factorial 
PUSHR A1 # a1: (n-1)!, s: ret, n, (n-1)!
MUL
POPR  A1 # a1: n!
RET
:n_is_one
PUSH  1
POPR  A1
RET
