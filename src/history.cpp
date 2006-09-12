#include "benthos.h"

///////////////////////////////
// maintains the game history stack.
///////////////////////////////

history_t history[MAXGAMELENGTH];
int       currentGamePly;

///////////////////////////////
// completely clears the history.
///////////////////////////////
void
reset_history(void)
{
	currentGamePly = 0;
	for (int i = 0; i < MAXGAMELENGTH; i++) {
		history[i].hashKey = 0;
		history[i].halfmoveClock = 0;
		history[i].prevMove = 0;
	}
}

///////////////////////////////
// resets the history, then stores the current root node's
// information into the first ply.
///////////////////////////////
void
history_new_game(void)
{
	reset_history();
	history[0].hashKey = HashKey(0);
	history[0].halfmoveClock = HalfmoveClock(0);
	history[0].prevMove = 0;
}

///////////////////////////////
// make_history_move is a sort of wrapper for make_move, which
// appends the new ply to the history stack, resets the state
// stack for the next search, etc. moves should be verified as
// fully legal before calling this.
//
// everything is done using the root node in the state stack.
///////////////////////////////
void
make_history_move(position_t *pos, move_t move)
{
	make_move(pos, move, 0);

	// since we move from state zero -> one, all we have to do is
	// copy it back to state zero.
	states[0] = states[1];

	currentGamePly++;
	history[currentGamePly].hashKey = HashKey(0);
	history[currentGamePly].halfmoveClock = HalfmoveClock(0);
	history[currentGamePly].prevMove = move;
}
