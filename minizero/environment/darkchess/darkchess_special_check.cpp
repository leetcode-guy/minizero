#include "darkchess.h"

namespace minizero::env::darkchess {

bool DarkChessEnv::checkCannonCanEat(std::pair<int, int> move) const
{
    int chess_cnt = 0; // 兩顆棋之間有多少棋

    // 可移動到相鄰空格
    if (checkNeighboring(move.first, move.second) && board_current_chess_[move.second] == '-') return true;

    if (move.first < move.second) std::swap(move.first, move.second);
    if (move.first / 4 == move.second / 4) { // 兩顆棋在同一個 row
        for (int i = move.first + 1; i < move.second; i++) {
            if (board_current_chess_[i] != '-') chess_cnt++;
        }
    } else { // 兩顆棋在同一個 column
        for (int i = move.first + 4; i < move.second; i += 4) {
            if (board_current_chess_[i] != '-') chess_cnt++;
        }
    }

    return (chess_cnt == 1);
}

bool DarkChessEnv::checkNeighboring(int src, int dst) const {
    int src_x = src % 4, src_y = src / 4;
    int dst_x = dst % 4, dst_y = dst / 4;

    if (src_x == dst_x && abs(src_y - dst_y) == 1) {
        return true;
    } else if (src_y == dst_y && abs(src_x - dst_x) == 1) {
        return true;
    }
    return false;
}

int DarkChessEnv::getRandomChessId() {
    int rand_num = std::uniform_int_distribution<int>{0, chess_count_[15] - 1}(random_);
    int rand_chess_id = 0;
    for (int i = 0; i <= 13; i++) {
        rand_num -= flipped_chess_count_[i];
        if (rand_num < 0) {
            rand_chess_id = i;
            break;
        }
    }
    return rand_chess_id;
}

} // namespace minizero::env::darkchess
