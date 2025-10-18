IN
POPR  A0
CALL  :fibonacci
PUSHR A1
OUT
HLT

:fibonacci
PUSHR S0 # saving reg
PUSHR A0
PUSH  1
JBE   :n_less_than_one

PUSHR A0
POPR  S0
PUSHR S0
PUSH  1
SUB
POPR  A0
CALL  :fibonacci
PUSHR A1
PUSHR S0
PUSH  2
SUB
POPR  A0
CALL  :fibonacci
PUSHR A1
ADD
POPR  A1
POPR  S0 # restoring reg value
RET

:n_less_than_one
PUSHR A0
POPR  A1
POPR  S0 # restoring reg value
RET
