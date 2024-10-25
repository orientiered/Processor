# Quadratic equation solver

call main
hlt


#-----------------------------------------#
main:
    call fillText       ; constant strings
    call readCoeffs     ; reading input
    call solveEquation  ; solving equation
    call printAnswer    ; printing answer
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
readCoeffs:
    in          ; a
    pop [1]
    in          ; b
    pop [2]
    in          ; c
    pop [3]
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
solveEquation:
    push 0
    push [1]
    jne   quadraticCase:
linearCase:
    call solveLinear
    ret
quadraticCase:
    call solveQuadratic
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
solveLinear:
    push [2] ; b
    push 0
    jne   B_NOT_ZERO
    push [3]
    push 0
    jne  C_NOT_ZERO
    push -1
    pop  rax
    ret

C_NOT_ZERO:
    push -1
    push rax
    ret

B_NOT_ZERO:
    push [3] ; c
    push -1
    mul      ; -c

    push [2] ; b

    div      ; -c/b
    pop  [4]
    push 1   ; rax = number of roots
    pop  rax
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
solveQuadratic:
    call calcDiscriminant
    push rax
    push -1
    jne  solveTwoRoots

    push 0   ; no roots because discriminant < 0
    pop  rax
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
solveTwoRoots:
    push [2]
    push -1
    mul
    pop  rbx    ; -b

    push 2
    push [1]
    mul
    pop  rcx    ; 2a

    push rbx    ; -b
    push rax    ; sqrt(D)
    sub         ; -b - sqrt(D)
    push rcx    ; 2a
    div         ; x_1
    pop [4]

    push 0
    push rax
    jne  calcTwoRoots  ;D = 0

    push 1
    pop rax
    ret
calcTwoRoots:
    push rbx    ; -b
    push rax    ; sqrt(D)
    add         ; -b + sqrt(D)
    push rcx    ; 2a
    div         ; x_2
    pop [5]

    push 2
    pop  rax
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
calcDiscriminant:
    push [2]
    push [2]
    mul         ; b^2

    push [1]
    push [3]
    mul
    push -4
    mul         ; -4ac

    add         ; b^2 - 4ac

    pop  rbx    ; storing in register

    push 0
    push rbx
    jbe  posValue
    push -1  ; -1 signals that sqrt cannot be calculated
    pop  rax
    ret

posValue:
    push rbx
    sqrt        ; discriminant
    pop   rax
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
printAnswer:
    push -1
    push rax
    je   infRoots
    push 0
    push rax
    je   noRoots
    push 1
    push rax
    je   oneRoot
    push 2
    push rax
    je   twoRoots

infRoots:
    push 110
    pop rbx
    call printText
    ret
noRoots:
    push 100
    pop  rbx
    call printText
    ret
oneRoot:
    push [4]
    out
    ret
twoRoots:
    push [4]
    out
    push [5]
    out
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
printText:
    push [rbx]
    push 0
    je   printEnd ; 0 is end of string
    chr  [rbx]
    push rbx + 1
    pop  rbx
    jmp  printText
printEnd:
    chr  10 ; '\n'
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


#-----------------------------------------#
fillText:
# 78 111 32 114 111 111 116 115
# N  o   _  r   o   o   t   s
    push 78
    pop [100]
    push 111
    pop [101]
    push 32
    pop [102]
    push 114
    pop [103]
    push 111
    pop [104]
    push 111
    pop [105]
    push 116
    pop [106]
    push 115
    pop [107]
    push 0
    pop [108]
# 73 110 102 32 114 111 111 116 115
# I  n   f   _  r   o   o   t   s
    push 73
    pop [110]
    push 110
    pop [111]
    push 102
    pop [112]
    push 32
    pop [113]
    push 114
    pop [114]
    push 111
    pop [115]
    push 111
    pop [116]
    push 116
    pop [117]
    push 115
    pop [118]
    push 0
    pop [119]
    ret
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
