#ifndef DARKCHESS_UNIT_H_
#define DARKCHESS_UNIT_H_

#include <string>

namespace minizero::env::darkchess {

const std::string kDarkChessName = "darkchess";
const int kDarkChessNumPlayer = 2;
const int kDarkChessActionSize = 1024;     // TODO action size 能不能縮小？
// TODO discrete value 應該是用於 reward transformation
// 601 的數字出現於 MuZero 的論文 https://arxiv.org/pdf/1911.08265.pdf
// 由於尚不清楚暗棋該用何值，暫時使用與其他 board game 相同的 1
const int kDarkChessDiscreteValueSize = 1;
const int kDarkChessBoardLength = 8;
const int kDarkChessBoardWidth = 4;

using DarkChessBitboard = unsigned long long;
using Move = std::vector<int>;  // {src, dst, src_chess, dst_chess}

} // namespace minizero::env::darkchess

#endif
