#include "darkchess.h"
#include <iostream>
#include <string>
#include <utility>

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

std::string getDarkChessActionString(int action_id)
{
    return std::to_string((31 - action_id) % 4) + std::to_string((31 - action_id) / 4);
}

DarkChessEnv& DarkChessEnv::operator=(const DarkChessEnv& env)
{
    turn_ = env.turn_;
    random_.seed(seed_ = env.seed_);
    winner_ = env.winner_;
    actions_ = env.actions_;
    board_current_chess_ = env.board_current_chess_;
    chess_count_ = env.chess_count_;
    flipped_chess_count_ = env.flipped_chess_count_;
    continuous_move_count_ = env.continuous_move_count_;
    return *this;
}

void DarkChessEnv::reset(int seed)
{
    turn_ = Player::kPlayerNone;
    random_.seed(seed_ = seed);
    winner_ = Player::kPlayerNone;
    actions_.clear();
    board_current_chess_.fill('X');
    chess_count_.fill(2);
    chess_count_[0] = 1;   // 帥
    chess_count_[6] = 5;   // 兵
    chess_count_[7] = 1;   // 將
    chess_count_[13] = 5;  // 卒
    chess_count_[14] = 0;  // 空格
    chess_count_[15] = 32; // 暗子
    flipped_chess_count_.fill(2);
    flipped_chess_count_[0] = 1;  // 帥
    flipped_chess_count_[6] = 5;  // 兵
    flipped_chess_count_[7] = 1;  // 將
    flipped_chess_count_[13] = 5; // 卒
    continuous_move_count_ = 0;
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

    if (src == dst) {                      // 翻棋
        int chess_id = getRandomChessId(); // 0 ~ 13

        if (action.getPlayer() == Player::kPlayerNone) {
            if (chess_id <= 6) {
                turn_ = Player::kPlayer2; // 自己是紅棋，下一步換黑棋
            } else if (chess_id >= 7) {
                turn_ = Player::kPlayer1; // 自己是黑棋，下一步換紅棋
            }
        }

        chess_count_[15]--;
        flipped_chess_count_[chess_id]--;
        board_current_chess_[src] = kDarkChessChessName[chess_id];
        continuous_move_count_ = 0;
    } else {
        if (board_current_chess_[dst] != '-') { // 吃子
            // 取得 dst 棋子的 id
            int chess_id = std::distance(kDarkChessChessName.begin(), std::find(kDarkChessChessName.begin(), kDarkChessChessName.end(), board_current_chess_[dst]));
            chess_count_[chess_id]--;

            // 場上只剩一顆棋時當前玩家獲勝
            if (std::accumulate(chess_count_.begin(), chess_count_.end() - 2, 0) == 1) {
                winner_ = player;
            }

            continuous_move_count_ = 0;
        } else {
            continuous_move_count_++;
        }
        board_current_chess_[dst] = board_current_chess_[src];
        board_current_chess_[src] = '-';
    }

    if (getLegalActions().size() == 0) {
        winner_ = getNextPlayer(turn_, 2); // 對手無步可走則己方獲勝
        std::cout << toString();
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
        if (action.getPlayer() == Player::kPlayerNone) { // 雙方顏色未知時只能翻棋
            return false;
        } else if (src == 'X' || src == '-' || dst == 'X') { // 起點/終點不能是暗子且起點不能是空棋
            return false;
        } else if (dst == '-') {
            return true;                                     // 終點是空格可以直接移動
        } else if (action.getPlayer() == Player::kPlayer1) { // 紅棋
            // 起點需為紅棋（大寫字母），終點需為黑棋（小寫字母）
            if (src - 'A' >= 32 && dst - 'a' < 0) {
                return false;
            } else if (src == 'K' && dst == 'p') {
                return false; // 帥不能吃卒
            } else if (src == 'P' && dst == 'k') {
                return true; // 兵可以吃將
            } else if (src == 'C') {
                return checkCannonCanEat(move); // 炮要特殊判定
            } else if (!checkNeighboring(move.first, move.second)) {
                return false;
            } else if (kDarkChessValue.at(src) < kDarkChessValue.at(dst)) {
                return false;
            }
        } else if (action.getPlayer() == Player::kPlayer2) { // 黑棋
            // 起點需為黑棋，終點需為紅棋
            if (src - 'a' < 32 && dst - 'A' >= 32) {
                return false;
            } else if (src == 'k' && dst == 'P') {
                return false; // 將不能吃兵
            } else if (src == 'p' && dst == 'K') {
                return true; // 卒可以吃帥
            } else if (src == 'c') {
                return checkCannonCanEat(move); // 炮要特殊判定
            } else if (!checkNeighboring(move.first, move.second)) {
                return false;
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
    if (winner_ != Player::kPlayerNone) { return true; }

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

float DarkChessEnv::getEvalScore(bool is_resign) const
{
    // 若認負則另一位玩家獲勝
    Player eval = (is_resign ? getNextPlayer(turn_, kDarkChessNumPlayer) : winner_);
    switch (eval) {
        case Player::kPlayer1: return 1.0f;
        case Player::kPlayer2: return -1.0f;
        default: return 0.0f; // 平局
    }
}

std::vector<float> DarkChessEnv::getFeatures(utils::Rotation rotation) const
{
    // 16 features:
    // 1 ~ 14: 各個棋子的位置
    // 15: 暗子
    // 16: 空格
    std::vector<float> vFeatures;
    for (int channel = 0; channel < 18; ++channel) {
        for (int pos = 0; pos < kDarkChessBoardHeight * kDarkChessBoardWidth; ++pos) {
            if (channel < 14) {
                vFeatures.push_back(board_current_chess_[pos] == kDarkChessChessName[channel] ? 1.0f : 0.0f);
            } else if (channel == 14) {
                vFeatures.push_back(board_current_chess_[pos] == 'X' ? 1.0f : 0.0f);
            } else if (channel == 15) {
                vFeatures.push_back(board_current_chess_[pos] == '-' ? 1.0f : 0.0f);
            }
        }
    }
    return vFeatures;
}

std::vector<float> DarkChessEnv::getActionFeatures(const DarkChessAction& action, utils::Rotation rotation) const
{
    // TODO 可能需要改進
    std::vector<float> action_features(kDarkChessActionSize, 0.0f);
    action_features[action.getActionID()] = 1.0f;
    return action_features;
}

std::string DarkChessEnv::toString() const
{
    std::string res("");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            res += board_current_chess_[i * 4 + (3 - j)];
            res += " ";
        }
        res += "\n";
    }
    return res;
}

std::vector<float> DarkChessEnvLoader::getActionFeatures(const int pos, utils::Rotation rotation) const
{
    // TODO what is action features?
    const DarkChessAction& action = action_pairs_[pos].first;
    std::vector<float> action_features(getPolicySize(), 0.0f);
    int action_id = ((pos < static_cast<int>(action_pairs_.size())) ? getRotateAction(action.getActionID(), rotation) : utils::Random::randInt() % action_features.size());
    action_features[action_id] = 1.0f;
    return action_features;
}

} // namespace minizero::env::darkchess
