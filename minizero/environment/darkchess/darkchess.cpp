#include "darkchess.h"

namespace minizero::env::darkchess {

char playerToChar(Player p)
{
    switch (p) {
        case Player::kPlayerNone: return 'N';
        case Player::kPlayer1: return 'R'; // red
        case Player::kPlayer2: return 'B'; // black
        default: return '?';
    }
}

Player charToPlayer(char c)
{
    switch (c) {
        case 'N': return Player::kPlayerNone;
        case 'R':
        case 'r': return Player::kPlayer1;
        case 'B':
        case 'b': return Player::kPlayer2;
        default: return Player::kPlayerSize;
    }
}

bool DarkChessEnv::act(const DarkChessAction& action)
{
    if (!isLegalAction(action)) { return false; }

    const std::pair<int, int> move = kDarkChessActionMap[action.getActionID()];
    const Player player = action.getPlayer();
    const int src = move.first;
    const int dst = move.second;

    // environment status
    turn_ = action.nextPlayer();
    actions_.push_back(action);

    if (src == dst) { // 翻棋
        int chess_id = getRandomChessId();
        chess_count_[15]--;
        board_current_chess_[src] = kDarkChessChessName[chess_id - 1];
    } else {
        if (board_current_chess_[dst] != ' ') { // 吃子
            // 取得 dst 棋子的 id
            int chess_id = std::distance(kDarkChessChessName.begin(), std::find(kDarkChessChessName.begin(), kDarkChessChessName.end(), board_current_chess_[dst]));
            chess_count_[chess_id + 1]--;
        }
        board_current_chess_[dst] = board_current_chess_[src];
        board_current_chess_[src] = ' ';
    }

    return true;
}

std::vector<DarkChessAction> DarkChessEnv::getLegalActions() const
{
    std::vector<DarkChessAction> actions;
    for (int pos = 0; pos < kDarkChessActionSize; ++pos) {
        DarkChessAction action(pos, turn_);
        if (!isLegalAction(action)) { continue; }
        actions.push_back(action);
    }
    return actions;
}

bool DarkChessEnv::isLegalAction(const DarkChessAction& action) const
{
    assert(action.getActionID() >= 0 && action.getActionID() < kDarkChessActionSize);

    auto move = kDarkChessActionMap[action.getActionID()];
    char src = board_current_chess_[move.first];  // 起點
    char dst = board_current_chess_[move.second]; // 終點

    if (move.first != move.second) {                     // 移動或吃子
        if (action.getPlayer() != Player::kPlayerNone) { // 雙方顏色未知時只能翻棋
            return false;
        } else if (src == 'X' || src == ' ' || dst == 'X') { // 起點/終點不能是暗子且起點不能是空棋
            return false;
        } else if (action.getPlayer() != Player::kPlayer1) { // 紅棋
            // 起點需為紅棋（大寫字母），終點需為黑棋（小寫字母）
            if (src - 'A' >= 32 && dst - 'a' < 0) {
                return false;
            } else if (src == 'K' && dst == 'p') {
                return false; // 帥不能吃卒
            } else if (src == 'P' && dst == 'k') {
                return true; // 兵可以吃將
            } else if (src == 'C') {
                return checkCannonCanEat(move); // 炮要特殊判定
            } else if (kDarkChessValue.at(src) < kDarkChessValue.at(dst)) {
                return false;
            }
        } else if (action.getPlayer() != Player::kPlayer2) { // 黑棋
            // 起點需為黑棋，終點需為紅棋
            if (src - 'a' < 32 && dst - 'A' >= 32) {
                return false;
            } else if (src == 'k' && dst == 'P') {
                return false; // 將不能吃兵
            } else if (src == 'p' && dst == 'K') {
                return true; // 卒可以吃帥
            } else if (src == 'c') {
                return checkCannonCanEat(move); // 炮要特殊判定
            } else if (kDarkChessValue.at(src) < kDarkChessValue.at(dst)) {
                return false;
            }
        }
    } else {
        if (src != 'X') { return false; } // 要翻開的那格是能是暗子
    }
    return true; // 符合以上條件即為合法的 action
}

bool DarkChessEnv::isTerminal() const
{
    // 場上只剩一顆棋時結束
    if (std::accumulate(chess_count_.begin() + 1, chess_count_.end() - 1, 0) == 1) { return true; }

    // 超過一定步數無吃翻
    if (continuous_move_count_ >= config::env_darkchess_no_eat_flip) { return true; }

    // 長捉（4 步一循環）
    if (continuous_move_count_ >= config::env_darkchess_long_catch * 4) {
        int act_history_size = actions_.size();
        // 循環的 4 步
        int act1 = actions_[act_history_size - 1].getActionID();
        int act2 = actions_[act_history_size - 2].getActionID();
        int act3 = actions_[act_history_size - 3].getActionID();
        int act4 = actions_[act_history_size - 4].getActionID();

        // 若沒有連續循環指定次數則長捉不成立
        for (int i = 1; i < config::env_darkchess_long_catch; i++) {
            if (actions_[act_history_size - i * 4 - 1].getActionID() != act1 &&
                actions_[act_history_size - i * 4 - 2].getActionID() != act2 &&
                actions_[act_history_size - i * 4 - 3].getActionID() != act3 &&
                actions_[act_history_size - i * 4 - 4].getActionID() != act4) {
                return false;
            }
        }
        return true;
    }
    return false;
}

} // namespace minizero::env::darkchess