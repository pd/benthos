#include "benthos.h"

static scored_move_t *generate_captures_of(const position_t *, scored_move_t *, uint8, uint8, int);
static scored_move_t *generate_blocks(const position_t *, scored_move_t *, uint8, uint8, uint8, int);
static scored_move_t *generate_pawn_captures(const position_t *, scored_move_t *, uint8, uint8, int);
static scored_move_t *generate_pawn_moves(const position_t *, scored_move_t *, uint8, uint8, int);
static scored_move_t *generate_piece_moves(const position_t *, scored_move_t *, uint8, uint8, int);

///////////////////////////////
// generates pseudo-legal noncaptures from the provided position, using
// the state information at the specified ply. the moves are appended to
// the scored_move_t pointer, and the incremented pointer is returned.
///////////////////////////////
scored_move_t *
generate_captures(const position_t *pos, scored_move_t *moves, int ply)
{
	uint8  stm  = Stm(ply);
	uint8  epsq = EpSquare(ply);
	uint8  from, to;
	uint8  pc;
	bitboard_t targets = Pieces(stm^1);
	bitboard_t pieces, dests;
	bitboard_t epmask = 0;
	move_t tmpmv;

	if (epsq != INVALID_SQUARE)
		epmask = Mask(epsq);

	pieces = Pawns(stm);
	if (stm == WHITE) {
		while (pieces) {
			from = poplsb(pieces);
			dests = WhitePawnAttacks(from) & targets;
			tmpmv = from | (WPAWN << 12);
			while (dests) {
				to = poplsb(dests);

				if (Rank(to) == RANK8) {
					move_t tmpmv2 = tmpmv | (to << 6) | (PieceOn(to) << 16);
					(moves++)->move = tmpmv2 | (WQUEEN  << 20);
					(moves++)->move = tmpmv2 | (WKNIGHT << 20);
					(moves++)->move = tmpmv2 | (WBISHOP << 20);
					(moves++)->move = tmpmv2 | (WROOK   << 20);
				} else
					(moves++)->move = tmpmv | (to << 6) | (PieceOn(to) << 16);
			}

			if (WhitePawnAttacks(from) & epmask)
				(moves++)->move = tmpmv | (epsq << 6) | (BPAWN << 16) | ENPASSANTMASK;
		}
	} else {
		while (pieces) {
			from = poplsb(pieces);
			dests = BlackPawnAttacks(from) & targets;
			tmpmv = from | (BPAWN << 12);
			while (dests) {
				to = poplsb(dests);

				if (Rank(to) == RANK1) {
					move_t tmpmv2 = tmpmv | (to << 6) | (PieceOn(to) << 16);
					(moves++)->move = tmpmv2 | (BQUEEN << 20);
					(moves++)->move = tmpmv2 | (BKNIGHT << 20);
					(moves++)->move = tmpmv2 | (BBISHOP << 20);
					(moves++)->move = tmpmv2 | (BROOK << 20);
				} else
					(moves++)->move = tmpmv | (to << 6) | (PieceOn(to) << 16);
			}

			if (BlackPawnAttacks(from) & epmask)
				(moves++)->move = tmpmv | (epsq << 6) | (WPAWN << 16) | ENPASSANTMASK;
		}
	}

	pieces = Knights(stm);
	pc = MakePiece(KNIGHT, stm);
	while (pieces) {
		from = poplsb(pieces);
		tmpmv = from | (pc << 12);
		dests = KnightMoves(from) & targets;
		while (dests) {
			to = poplsb(dests);
			(moves++)->move = tmpmv | (to << 6) | (PieceOn(to) << 16);
		}
	}

	pieces = Bishops(stm);
	pc = MakePiece(BISHOP, stm);
	while (pieces) {
		from = poplsb(pieces);
		tmpmv = from | (pc << 12);
		dests = BishopMoves(from) & targets;
		while (dests) {
			to = poplsb(dests);
			(moves++)->move = tmpmv | (to << 6) | (PieceOn(to) << 16);
		}
	}

	pieces = Rooks(stm);
	pc = MakePiece(ROOK, stm);
	while (pieces) {
		from = poplsb(pieces);
		tmpmv = from | (pc << 12);
		dests = RookMoves(from) & targets;
		while (dests) {
			to = poplsb(dests);
			(moves++)->move = tmpmv | (to << 6) | (PieceOn(to) << 16);
		}
	}

	pieces = Queens(stm);
	pc = MakePiece(QUEEN, stm);
	while (pieces) {
		from = poplsb(pieces);
		tmpmv = from | (pc << 12);
		dests = QueenMoves(from) & targets;
		while (dests) {
			to = poplsb(dests);
			(moves++)->move = tmpmv | (to << 6) | (PieceOn(to) << 16);
		}
	}

	from = KingSq(stm);
	pc = MakePiece(KING, stm);
	tmpmv = from | (pc << 12);
	dests = KingMoves(from) & targets;
	while (dests) {
		to = poplsb(dests);
		(moves++)->move = tmpmv | (to << 6) | (PieceOn(to) << 16);
	}

	return moves;
}

///////////////////////////////
// generates pseudo-legal noncaptures from the provided position, using
// the state information at the specified ply. the moves are appended to
// the scored_move_t pointer, and the incremented pointer is returned.
///////////////////////////////
scored_move_t *
generate_noncaptures(const position_t *pos, scored_move_t *moves, int ply)
{
	uint8 stm = Stm(ply);
	uint8 from, to;
	uint8 pc;
	bitboard_t targets = ~Occupied;
	bitboard_t pieces, dests;
	move_t tmpmv;

	// castling comes first if it's possible
	if (stm == WHITE) {
		if (CanWhiteKS(ply))
			(moves++)->move = MOVE_WHITE_OO;
		if (CanWhiteQS(ply))
			(moves++)->move = MOVE_WHITE_OOO;
	} else {
		if (CanBlackKS(ply))
			(moves++)->move = MOVE_BLACK_OO;
		if (CanBlackQS(ply))
			(moves++)->move = MOVE_BLACK_OOO;
	}

	pieces = Pawns(stm);
	if (stm == WHITE) {
		while (pieces) {
			from = poplsb(pieces);
			tmpmv = from | (WPAWN << 12);

			to = from + 8;
			if (Occupied & Mask(to))
				continue;
			if (Rank(from) == RANK2 && !(Occupied & Mask(to + 8))) {
				(moves++)->move = tmpmv | ((to + 8) << 6) | PAWNJUMPMASK;
				(moves++)->move = tmpmv | (to << 6);
			} else if (Rank(to) == RANK8) {
				move_t tmpmv2 = tmpmv | (to << 6);
				(moves++)->move = tmpmv2 | (WQUEEN << 20);
				(moves++)->move = tmpmv2 | (WKNIGHT << 20);
				(moves++)->move = tmpmv2 | (WROOK << 20);
				(moves++)->move = tmpmv2 | (WBISHOP << 20);
			} else
				(moves++)->move = tmpmv | (to << 6);
		}
	} else {
		while (pieces) {
			from = poplsb(pieces);
			tmpmv = from | (BPAWN << 12);

			to = from - 8;
			if (Occupied & Mask(to))
				continue;
			if (Rank(from) == RANK7 && !(Occupied & Mask(to - 8))) {
				(moves++)->move = tmpmv | ((to - 8) << 6) | PAWNJUMPMASK;
				(moves++)->move = tmpmv | (to << 6);
			} else if (Rank(to) == RANK1) {
				move_t tmpmv2 = tmpmv | (to << 6);
				(moves++)->move = tmpmv2 | (BQUEEN  << 20);
				(moves++)->move = tmpmv2 | (BKNIGHT << 20);
				(moves++)->move = tmpmv2 | (BROOK   << 20);
				(moves++)->move = tmpmv2 | (BBISHOP << 20);
			} else
				(moves++)->move = tmpmv | (to << 6);
		}
	}

	pieces = Knights(stm);
	pc = MakePiece(KNIGHT, stm);
	while (pieces) {
		from = poplsb(pieces);
		tmpmv = from | (pc << 12);
		dests = KnightMoves(from) & targets;
		while (dests) {
			to = poplsb(dests);
			(moves++)->move = tmpmv | (to << 6);
		}
	}

	pieces = Bishops(stm);
	pc = MakePiece(BISHOP, stm);
	while (pieces) {
		from = poplsb(pieces);
		tmpmv = from | (pc << 12);
		dests = BishopMoves(from) & targets;
		while (dests) {
			to = poplsb(dests);
			(moves++)->move = tmpmv | (to << 6);
		}
	}

	pieces = Rooks(stm);
	pc = MakePiece(ROOK, stm);
	while (pieces) {
		from = poplsb(pieces);
		tmpmv = from | (pc << 12);
		dests = RookMoves(from) & targets;
		while (dests) {
			to = poplsb(dests);
			(moves++)->move = tmpmv | (to << 6);
		}
	}

	pieces = Queens(stm);
	pc = MakePiece(QUEEN, stm);
	while (pieces) {
		from = poplsb(pieces);
		tmpmv = from | (pc << 12);
		dests = QueenMoves(from) & targets;
		while (dests) {
			to = poplsb(dests);
			(moves++)->move = tmpmv | (to << 6);
		}
	}

	from = KingSq(stm);
	pc = MakePiece(KING, stm);
	tmpmv = from | (pc << 12);
	dests = KingMoves(from) & targets;
	while (dests) {
		to = poplsb(dests);
		(moves++)->move = tmpmv | (to << 6);
	}

	return moves;
}

///////////////////////////////
// generates pseudolegal check evasions. this is presumably
// faster than generating all normal moves when most are useless,
// but my code is somewhat dirty, so it might not be.
///////////////////////////////
scored_move_t *
generate_evasions(const position_t *pos, scored_move_t *moves, int ply)
{
	uint8 stm = Stm(ply);
	uint8 opp = stm^1;
	uint8 ksq = KingSq(stm);
	bitboard_t attackers = attacks_to(pos, ksq) & Pieces(opp);
	uint8 pc, to;
	bitboard_t dests;
	move_t tmpmv;

	// first generate standard king moves
	dests = KingMoves(ksq) & ~Pieces(stm);
	pc = MakePiece(KING, stm);
	tmpmv = ksq | (pc << 12);
	while (dests) {
		to = poplsb(dests);
		if (AttackedBy(opp, to))
			continue;

		pc = PieceOn(to);
		if (pc == EMPTY)
			(moves++)->move = tmpmv | (to << 6);
		else
			(moves++)->move = tmpmv | (to << 6) | (pc << 16);
	}

	// if this is a double check, there's no other way to get out of check.
	if (attackers & (attackers - ULL(1)))
		return moves;

	// capture or block the attacking piece.
	to = lsb(attackers);
	moves = generate_captures_of(pos, moves, to, stm, ply);
	if (Slides(PieceOn(to)))
		moves = generate_blocks(pos, moves, to, ksq, stm, ply);

	return moves;
}

///////////////////////////////
// helper function for generate_evasions. generates captures of the piece
// on a given square by the specified side to move.
///////////////////////////////
static scored_move_t *
generate_captures_of(const position_t *pos, scored_move_t *moves, uint8 sq, uint8 stm, int ply)
{
	moves = generate_pawn_captures(pos, moves, sq, stm, ply);
	return generate_piece_moves(pos, moves, sq, stm, ply);
}

///////////////////////////////
// helper function for generate_evasions. generates moves that will block
// a sliding attack from sq1 to sq2, by any piece but a king of the specified
// side to move.
///////////////////////////////
static scored_move_t *
generate_blocks(const position_t *pos, scored_move_t *moves, uint8 sq1, uint8 sq2, uint8 stm, int ply)
{
	uint8 to;
	bitboard_t ray = RayBetween(sq1, sq2);

	while (ray) {
		to = poplsb(ray);
		moves = generate_pawn_moves(pos, moves, to, stm, ply);
		moves = generate_piece_moves(pos, moves, to, stm, ply);
	}

	return moves;
}

///////////////////////////////
// helper function for generate_evasions. generates captures of the piece
// on a given square by the pawns of the specified side to move.
///////////////////////////////
static scored_move_t *
generate_pawn_captures(const position_t *pos, scored_move_t *moves, uint8 sq, uint8 stm, int ply)
{
	uint8 cappc = PieceOn(sq);
	uint8 epsq  = EpSquare(ply);
	uint8 from;
	bitboard_t target = Mask(sq);
	bitboard_t pieces = attacks_to(pos, sq) & Pawns(stm);
	move_t tmpmv;

	if (stm == WHITE) {
		while (pieces) {
			from = poplsb(pieces);

			if (WhitePawnAttacks(from) & target) {
				tmpmv = from | (sq << 6) | (WPAWN << 12) | (cappc << 16);
				if (Rank(sq) == RANK8) {
					(moves++)->move = tmpmv | (WQUEEN  << 20);
					(moves++)->move = tmpmv | (WKNIGHT << 20);
					(moves++)->move = tmpmv | (WROOK   << 20);
					(moves++)->move = tmpmv | (WBISHOP << 20);
				} else
					(moves++)->move = tmpmv;
			}
		}

		// special case afterwards: try to capture with en passant
		if (cappc == BPAWN && sq == epsq - 8) {
			bitboard_t mask = 0;
			if (Distance(epsq, epsq - 9) == 1)
				mask |= Mask(epsq - 9);
			if (Distance(epsq, epsq - 7) == 1)
				mask |= Mask(epsq - 7);

			pieces = Pawns(stm) & mask;
			while (pieces) {
				from = poplsb(pieces);
				(moves++)->move = from | (epsq << 6) | (WPAWN << 12) | (BPAWN << 12) | ENPASSANTMASK;
			}
		}
	} else {
		while (pieces) {
			from = poplsb(pieces);

			if (BlackPawnAttacks(from) & target) {
				tmpmv = from | (sq << 6) | (BPAWN << 12) | (cappc << 16);
				if (Rank(sq) == RANK1) {
					(moves++)->move = tmpmv | (BQUEEN  << 20);
					(moves++)->move = tmpmv | (BKNIGHT << 20);
					(moves++)->move = tmpmv | (BROOK   << 20);
					(moves++)->move = tmpmv | (BBISHOP << 20);
				} else
					(moves++)->move = tmpmv;
			}
		}

		// special case afterwards: try to capture with en passant
		if (cappc == WPAWN && sq == epsq + 8) {
			bitboard_t mask = 0;
			if (Distance(epsq, epsq + 9) == 1)
				mask |= Mask(epsq + 9);
			if (Distance(epsq, epsq + 7) == 1)
				mask |= Mask(epsq + 7);

			pieces = Pawns(stm) & mask;
			while (pieces) {
				from = poplsb(pieces);
				(moves++)->move = from | (epsq << 6) | (BPAWN << 12) | (WPAWN << 12) | ENPASSANTMASK;
			}
		}
	}

	return moves;
}

///////////////////////////////
// helper function for generate_evasions. generates non-capturing
// moves to a given square by pawns of the specified side to move.
///////////////////////////////
static scored_move_t *
generate_pawn_moves(const position_t *pos, scored_move_t *moves, uint8 sq, uint8 stm, int ply)
{
	uint8 epsq = EpSquare(ply);
	uint8 from;
	bitboard_t pieces = Pawns(stm);
	move_t tmpmv;

	if (stm == WHITE) {
		while (pieces) {
			from = poplsb(pieces);

			if (from == sq - 8) {
				tmpmv = from | (sq << 6) | (WPAWN << 12);

				if (Rank(sq) == RANK8) {
					(moves++)->move = tmpmv | (WQUEEN  << 20);
					(moves++)->move = tmpmv | (WKNIGHT << 20);
					(moves++)->move = tmpmv | (WROOK   << 20);
					(moves++)->move = tmpmv | (WBISHOP << 20);
				} else
					(moves++)->move = tmpmv;
			} else if (Rank(sq) == RANK4 && from == sq - 16 && PieceOn(sq - 8) == EMPTY)
				(moves++)->move = from | (sq << 6) | (WPAWN << 12) | PAWNJUMPMASK;
		}

		if (sq == epsq) {
			bitboard_t mask = 0;
			if (Distance(epsq, epsq - 9) == 1)
				mask |= Mask(epsq - 9);
			if (Distance(epsq, epsq - 7) == 1)
				mask |= Mask(epsq - 7);

			pieces = Pawns(stm) & mask;
			while (pieces) {
				from = poplsb(pieces);
				(moves++)->move = from | (epsq << 6) | (WPAWN << 12) | (BPAWN << 16) | ENPASSANTMASK;
			}
		}
	} else {
		while (pieces) {
			from = poplsb(pieces);

			if (from == sq + 8) {
				tmpmv = from | (sq << 6) | (BPAWN << 12);

				if (Rank(sq) == RANK8) {
					(moves++)->move = tmpmv | (BQUEEN  << 20);
					(moves++)->move = tmpmv | (BKNIGHT << 20);
					(moves++)->move = tmpmv | (BROOK   << 20);
					(moves++)->move = tmpmv | (BBISHOP << 20);
				} else
					(moves++)->move = tmpmv;
			} else if (Rank(sq) == RANK5 && from == sq + 16 && PieceOn(sq + 8) == EMPTY)
				(moves++)->move = from | (sq << 6) | (BPAWN << 12) | PAWNJUMPMASK;
		}

		if (sq == epsq) {
			bitboard_t mask = 0;
			if (Distance(epsq, epsq + 9) == 1)
				mask |= Mask(epsq + 9);
			if (Distance(epsq, epsq + 7) == 1)
				mask |= Mask(epsq + 7);

			pieces = Pawns(stm) & mask;
			while (pieces) {
				from = poplsb(pieces);
				(moves++)->move = from | (epsq << 6) | (BPAWN << 12) | (WPAWN << 16) | ENPASSANTMASK;
			}
		}
	}

	return moves;
}

///////////////////////////////
// helper function for generate_evasions. generates moves to a given
// square by all pieces except pawns or the king.
///////////////////////////////
static scored_move_t *
generate_piece_moves(const position_t *pos, scored_move_t *moves, uint8 sq, uint8 stm, int ply)
{
	uint8 cappc = PieceOn(sq);
	uint8 from, pc;
	bitboard_t attackers = attacks_to(pos, sq) & Pieces(stm);
	bitboard_t pieces;
	move_t tmpmv = (sq << 6) | (cappc << 16);

	pieces = Knights(stm) & attackers;
	pc = MakePiece(KNIGHT, stm);
	while (pieces) {
		from = poplsb(pieces);
		(moves++)->move = tmpmv | from | (pc << 12);
	}

	pieces = Bishops(stm) & attackers;
	pc = MakePiece(BISHOP, stm);
	while (pieces) {
		from = poplsb(pieces);
		(moves++)->move = tmpmv | from | (pc << 12);
	}

	pieces = Rooks(stm) & attackers;
	pc = MakePiece(ROOK, stm);
	while (pieces) {
		from = poplsb(pieces);
		(moves++)->move = tmpmv | from | (pc << 12);
	}

	pieces = Queens(stm) & attackers;
	pc = MakePiece(QUEEN, stm);
	while (pieces) {
		from = poplsb(pieces);
		(moves++)->move = tmpmv | from | (pc << 12);
	}

	return moves;
}
