#include "darkchess.h"

namespace minizero::env::darkchess {

bool DarkChessEnv::checkCannonCanEat(std::pair<int, int> move) const
{
    int chess_cnt = 0; // 兩顆棋之間有多少棋

    if (move.first < move.second) std::swap(move.first, move.second);
    if (move.first / 4 == move.second / 4) { // 兩顆棋在同一個 row
        for (int i = move.first; i < move.second; i++) {
            if (board_current_chess_[i] != ' ') chess_cnt++;
        }
    } else { // 兩顆棋在同一個 column
        for (int i = move.first; i < move.second; i += 4) {
            if (board_current_chess_[i] != ' ') chess_cnt++;
        }
    }

    return (chess_cnt == 1);
}

} // namespace minizero::env::darkchess