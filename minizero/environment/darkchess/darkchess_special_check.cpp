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

int DarkChessEnv::getRandomChessId() {
    int rand_num = std::uniform_int_distribution<int>{0, chess_count_[15] - 1}(random_);
    int rand_chess_id = 0;
    for (int i = 1; i <= 14; i++) {
        rand_num -= chess_count_[i];
        if (rand_num < 0) {
            rand_chess_id = i;
            break;
        }
    }
    return rand_chess_id;
}

} // namespace minizero::env::darkchess