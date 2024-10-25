; Create picture of Mandelbrot set

call main
hlt

; rbx -> i
; rcx -> j
; rdx -> x
; rex -> y

main:
    call fillMandelbrot
    draw
    ret

fillMandelbrot:
    push 0
    pop  rbx
    push 0
    pop  rcx

I_LOOP:
    push rbx
    push 36
    jae  I_LOOP_END

    push 0
    pop  rcx
    J_LOOP:
        push rcx
        push 96
        jae  J_LOOP_END

        call convertCoords
        call checkPixel    ; rax is color

        push rbx
        push 96
        mul
        push rcx
        add
        pop  rdx ; rdx = i*96 + rcx

        push rax
        pop  [rdx]

        push rcx + 1
        pop  rcx
    jmp  J_LOOP

J_LOOP_END:
    push rbx + 1
    pop  rbx
    jmp  I_LOOP

I_LOOP_END:
    ret


convertCoords:
    ;push rbx
    ;out
    ; y = (j / 36) * 2 - 1 = j / 18 - 1
    push rbx
    push 13.5
    div
    push 1.333
    sub
    pop rex

    ;push rex
    ;out
    ;push rcx
    ;out
    ; x = (i / 96) * 4 - 2  = i / 24 - 2
    push rcx
    push 36
    div
    push 1.667
    sub
    pop rdx

    ;push rdx
    ;out
    ret

checkPixel:
    ; maxIter = 50
    ; inf     = 4
    ; z_n+1 = z_n^2 + c
    ; (c + di) = (a + bi)^2 + x + iy = (a^2 - b^2 + x) + (2ab + y)i
    push 0
    pop  rax
    push 0
    pop  [4000]
    push 0
    pop  [4001]
CYCLE_START:
    push rax
    push 100
    jae  CYCLE_END
    push rax
    pop  [3999]
    call checkModule
    push rax
    push 0
    je   CYCLE_END
    push [3999]
    pop  rax
    push [4000]
    push [4000]
    mul
    push [4001]
    push [4001]
    mul
    sub
    push rdx
    add
    pop  [4003]

    push [4000]
    push [4001]
    mul
    push 2
    mul
    push rex
    add
    pop  [4001]

    push [4003]
    pop  [4000]

    push rax + 1
    pop  rax
    jmp  CYCLE_START


CYCLE_END:
    push [4000]
    out
    push [4001]
    out

    call checkModule
    ret

checkModule:
    push [4000]
    push [4000]
    mul
    push [4001]
    push [4001]
    mul
    add

    push 4
    ja   WHITE:
    push 1
    pop  rax
    ret
WHITE:
    push 0
    pop rax
    ret
