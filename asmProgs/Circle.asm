# Draws circle in console
# 19.10.24

    in
    pop     rax
    push    rax
    draw

    push    5
    pop     rex

ANIM_START:
    ;dump
    call    fillCircle:
    drawr
    sleep   10
    ;dump
    call    updateRax:
    ;dump
    jmp ANIM_START:
    hlt


# Fill ram with circle pattern (x^2 + y^2 <= rax)
fillCircle:
    push    0
    pop     rbx
    LOOP1_START:
        push    rbx
        push    36
        jbe     LOOP1_END:

        push    0
        pop     rcx
    LOOP2_START:
        push    rcx
        push    96
        jbe     LOOP2_END:

        push    96
        push    rbx
        mul
        push    rcx
        add
        pop     rdx     ; rdx = 96*rbx + rcx

        push    0
        pop     [rdx]   ;Clearing pixel

        push    rax

        push    rbx + -18
        push    5
        mul
        push    4
        div
        push    rbx + -18
        push    5
        mul
        push    4
        div
        mul

        push    rcx + -48
        push    2
        div
        push    rcx + -48
        push    2
        div
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
    # if (rax == 700)
    #   rex = -5
    # if (rax == 1)
    #   rex = 5
    push    300
    push    rax
    jbe     ELSE1:
    push    -5
    pop     rex
ELSE1:
    push    rax
    push    1
    jbe     ELSE2:
    push    5
    pop     rex
ELSE2:
    ret
