#include "benthos.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "search.h" // for MATE

///////////////////////////////
// implements the interface the UCI UI.
///////////////////////////////

static bool parse_command(char *);
static bool cmd_uci(const char *);
static bool cmd_isready(const char *);
static bool cmd_ucinewgame(const char *);
static bool cmd_position(const char *);
static bool cmd_go(const char *);
static bool cmd_stop(const char *);
static bool cmd_quit(const char *);

static bool cmd_material(const char *);
static bool cmd_fen(const char *);
static bool cmd_print(const char *);

static bool get_int_arg(const char *, const char *, int&);
static bool get_long_arg(const char *, const char *, long&);

typedef struct parser {
	char   *cmd;
	bool  (*pfunc)(const char *); // or deeper still, the mothership connection
} parser_t;

parser_t uci_parsers[] = {
	{ "uci",        cmd_uci },
	{ "isready",    cmd_isready },
	{ "ucinewgame", cmd_ucinewgame },
	{ "position",   cmd_position },
	{ "go",         cmd_go },
	{ "stop",       cmd_stop },
	{ "quit",       cmd_quit },

	{ "material",   cmd_material },
	{ "fen",        cmd_fen },
	{ "print",      cmd_print },

	{ 0,            NULL },
};

// used by epdtest to silence the search status report 
bool suppressSearchStatus = false;

///////////////////////////////
// the primary focal point of the program. reads input from the UI
// and passes it on to the command parser.
///////////////////////////////
void
ui_loop(void)
{
	char buf[2048];
	while (true) {
		cin.getline(buf, 2047);
		parse_command(buf);
		cout.flush();
	}
}

///////////////////////////////
// determines if there is currently input waiting on stdin.
// this is completely non-portable. i don't much care.
///////////////////////////////
bool
input_available(void)
{
	static fd_set readfds;
	static struct timeval timeout;

	FD_ZERO(&readfds);
	FD_SET(0, &readfds);
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	select(1, &readfds, NULL, NULL, &timeout);

	if (FD_ISSET(0, &readfds))
		return true;
	return false;
}

///////////////////////////////
// a "streamlined" version of parse_command, discards any input
// besides stop or quit. for use inside of the search loop
// whenever input is available.
//
// TODO: ponderhit
///////////////////////////////
void
parse_input_while_searching(void)
{
	static char buf[2048];
	cin.getline(buf, 2047);
	if (!strncmp(buf, "stop", 4))
		searchInfo->status = ABORTED;
	else if (!strncmp(buf, "quit", 4))
		exit(0);
}

///////////////////////////////
// sends the 'info' command to the UI, reporting the current
// search status.
///////////////////////////////
void
report_search_info(void)
{
	clock_t time = clock() - searchInfo->startTime;
	float nps = (float)searchInfo->nodes / (time / 1000);
	int rmn = searchInfo->curRootMoveNum;
	int score = searchInfo->bestRootScore;

	if (suppressSearchStatus || time < 1000)
		return;

	printf("info currmove %s currmovenumber %d\n",
			move2str(searchInfo->rootMoves[rmn].move), rmn + 1);
	printf("info depth %d ", searchInfo->depth);
	if (abs(score) < MATE - 200)
		printf("score cp %d ", score);
	else
		printf("score mate %d ", score > 0 ? (score - MATE + 1) / 2 : (score + MATE) / 2);
	printf("time %lu nodes %llu nps %.0f pv %s\n",
			time, searchInfo->nodes, nps, move2str(searchInfo->bestRootMove));
	fflush(stdout);
}

///////////////////////////////
// attempts to parse a command. returns non-zero on error.
///////////////////////////////
static bool
parse_command(char *cmdline)
{
	parser_t *p;
	char *cmd  = cmdline;
	char *args = strchr(cmdline, ' ');

	// trim the command and skip the space if we have arguments
	if (args != NULL) {
		int len = strlen(cmdline) - strlen(args);
		*(cmd+len) = '\0';
		args++;
	}

	// look for the parser for the command we received
	for (p = uci_parsers; p->cmd; p++)
		if (!strncmp(cmd, p->cmd, strlen(cmd)))
			return p->pfunc(args);

	// unknown command. whine about it.
	cout << "Error (unknown command): " << cmdline << endl;
	cout.flush();
	return false;
}

///////////////////////////////
// identifies ourselves to the ui
///////////////////////////////
static bool
cmd_uci(const char *args)
{
	cout << "id name " << ENGINE_NAME << " " << ENGINE_VERSION << endl;
	cout << "id author " << ENGINE_AUTHOR << endl;
	cout << "uciok" << endl;
	cout.flush();
	return true;
}

///////////////////////////////
// tells the UI we're ready.
///////////////////////////////
static bool
cmd_isready(const char *args)
{
	cout << "readyok" << endl;
	cout.flush();
	return true;
}

///////////////////////////////
// resets the board to the initial position suitable for starting
// a new game. this is actually useless, but just for the sake of it...
///////////////////////////////
static bool
cmd_ucinewgame(const char *args)
{
	position_from_fen(rootPosition, STARTING_FEN);
	history_new_game();
	return true;
}

///////////////////////////////
// sets up the current board position, clears all previous game
// history, and then plays any provided moves. starting over like
// this every move, while seemingly annoying, actually makes my
// life much easier, as i needn't bother with "undo" and such.
///////////////////////////////
static bool
cmd_position(const char *args)
{
	move_t mv;
	char *fen = strstr(args, "fen");
	char *moves = strstr(args, "moves");
	char move_text[10];
	char *p;

	if (fen != NULL) {
		if (moves != NULL) {
			int len = strlen(fen) - strlen(moves) - 1;
			*(fen+len) = '\0';
		}
		if (!position_from_fen(rootPosition, fen+4)) {
			rootPosition = NULL;
			cout << "Error: illegal position: " << fen+4 << endl;
			return false;
		}
	} else if (!strncmp(args, "startpos", 8)) {
		position_from_fen(rootPosition, STARTING_FEN);
	} else {
		cout << "Error: unrecognized position arguments: " << args << endl;
		return false;
	}

	history_new_game();
	if (moves == NULL)
		return false;

	moves += 5;
	while (*moves) {
		while (isspace(*moves)) moves++;
		p = move_text;
		while (*moves && !isspace(*moves))
			*p++ = *moves++;
		*p = '\0';

		mv = str2move(rootPosition, move_text);
		if (!mv) {
			cout << "Error: illegal move: " << move_text << endl;
			rootPosition = NULL;
			return false;
		}

		make_history_move(rootPosition, mv);
	}

	return true;
}

///////////////////////////////
// reads in the time controls (a hassle), stores the information
// into the search info structure, then calls the search.
///////////////////////////////
static bool
cmd_go(const char *args)
{
	int mtg = 0, depthmax = 0;
	int wtime = 0, btime = 0, winc = 0, binc = 0;
	long nodemax = 0, movetime = 0;
	int time, inc;

	if (rootPosition == NULL) {
		cout << "Invalid position: can't go." << endl;
		return false;
	}

	if (args == NULL || strstr(args, "infinite") != NULL)
		searchInfo->inf = true;
	else {
		searchInfo->inf = false;
		if (get_long_arg(args, "movetime", movetime))
			searchInfo->endTime = clock() + movetime;
		else {
			get_int_arg(args,  "wtime",     wtime);
			get_int_arg(args,  "btime",     btime);
			get_int_arg(args,  "winc",      winc);
			get_int_arg(args,  "binc",      binc);
			get_int_arg(args,  "movestogo", mtg);
			get_int_arg(args,  "depth",     depthmax);
			get_long_arg(args, "nodes",     nodemax);

			if (depthmax != 0)
				searchInfo->depthLimit = depthmax;
			if (nodemax != 0)
				searchInfo->nodeLimit = nodemax;

			// found this time calculation in scatha.
			// much better than what i came up with.
			if (Stm(0) == WHITE) {
				time = wtime;
				inc  = winc;
			} else {
				time = btime;
				inc  = binc;
			}

			if (mtg == 0) {
				if (inc != 0)
					time = time / 30 + inc;
				else
					time = time / 40;
			} else {
				if (mtg == 1)
					time = time / 2;
				else
					time = time / Min(mtg, 20);
			}

			searchInfo->endTime = clock() + time;
		}
	}

	move_t move = search(rootPosition);
	if (!move) {
		cout << "Error: Search failed to find a move..." << endl;
		return false;
	}

	make_history_move(rootPosition, move);
	cout << "bestmove " << move2str(move) << endl;
	cout.flush();
	return true;
}

///////////////////////////////
// tells the engine to stop ASAP, and give us the best
// move it's got. presumably this won't show up outside of
// parse_input_while_searching(), so there's nothing to do here.
///////////////////////////////
static bool
cmd_stop(const char *args)
{
	return true;
}

///////////////////////////////
// exits.
///////////////////////////////
static bool
cmd_quit(const char *args)
{
	exit(0);
}

///////////////////////////////
// helper function to extract arguments from the command line.
// returns true if the argument was found, false otherwise.
///////////////////////////////
static bool
get_int_arg(const char *line, const char *arg, int &val)
{
	char *p = strstr(line, arg);
	if (p == NULL) {
		val = 0;
		return false;
	}

	// conceivably, the argument isn't there and this goes awry.
	// oh well.
	p += strlen(arg) + 1;
	val = atoi(p);

	return true;
}

///////////////////////////////
// helper function to extract arguments from the command line.
// returns true if the argument was found, false otherwise.
///////////////////////////////
static bool
get_long_arg(const char *line, const char *arg, long &val)
{
	char *p = strstr(line, arg);
	if (p == NULL) {
		val = 0;
		return false;
	}

	p += strlen(arg) + 1;
	val = strtol(p, NULL, 10);

	return true;
}

static bool
cmd_material(const char *args)
{
	cout << "wmaterial = " << Material(0, WHITE) << endl;
	cout << "bmaterial = " << Material(0, BLACK) << endl;
	return true;
}

static bool
cmd_fen(const char *args)
{
	char buf[256];
	strcpy(buf, position_to_fen(rootPosition, 0));
	cout << buf << endl;
	return true;
}

static bool
cmd_print(const char *args)
{
	print_board(rootPosition);
	return true;
}
