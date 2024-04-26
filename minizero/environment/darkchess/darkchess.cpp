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

} // namespace minizero::env::darkchess