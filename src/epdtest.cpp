#include "benthos.h"

#include <fstream>
#include <vector>

///////////////////////////////
// as with perft, i've split the EPD testing suite into a separate
// program. duplicates some code, but generally makes life a bit simpler.
///////////////////////////////

void get_contents(void);
void run_tests(void);
void usage(void);

position_t *rootPosition;
state_t     states[MAXPLY];

int             moveTime = 0;
char           *epdFilename = NULL;
vector<string>  positions;
vector<string>  epdIds;
vector<move_t>  expectedMoves;

int
main(int argc, char *argv[])
{
	position_t *pos = (position_t *)malloc(sizeof(position_t));
	rootPosition = pos;

	init_bitboards();
	init_mersenne();
	init_zobrist();
	init_hash(33554432);
	init_search();

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-help"))
			usage();
		else if (!strcmp(argv[i], "-time")) {
			if (argc < i + 1)
				usage();
			moveTime = atoi(argv[++i]) * 1000;
		} else if (!strcmp(argv[i], "-file")) {
			if (argc < i + 1)
				usage();
			epdFilename = argv[++i];
		} else
			usage();
	}

	if (epdFilename == NULL)
		usage();

	if (moveTime == 0)
		moveTime = 10000;

	get_contents();
	run_tests();

	return 0;
}

void
get_contents(void)
{
	ifstream fin(epdFilename, ios::in);
	char buf[256];
	char *bmstr, *idstr, *p;
	char bm[12], id[128];
	int trim;
	move_t mv;

	if (!fin.is_open()) {
		cout << "Could not open input file: " << epdFilename << endl;
		exit(1);
	}

	// ugly, i'm sure there's a better way to do it, but meh.
	// java's string parsing spoiled me, too lazy to learn c++'s.
	while (fin.peek() != EOF) {
		fin.getline(buf, 256);

		bmstr = strstr(buf, " bm");
		if (bmstr == NULL) {
			cout << "No best move specified in line, skipping: " << buf << endl;
			continue;
		}

		idstr = strstr(bmstr, "; id");
		if (idstr == NULL) {
			cout << "No ID specified in line, skipping: " << buf << endl;
			continue;
		}

		bmstr += 4; // skip ' bm '
		idstr += 6; // skip '; id "'

		// for now, i only copy the first in a (possible) series of best moves
		p = bm;
		while (!isspace(*bmstr) && *bmstr != ';')
			*p++ = *bmstr++;
		*p = '\0';

		p = id;
		while (*idstr != '"')
			*p++ = *idstr++;
		*p = '\0';

		bmstr = strstr(buf, " bm");
		trim = strlen(buf) - strlen(bmstr);
		buf[trim] = '\0';

		position_from_fen(rootPosition, buf);
		mv = san2move(rootPosition, bm, 0);
		if (!mv) {
			cout << "Failed to parse best move of " << id << ", skipping: " << bm << endl;
			continue;
		}

		positions.push_back(string(buf));
		epdIds.push_back(string(id));
		expectedMoves.push_back(mv);
	}

	fin.close();
}

void
run_tests(void)
{
	char fen[256];
	vector<string> successes, failures;

	// tell report_search_info() to be quit
	suppressSearchStatus = true;

	if (positions.size() == 0) {
		cout << "No positions to test, quitting." << endl;
		exit(1);
	}

	cout << "Running all available tests, " << moveTime << "ms per move." << endl;
	for (uint32 i = 0; i < positions.size(); i++) {
		strcpy(fen, positions[i].c_str());
		move_t bmv = expectedMoves[i];

		cout << endl << "Testing " << epdIds[i] << ": " << fen << endl;
		position_from_fen(rootPosition, fen);
		searchInfo->endTime = clock() + moveTime;

		move_t found = search(rootPosition);
		if (found != bmv) {
			cout << "\tFailed; found move: " << move2san(found);
			cout << "; expected: " << move2san(bmv) << endl;
			cout << "\t\tint(found) = " << found << "; int(bmv) = " << bmv << endl;
			failures.push_back(epdIds[i]);
		} else {
			cout << "\tSucceeded; found move: " << move2san(found) << endl;
			successes.push_back(epdIds[i]);
		}
	}

	cout << endl << "End results:" << endl;
	cout << "\t" << successes.size() << " correct searches." << endl;
	cout << "\t" << failures.size()  << " incorrect searches." << endl;
}

void
usage(void)
{
	printf("usage: epdtest [-help] [-time <sec>] [-file <file.epd>]\n");
	printf("       -help: prints this.\n");
	printf("       -time: specifies the time allowed for the search. (default: 10s)\n");
	printf("       -file: specifies the file to read the EPD positions from.\n");
	exit(1);
}
