board = [
    [28, 29, 30, 31],
    [24, 25, 26, 27],
    [20, 21, 22, 23],
    [16, 17, 18, 19],
    [12, 13, 14, 15],
    [8, 9, 10, 11],
    [4, 5, 6, 7],
    [0, 1, 2, 3],
]

cnt = 0
for i in range(8):
    for j in range(4):
        print(f"{{{board[i][j]}, {board[i][j]}}}, ", end="")
        for k in range(8):
            if k != i:
                print(f"{{{board[i][j]}, {board[k][j]}}}, ", end="")
        for k in range(4):
            if k != j:
                print(f"{{{board[i][j]}, {board[i][k]}}}, ", end="")
        print()
