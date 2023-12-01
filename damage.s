.global damage
damage:
    cmp r0, #0
    beq .end
    sub r1, r0, #1
.end:
    mov r0, r1
    mov pc, lr
