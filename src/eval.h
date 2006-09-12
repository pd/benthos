#ifndef BENTHOS_EVAL_H
#define BENTHOS_EVAL_H

// the scaled values for material imbalances were taken from the paper
// by Larry Kaufmann, "Evaluation of Material Imbalance in Chess"

// bonus for bishop pairs: 1/2 pawn
#define BISHOP_PAIR_BONUS  50
// bonus/penalty for knights, per pawn > and < 5, respectively: 1/16 pawn
#define KNIGHT_PAWN_BONUS   6
// bonus/penalty for rooks, per pawn < and > 5, respectively: 1/8 pawn
#define ROOK_PAWN_BONUS    12

#endif // !defined(BENTHOS_EVAL_H)
