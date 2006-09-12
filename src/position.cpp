#include "benthos.h"

///////////////////////////////
// clears the pieces from the given position structure
///////////////////////////////
void
clear_position(position_t *pos)
{
	for (int sq = 0; sq < 64; sq++)
		PieceOn(sq) = EMPTY;

	for (int c = WHITE; c <= BLACK; c++) {
		Pieces(c) = 0;
		Pawns(c)  = Knights(c) = Bishops(c) = 0;
		Rooks(c)  = Queens(c)  = Kings(c)   = 0;
		KingSq(c) = INVALID_SQUARE;
	}

	Occupied = Occupied90L = Occupied45L = Occupied45R = 0;
}

///////////////////////////////
// resets the state information at the given ply number to
// the default empty values
///////////////////////////////
void
reset_state(int ply)
{
	Stm(ply) = WHITE;
	Castling(ply) = 0;
	HalfmoveClock(ply) = 0;
	HashKey(ply) = 0;
	PawnHashKey(ply) = 0;

	for (int c = WHITE; c <= BLACK; c++) {
		Material(ply, c) = 0;
		PawnCount(ply, c) = 0;
		MinorCount(ply, c) = 0;
		MajorCount(ply, c) = 0;
	}
}

///////////////////////////////
// sets up the provided position structure according to the
// fen string. note that this also overwrites the entry for
// ply zero in the state stack.
//
// returns false on error, but error checking is extremely
// minimal. this could easily break from a malformed string.
///////////////////////////////
bool
position_from_fen(position_t *pos, char *fen)
{
	char *p = fen;
	int sq;
	int hmc;

	clear_position(pos);
	reset_state(0);

	sq = 56;
	do {
		if (isdigit(*p)) {
			sq += atoi(p);
			p++;
			continue;
		} else if (*p == '/') {
			sq -= 16;
			p++;
			continue;
		}

		switch (*p++) {
		case 'P': PieceOn(sq) = WPAWN;   break;
		case 'N': PieceOn(sq) = WKNIGHT; break;
		case 'B': PieceOn(sq) = WBISHOP; break;
		case 'R': PieceOn(sq) = WROOK;   break;
		case 'Q': PieceOn(sq) = WQUEEN;  break;
		case 'K': PieceOn(sq) = WKING;   break;
		case 'p': PieceOn(sq) = BPAWN;   break;
		case 'n': PieceOn(sq) = BKNIGHT; break;
		case 'b': PieceOn(sq) = BBISHOP; break;
		case 'r': PieceOn(sq) = BROOK;   break;
		case 'q': PieceOn(sq) = BQUEEN;  break;
		case 'k': PieceOn(sq) = BKING;   break;
		default:  return false;
		}

		sq++;
	} while (*p && !isspace(*p));

	while (isspace(*p)) p++;
	switch (*p++) {
	case 'w': Stm(0) = WHITE; break;
	case 'b': Stm(0) = BLACK; break;
	default:  return false;
	}

	while (isspace(*p)) p++;
	while (!isspace(*p)) {
		if (*p == '-') {
			Castling(0) = 0;
			p++;
			break;
		}
		switch (*p++) {
		case 'K': Castling(0) |= WHITE_CAN_CASTLE_KS; break;
		case 'Q': Castling(0) |= WHITE_CAN_CASTLE_QS; break;
		case 'k': Castling(0) |= BLACK_CAN_CASTLE_KS; break;
		case 'q': Castling(0) |= BLACK_CAN_CASTLE_QS; break;
		default:  return false;
		}
	}

	while (isspace(*p)) p++;
	if (*p == '-')
		EpSquare(0) = INVALID_SQUARE;
	else {
		uint8 f = *p++ - 'a';
		uint8 r = *p++ - '1';
		EpSquare(0) = (r << 3) | f;
	}

	if (*p != '\0') {
		while (isspace(*p)) p++;
		sscanf(p, "%d", &hmc);
		HalfmoveClock(0) = hmc;
	}

	for (sq = 0; sq < 64; sq++) {
		uint8 pc    = PieceOn(sq);
		uint8 type  = PieceType(pc);
		uint8 color = PieceColor(pc);
		if (pc == EMPTY)
			continue;

		Material(0, color) += PieceValue(pc);

		Pieces(color) |= Mask(sq);
		switch (type) {
		case PAWN:
			Pawns(color) |= Mask(sq);
			PawnCount(0, color)++;
			break;
		case KNIGHT:
			Knights(color) |= Mask(sq);
			MinorCount(0, color)++;
			break;
		case BISHOP:
			Bishops(color) |= Mask(sq);
			MinorCount(0, color)++;
			break;
		case ROOK:
			Rooks(color) |= Mask(sq);
			MajorCount(0, color)++;
			break;
		case QUEEN:
			Queens(color) |= Mask(sq);
			MajorCount(0, color)++;
			break;
		case KING:
			if (KingSq(color) != INVALID_SQUARE)
				return 1;
			Kings(color) |= Mask(sq);
			KingSq(color) = sq;
			break;
		}
	}

	Occupied    = Pieces(WHITE) | Pieces(BLACK);
	Occupied90L = rotate90L(Occupied);
	Occupied45L = rotate45L(Occupied);
	Occupied45R = rotate45R(Occupied);

	// need at least both kings, and they can't be in passive check
	if (KingSq(WHITE) == INVALID_SQUARE || KingSq(BLACK) == INVALID_SQUARE)
		return false;
	if (Checked(Stm(0)^1))
		return false;

	calculate_hash_keys(pos, 0);

	return true;
}

///////////////////////////////
// returns the FEN description of the provided position, using the
// state information at the specified ply.
///////////////////////////////
char *
position_to_fen(const position_t *pos, int ply)
{
	static char buf[256];
	char *p = buf;
	int empties = 0;
	uint8 cr = Castling(ply);

	for (int rank = RANK8; rank >= RANK1; rank--) {
		for (int file = FILEA; file <= FILEH; file++) {
			int sq = (rank << 3) | file;
			uint8 pc = PieceOn(sq);
			if (pc == EMPTY) {
				empties++;
				continue;
			}

			if (empties > 0) {
				*p++ = empties + '0';
				empties = 0;
			}
			*p++ = PieceFEN(pc);
		}

		if (empties > 0) {
			*p++ = empties + '0';
			empties = 0;
		}
		if (rank > RANK1)
			*p++ = '/';
	}

	*p++ = ' ';
	if (Stm(ply) == WHITE)
		*p++ = 'w';
	else
		*p++ = 'b';

	*p++ = ' ';
	if (!(cr & WHITE_CAN_CASTLE) && !(cr & BLACK_CAN_CASTLE))
		*p++ = '-';
	else {
		if (cr & WHITE_CAN_CASTLE_KS)
			*p++ = 'K';
		if (cr & WHITE_CAN_CASTLE_QS)
			*p++ = 'Q';
		if (cr & BLACK_CAN_CASTLE_KS)
			*p++ = 'k';
		if (cr & BLACK_CAN_CASTLE_QS)
			*p++ = 'q';
	}

	*p++ = ' ';
	if (EpSquare(ply) == INVALID_SQUARE)
		*p++ = '-';
	else {
		*p++ = File(EpSquare(ply)) + 'a';
		*p++ = Rank(EpSquare(ply)) + '1';
	}

	// XXX: i don't keep track of game ply anywhere that's accessible from
	// here. so i'm just dumping fmn = 1, even if it doesn't make sense.
	sprintf(p, " %d 1", HalfmoveClock(ply));

	return buf;
}
