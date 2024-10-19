# Draws circle in console
# 19.10.24

    in
    pop     rex
    draw

    push    0
    pop     rax

    call    fillCircle:
    draw

    hlt


fillCircle:
    push    0
    pop     rbx
    LOOP1_START:
        push    rbx
        push    11
        jbe     LOOP1_END:

        push    0
        pop     rcx
    LOOP2_START:
        push    rcx
        push    11
        jbe     LOOP2_END:

        push    11
        push    rbx
        mul
        push    rcx
        add
        pop     rdx     ; rdx = 11*rbx + rcx

        push    rex

        push    rbx + -5
        push    rbx + -5
        mul
        push    rcx + -5
        push    rcx + -5
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
