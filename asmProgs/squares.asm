in
pop     rbx ; Hello, there's comment example
push    0
pop     rax

START:

push    rax push    rax
mul
out

push    rax 1
pop     rax

push    rax
push    rbx

ja     START:


hlt
