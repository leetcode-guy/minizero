#ifndef MINIZERO_ENVIRONMENT_DARKCHESS_DARKCHESS_H_
#define MINIZERO_ENVIRONMENT_DARKCHESS_DARKCHESS_H_

#include "base_env.h"
#include "configuration.h"
#include "darkchess_unit.h"
#include "random.h"
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace minizero::env::darkchess {

char playerToChar(Player p);
Player charToPlayer(char c);
std::string getDarkChessActionString(int action_id);

class DarkChessAction : public BaseAction {
public:
    DarkChessAction() : BaseAction() {}
    DarkChessAction(int action_id, Player player) : BaseAction(action_id, player) {}
    DarkChessAction(const std::vector<std::string>& action_string_args) {}

    inline Player nextPlayer() const override { return getNextPlayer(player_, kDarkChessNumPlayer); }
    inline std::string toConsoleString() const override { return getDarkChessActionString(action_id_); }
};

class DarkChessEnv : public BaseEnv<DarkChessAction> {
public:
    DarkChessEnv() { reset(); }
    DarkChessEnv(const DarkChessEnv& env) { *this = env; }
    DarkChessEnv& operator=(const DarkChessEnv& env);

    void reset() override { reset(utils::Random::randInt()); }
    void reset(int seed);
    bool act(const DarkChessAction& action) override;
    bool act(const std::vector<std::string>& action_string_args) override { return act(DarkChessAction(action_string_args)); };
    std::vector<DarkChessAction> getLegalActions() const override;
    bool isLegalAction(const DarkChessAction& action) const override;
    bool isTerminal() const override;
    float getReward() const override { return 0.0f; }
    float getEvalScore(bool is_resign = false) const override;
    std::vector<float> getFeatures(utils::Rotation rotation = utils::Rotation::kRotationNone) const override;
    std::vector<float> getActionFeatures(const DarkChessAction& action, utils::Rotation rotation = utils::Rotation::kRotationNone) const override;
    inline int getNumInputChannels() const override { return 16; }
    inline int getNumActionFeatureChannels() const override { return 1; }
    inline int getInputChannelHeight() const override { return kDarkChessBoardHeight; }
    inline int getInputChannelWidth() const override { return kDarkChessBoardWidth; }
    inline int getHiddenChannelHeight() const override { return kDarkChessBoardHeight; }
    inline int getHiddenChannelWidth() const override { return kDarkChessBoardWidth; }
    inline int getDiscreteValueSize() const override { return 1; }
    inline int getPolicySize() const override { return kDarkChessActionSize; }
    std::string toString() const override;
    inline std::string name() const override { return kDarkChessName; }
    inline int getNumPlayer() const override { return kDarkChessNumPlayer; }
    inline int getRotatePosition(int position, utils::Rotation rotation) const override { return position; }
    inline int getRotateAction(int action_id, utils::Rotation rotation) const override { return action_id; }
    inline int getSeed() const { return seed_; }

    // special check
    bool checkCannonCanEat(std::pair<int, int> move) const;
    int getRandomChessId();

protected:
    std::mt19937 random_;
    int seed_;
    Player winner_;

    // 使用 Bitboard 儲存每種棋子在棋盤上的位置
    // 0: 空格 (-)，1 ~ 7: 帥 (K)、仕 (G)、相 (M)、俥 (R)、傌 (N)、炮 (C)、兵 (P)
    // 15: 未翻開 (X)，8 ~ 14: 將 (k)、士 (g)、象 (m)、車 (r)、馬 (n)、包 (c)、卒 (p)
    // 棋盤格式（數字為方便辨識，輸出需使用如 a2 的座標）：
    // 8 |  3  2  1  0
    // 7 |  7  6  5  4
    // 6 | 11 10  9  8
    // 5 | 15 14 13 12
    // 4 | 19 18 17 16
    // 3 | 23 22 21 20
    // 2 | 27 26 25 24
    // 1 | 31 30 29 28
    //    ‾‾‾‾‾‾‾‾‾‾‾‾
    //      a  b  c  d
    // 位置以 bitwise 儲存，least significant bit 為 1
    // Ex: board[2] = 3 (00000000000000000000000000000101)_2 表示仕在 a1 與 c1
    std::array<DarkChessBitboard, 16> board_current_position_;
    // 當前每個位置上的棋子
    std::array<char, 32> board_current_chess_;
    // 有棋子的位置
    DarkChessBitboard occupied_position_;
    // 每種棋子剩餘的數量，順序與上面說明相同
    std::array<int, 16> chess_count_;

    // GamePair 中分成 black, white，但暗棋的顏色是紅跟黑
    // 因此在此處定義 GamePair 的 black 為紅色，white 為黑色

    // 雙方棋子在棋盤上的位置
    GamePair<DarkChessBitboard> chess_position_;
    // 雙方可吃子的走步
    GamePair<std::vector<std::pair<int, int>>> eatable_position_;
    // 雙方可移動的走步
    GamePair<std::vector<std::pair<int, int>>> movable_position_;
    // 每種棋子剩餘的數量
    std::array<int, 14> piece_count_;
    // 當前連續移動的次數，吃子或翻棋會歸零
    int continuous_move_count_;
};

class DarkChessEnvLoader : public BaseEnvLoader<DarkChessAction, DarkChessEnv> {
public:
    void loadFromEnvironment(const DarkChessEnv& env, const std::vector<std::vector<std::pair<std::string, std::string>>>& action_info_history = {}) override
    {
        BaseEnvLoader::loadFromEnvironment(env, action_info_history);
        addTag("SD", std::to_string(env.getSeed()));
        // TODO other tags?
    }
    std::vector<float> getActionFeatures(const int pos, utils::Rotation rotation = utils::Rotation::kRotationNone) const override;
    inline std::vector<float> getValue(const int pos) const { return {getReturn()}; }

    inline std::string name() const override { return kDarkChessName; }
    inline int getPolicySize() const override { return kDarkChessActionSize; }
    inline int getRotatePosition(int position, utils::Rotation rotation) const override { return position; }
    inline int getRotateAction(int action_id, utils::Rotation rotation) const override { return action_id; }
};

} // namespace minizero::env::darkchess

#endif // MINIZERO_ENVIRONMENT_DARKCHESS_DARKCHESS_H_
