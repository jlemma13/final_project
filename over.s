.global over
over:
    cmp r0, #0
    beq .end
    cmp r1, #0
    beq .end
    mov r2, #0
    b .none
.end:
<<<<<<< HEAD
    @bl game_over
    bl reset
=======
    mov r2, #1
>>>>>>> 4e3d43086952405864d55daa7d1a34731febf49c
.none:
    mov r0, r2
    mov pc, lr
