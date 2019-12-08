State at case A

B = Free block; A = Address of free block; S = Free size
Allocation available bytes:  120 bytes

B  0: A: 0x00E7B160, S:    0, next B: 0x00E7B520; Start block
B  1: A: 0x00E7B520, S:  120, next B: 0x00E7B598
B  2: A: 0x00E7B598, S:    0, next B: 0x00000000; End of region

State at case B

B = Free block; A = Address of free block; S = Free size
Allocation available bytes:   24 bytes

B  0: A: 0x00E7B160, S:    0, next B: 0x00E7B580; Start block
B  1: A: 0x00E7B580, S:   24, next B: 0x00E7B598
B  2: A: 0x00E7B598, S:    0, next B: 0x00000000; End of region

State after first realloc

B = Free block; A = Address of free block; S = Free size
Allocation available bytes:   24 bytes

B  0: A: 0x00E7B160, S:    0, next B: 0x00E7B580; Start block
B  1: A: 0x00E7B580, S:   24, next B: 0x00E7B598
B  2: A: 0x00E7B598, S:    0, next B: 0x00000000; End of region

State at case C

B = Free block; A = Address of free block; S = Free size
Allocation available bytes:   32 bytes

B  0: A: 0x00E7B160, S:    0, next B: 0x00E7B530; Start block
B  1: A: 0x00E7B530, S:    8, next B: 0x00E7B580
B  2: A: 0x00E7B580, S:   24, next B: 0x00E7B598
B  3: A: 0x00E7B598, S:    0, next B: 0x00000000; End of region