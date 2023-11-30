.global over
over:
    cmp r0, #0
    beq .end
    cmp r1, #0
    beq .end
    b .none
.end:
    bl game_over
    bl reset
.none:
    mov pc, lr
