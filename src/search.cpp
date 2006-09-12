#include "benthos.h"
#include "search.h"

//#define DEBUG

///////////////////////////////
// the heart of the engine.
//
// TODO: besides "everything", time controls should be implemented
// ASAP, to make iterative deepening+hash table worth doing.
///////////////////////////////

static void search_root(position_t *, scored_move_t *, int, int, int);
static int  alphabeta(position_t *, scored_move_t *, int, int, int, int);
static int  compare_moves(const void *, const void *);
static bool is_search_draw(int);
static void new_search(void);
static inline bool should_stop(void);

search_info_t *searchInfo = NULL;

///////////////////////////////
// just allocates memory for the search info structure
///////////////////////////////
void
init_search(void)
{
	searchInfo = (search_info_t *)malloc(sizeof(search_info_t));
	memset(searchInfo, 0, sizeof(search_info_t));
}

///////////////////////////////
// the entry point to the search. prepares everything that needs to
// be done, and then starts the search.
//
// the search assumes that it is starting at ply zero of the state
// stack, so everything needs to be ready there before a search is
// started.
///////////////////////////////
move_t
search(position_t *pos)
{
	int depth = 0;
	scored_move_t moveStack[2048];
	scored_move_t *rms = searchInfo->rootMoves;

	new_search();

	// generate root move list
	if (!Checked(Stm(0))) {
		rms = generate_captures(pos, rms, 0);
		rms = generate_noncaptures(pos, rms, 0);
	} else
		rms = generate_evasions(pos, rms, 0);
	searchInfo->rootMoveCount = rms - searchInfo->rootMoves;

	// iterative deepening
	while (!should_stop()) {
		searchInfo->depth = ++depth;
		search_root(pos, moveStack, -INFINITY, INFINITY, depth);
		report_search_info();
		if (searchInfo->matesFound >= 2)
			break;
		qsort(searchInfo->rootMoves, searchInfo->rootMoveCount, sizeof(scored_move_t), compare_moves);
	}

	return searchInfo->bestRootMove;
}

///////////////////////////////
// the alpha-beta method used for the root node. separated since it
// can't use things like the hash table, it has to keep track of the
// best root move so far, etc.
///////////////////////////////
static void
search_root(position_t *pos, scored_move_t *ms, int alpha, int beta, int depth)
{
	uint8 stm = Stm(0);
	int val;
	searchInfo->bestRootScore = alpha;

	for (int i = 0; i < searchInfo->rootMoveCount; i++) {
		// skip moves previously marked as illegal
		if (!searchInfo->rootMoves[i].move)
			continue;

		if (should_stop())
			break;

		searchInfo->curRootMoveNum = i;

		// clear illegal moves so we don't bother with them again
		make_move(pos, searchInfo->rootMoves[i].move, 0);
		if (depth == 1 && Checked(stm)) {
			unmake_move(pos, searchInfo->rootMoves[i].move, 0);
			searchInfo->rootMoves[i].move = 0;
			searchInfo->rootMoves[i].score = -INFINITY;
			continue;
		}

		// store the key in the array for checking threefold repetition
		searchInfo->keyLog[searchInfo->keyidx] = HashKey(1);

		// go into normal alpha beta for search ply 1
		val = -alphabeta(pos, ms, -beta, -alpha, 1, depth - 1);
		unmake_move(pos, searchInfo->rootMoves[i].move, 0);

		searchInfo->rootMoves[i].score = val;
		if (val > searchInfo->bestRootScore) {
			searchInfo->bestRootScore = val;
			searchInfo->bestRootMove = searchInfo->rootMoves[i].move;
		}
		if (val > MATE)
			searchInfo->matesFound++;
	}
}

///////////////////////////////
// the alpha-beta method used for all nodes beyond the root.
//
// sply  = the current _search_ ply (root = 0, moves from root -> 1, etc)
// depth = the depth remaining for the search
///////////////////////////////
static int
alphabeta(position_t *pos, scored_move_t *ms, int alpha, int beta, int sply, int depth)
{
	scored_move_t *msbase = ms;
	scored_move_t *mv;
	uint8 stm = Stm(sply);
	move_t bestMove = 0, hashMove = 0;
	hashkey_t hashKey = HashKey(sply);
	int hashScoreType = ALPHA;
	int val, legals = 0;

	searchInfo->nodes++;

	if (depth == 0)
		return eval(pos, sply);

	if (is_search_draw(sply))
		return 0;

	if (should_stop())
		return alpha;

	val = probe_hash(hashKey, depth, alpha, beta, &hashMove);
	if (val != HASH_VAL_UNKNOWN)
		return val;

	if (hashMove) {
		make_move(pos, hashMove, sply);
		if (!Checked(stm)) {
			legals++;
			searchInfo->keyLog[searchInfo->keyidx + sply] = HashKey(sply + 1);
			val = -alphabeta(pos, ms, -beta, -alpha, sply + 1, depth - 1);
			unmake_move(pos, hashMove, sply);
			if (val >= beta) {
				store_hash(hashKey, depth, BETA, beta, hashMove);
				return beta;
			}
			if (val > alpha) {
				bestMove = hashMove;
				hashScoreType = EXACT;
				alpha = val;
			}
		} else
			unmake_move(pos, hashMove, sply);
	}

	if (!Checked(stm)) {
		ms = generate_captures(pos, ms, sply);
		ms = generate_noncaptures(pos, ms, sply);
	} else
		ms = generate_evasions(pos, ms, sply);

	for (mv = msbase; mv < ms; mv++) {
		if (mv->move == hashMove)
			continue;

		make_move(pos, mv->move, sply);
		if (!Checked(stm)) {
			legals++;
			searchInfo->keyLog[searchInfo->keyidx + sply] = HashKey(sply + 1);
			val = -alphabeta(pos, ms, -beta, -alpha, sply + 1, depth - 1);
			unmake_move(pos, mv->move, sply);
			if (val >= beta) {
				store_hash(hashKey, depth, beta, BETA, mv->move);
				return beta;
			}
			if (val > alpha) {
				bestMove = mv->move;
				hashScoreType = EXACT;
				alpha = val;
			}
		} else
			unmake_move(pos, mv->move, sply);
	}

	if (!legals) {
		if (Checked(stm))
			return -MATE-sply;
		else
			return 0;
	}

	store_hash(hashKey, depth, hashScoreType, alpha, bestMove);
	return alpha;
}

///////////////////////////////
// compares two scored_move_t values for qsort()
///////////////////////////////
static int
compare_moves(const void *a, const void *b)
{
	scored_move_t *m1 = (scored_move_t *)a;
	scored_move_t *m2 = (scored_move_t *)b;
	if (m1->score > m2->score)
		return -1;
	if (m1->score == m2->score)
		return 0;
	return 1;
}

///////////////////////////////
// checks for threefold repetition or 50 move rule draws
///////////////////////////////
static bool
is_search_draw(int sply)
{
	hashkey_t key = HashKey(sply);
	int hmc = HalfmoveClock(sply);
	int idx = searchInfo->keyidx;
	int reps = 1;

	if (hmc >= 100)
		return true;

	while (--hmc >= 0) {
		if (key == searchInfo->keyLog[--idx])
			reps++;
		if (reps == 3)
			return true;
	}

	return false;
}

///////////////////////////////
// checks to see if the search is out of time, if we've reached the
// max depth/node count, or if we've been asked to stop.
///////////////////////////////
static inline bool
should_stop(void)
{
	if (searchInfo->status == ABORTED)
		return true;

	if (!searchInfo->inf && clock() >= searchInfo->endTime) {
		searchInfo->status = ABORTED;
		return true;
	}

	if (searchInfo->depthLimit != 0 && searchInfo->depth > searchInfo->depthLimit) {
		searchInfo->status = ABORTED;
		return true;
	}

	if (searchInfo->nodeLimit != 0 && searchInfo->nodes >= searchInfo->nodeLimit) {
		searchInfo->status = ABORTED;
		return true;
	}

	// check for input every 50k nodes. this half works for now.
	if (searchInfo->nodes % 50000 == 0) {
		if (input_available()) {
			parse_input_while_searching();
			if (searchInfo->status == ABORTED)
				return true;
		}
	}

	return false;
}

///////////////////////////////
// resets the search_info_t data for a new search.
///////////////////////////////
static void
new_search(void)
{
	int ply = currentGamePly - HalfmoveClock(0);

	searchInfo->status = THINKING;
	searchInfo->depth = 0;
	searchInfo->nodes = 0;
	searchInfo->curRootMoveNum = 0;
	searchInfo->bestRootMove = 0;
	searchInfo->bestRootScore = -INFINITY;
	searchInfo->matesFound = 0;
	searchInfo->startTime = clock();

	// we now refill the list of hash keys from the history_t array,
	// but we don't bother filling in any before the last half move
	// clock reset.
	searchInfo->keyidx = 0;
	while (ply <= currentGamePly)
		searchInfo->keyLog[searchInfo->keyidx++] = history[ply++].hashKey;
}
