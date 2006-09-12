#include "benthos.h"

hash_entry_t *hashTable;
int hashMaxEntries;

///////////////////////////////
// prepares the hash table.
///////////////////////////////
void
init_hash(int size)
{
	int max;

	max = 1;
	while (true) {
		if ((int)(max * 2 * sizeof(hash_entry_t)) > size)
			break;
		max *= 2;
	}
	if ((hashTable = (hash_entry_t *)malloc(max * sizeof(hash_entry_t))) == NULL) {
		cout << "Failed to allocate hash memory: " << (max * sizeof(hash_entry_t)) << " bytes" << endl;
		return;
	}
	hashMaxEntries = max;
	clear_hash();
}

void
clear_hash()
{
	memset(hashTable, 0, hashMaxEntries * sizeof(hash_entry_t));
}

int
probe_hash(hashkey_t key, int depth, int alpha, int beta, move_t *move)
{
	hash_entry_t *entry = &hashTable[key % hashMaxEntries];

	if (entry->key != key)
		return HASH_VAL_UNKNOWN;

	if (depth <= entry->depth) {
		if (entry->type == EXACT)
			return entry->score;
		if (entry->type == ALPHA && entry->score <= alpha)
			return alpha;
		if (entry->type == BETA  && entry->score >= beta)
			return beta;
	}

	if (entry->move != 0)
		*move = entry->move;

	return HASH_VAL_UNKNOWN;
}

void
store_hash(hashkey_t key, int depth, int type, int score, move_t move)
{
	hash_entry_t *entry = &hashTable[key % hashMaxEntries];
	entry->key = key;
	entry->depth = depth;
	entry->type = type;
	entry->score = score;
	entry->move = move;
}
