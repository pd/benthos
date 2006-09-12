#ifndef BENTHOS_H
#define BENTHOS_H

#include <algorithm>
#include <iostream>
#include <string>
#include <ctime>

#include "types.h"
#include "bitboard.h"

using namespace std;

#define ENGINE_NAME    "Benthos"
#define ENGINE_VERSION "0.1"
#define ENGINE_AUTHOR  "Phil O. Despotos"

#define MAXPLY 64
#define MAXGAMELENGTH 512
#define MOVESTACKSIZE 4096

#define STARTING_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define Max(a,b)   ((a) > (b) ? (a) : (b))
#define Min(a,b)   ((a) > (b) ? (b) : (a))

#define Mask(x)    (mask00L[x])
#define Mask90L(x) (mask90L[x])
#define Mask45L(x) (mask45L[x])
#define Mask45R(x) (mask45R[x])

///////////////////////////////
// PIECEMAX is larger than the actual number of types of pieces, as
// the values 4, 8 and 12 are left unused. this increases the size
// of things like the zobrist arrays, but encoding the color into the
// uint8 value of a piece seems to require this either way.
///////////////////////////////
#define COLORMAX 2
#define PIECEMAX 16
enum colors { WHITE, BLACK };
enum pieces {
  EMPTY   =  0,
  WPAWN   =  1, WKNIGHT =  2, WKING  =  3,
  WBISHOP =  5, WROOK   =  6, WQUEEN =  7,
  BPAWN   =  9, BKNIGHT = 10, BKING  = 11,
  BBISHOP = 13, BROOK   = 14, BQUEEN = 15
};
enum piece_types {
	PAWN   = 1, KNIGHT = 2, KING  = 3,
	BISHOP = 5, ROOK   = 6, QUEEN = 7
};

#define PieceType(pc)        ((pc) & 7)
#define PieceColor(pc)       ((pc) >> 3)
#define PieceValue(pc)       (pieceValues[pc])
#define PieceFEN(pc)         (fenChars[pc])
#define PieceSAN(pc)         (sanChars[pc])
#define MakePiece(pc, color) ((pc)|((color) << 3))
#define Slides(pc)           ((pc) & 4)
#define SlidesBishop(pc)     (((pc) & 5) == 5)
#define SlidesRook(pc)       (((pc) & 6) == 6)

///////////////////////////////
// squares are number 0..63, where A1 = 0, A8 = 7, and H8 = 63.
// 64 is used as a constant for an invalid square.
///////////////////////////////
#define SQUAREMAX 64
enum squares {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	INVALID_SQUARE
};
enum files { FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH };
enum ranks { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 };

enum directions {
	DIRW = -1, DIRNW =  7, DIRN =  8, DIRNE =  9,
	DIRE =  1, DIRSE = -7, DIRS = -8, DIRSW = -9
};

#define File(sq)        ((sq) & 7)
#define Rank(sq)        ((sq) >> 3)
#define FirstInRank(sq) ((sq) & 0x38)
#define SquareColor(sq) ((((sq) ^ ((sq) >> 3)) & 1) ^ 1)
#define FileMask(f)     (fileMasks[f])
#define RankMask(r)     (rankMasks[r])
#define SquareName(sq)  (squareNames[sq])
#define Distance(a,b)   (distances[a][b])
#define Direction(a,b)  (direction[a][b])
#define RayBetween(a,b) (rays[a][b])

///////////////////////////////
// moves are represented with a 32-bit int, encoded as follows:
// 0000 0000 0000 0000 0000 0000 0011 1111 source square           [0..5]
// 0000 0000 0000 0000 0000 1111 1100 0000 destination square      [6..11]
// 0000 0000 0000 0000 1111 0000 0000 0000 moving piece            [12..15]
// 0000 0000 0000 1111 0000 0000 0000 0000 captured piece          [16..19]
// 0000 0000 1111 0000 0000 0000 0000 0000 promotion piece         [20..23]
// 0000 0001 0000 0000 0000 0000 0000 0000 pawn jump flag          [24]
// 0000 0010 0000 0000 0000 0000 0000 0000 castle flag             [25]
// 0000 0100 0000 0000 0000 0000 0000 0000 en passant capture flag [26]
///////////////////////////////
#define From(mv)        (((mv)      ) & 0x3f)
#define To(mv)          (((mv) >>  6) & 0x3f)
#define Piece(mv)       (((mv) >> 12) & 0xf)
#define Capture(mv)     (((mv) >> 16) & 0xf)
#define Promote(mv)     (((mv) >> 20) & 0xf)
#define IsPawnJump(mv)  ((mv) & PAWNJUMPMASK)
#define IsCastle(mv)    ((mv) & CASTLEMASK)
#define IsEnPassant(mv) ((mv) & ENPASSANTMASK)

#define PAWNJUMPMASK    0x1000000
#define CASTLEMASK      0x2000000
#define ENPASSANTMASK   0x4000000

// no reason to calculate these if we already know the value
#define MOVE_WHITE_OO   0x2003184
#define MOVE_WHITE_OOO  0x2003084
#define MOVE_BLACK_OO   0x200bfbc
#define MOVE_BLACK_OOO  0x200bebc

///////////////////////////////
// castling status is represented with an 8-bit int.
// the first four bits are used for castling availability (not legality),
// the final four bits are used for castling history.
///////////////////////////////
#define WHITE_CAN_CASTLE     0x03
#define BLACK_CAN_CASTLE     0x0c
#define WHITE_CAN_CASTLE_KS  0x01
#define WHITE_CAN_CASTLE_QS  0x02
#define BLACK_CAN_CASTLE_KS  0x04
#define BLACK_CAN_CASTLE_QS  0x08
#define WHITE_HAS_CASTLED_KS 0x10
#define WHITE_HAS_CASTLED_QS 0x20
#define BLACK_HAS_CASTLED_KS 0x40
#define BLACK_HAS_CASTLED_QS 0x80

///////////////////////////////
// masks for squares that need to be unoccupied to castle
///////////////////////////////
#define CASTLE_UNOCC_WKS (ULL(0x60))
#define CASTLE_UNOCC_WQS (ULL(0xe))
#define CASTLE_UNOCC_BKS (ULL(0x6000000000000000))
#define CASTLE_UNOCC_BQS (ULL(0x0e00000000000000))

///////////////////////////////
// these are macros to clean up some of the repetitive, verbose code
// that shows up throughout manipulation of the position_t and state_t
// structs. for many, you need a position_t *pos in scope.
//
// i found this style of improving readability in crafty and just
// adapted it to my structure.
///////////////////////////////
#define Pieces(stm)  (pos->occ[stm])
#define Pawns(stm)   (pos->pawns[stm])
#define Knights(stm) (pos->knights[stm])
#define Bishops(stm) (pos->bishops[stm])
#define Rooks(stm)   (pos->rooks[stm])
#define Queens(stm)  (pos->queens[stm])
#define Kings(stm)   (pos->kings[stm])
#define KingSq(stm)  (pos->kingSq[stm])

#define Occupied     (pos->occupied)
#define Occupied90L  (pos->occupied90L)
#define Occupied45L  (pos->occupied45L)
#define Occupied45R  (pos->occupied45R)
#define PieceOn(sq)  (pos->pieces[sq])

#define Stm(ply)             (states[ply].stm)
#define EpSquare(ply)        (states[ply].epSquare)
#define Castling(ply)        (states[ply].castling)
#define HalfmoveClock(ply)   (states[ply].halfmoveClock)
#define Material(ply, stm)   (states[ply].material[stm])
#define PawnCount(ply, stm)  (states[ply].pawnCount[stm])
#define MinorCount(ply, stm) (states[ply].minorCount[stm])
#define MajorCount(ply, stm) (states[ply].majorCount[stm])
#define HashKey(ply)         (states[ply].hashKey)
#define PawnHashKey(stm)     (states[ply].pawnHashKey)

///////////////////////////////
// bitboard move map macros. sliders require position_t *pos in scope
///////////////////////////////
#define WhitePawnAttacks(sq) (whitePawnAttacks[sq])
#define BlackPawnAttacks(sq) (blackPawnAttacks[sq])
#define KnightMoves(sq)      (knightMoves[sq])
#define KingMoves(sq)        (kingMoves[sq])
#define RookMoves(sq)        (RankMoves(sq) | FileMoves(sq))
#define BishopMoves(sq)      (DiagMovesA8H1(sq) | DiagMovesA1H8(sq))
#define QueenMoves(sq)       (RookMoves(sq) | BishopMoves(sq))
#define RankMoves(sq)        (rookMoves00L[sq][(Occupied >> (FirstInRank(sq) + 1) & 0x3f)])
#define FileMoves(sq)        (rookMoves90L[sq][(Occupied90L >> shift90L[sq]) & 0x3f])
#define DiagMovesA8H1(sq)    (bishopMoves45L[sq][(Occupied45L >> shift45L[sq]) & 0x3f])
#define DiagMovesA1H8(sq)    (bishopMoves45R[sq][(Occupied45R >> shift45R[sq]) & 0x3f])

#define WhiteAttacking(sq) (white_attacking(pos, (sq)))
#define BlackAttacking(sq) (black_attacking(pos, (sq)))
#define AttackedBy(stm,sq) ((stm) ? BlackAttacking(sq) : WhiteAttacking(sq))
#define Checked(stm)       ((stm) ? WhiteAttacking(KingSq(BLACK)) : BlackAttacking(KingSq(WHITE)))

///////////////////////////////
// determines castling legality.
///////////////////////////////
#define CanWhiteKS(ply)                           \
	((Castling(ply) & WHITE_CAN_CASTLE_KS)          \
	 && !(Occupied & CASTLE_UNOCC_WKS)              \
	 && !BlackAttacking(E1)                         \
	 && !BlackAttacking(F1) && !BlackAttacking(G1))
#define CanWhiteQS(ply)                           \
	((Castling(ply) & WHITE_CAN_CASTLE_QS)          \
	 && !(Occupied & CASTLE_UNOCC_WQS)              \
	 && !BlackAttacking(E1)                         \
	 && !BlackAttacking(D1) && !BlackAttacking(C1))
#define CanBlackKS(ply)                           \
	((Castling(ply) & BLACK_CAN_CASTLE_KS)          \
	 && !(Occupied & CASTLE_UNOCC_BKS)              \
	 && !WhiteAttacking(E8)                         \
	 && !WhiteAttacking(F8) && !WhiteAttacking(G8))
#define CanBlackQS(ply)                           \
	((Castling(ply) & BLACK_CAN_CASTLE_QS)          \
	 && !(Occupied & CASTLE_UNOCC_BQS)              \
	 && !WhiteAttacking(E8)                         \
	 && !WhiteAttacking(D8) && !WhiteAttacking(C8))

///////////////////////////////
// zobrist key access macros. it's just a little prettier.
///////////////////////////////
#define Zobrist(pc, sq)     (zobrist[pc][sq])
#define ZobristCastling(cr) (zobristCastling[((cr) & 0xf)])
#define ZobristEp(sq)       (zobristEp[sq])
#define ZobristStm          (zobristStm)

///////////////////////////////
// constants for the hash table entries
///////////////////////////////
#define HASH_VAL_UNKNOWN 0x0
#define ALPHA 0x1
#define BETA  0x2
#define EXACT 0x4

///////////////////////////////
// search status.
///////////////////////////////
enum search_status { IDLE, THINKING, ABORTED };

///////////////////////////////
// the position structure is kept minimal, as it must be updated
// by both make_move and unmake_move. anything which can simply be
// "rolled back" should probably be kept in the state stack.
///////////////////////////////
typedef struct position {
	bitboard_t occupied, occupied90L, occupied45L, occupied45R;
	bitboard_t occ[2];
	bitboard_t pawns[2];
	bitboard_t knights[2];
	bitboard_t bishops[2];
	bitboard_t rooks[2];
	bitboard_t queens[2];
	bitboard_t kings[2];
	square_t   kingSq[2];
	piece_t    pieces[64];
} position_t;

///////////////////////////////
// defines the various state-related qualities of the board at a
// particular ply.
///////////////////////////////
typedef struct state {
	uint8     stm;
	uint8     epSquare;
	uint8     castling;
	uint8     halfmoveClock;
	int       material[2];
	uint8     pawnCount[2];
	uint8     minorCount[2];
	uint8     majorCount[2];
	hashkey_t hashKey;
	hashkey_t pawnHashKey;
} state_t;

///////////////////////////////
// used for easy checking of the draw by repetition rule.
// maybe more if i can find a use for it.
///////////////////////////////
typedef struct history {
	hashkey_t hashKey;
	uint8     halfmoveClock;
	move_t    prevMove;
} history_t;

///////////////////////////////
// used to form the move stack, so that a separate array isn't
// necessary in order to store the scores when sorting
///////////////////////////////
typedef struct scored_move {
	move_t move;
	int score;
} scored_move_t;

///////////////////////////////
// stores information that needs to be reinitialized before each
// search, such as the list of hash keys along the current line
// for threefold repetition detection, killer and history heuristic
// data, etc.
///////////////////////////////
typedef struct search_info {
	// general status
	int    status;
	int    depth;
	int    seldepth;
	uint64 nodes;

	// time control
	bool    inf;
	int     depthLimit;
	uint64  nodeLimit;
	clock_t startTime;
	clock_t endTime;

	// root move list
	scored_move_t rootMoves[64];    // maybe oversized, but just in case.
	uint8         rootMoveCount;
	uint8         curRootMoveNum;

	// best root move known
	move_t    bestRootMove;
	int       bestRootScore;
	int       matesFound;           // search is stopped early if this >= 2

	// for threefold repetition
	int       keyidx;               // the number of keys in keylog[] _prior_ to the search
	                                // thus, keyidx+ply = where to store the key for a new node
	hashkey_t keyLog[MAXPLY + 100]; // 100 = maximal hmclock size; see rep detection code
} search_info_t;

///////////////////////////////
// an entry in the hash table. this could be further condensed, as
// some other engines do. perhaps in the future. TODO
///////////////////////////////
typedef struct hash_entry {
	hashkey_t key;
	int       depth;
	int       type;
	int       score;
	move_t    move;
	int       seq;
} hash_entry_t;

///////////////////////////////
// an entry in the pawn hash table. this needs to be expanded. TODO
///////////////////////////////
typedef struct phash_entry {
	hashkey_t key;
	int       score;
} phash_entry_t;

// externs
// bitboard.cpp:
extern bitboard_t        mask00L[64];
extern bitboard_t        mask90L[64];
extern bitboard_t        mask45L[64];
extern bitboard_t        mask45R[64];
extern int               distances[64][64];
extern int               direction[64][64];
extern bitboard_t        rays[64][64];
extern bitboard_t        whitePawnAttacks[64];
extern bitboard_t        blackPawnAttacks[64];
extern bitboard_t        knightMoves[64];
extern bitboard_t        kingMoves[64];
extern bitboard_t        rookMoves00L[64][64];
extern bitboard_t        rookMoves90L[64][64];
extern bitboard_t        bishopMoves45L[64][64];
extern bitboard_t        bishopMoves45R[64][64];
// data.cpp:
extern const bitboard_t  fileMasks[8];
extern const bitboard_t  rankMasks[8];
extern const char       *squareNames[65];
extern const int         pieceValues[16];
extern const char        fenChars[16];
extern const char        sanChars[16];
// hash.cpp:
extern hash_entry_t     *hashTable;
extern int               hashMaxEntries;
// history.cpp:
extern history_t         history[MAXGAMELENGTH];
extern int               currentGamePly;
// main.cpp:
extern position_t       *rootPosition;
extern state_t           states[MAXPLY];
extern int               currentPly;
// search.cpp:
extern search_info_t    *searchInfo;
// ui.cpp:
extern bool              suppressSearchStatus;
// zobrist.cpp:
extern hashkey_t         zobrist[16][64];
extern hashkey_t         zobristCastling[16];
extern hashkey_t         zobristEp[64];
extern hashkey_t         zobristStm;

// prototypes
// attacks.cpp:
uint64         attacks_to(const position_t *, uint8);
bool           white_attacking(const position_t *, uint8);
bool           black_attacking(const position_t *, uint8);
// bitboard.cpp:
bitboard_t     rotate90L(bitboard_t);
bitboard_t     rotate45L(bitboard_t);
bitboard_t     rotate45R(bitboard_t);
void           init_bitboards(void);
// eval.cpp:
int            eval(const position_t *, int);
// hash.cpp:
void           init_hash(int);
void           clear_hash();
int            probe_hash(hashkey_t, int, int, int, move_t *);
void           store_hash(hashkey_t, int, int, int, move_t);
// history.cpp:
void           reset_history(void);
void           history_new_game(void);
void           make_history_move(position_t *pos, move_t move);
// make.cpp:
void           make_move(position_t *, move_t, int);
void           unmake_move(position_t *, move_t, int);
// mersenne.cpp:
uint32         genrand_int32(void);
uint64         genrand_int64(void);
void           init_mersenne(void);
// movegen.cpp:
scored_move_t *generate_captures(const position_t *, scored_move_t *, int);
scored_move_t *generate_noncaptures(const position_t *, scored_move_t *, int);
scored_move_t *generate_evasions(const position_t *, scored_move_t *, int);
// position.cpp:
void           clear_position(void);
void           reset_state(int);
bool           position_from_fen(position_t *, char *);
char          *position_to_fen(const position_t *, int);
// search.cpp:
move_t         search(position_t *);
void           init_search(void);
// ui.cpp:
void           ui_loop(void);
void           parse_input_while_searching(void);
void           report_search_info(void);
bool           input_available(void);
// util.cpp:
char          *move2str(move_t);
move_t         str2move(const position_t *, const char *);
char          *move2san(move_t);
move_t         san2move(const position_t *, const char *, int);
void           print_board(const position_t *);
void           print_bitboard(const bitboard_t);
// zobrist.cpp:
void           calculate_hash_keys(const position_t *, int);
void           init_zobrist(void);

#endif // !defined(BENTHOS_H)
