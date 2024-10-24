# Calculate factorial of given number

BEGIN:
    call ReadNumber

    push    rax
    push    -1
    jae     END:


    call Factorial
    push    rax
    out
    jmp     BEGIN

#----------
ReadNumber:
    in
    pop rax
    ret
#----------

#----------
Factorial:
    push    rax
    push    1
    jbe     Factorial_main:      ; jump when rax > 1
    push    1                    ; factorial(n) = 1, n = 0 or 1
    pop     rax
    ret

Factorial_main:
    push    rax

    push    rax + -1
    pop     rax
    call    Factorial:

    push    rax
    mul
    pop     rax
    ret
#----------

END:
    hlt
