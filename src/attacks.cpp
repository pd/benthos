#include "benthos.h"

///////////////////////////////
// returns a map of all attackers to the given square
///////////////////////////////
bitboard_t
attacks_to(const position_t *pos, uint8 sq)
{
	return (BlackPawnAttacks(sq) & Pawns(WHITE))
			| (WhitePawnAttacks(sq) & Pawns(BLACK))
			| (KnightMoves(sq) & (Knights(WHITE) | Knights(BLACK)))
			| (BishopMoves(sq) & (Bishops(WHITE) | Queens(WHITE) | Bishops(BLACK) | Queens(BLACK)))
			| (RookMoves(sq) & (Rooks(WHITE) | Queens(WHITE) | Rooks(BLACK) | Queens(BLACK)))
			| (KingMoves(sq) & (Kings(WHITE) | Kings(BLACK)));
}

///////////////////////////////
// determines if white is attacking the specified square
///////////////////////////////
bool
white_attacking(const position_t *pos, uint8 sq)
{
	if (BlackPawnAttacks(sq) & Pawns(WHITE))
		return true;
	if (KnightMoves(sq) & Knights(WHITE))
		return true;
	if (KingMoves(sq) & Kings(WHITE))
		return true;
	if (BishopMoves(sq) & (Bishops(WHITE) | Queens(WHITE)))
		return true;
	if (RookMoves(sq) & (Rooks(WHITE) | Queens(WHITE)))
		return true;
	return false;
}

///////////////////////////////
// determines if black is attacking the specified square
///////////////////////////////
bool
black_attacking(const position_t *pos, uint8 sq)
{
	if (WhitePawnAttacks(sq) & Pawns(BLACK))
		return true;
	if (KnightMoves(sq) & Knights(BLACK))
		return true;
	if (KingMoves(sq) & Kings(BLACK))
		return true;
	if (BishopMoves(sq) & (Bishops(BLACK) | Queens(BLACK)))
		return true;
	if (RookMoves(sq) & (Rooks(BLACK) | Queens(BLACK)))
		return true;
	return false;
}
