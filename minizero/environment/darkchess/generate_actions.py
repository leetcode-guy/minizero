board = [
    ["a8", "b8", "c8", "d8"],
    ["a7", "b7", "c7", "d7"],
    ["a6", "b6", "c6", "d6"],
    ["a5", "b5", "c5", "d5"],
    ["a4", "b4", "c4", "d4"],
    ["a3", "b3", "c3", "d3"],
    ["a2", "b2", "c2", "d2"],
    ["a1", "b1", "c1", "d1"],
]

cnt = 0
for i in range(8):
    for j in range(4):
        print(f"{board[i][j]} {board[i][j]}")
        for k in range(8):
            if k != i:
                print(f"{board[i][j]} {board[k][j]}")
        for k in range(4):
            if k != j:
                print(f"{board[i][j]} {board[i][k]}")


