; Test of jumpLuck operator


push    1

BEGIN:
jl      L2:

L1:
push    3
mul
jl      END:
jmp     BEGIN:
L2:
push    1
add
jl      L1:
jmp     BEGIN:

END:
out

hlt
