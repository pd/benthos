#include "benthos.h"

// these are used all over the place. might as well situate them here.
position_t *rootPosition;
state_t     states[MAXPLY];

void
init(void)
{
	rootPosition = (position_t *)malloc(sizeof(position_t));
	init_mersenne();
	init_bitboards();
	init_zobrist();
	init_hash(33554432); // 32mb, move elsewhere TODO
	init_search();
}

int
main(void)
{
	init();
	position_from_fen(rootPosition, STARTING_FEN);
	history_new_game();
	ui_loop();

	return 0;
}
