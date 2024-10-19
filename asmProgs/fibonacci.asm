;--Fibonacci sequence--

in                  ; Count of numbers to print
pop rdx

push 0 pop rax      ; first number
push 1 pop rbx      ; second number
push 0 pop rcx      ; Cnt of printed numbers

push    rax         ; Printing fib[i]
out
push    rcx +1       ; i++
pop     rcx

push    rbx         ; Stack: fib[i-1]
push    rbx
push    rax         ; fib[i] = fib[i-2] + fib[i-1]
add                 ; Stack: fib[i-1] fib[i]
pop     rbx
pop     rax

push    rcx         ; if n > i continue
push    rdx
ja      15

hlt

