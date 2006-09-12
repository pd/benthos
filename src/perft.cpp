#include "benthos.h"

///////////////////////////////
// for my own sanity, perft runs as a completely separate program.
// i shouldn't ever actually need the code in the engine itself, and
// i prefer it to just be a self-sufficient test suite to make sure
// i haven't broken my move generation code.
//
// the positions for testing were taken from:
//   1) crbmg, a clean and very legible perfting c++ program by Sune Fischer.
//   2) scharnagl's site chessbox.de, where he has 5 positions (smirf1-5)
//      with (presumably) correct answers
//
// the perft code itself is largely crafty's.
///////////////////////////////

void perft(position_t *, int);
void do_perft(position_t *, scored_move_t *, int, int);
void test_all(position_t *);
void test(position_t *, char *, int);
void usage(void);

position_t *rootPosition;
state_t     states[MAXPLY];
int         iterate = 1;
uint64      total_moves;

#define KNOWN_POSITIONS 11

// the positions available for the tests
char *positions[KNOWN_POSITIONS] = {
	// starting
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	// mixed
	"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
	// castle
	"r3k2r/3q4/2n1b3/7n/1bB5/2N2N2/1B2Q3/R3K2R w KQkq - 0 1",
	// ep
	"rnbq1bnr/1pppkp1p/4p3/2P1P3/p5p1/8/PP1PKPPP/RNBQ1BNR w - - 0 1",
	// promotion
	"rn1q1bnr/1bP1kp1P/1p2p3/p7/8/8/PP1pKPpP/RNBQ1BNR w - - 0 1",
	// minimal
	"r6r/3qk3/2n1b3/7n/1bB5/2N2N2/1B2QK2/R6R w - - 0 1",
	// smirf1
	"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 25",
	// smirf2
	"8/PPP4k/8/8/8/8/4Kppp/8 w - - 0 1",
	// smirf3
	"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
	// smirf4
	"8/3K4/2p5/p2b2r1/5k2/8/8/1q6 b - - 1 67",
	// smirf5
	"8/7p/p5pb/4k3/P1pPn3/8/P5PP/1rB2RK1 b - d3 0 28",
	// sharper1
};
// the names of each position
char *names[KNOWN_POSITIONS] = {
	"starting",
	"mixed",
	"castle",
	"ep",
	"promotion",
	"minimal",
	"smirf1",
	"smirf2",
	"smirf3",
	"smirf4",
	"smirf5",
};
// first value is the maximum depth to which we will search in an
// automated search; the second value is the maximum depth to which
// we know valid results
int depths[KNOWN_POSITIONS][2] = {
	// starting
	{ 6, 9 },
	// mixed
	{ 5, 6 },
	// castle
	{ 5, 6 },
	// ep
	{ 6, 6 },
	// promotion
	{ 5, 6 },
	// minimal
	{ 5, 5 },
	// smirf1
	{ 5, 6 },
	// smirf2
	{ 6, 8 },
	// smirf3
	{ 7, 8 },
	// smirf4
	{ 6, 8 },
	// smirf5
	{ 6, 7 },
};
// the expected results for each depth of each position.
uint64 expected_results[KNOWN_POSITIONS][9] = {
	// starting
	{
		ULL(20),        ULL(400),         ULL(8902),       ULL(197281),
		ULL(4865609),   ULL(119060324),   ULL(3195901860), ULL(84998978956),
		ULL(2439530234167),
	},
	// mixed
	{
		ULL(48),        ULL(2039),        ULL(97862),      ULL(4085603),
		ULL(193690690), ULL(8031647685),  ULL(0),          ULL(0),
		ULL(0),
	},
	// castle
	{
		ULL(47),        ULL(2409),        ULL(111695),     ULL(5664262),
		ULL(269506799), ULL(13523904666), ULL(0),          ULL(0),
		ULL(0),
	},
	// ep
	{
		ULL(22),        ULL(491),         ULL(12571),      ULL(295376),
		ULL(8296614),   ULL(205958173),   ULL(0),          ULL(0),
		ULL(0),
	},
	// promotion
	{
		ULL(37),         ULL(1492),       ULL(48572),      ULL(2010006),
		ULL(67867493),   ULL(2847190653), ULL(0),          ULL(0),
		ULL(0),
	},
	// minimal
	{
		ULL(62),         ULL(3225),       ULL(176531),     ULL(8773247),
		ULL(461252378),  ULL(0),          ULL(0),          ULL(0),
		ULL(0),
	},
	// smirf1
	{
		ULL(48),         ULL(2039),       ULL(97862),      ULL(4085603),
		ULL(193690690),  ULL(8031647685), ULL(0),          ULL(0),
		ULL(0),
	},
	// smirf2
	{
		ULL(18),         ULL(290),        ULL(5044),       ULL(89363),
		ULL(1745545),    ULL(34336777),   ULL(749660761),  ULL(16303466487),
		ULL(0),
	},
	// smirf3
	{
		ULL(14),         ULL(191),        ULL(2812),       ULL(43238),
		ULL(674624),     ULL(11030083),   ULL(178633661),  ULL(3009794393),
		ULL(0),
	},
	// smirf4
	{
		ULL(50),         ULL(279),        ULL(13310),      ULL(54703),
		ULL(2538084),    ULL(10809689),   ULL(493407574),  ULL(2074492344),
		ULL(0),
	},
	// smirf5
	{
		ULL(5),          ULL(117),        ULL(3293),       ULL(67197),
		ULL(1881089),    ULL(38633283),   ULL(1069189070), ULL(22488501780),
		ULL(0),
	},
};

int
main(int argc, char *argv[])
{
	position_t *pos = (position_t *)malloc(sizeof(position_t));
	rootPosition = pos;

	init_bitboards();
	init_mersenne();
	init_zobrist();

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-help"))
			usage();
		else if (!strcmp(argv[i], "-noit"))
			iterate = 0;
		else if (!strcmp(argv[i], "-all"))
			test_all(pos);
		else if (!strcmp(argv[i], "-test")) {
			if (argc <= i + 2)
				usage();
			test(pos, argv[i+1], atoi(argv[i+2]));
			exit(0);
		} else
			usage();
	}

	if (argc == 1)
		test(pos, names[0], depths[0][0]);

	return 0;
}

void
test_all(position_t *pos)
{
	int i;

	cout << "Testing all known positions." << endl << endl;
	for (i = 0; i < KNOWN_POSITIONS; i++) {
		test(pos, names[i], depths[i][0]);
		cout << endl;
	}
}

void
test(position_t *pos, char *name, int max_depth)
{
	char *fen = NULL;
	uint64 *expected = NULL;
	clock_t start_time, end_time;
	double time_used;
	int depth = iterate ? 1 : max_depth;
	int i;

	for (i = 0; i < KNOWN_POSITIONS; i++)
		if (!strcmp(names[i], name)) {
			fen = positions[i];
			expected = expected_results[i];
			break;
		}

	if (fen == NULL) {
		cout << "Unknown position: " << name << endl;
		usage();
	}

	position_from_fen(pos, fen);
	cout << "'" << name << "' test: " << fen << endl;

	for (; depth <= max_depth; depth++) {
		start_time = clock();
		perft(pos, depth);
		end_time = clock();
		time_used = ((double)end_time - (double)start_time) / 1000;

		// if we have something to match again, see if we were right
		printf("depth %d: %12llu [%6.2f secs - %8.0f nps]", depth, total_moves,
				time_used, (total_moves / time_used));
		if (depth <= depths[i][1]) {
			if (expected[depth - 1] == total_moves)
				printf(" [correct]\n");
			else
				printf(" [incorrect: expected %llu]\n", expected[depth - 1]);
		} else
			printf(" [unknown validity]\n");
	}
}

void
perft(position_t *pos, int depth)
{
	scored_move_t moveStack[4096];
	memset(moveStack, 0, sizeof(scored_move_t)*4096);

	total_moves = 0;
	states[1] = states[0];
	do_perft(pos, moveStack, 1, depth);
}

void
do_perft(position_t *pos, scored_move_t *ms, int ply, int depth)
{
	scored_move_t *msbase = ms;
	uint8 stm = Stm(ply);
	scored_move_t *mv;

	if (Checked(stm^1))
		return;

	if (Checked(stm)) {
		ms = generate_evasions(pos, ms, ply);
		for (mv = msbase; mv < ms; mv++)
			if (PieceType(Capture(mv->move)) == KING)
				return;
	} else {
		ms = generate_captures(pos, ms, ply);
		for (mv = msbase; mv < ms; mv++)
			if (PieceType(Capture(mv->move)) == KING)
				return;
		ms = generate_noncaptures(pos, ms, ply);
	}

	for (mv = msbase; mv < ms; mv++) {
		make_move(pos, mv->move, ply);
		if (depth - 1)
			do_perft(pos, ms, ply + 1, depth - 1);
		else if (!Checked(stm))
			total_moves++;
		unmake_move(pos, mv->move, ply);
	}
}

void
usage(void)
{
	printf("usage: perft [-help|-noit] [-test <name> <depth>|-all]\n");
	printf("       -help: prints this.\n");
	printf("       -noit: disables \"iterative\" testing, which starts over for each depth.\n");
	printf("       -test: runs a perft on position <name> to depth <depth>.\n");
	printf("       -all : runs a perft on all available positions to default depth.\n\n");

	printf("available positions:\n");
	printf("  format: <name> (<default depth>): <fen>\n");
	for (int i = 0; i < KNOWN_POSITIONS; i++)
		printf("%15s (%d): %s\n", names[i], depths[i][0], positions[i]);

	exit(1);
}
