#include "benthos.h"

///////////////////////////////
// lsb, poplsb, and the popcnt methods are in bitboard.h
///////////////////////////////

bitboard_t mask00L[64];
bitboard_t mask90L[64];
bitboard_t mask45L[64];
bitboard_t mask45R[64];

int        distances[64][64];
int        direction[64][64];
bitboard_t rays[64][64];

bitboard_t whitePawnAttacks[64];
bitboard_t blackPawnAttacks[64];
bitboard_t knightMoves[64];
bitboard_t kingMoves[64];
bitboard_t rookMoves00L[64][64];
bitboard_t rookMoves90L[64][64];
bitboard_t bishopMoves45L[64][64];
bitboard_t bishopMoves45R[64][64];

///////////////////////////////
// returns the provided bitboard rotated using the provided map
///////////////////////////////
static inline bitboard_t
rotate(const bitboard_t bb, const uint8 map[])
{
	bitboard_t rot = 0;
	for (int i = 0; i < 64; i++)
		if (bb & Mask(i))
			rot |= ULL(1) << map[i];
	return rot;
}

///////////////////////////////
// returns the provided bitboard rotated 90 degrees to the left
///////////////////////////////
bitboard_t
rotate90L(bitboard_t bb)
{
	return rotate(bb, rot90L);
}

///////////////////////////////
// returns the provided bitboard rotated 45 degrees to the left
///////////////////////////////
bitboard_t
rotate45L(bitboard_t bb)
{
	return rotate(bb, rot45L);
}

///////////////////////////////
// returns the provided bitboard rotated 45 degrees to the right
///////////////////////////////
bitboard_t
rotate45R(bitboard_t bb)
{
	return rotate(bb, rot45R);
}

///////////////////////////////
// fills in the array specifying the distance between two squares
///////////////////////////////
static void
init_distance(void)
{
	int src, dest, dist;
	for (src = 0; src < 64; src++) {
		for (dest = 0; dest < 64; dest++) {
			// chebyshev distance:
			dist = Max(abs(Rank(dest) - Rank(src)), abs(File(dest) - File(src)));
			Distance(src, dest) = dist;
		}
	}
}

///////////////////////////////
// fills in a bitboard attack map for the nonsliding pieces, based
// on the provided array of delta values
///////////////////////////////
static void
init_nonslide_map(bitboard_t maps[], int deltas[], int cnt)
{
	int src, dest;
	bitboard_t map;

	for (src = 0; src < 64; src++) {
		map = 0;
		for (int i = 0; i < cnt; i++) {
			dest = src + deltas[i];
			if (dest < 0 || dest > 63 || Distance(src, dest) > 2)
				continue;
			map |= Mask(dest);
		}
		maps[src] = map;
	}
}

///////////////////////////////
// helper function for determining the mobility of a sliding
// piece along a rank, given the contents of the rank
///////////////////////////////
static int
calc_slide(int src, int contents)
{
	int sq = src;
	int slide = 0;
	int mask;

	while (++sq < 8) {
		mask = 1 << sq;
		slide |= mask;
		if (contents & mask)
			break;
	}

	sq = src;
	while (--sq >= 0) {
		mask = 1 << sq;
		slide |= mask;
		if (contents & mask)
			break;
	}

	return slide;
}

// masks for clearing unnecessary bits on short diagonals.
static int diag_masks45L[64] = {
	 0x1,  0x3,  0x7,  0xf, 0x1f, 0x3f, 0x7f, 0xff,
	 0x3,  0x7,  0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x3f,
	 0x7,  0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f,
	 0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,
	0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,  0xf,
	0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,  0xf,  0x7,
	0x7f, 0xff, 0x7f, 0x3f, 0x1f,  0xf,  0x7,  0x3,
	0xff, 0x7f, 0x3f, 0x1f,  0xf,  0x7,  0x3,  0x1,
};
static int diag_masks45R[64] = {
	0xff, 0x7f, 0x3f, 0x1f,  0xf,  0x7,  0x3,  0x1,
	0x7f, 0xff, 0x7f, 0x3f, 0x1f,  0xf,  0x7,  0x3,
	0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,  0xf,  0x7,
	0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,  0xf,
	 0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,
	 0x7,  0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f,
	 0x3,  0x7,  0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x7f,
	 0x1,  0x3,  0x7,  0xf, 0x1f, 0x3f, 0x7f, 0xff,
};

///////////////////////////////
// initializes the sliding piece attack maps.
//
// this method of initialization was taken from King's Out
// by Brad Nürnberg; it's amazingly legible compared to most.
///////////////////////////////
static void
init_slide_maps(void)
{
	int sq, contents, middle;
	int src00L, src90L, src45L, src45R;
	bitboard_t map00L, map90L, map45L, map45R;

	for (sq = 0; sq < 64; sq++) {
		for (contents = 0; contents < 64; contents++) {
			// shift the contents into the middle
			middle = contents << 1;

			// find the source position in the rank
			src00L = sq - FirstInRank(sq);
			src90L = rot90L[sq] - (shift90L[sq] - 1);
			src45L = rot45L[sq] - (shift45L[sq] - 1);
			src45R = rot45R[sq] - (shift45R[sq] - 1);

			// determine slide mobility
			map00L = (bitboard_t)calc_slide(src00L, middle);
			map90L = (bitboard_t)calc_slide(src90L, middle);
			map45L = (bitboard_t)calc_slide(src45L, middle);
			map45R = (bitboard_t)calc_slide(src45R, middle);

			// remove extra bits from short diagonals
			map45L &= diag_masks45L[sq];
			map45R &= diag_masks45R[sq];

			// shift the map into place on the full bitboard
			map00L <<= FirstInRank(sq);
			map90L <<= shift90L[sq] - 1;
			map45L <<= shift45L[sq] - 1;
			map45R <<= shift45R[sq] - 1;

			// store, rotating the board back to normal where needed
			rookMoves00L[sq][contents]   = map00L;
			rookMoves90L[sq][contents]   = rotate(map90L, unrot90L);
			bishopMoves45L[sq][contents] = rotate(map45L, unrot45L);
			bishopMoves45R[sq][contents] = rotate(map45R, unrot45R);
		}
	}
}

static void
init_rays(void)
{
	int src, dest, sq, dir;
	bitboard_t mask;

	// fills in the direction between two squares
	// it's a little ugly, so shoot me...
	for (src = 0; src < 64; src++) {
		for (dest = 0; dest < 64; dest++) {
			if (src == dest)
				continue;

			dir = 0;
			if (src == 0 && dest == 63) // special cases prettier than more checks below
				dir = 9;
			else if (src == 63 && dest == 0)
				dir = -9;
			else if (Rank(src) == Rank(dest))
				dir = src < dest ? 1 : -1;
			else if (File(src) == File(dest))
				dir = src < dest ? 8 : -8;
			else if ((src - dest) % 7 == 0)
				dir = src < dest ? 7 : -7;
			else if ((src - dest) % 9 == 0)
				dir = src < dest ? 9 : -9;

			if (dir) {
				bool found = false;
				int sq = src + dir, lastsq = src;
				while (sq >= 0 && sq <= 63 && Distance(sq, lastsq) == 1) {
					if (sq == dest) {
						found = true;
						break;
					}
					lastsq = sq;
					sq += dir;
				}

				if (!found)
					dir = 0;
			}

			Direction(src, dest) = dir;
		}
	}

	// fills in the ray between two squares. endpoints are not included.
	for (src = 0; src < 64; src++) {
		for (dest = 0; dest < 64; dest++) {
			dir = Direction(src, dest);
			if (!dir) {
				RayBetween(src, dest) = 0;
				continue;
			}
			mask = 0;
			sq = src + dir;
			while (sq != dest) {
				mask |= Mask(sq);
				sq += dir;
			}
			RayBetween(src, dest) = mask;
		}
	}
}

///////////////////////////////
// performs the bitboard and attack map initialization
///////////////////////////////
void
init_bitboards(void)
{
  int wpawn_delta[]  = {  7,  9 };
  int bpawn_delta[]  = { -7, -9 };
  int knight_delta[] = { -17, -15, -10, -6, 6, 10, 15, 17 };
  int king_delta[]   = {  -9,  -8,  -7, -1, 1,  7,  8,  9 };

	// fill in the single bit mask arrays
	for (int i = 0; i < 64; i++) {
		mask00L[i] = ULL(1) << i;
		mask90L[i] = ULL(1) << rot90L[i];
		mask45L[i] = ULL(1) << rot45L[i];
		mask45R[i] = ULL(1) << rot45R[i];
	}

	// fills in the distance array
	init_distance();

	// generate attack maps for non-sliding pieces
	init_nonslide_map(whitePawnAttacks, wpawn_delta,  2);
	init_nonslide_map(blackPawnAttacks, bpawn_delta,  2);
	init_nonslide_map(knightMoves,      knight_delta, 8);
	init_nonslide_map(kingMoves,        king_delta,   8);

	// generate sliding attack maps
	init_slide_maps();

	// fills in the directional relation and "ray between" arrays
	init_rays();
}
