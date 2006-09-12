#include "benthos.h"

hashkey_t zobrist[16][64];
hashkey_t zobristCastling[16];
hashkey_t zobristEp[64];
hashkey_t zobristStm;

///////////////////////////////
// helper function for calculating hash keys. just cleans up
// code elsewhere.
///////////////////////////////
void
calculate_hash_keys(const position_t *pos, int ply)
{
	uint8 pc;

	for (int sq = 0; sq < 64; sq++) {
		pc = PieceOn(sq);
		if (pc == EMPTY)
			continue;

		HashKey(ply) ^= Zobrist(pc, sq);
		if (PieceType(pc) == PAWN)
			PawnHashKey(ply) ^= Zobrist(pc, sq);
	}

	HashKey(ply) ^= ZobristCastling(Castling(ply));
	if (EpSquare(ply) != INVALID_SQUARE)
		HashKey(ply) ^= ZobristEp(EpSquare(ply));
	if (Stm(ply) == BLACK)
		HashKey(ply) ^= ZobristStm;
}

///////////////////////////////
// fills in the zobrist key arrays with pseudorandom
// unsigned 64-bit integers
///////////////////////////////
void
init_zobrist(void)
{
	for (int i = 0; i < PIECEMAX; i++)
		for (int j = 0; j < SQUAREMAX; j++) {
			Zobrist(i, j) = genrand_int64();
			ZobristEp(j)  = genrand_int64();
		}

	for (int i = 0; i < 16; i++)
		ZobristCastling(i) = genrand_int64();

	ZobristStm = genrand_int64();
}
