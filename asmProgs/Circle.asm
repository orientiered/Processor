# Draws circle in console
# 19.10.24

    in
    pop     rax
    push    rax
    draw

    push    1
    pop     rex

ANIM_START:
    call    fillCircle:
    drawr
    sleep   16
    call    updateRax:
    jmp ANIM_START:

    hlt


# Fill ram with circle pattern (x^2 + y^2 <= rax)
fillCircle:
    push    0
    pop     rbx
    LOOP1_START:
        push    rbx
        push    21
        jbe     LOOP1_END:

        push    0
        pop     rcx
    LOOP2_START:
        push    rcx
        push    21
        jbe     LOOP2_END:

        push    21
        push    rbx
        mul
        push    rcx
        add
        pop     rdx     ; rdx = 21*rbx + rcx

        push    0
        pop     [rdx]   ;Clearing pixel

        push    rax

        push    rbx + -10
        push    rbx + -10
        mul
        push    rcx + -10
        push    rcx + -10
        mul
        add

        ja      SKIP_PIXEL:
        push    1
        pop     [rdx]
    SKIP_PIXEL:


        push    rcx + 1
        pop     rcx
        jmp     LOOP2_START:
    LOOP2_END:

        push    rbx + 1
        pop     rbx
        jmp     LOOP1_START:
    LOOP1_END:

    ret

updateRax:
    push    rax
    push    rex
    add
    pop     rax
    # if (rax == 100)
    #   rex = -1
    # if (rax == 1)
    #   rex = 1
    push    rax
    push    100
    jne     ELSE1:
    push    -1
    pop     rex
ELSE1:
    push    rax
    push    1
    jne     ELSE2:
    push    1
    pop     rex
ELSE2:
    ret
