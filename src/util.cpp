#include "benthos.h"

///////////////////////////////
// returns a piece type for a given SAN character.
///////////////////////////////
uint8
piece_from_san(char c)
{
	switch (c) {
	case 'P': return PAWN;
	case 'N': return KNIGHT;
	case 'B': return BISHOP;
	case 'R': return ROOK;
	case 'Q': return QUEEN;
	case 'K': return KING;
	default:  return EMPTY;
	}
}

///////////////////////////////
// returns a move in coordinate notation (e7e8q)
///////////////////////////////
char *
move2str(move_t mv)
{
	static char buf[6];
	char *p = buf;

	if (!mv)
		return "0000";

	*p++ = File(From(mv)) + 'a';
	*p++ = Rank(From(mv)) + '1';
	*p++ = File(To(mv))   + 'a';
	*p++ = Rank(To(mv))   + '1';
	if (Promote(mv) != EMPTY)
		*p++ = PieceFEN(MakePiece(PieceType(Promote(mv)), BLACK));
	*p++ = '\0';

	return buf;
}

///////////////////////////////
// returns a move's move_t value based on the provided string,
// using coordinate notation (e7f8q).
// zero returned on error.
///////////////////////////////
move_t
str2move(const position_t *pos, const char *str)
{
	int rank, file;
	int stm;
	int from, to;
	int pc = EMPTY, cap = EMPTY, prom = EMPTY;
	int flag = 0;

	if (strlen(str) < 4)
		return 0;

	file = *str++ - 'a';
	rank = *str++ - '1';
	from = (rank << 3) | file;
	file = *str++ - 'a';
	rank = *str++ - '1';
	to   = (rank << 3) | file;

	if (from < 0 || to < 0 || from > 63 || to > 63)
		return 0;

	pc = PieceOn(from);
	if (pc == EMPTY)
		return 0;

	stm = PieceColor(pc);
	cap = PieceOn(to);

	if (*str != '\0')
		switch (*str) {
		case 'q': prom = MakePiece(QUEEN,  stm); break;
		case 'n': prom = MakePiece(KNIGHT, stm); break;
		case 'r': prom = MakePiece(ROOK,   stm); break;
		case 'b': prom = MakePiece(BISHOP, stm); break;
		default:  return 0;
		}

	if (PieceType(pc) == PAWN) {
		if (Distance(from, to) == 2)
			flag = PAWNJUMPMASK;
		else if (cap == EMPTY && abs(Direction(from, to)) != 8)
			flag = ENPASSANTMASK;
	} else if (PieceType(pc) == KING && Distance(from, to) == 2)
		flag = CASTLEMASK;

	return from | (to << 6) | (pc << 12) | (cap << 16) | (prom << 20) | flag;
}

///////////////////////////////
// returns a move in pseudo-san notation (Na2xd3)
// TODO: proper SAN with disambiguation
///////////////////////////////
char *
move2san(move_t mv)
{
	static char buf[10];
	char *p = buf;
	uint8 from = From(mv), to = To(mv);
	uint8 pc = Piece(mv), cap = Capture(mv), prom = Promote(mv);

	if (!mv)
		return "0000";

	if (PieceType(pc) != PAWN)
		*p++ = PieceSAN(pc);
	*p++ = File(from) + 'a';
	*p++ = Rank(from) + '1';
	if (cap != EMPTY)
		*p++ = 'x';
	*p++ = File(to) + 'a';
	*p++ = Rank(to) + '1';
	if (prom != EMPTY) {
		*p++ = '=';
		*p++ = PieceSAN(prom);
	}
	*p++ = '\0';

	return buf;
}

///////////////////////////////
// converts the SAN description of a move into the corresponding
// move_t value. returns zero on error.
//
// oh my this is ugly. maybe i should've just ripped off someone
// else's. hopefully it works properly...
///////////////////////////////
move_t
san2move(const position_t *pos, const char *san, int ply)
{
	char   buf[32];
	char   sq[3];
	char  *p, *s;
	uint8  from = INVALID_SQUARE, to = INVALID_SQUARE;
	uint8  pc = EMPTY, cap = EMPTY, prom = EMPTY;
	move_t flags = 0;
	uint8  stm = Stm(ply);
	int    rank, file;

	// work out of a temporary buffer
	strncpy(buf, san, 32);

	// we've already got constants for castling.
	if (!strcmp(buf, "O-O-O"))
		return stm == WHITE ? MOVE_WHITE_OOO : MOVE_BLACK_OOO;
	else if (!strcmp(buf, "O-O"))
		return stm == WHITE ? MOVE_WHITE_OO  : MOVE_BLACK_OO;

	// first task: strip useless annotations
	p = buf+strlen(buf)-1;
	while (p >= buf && (*p == '!' || *p == '?' || *p == '#' || *p == '+'))
		*p-- = '\0';

	// now grab the piece type from the beginning of the string;
	// if it's not a valid piece, it's a pawn.
	p = buf;
	pc = piece_from_san(*p++);
	if (pc == EMPTY)
		pc = MakePiece(PAWN, stm);
	else
		pc = MakePiece(pc, stm);

	// store the source square if we have one, for later.
	if (strlen(p) > 2) {
		s = sq;
		while ((*p >= 'a' && *p <= 'h') || (*p >= '1' && *p <= '8'))
			*s++ = *p++;
		*s++ = '\0';
	} else
		sq[0] = '\0';

	// check for a promotion
	p = buf+strlen(buf)-1;
	if (*(p-1) == '=') {
		prom = piece_from_san(*p);
		if (prom == EMPTY)
			return 0;
		prom = MakePiece(prom, stm);
		p -= 2;
	}

	// the two endmost characters are now the destination square
	rank = *p     - '1';
	file = *(p-1) - 'a';
	to   = (rank << 3) | file;
	if (to > 63)
		return 0;
	p -= 2;

	// store a capture if it was one.
	if (*p == 'x') {
		cap = PieceOn(to);
		p--;
	}

	// now, parse the source square we stripped earlier.
	if (strlen(sq) == 2) {
		file = sq[0] - 'a';
		rank = sq[1] - '1';
		from = (rank << 3) | file;
		goto finished;
	}

	// if it was a king, we already know everything.
	if (PieceType(pc) == KING) {
		from = KingSq(stm);
		goto finished;
	}

	// if it was a pawn, we just have to find the pawn behind the destination.
	if (PieceType(pc) == PAWN && cap == EMPTY) {
		bitboard_t pawns = Pawns(stm) & FileMask(File(to));
		if (!pawns)
			return 0;

		if (stm == WHITE) {
			while (pawns) {
				uint8 sq = poplsb(pawns);
				if (Rank(sq) > Rank(to))
					continue;
				from = sq;
			}
		} else {
			while (pawns) {
				uint8 sq = popmsb(pawns);
				if (Rank(sq) < Rank(to))
					continue;
				from = sq;
			}
		}
		if (from == INVALID_SQUARE)
			return 0;
		goto finished;
	}

	// we have to disambiguate. first, figure out the possible attackers.
	bitboard_t candidates, attackers;
	switch (PieceType(pc)) {
	case PAWN:   candidates = Pawns(stm);   break;
	case KNIGHT: candidates = Knights(stm); break;
	case BISHOP: candidates = Bishops(stm); break;
	case ROOK:   candidates = Rooks(stm);   break;
	case QUEEN:  candidates = Queens(stm);  break;
	default:     return 0;
	}
	attackers = attacks_to(pos, to) & candidates;

	// if there's only one of that piece type attacking the square, we're done.
	if (popcnt(attackers) == 1) {
		from = poplsb(attackers);
		goto finished;
	}

	// otherwise, search on the specified rank or file.
	if (sq[0] >= 'a' && sq[0] <= 'h') {
		file = sq[0] - 'a';
		attackers &= FileMask(file);
		if (popcnt(attackers) > 1)
			return 0;
		from = poplsb(attackers);
		goto finished;
	} else if (sq[0] >= '1' && sq[0] <= '8') {
		rank = sq[0] - '1';
		attackers &= RankMask(rank);
		if (popcnt(attackers) > 1)
			return 0;
		from = poplsb(attackers);
		goto finished;
	} else
		return 0;

finished:
	// set any extra flags, and do a little sanity checking
	if (PieceType(pc) == KING && Distance(from, to) == 2)
		flags = CASTLEMASK;
	else if (PieceType(pc) == PAWN) {
		if (Distance(from, to) == 2)
			flags = PAWNJUMPMASK;
		else if (abs(from - to) != 8 && PieceOn(to) == EMPTY) {
			if (to != EpSquare(ply))
				return 0;
			flags = ENPASSANTMASK;
			cap = stm == WHITE ? PieceOn(to - 8) : PieceOn(to + 8);
		}
	}

	if (PieceOn(from) != pc)
		return 0;

	return from | (to << 6) | (pc << 12) | (cap << 16) | (prom << 20) | flags;
}

///////////////////////////////
// prints a "pretty" ascii board. for my own debugging purposes.
///////////////////////////////
void
print_board(const position_t *pos)
{
	cout << "+---+---+---+---+---+---+---+---+" << endl;
	for (int rank = RANK8; rank >= RANK1; rank--) {
		for (int file = FILEA; file <= FILEH; file++) {
			int sq = (rank << 3) | file;
			int pc = pos->pieces[sq];
			if (pc != EMPTY)
				cout << "| " << PieceFEN(pc) << " ";
			else {
				cout << "| ";
				if (SquareColor(sq) == BLACK)
					cout << ".";
				else
					cout << " ";
				cout << " ";
			}
		}
		cout << "|" << endl;
		cout << "+---+---+---+---+---+---+---+---+" << endl;
	}
}

///////////////////////////////
// prints a "pretty" version of a bitboard. for debugging.
///////////////////////////////
void
print_bitboard(const bitboard_t bb)
{
	for (int rank = RANK8; rank >= RANK1; rank--) {
		for (int file = FILEA; file <= FILEH; file++) {
			int sq = (rank << 3)|file;
			if (bb & Mask(sq))
				cout << "X";
			else
				cout << "-";
		}
		cout << endl;
	}
}
