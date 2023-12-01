.global over
over:
    cmp r0, #0
    beq .end
    cmp r1, #0
    beq .end
    mov r2, #0
    b .none
.end:
    mov r2, #1
.none:
    mov r0, r2
    mov pc, lr
