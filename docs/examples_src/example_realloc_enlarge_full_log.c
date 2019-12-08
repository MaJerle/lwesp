Manager is ready!
|-------|----------|--------|------|------------------|----------------|
| Block | Address  | IsFree | Size | MaxUserAllocSize | Meta           |
|-------|----------|--------|------|------------------|----------------|
|     0 | 00179154 |      0 |    0 |                0 |Start block     |
|     1 | 0034A508 |      1 |  120 |              112 |Free block      |
|     2 | 0034A580 |      0 |    0 |                0 |End of region   |
|-------|----------|--------|------|------------------|----------------|


Allocating 4 pointers and freeing first and third..
|-------|----------|--------|------|------------------|----------------|
| Block | Address  | IsFree | Size | MaxUserAllocSize | Meta           |
|-------|----------|--------|------|------------------|----------------|
|     0 | 00179154 |      0 |    0 |                0 |Start block     |
|     1 | 0034A508 |      1 |   16 |                8 |Free block      |
|     2 | 0034A518 |      0 |   12 |                0 |Allocated block |
|     3 | 0034A524 |      1 |   12 |                4 |Free block      |
|     4 | 0034A530 |      0 |   24 |                0 |Allocated block |
|     5 | 0034A548 |      1 |   56 |               48 |Free block      |
|     6 | 0034A580 |      0 |    0 |                0 |End of region   |
|-------|----------|--------|------|------------------|----------------|
Debug above is effectively state 3
 -- > Current state saved!

------------------------------------------------------------------------
 -- > State restored to last saved!
State 3a
|-------|----------|--------|------|------------------|----------------|
| Block | Address  | IsFree | Size | MaxUserAllocSize | Meta           |
|-------|----------|--------|------|------------------|----------------|
|     0 | 00179154 |      0 |    0 |                0 |Start block     |
|     1 | 0034A508 |      1 |   16 |                8 |Free block      |
|     2 | 0034A518 |      0 |   16 |                0 |Allocated block |
|     3 | 0034A528 |      1 |    8 |                0 |Free block      |
|     4 | 0034A530 |      0 |   24 |                0 |Allocated block |
|     5 | 0034A548 |      1 |   56 |               48 |Free block      |
|     6 | 0034A580 |      0 |    0 |                0 |End of region   |
|-------|----------|--------|------|------------------|----------------|
Assert passed with condition (rptr1 == ptr2)

------------------------------------------------------------------------
 -- > State restored to last saved!
State 3b
|-------|----------|--------|------|------------------|----------------|
| Block | Address  | IsFree | Size | MaxUserAllocSize | Meta           |
|-------|----------|--------|------|------------------|----------------|
|     0 | 00179154 |      0 |    0 |                0 |Start block     |
|     1 | 0034A508 |      0 |   28 |                0 |Allocated block |
|     2 | 0034A524 |      1 |   12 |                4 |Free block      |
|     3 | 0034A530 |      0 |   24 |                0 |Allocated block |
|     4 | 0034A548 |      1 |   56 |               48 |Free block      |
|     5 | 0034A580 |      0 |    0 |                0 |End of region   |
|-------|----------|--------|------|------------------|----------------|
Assert failed with condition (rptr2 == ptr2)

------------------------------------------------------------------------
 -- > State restored to last saved!
State 3c
|-------|----------|--------|------|------------------|----------------|
| Block | Address  | IsFree | Size | MaxUserAllocSize | Meta           |
|-------|----------|--------|------|------------------|----------------|
|     0 | 00179154 |      0 |    0 |                0 |Start block     |
|     1 | 0034A508 |      0 |   32 |                0 |Allocated block |
|     2 | 0034A528 |      1 |    8 |                0 |Free block      |
|     3 | 0034A530 |      0 |   24 |                0 |Allocated block |
|     4 | 0034A548 |      1 |   56 |               48 |Free block      |
|     5 | 0034A580 |      0 |    0 |                0 |End of region   |
|-------|----------|--------|------|------------------|----------------|
Assert passed with condition (rptr3 == ptr1)

------------------------------------------------------------------------
 -- > State restored to last saved!
State 3d
|-------|----------|--------|------|------------------|----------------|
| Block | Address  | IsFree | Size | MaxUserAllocSize | Meta           |
|-------|----------|--------|------|------------------|----------------|
|     0 | 00179154 |      0 |    0 |                0 |Start block     |
|     1 | 0034A508 |      1 |   40 |               32 |Free block      |
|     2 | 0034A530 |      0 |   24 |                0 |Allocated block |
|     3 | 0034A548 |      0 |   44 |                0 |Allocated block |
|     4 | 0034A574 |      1 |   12 |                4 |Free block      |
|     5 | 0034A580 |      0 |    0 |                0 |End of region   |
|-------|----------|--------|------|------------------|----------------|
Assert passed with condition (rptr4 != ptr1 && rptr4 != ptr2 && rptr4 != ptr3 && rptr4 != ptr4)