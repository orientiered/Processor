START:
    push rax
    push 256
    jb   END:
    push rax
    out
    chr  rax
    chr 10

    push rax + 1
    pop  rax
    jmp  START:
END:
hlt
