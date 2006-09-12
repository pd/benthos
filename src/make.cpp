#include "benthos.h"

///////////////////////////////
// helper function to update the castling availability key
// in the zobrist hash. just cleans up make_move a bit.
///////////////////////////////
static inline hashkey_t
change_castling(int ply, hashkey_t oldKey, uint8 andMask)
{
	oldKey ^= ZobristCastling(Castling(ply));
	Castling(ply) &= andMask;
	return oldKey ^ ZobristCastling(Castling(ply));
}

///////////////////////////////
// performs the provided move on given position, and places all
// new state information into the index after the specified ply.
///////////////////////////////
void
make_move(position_t *pos, move_t move, int ply)
{
	uint8 newply = ply + 1;
	uint8 stm    = Stm(ply);
	uint8 opp    = stm^1;

	uint8 from = From(move),  to  = To(move);
	uint8 pc   = Piece(move), cap = Capture(move), prom = Promote(move);

	bitboard_t fromMask = Mask(from), toMask = Mask(to);
	bitboard_t moveMask = fromMask | toMask;
	hashkey_t  hashKey  = HashKey(ply);
	hashkey_t  pHashKey = PawnHashKey(ply);

	// update the state information of the new ply
	states[newply] = states[ply];
	Stm(newply) = opp;
	EpSquare(newply) = INVALID_SQUARE;
	HalfmoveClock(newply)++;

	// common bitboard updates:
	if (stm == WHITE)
		Pieces(WHITE) ^= moveMask;
	else
		Pieces(BLACK) ^= moveMask;
	Occupied     ^= moveMask;
	Occupied90L  ^= Mask90L(from) | Mask90L(to);
	Occupied45L  ^= Mask45L(from) | Mask45L(to);
	Occupied45R  ^= Mask45R(from) | Mask45R(to);
	PieceOn(from) = EMPTY;
	PieceOn(to)   = pc;

	// predictable hash key updates
	hashKey ^= ZobristStm;
	hashKey ^= Zobrist(pc, from) ^ Zobrist(pc, to);
	if (EpSquare(ply) != INVALID_SQUARE)
		hashKey ^= ZobristEp(EpSquare(ply));

	switch (PieceType(pc)) {
	case PAWN:
		Pawns(stm) ^= moveMask;
		pHashKey   ^= Zobrist(pc, from) ^ Zobrist(pc, to);
		HalfmoveClock(newply) = 0;

		if (stm == WHITE) {
			if (IsPawnJump(move)) {
				EpSquare(newply) = from + 8;
				hashKey ^= ZobristEp(from + 8);
			} else if (prom != EMPTY) {
				Pawns(WHITE) ^= toMask;
				hashKey      ^= Zobrist(WPAWN, to);
				pHashKey     ^= Zobrist(WPAWN, to);
				PawnCount(newply, WHITE)--;
				Material(newply, WHITE) -= PieceValue(WPAWN);

				switch (prom) {
				case WQUEEN:
					Queens(WHITE) ^= toMask;
					PieceOn(to)    = WQUEEN;
					hashKey       ^= Zobrist(WQUEEN, to);
					MajorCount(newply, WHITE)++;
					Material(newply, WHITE) += PieceValue(WQUEEN);
					break;
				case WKNIGHT:
					Knights(WHITE) ^= toMask;
					PieceOn(to)     = WKNIGHT;
					hashKey        ^= Zobrist(WKNIGHT, to);
					MinorCount(newply, WHITE)++;
					Material(newply, WHITE) += PieceValue(WKNIGHT);
					break;
				case WROOK:
					Rooks(WHITE) ^= toMask;
					PieceOn(to)   = WROOK;
					hashKey      ^= Zobrist(WROOK, to);
					MajorCount(newply, WHITE)++;
					Material(newply, WHITE) += PieceValue(WROOK);
					break;
				case WBISHOP:
					Bishops(WHITE) ^= toMask;
					PieceOn(to)     = WBISHOP;
					hashKey        ^= Zobrist(WBISHOP, to);
					MinorCount(newply, WHITE)++;
					Material(newply, WHITE) += PieceValue(WBISHOP);
					break;
				}
			} else if (IsEnPassant(move)) {
				uint8 capsq = to - 8;
				bitboard_t capMask = Mask(capsq);

				Pieces(BLACK) ^= capMask;
				Pawns(BLACK)  ^= capMask;
				Occupied      ^= capMask;
				Occupied90L   ^= Mask90L(capsq);
				Occupied45L   ^= Mask45L(capsq);
				Occupied45R   ^= Mask45R(capsq);
				PieceOn(capsq) = EMPTY;
				hashKey       ^= Zobrist(BPAWN, capsq);
				pHashKey      ^= Zobrist(BPAWN, capsq);
				PawnCount(newply, BLACK)--;
				Material(newply, BLACK) -= PieceValue(BPAWN);

				// clear so we don't bother with handling the capture later
				cap = EMPTY;
			}
		} else {
			if (IsPawnJump(move)) {
				EpSquare(newply) = from - 8;
				hashKey ^= ZobristEp(from - 8);
			} else if (prom != EMPTY) {
				Pawns(BLACK) ^= toMask;
				hashKey      ^= Zobrist(BPAWN, to);
				pHashKey     ^= Zobrist(BPAWN, to);
				PawnCount(newply, BLACK)--;
				Material(newply, BLACK) -= PieceValue(BPAWN);

				switch (prom) {
				case BQUEEN:
					Queens(BLACK) ^= toMask;
					PieceOn(to)    = BQUEEN;
					hashKey       ^= Zobrist(BQUEEN, to);
					MajorCount(newply, BLACK)++;
					Material(newply, BLACK) += PieceValue(BQUEEN);
					break;
				case BKNIGHT:
					Knights(BLACK) ^= toMask;
					PieceOn(to)     = BKNIGHT;
					hashKey        ^= Zobrist(BKNIGHT, to);
					MinorCount(newply, BLACK)++;
					Material(newply, BLACK) += PieceValue(BKNIGHT);
					break;
				case BROOK:
					Rooks(BLACK) ^= toMask;
					PieceOn(to)   = BROOK;
					hashKey      ^= Zobrist(BROOK, to);
					MajorCount(newply, BLACK)++;
					Material(newply, BLACK) += PieceValue(BROOK);
					break;
				case BBISHOP:
					Bishops(BLACK) ^= toMask;
					PieceOn(to)     = BBISHOP;
					hashKey        ^= Zobrist(BBISHOP, to);
					MinorCount(newply, BLACK)++;
					Material(newply, BLACK) += PieceValue(BBISHOP);
					break;
				}
			} else if (IsEnPassant(move)) {
				uint8 capsq = to + 8;
				bitboard_t capMask = Mask(capsq);

				Pieces(WHITE) ^= capMask;
				Pawns(WHITE)  ^= capMask;
				Occupied      ^= capMask;
				Occupied90L   ^= Mask90L(capsq);
				Occupied45L   ^= Mask45L(capsq);
				Occupied45R   ^= Mask45R(capsq);
				PieceOn(capsq) = EMPTY;
				hashKey       ^= Zobrist(WPAWN, capsq);
				pHashKey      ^= Zobrist(WPAWN, capsq);
				PawnCount(newply, WHITE)--;
				Material(newply, WHITE) -= PieceValue(WPAWN);

				// clear so we don't bother with handling the capture later
				cap = EMPTY;
			}
		}
		break;
	case KNIGHT:
		Knights(stm) ^= moveMask;
		break;
	case BISHOP:
		Bishops(stm) ^= moveMask;
		break;
	case ROOK:
		Rooks(stm) ^= moveMask;
		if (stm == WHITE) {
			if (from == H1)
				hashKey = change_castling(newply, hashKey, ~WHITE_CAN_CASTLE_KS);
			else if (from == A1)
				hashKey = change_castling(newply, hashKey, ~WHITE_CAN_CASTLE_QS);
		} else {
			if (from == H8)
				hashKey = change_castling(newply, hashKey, ~BLACK_CAN_CASTLE_KS);
			else if (from == A8)
				hashKey = change_castling(newply, hashKey, ~BLACK_CAN_CASTLE_QS);
		}
		break;
	case QUEEN:
		Queens(stm) ^= moveMask;
		break;
	case KING:
		if (stm == WHITE) {
			Kings(WHITE) ^= moveMask;
			KingSq(WHITE) = to;
			hashKey = change_castling(newply, hashKey, ~WHITE_CAN_CASTLE);

			if (IsCastle(move)) {
				if (to == G1) {
					// constants = Mask{00L,90L,...} H1 | F1
					uint64 rookMoveMask = 0xa0;
					Pieces(WHITE) ^= rookMoveMask;
					Rooks(WHITE)  ^= rookMoveMask;
					Occupied      ^= rookMoveMask;
					Occupied90L   ^= ULL(0x8000800000000000);
					Occupied45L   ^= ULL(0x800100000);
					Occupied45R   ^= ULL(0x9);
					PieceOn(H1)    = EMPTY;
					PieceOn(F1)    = WROOK;
					hashKey       ^= Zobrist(WROOK, H1) ^ Zobrist(WROOK, F1);
				} else if (to == C1) {
					// constants = Mask{00L,90L,...} A1 | D1
					uint64 rookMoveMask = ULL(0x9);
					Pieces(WHITE) ^= rookMoveMask;
					Rooks(WHITE)  ^= rookMoveMask;
					Occupied      ^= rookMoveMask;
					Occupied90L   ^= ULL(0x80000080);
					Occupied45L   ^= ULL(0x201);
					Occupied45R   ^= ULL(0x10000400);
					PieceOn(A1)    = EMPTY;
					PieceOn(D1)    = WROOK;
					hashKey       ^= Zobrist(WROOK, A1) ^ Zobrist(WROOK, D1);
				}
			}
		} else {
			Kings(BLACK) ^= moveMask;
			KingSq(BLACK) = to;
			hashKey = change_castling(newply, hashKey, ~BLACK_CAN_CASTLE);

			if (IsCastle(move)) {
				if (to == G8) {
					// constants = Mask{00L,90L,...} H8 | F8
					uint64 rookMoveMask = ULL(0xa000000000000000);
					Pieces(BLACK) ^= rookMoveMask;
					Rooks(BLACK)  ^= rookMoveMask;
					Occupied      ^= rookMoveMask;
					Occupied90L   ^= ULL(0x100010000000000);
					Occupied45L   ^= ULL(0x8400000000000000);
					Occupied45R   ^= ULL(0x1000800000000);
					PieceOn(H8)    = EMPTY;
					PieceOn(F8)    = BROOK;
					hashKey       ^= Zobrist(BROOK, H8) ^ Zobrist(BROOK, F8);
				} else if (to == C8) {
					// constants = Mask{00L,90L,...} A8 | D8
					uint64 rookMoveMask = ULL(0x900000000000000);
					Pieces(BLACK) ^= rookMoveMask;
					Rooks(BLACK)  ^= rookMoveMask;
					Occupied      ^= rookMoveMask;
					Occupied90L   ^= ULL(0x1000001);
					Occupied45L   ^= ULL(0x2000010000000);
					Occupied45R   ^= ULL(0x8200000000000000);
					PieceOn(A8)    = EMPTY;
					PieceOn(D8)    = BROOK;
					hashKey       ^= Zobrist(BROOK, A8) ^ Zobrist(BROOK, D8);
				}
			}
		}
		break;
	}

	// if there was nothing captured, we're done.
	// note that en passant captures were handled with pawn moves above.
	if (cap == EMPTY) {
		HashKey(newply) = hashKey;
		PawnHashKey(newply) = pHashKey;
		return;
	}

	// finish up by removing the captured piece
	if (opp == WHITE)
		Pieces(WHITE) ^= toMask;
	else
		Pieces(BLACK) ^= toMask;
	Occupied    ^= toMask;
	Occupied90L ^= Mask90L(to);
	Occupied45L ^= Mask45L(to);
	Occupied45R ^= Mask45R(to);
	HalfmoveClock(newply) = 0;
	Material(newply, opp) -= PieceValue(cap);
	hashKey ^= Zobrist(cap, to);

	switch (PieceType(cap)) {
	case PAWN:
		Pawns(opp) ^= toMask;
		pHashKey   ^= Zobrist(cap, to);
		PawnCount(newply, opp)--;
		break;
	case KNIGHT:
		Knights(opp) ^= toMask;
		MinorCount(newply, opp)--;
		break;
	case BISHOP:
		Bishops(opp) ^= toMask;
		MinorCount(newply, opp)--;
		break;
	case ROOK:
		Rooks(opp) ^= toMask;
		MajorCount(newply, opp)--;
		switch (to) {
		case H1:
			hashKey = change_castling(newply, hashKey, ~WHITE_CAN_CASTLE_KS);
			break;
		case A1:
			hashKey = change_castling(newply, hashKey, ~WHITE_CAN_CASTLE_QS);
			break;
		case H8:
			hashKey = change_castling(newply, hashKey, ~BLACK_CAN_CASTLE_KS);
			break;
		case A8:
			hashKey = change_castling(newply, hashKey, ~BLACK_CAN_CASTLE_QS);
			break;
		}
		break;
	case QUEEN:
		Queens(opp) ^= toMask;
		MajorCount(newply, opp)--;
		break;
	}

	HashKey(newply) = hashKey;
	PawnHashKey(newply) = pHashKey;
}

///////////////////////////////
// performs the opposite of the operations that make_move performs.
// this is not responsible for updating the state stack values at
// all, as their values should still be available in the previous
// ply's entry.
///////////////////////////////
void
unmake_move(position_t *pos, move_t move, int ply)
{
	uint8 stm = Stm(ply);
	uint8 opp = stm ^ 1;

	uint8 from = From(move),  to  = To(move);
	uint8 pc   = Piece(move), cap = Capture(move), prom = Promote(move);

	uint64 fromMask = Mask(from), toMask = Mask(to);
	uint64 moveMask = fromMask | toMask;

	if (stm == WHITE)
		Pieces(WHITE) ^= moveMask;
	else
		Pieces(BLACK) ^= moveMask;
	Occupied     ^= moveMask;
	Occupied90L  ^= Mask90L(from) | Mask90L(to);
	Occupied45L  ^= Mask45L(from) | Mask45L(to);
	Occupied45R  ^= Mask45R(from) | Mask45R(to);
	PieceOn(from) = pc;
	PieceOn(to)   = EMPTY;

	switch (PieceType(pc)) {
	case PAWN:
		Pawns(stm) ^= moveMask;
		if (prom != EMPTY) {
			Pawns(stm) ^= toMask;
			switch (PieceType(prom)) {
			case QUEEN:  Queens(stm)  ^= toMask; break;
			case KNIGHT: Knights(stm) ^= toMask; break;
			case BISHOP: Bishops(stm) ^= toMask; break;
			case ROOK:   Rooks(stm)   ^= toMask; break;
			}
		} else if (IsEnPassant(move)) {
			uint8  capsq    = stm == WHITE ? to - 8 : to + 8;
			uint64 capMask  = Mask(capsq);
			Pieces(opp)    ^= capMask;
			Pawns(opp)     ^= capMask;
			Occupied       ^= capMask;
			Occupied90L    ^= Mask90L(capsq);
			Occupied45L    ^= Mask45L(capsq);
			Occupied45R    ^= Mask45R(capsq);
			PieceOn(capsq)  = MakePiece(PAWN, opp);

			// clear so we don't bother with this later
			cap = EMPTY;
		}
		break;
	case KNIGHT:
		Knights(stm) ^= moveMask;
		break;
	case BISHOP:
		Bishops(stm) ^= moveMask;
		break;
	case ROOK:
		Rooks(stm) ^= moveMask;
		break;
	case QUEEN:
		Queens(stm) ^= moveMask;
		break;
	case KING:
		if (stm == WHITE) {
			Kings(WHITE) ^= moveMask;
			KingSq(WHITE) = from;

			if (IsCastle(move)) {
				if (to == G1) {
					// constants = Mask{00L,90L,...} H1 | F1
					uint64 rookMoveMask = 0xa0;
					Pieces(WHITE) ^= rookMoveMask;
					Rooks(WHITE)  ^= rookMoveMask;
					Occupied      ^= rookMoveMask;
					Occupied90L   ^= ULL(0x8000800000000000);
					Occupied45L   ^= ULL(0x800100000);
					Occupied45R   ^= ULL(0x9);
					PieceOn(H1)    = WROOK;
					PieceOn(F1)    = EMPTY;
				} else if (to == C1) {
					// constants = Mask{00L,90L,...} A1 | D1
					uint64 rookMoveMask = ULL(0x9);
					Pieces(WHITE) ^= rookMoveMask;
					Rooks(WHITE)  ^= rookMoveMask;
					Occupied      ^= rookMoveMask;
					Occupied90L   ^= ULL(0x80000080);
					Occupied45L   ^= ULL(0x201);
					Occupied45R   ^= ULL(0x10000400);
					PieceOn(A1)    = WROOK;
					PieceOn(D1)    = EMPTY;
				}
			}
		} else {
			Kings(BLACK) ^= moveMask;
			KingSq(BLACK) = from;

			if (IsCastle(move)) {
				if (to == G8) {
					// constants = Mask{00L,90L,...} H8 | F8
					uint64 rookMoveMask = ULL(0xa000000000000000);
					Pieces(BLACK) ^= rookMoveMask;
					Rooks(BLACK)  ^= rookMoveMask;
					Occupied      ^= rookMoveMask;
					Occupied90L   ^= ULL(0x100010000000000);
					Occupied45L   ^= ULL(0x8400000000000000);
					Occupied45R   ^= ULL(0x1000800000000);
					PieceOn(H8)    = BROOK;
					PieceOn(F8)    = EMPTY;
				} else if (to == C8) {
					// constants = Mask{00L,90L,...} A8 | D8
					uint64 rookMoveMask = ULL(0x900000000000000);
					Pieces(BLACK) ^= rookMoveMask;
					Rooks(BLACK)  ^= rookMoveMask;
					Occupied      ^= rookMoveMask;
					Occupied90L   ^= ULL(0x1000001);
					Occupied45L   ^= ULL(0x2000010000000);
					Occupied45R   ^= ULL(0x8200000000000000);
					PieceOn(A8)    = BROOK;
					PieceOn(D8)    = EMPTY;
				}
			}
		}
		break;
	}

	// if there was no capture, we're done.
	// note that en passant was handled with the pawns above.
	if (cap == EMPTY)
		return;

	// restore the captured piece
	if (opp == WHITE)
		Pieces(WHITE) ^= toMask;
	else
		Pieces(BLACK) ^= toMask;
	Occupied    ^= toMask;
	Occupied90L ^= Mask90L(to);
	Occupied45L ^= Mask45L(to);
	Occupied45R ^= Mask45R(to);
	PieceOn(to)  = cap;

	switch (PieceType(cap)) {
	case PAWN:
		Pawns(opp) ^= toMask;
		break;
	case KNIGHT:
		Knights(opp) ^= toMask;
		break;
	case BISHOP:
		Bishops(opp) ^= toMask;
		break;
	case ROOK:
		Rooks(opp) ^= toMask;
		break;
	case QUEEN:
		Queens(opp) ^= toMask;
		break;
	}
}
