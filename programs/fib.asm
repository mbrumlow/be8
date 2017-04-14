ldi 0x1
sta [y]
ldi 0x0
out 
add [y]
sta [z]
lda [y]
sta [x]
lda [z]
sta [y]
lda [x]
jc 0x0
jmp 0x3
db x 0
db y 0
db z 0
