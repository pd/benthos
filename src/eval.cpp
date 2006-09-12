#include "benthos.h"
#include "eval.h"

///////////////////////////////
// should return scores from perspective of side to move
///////////////////////////////
int
eval(const position_t *pos, int ply)
{
	int score = Material(ply, WHITE) + Material(ply, BLACK);

	if (Bishops(WHITE) != 0 && (Bishops(WHITE) & (Bishops(WHITE) - 1)) != 0)
		score += BISHOP_PAIR_BONUS;
	if (Bishops(BLACK) != 0 && (Bishops(BLACK) & (Bishops(BLACK) - 1)) != 0)
		score -= BISHOP_PAIR_BONUS;

	if (PawnCount(ply, WHITE) > 5) {
		int wKnights = popcnt(Knights(WHITE));
		int wRooks   = popcnt(Rooks(WHITE));
		score += wKnights * KNIGHT_PAWN_BONUS;
		score -= wRooks   * ROOK_PAWN_BONUS;
	} else if (PawnCount(ply, WHITE) < 5) {
		int wKnights = popcnt(Knights(WHITE));
		int wRooks   = popcnt(Rooks(WHITE));
		score -= wKnights * KNIGHT_PAWN_BONUS;
		score += wRooks   * ROOK_PAWN_BONUS;
	}

	if (PawnCount(ply, BLACK) > 5) {
		int bKnights = popcnt(Knights(BLACK));
		int bRooks   = popcnt(Rooks(BLACK));
		score -= bKnights * KNIGHT_PAWN_BONUS;
		score += bRooks   * ROOK_PAWN_BONUS;
	} else if (PawnCount(ply, BLACK) < 5) {
		int bKnights = popcnt(Knights(BLACK));
		int bRooks   = popcnt(Rooks(BLACK));
		score += bKnights * KNIGHT_PAWN_BONUS;
		score -= bRooks   * ROOK_PAWN_BONUS;
	}

	return Stm(ply) == WHITE ? score : -score;
}
