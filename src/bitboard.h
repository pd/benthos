#ifndef BENTHOS_BITBOARD_H
#define BENTHOS_BITBOARD_H

///////////////////////////////
// population count and lsb routines were taken from greko
///////////////////////////////

// magic lookup table for lsb/poplsb
static const uint8 lsb_magics[64] = {
	63, 30,  3, 32, 59, 14, 11, 33,
	60, 24, 50,  9, 55, 19, 21, 34,
	61, 29,  2, 53, 51, 23, 41, 18,
	56, 28,  1, 43, 46, 27,  0, 35,
	62, 31, 58,  4,  5, 49, 54,  6,
	15, 52, 12, 40,  7, 42, 45, 16,
	25, 57, 48, 13, 10, 39,  8, 44,
	20, 47, 38, 22, 17, 37, 36, 26,
};

///////////////////////////////
// returns the population count of the provided uint64
///////////////////////////////
static inline int
popcnt(const uint64 &i)
{
	static const uint64 mask1  = ULL(0xAAAAAAAAAAAAAAAA); 
	static const uint64 mask2  = ULL(0xCCCCCCCCCCCCCCCC);
	static const uint64 mask32 = ULL(0xFFFFFFFF00000000);
	if (!i)
		return 0;

	uint64 x = ((i & mask1) >> 1) + (i & (mask1 >> 1));
	x = ((x & mask2) >> 2) + (x & (mask2 >> 2));

	int y = (int)(((x & mask32) >> 32) + x);
	y = ((y & 0xF0F0F0F0) >> 4) + (y & (0xF0F0F0F0 >> 4));
	y += (y & 0xFF00FF00) >> 8;
	y += (y & 0xFFFF0000) >> 16;

	return y & 0xFF;
}

///////////////////////////////
// returns the least significant bit set in a uint64. don't pass
// this function zero.
///////////////////////////////
static inline uint8
lsb(const uint64 &i)
{
	uint64 lsb = i ^ (i - 1);
	uint32 folded = (int)lsb ^ (int)(lsb >> 32);
	int idx = (folded * 0x78291ACF) >> 26;
	return lsb_magics[idx];
}

///////////////////////////////
// returns the least significant bit set in a uint64, and clears
// that bit from the provided integer.
///////////////////////////////
static inline uint8
poplsb(uint64 &i)
{
	uint64 lsb = i ^ (i - 1);
	uint32 folded = (int)lsb ^ (int)(lsb >> 32);
	int idx = (folded * 0x78291ACF) >> 26;
	i &= ~lsb;
	return lsb_magics[idx];
}

///////////////////////////////
// returns a 32-bit unsigned integer with only the most significant
// bit from the provided value set.
///////////////////////////////
static inline uint32
getmsbval(uint32 i)
{
	i |= (i >> 1);
	i |= (i >> 2);
	i |= (i >> 4);
	i |= (i >> 8);
	i |= (i >> 16);
	return i & ~(i >> 1);
}

///////////////////////////////
// returns the most significant bit set in a uint64. this is much
// worse than the least significant bit function, so don't use it
// unless it's needed.
///////////////////////////////
static inline uint8
msb(const uint64 &i)
{
	uint32 half = i >> 32;
	if (half) {
		uint32 msbval = getmsbval(half);
		return lsb(((uint64)msbval) << 32);
	}

	half = i & 0xffffffff;
	uint32 msbval = getmsbval(half);
	return lsb((uint64)msbval);
}

///////////////////////////////
// returns the msb from a uint64, and clears that bit.
///////////////////////////////
static inline uint8
popmsb(uint64 &i)
{
	uint8 b = msb(i);
	i &= ~(ULL(1) << b);
	return b;
}

// the new location of bits in a rotated bitboard
const uint8 rot90L[64] = {
	 7, 15, 23, 31, 39, 47, 55, 63,
	 6, 14, 22, 30, 38, 46, 54, 62,
	 5, 13, 21, 29, 37, 45, 53, 61,
	 4, 12, 20, 28, 36, 44, 52, 60,
	 3, 11, 19, 27, 35, 43, 51, 59,
	 2, 10, 18, 26, 34, 42, 50, 58,
	 1,  9, 17, 25, 33, 41, 49, 57,
	 0,  8, 16, 24, 32, 40, 48, 56
};
const uint8 rot45L[64] = {
	 0,  2,  5,  9, 14, 20, 27, 35,
	 1,  4,  8, 13, 19, 26, 34, 42,
	 3,  7, 12, 18, 25, 33, 41, 48,
	 6, 11, 17, 24, 32, 40, 47, 53,
	10, 16, 23, 31, 39, 46, 52, 57,
	15, 22, 30, 38, 45, 51, 56, 60,
	21, 29, 37, 44, 50, 55, 59, 62,
	28, 36, 43, 49, 54, 58, 61, 63,
};
const uint8 rot45R[64] = {
	28, 21, 15, 10,  6,  3,  1,  0,
	36, 29, 22, 16, 11,  7,  4,  2,
	43, 37, 30, 23, 17, 12,  8,  5,
	49, 44, 38, 31, 24, 18, 13,  9,
	54, 50, 45, 39, 32, 25, 19, 14,
	58, 55, 51, 46, 40, 33, 26, 20,
	61, 59, 56, 52, 47, 41, 34, 27,
	63, 62, 60, 57, 53, 48, 42, 35,
};

// map from the square of the rotated bitboards to the original
// square in an unrotated bitboard
const uint8 unrot90L[64] = {
	56, 48, 40, 32, 24, 16,  8,  0,
	57, 49, 41, 33, 25, 17,  9,  1,
	58, 50, 42, 34, 26, 18, 10,  2,
	59, 51, 43, 35, 27, 19, 11,  3,
	60, 52, 44, 36, 28, 20, 12,  4,
	61, 53, 45, 37, 29, 21, 13,  5,
	62, 54, 46, 38, 30, 22, 14,  6,
	63, 55, 47, 39, 31, 23, 15,  7,
};
const uint8 unrot45L[64] = {
	 0,  8,  1, 16,  9,  2, 24, 17,
	10,  3, 32, 25, 18, 11,  4, 40,
	33, 26, 19, 12,  5, 48, 41, 34,
	27, 20, 13,  6, 56, 49, 42, 35,
	28, 21, 14,  7, 57, 50, 43, 36,
	29, 22, 15, 58, 51, 44, 37, 30,
	23, 59, 52, 45, 38, 31, 60, 53,
	46, 39, 61, 54, 47, 62, 55, 63,
};
const uint8 unrot45R[64] = {
	 7,  6, 15,  5, 14, 23,  4, 13,
	22, 31,  3, 12, 21, 30, 39,  2,
	11, 20, 29, 38, 47,  1, 10, 19,
	28, 37, 46, 55,  0,  9, 18, 27,
	36, 45, 54, 63,  8, 17, 26, 35,
	44, 53, 62, 16, 25, 34, 43, 52,
	61, 24, 33, 42, 51, 60, 32, 41,
	50, 59, 40, 49, 58, 48, 57, 56,
};

// the shift amount to get the middle contents of a "rank"
// into the first six bits of a uint64
const uint8 shift90L[64] = {
	 1,  9, 17, 25, 33, 41, 49, 57,
	 1,  9, 17, 25, 33, 41, 49, 57,
	 1,  9, 17, 25, 33, 41, 49, 57,
	 1,  9, 17, 25, 33, 41, 49, 57,
	 1,  9, 17, 25, 33, 41, 49, 57,
	 1,  9, 17, 25, 33, 41, 49, 57,
	 1,  9, 17, 25, 33, 41, 49, 57,
	 1,  9, 17, 25, 33, 41, 49, 57,
};
const uint8 shift45L[64] = {
	 1,  2,  4,  7, 11, 16, 22, 29,
	 2,  4,  7, 11, 16, 22, 29, 37,
	 4,  7, 11, 16, 22, 29, 37, 44,
	 7, 11, 16, 22, 29, 37, 44, 50,
	11, 16, 22, 29, 37, 44, 50, 55,
	16, 22, 29, 37, 44, 50, 55, 59,
	22, 29, 37, 44, 50, 55, 59, 62,
	29, 37, 44, 50, 55, 59, 62, 64,
};
const uint8 shift45R[64] = {
	29, 22, 16, 11,  7,  4,  2,  1,
	37, 29, 22, 16, 11,  7,  4,  2,
	44, 37, 29, 22, 16, 11,  7,  4,
	50, 44, 37, 29, 22, 16, 11,  7,
	55, 50, 44, 37, 29, 22, 16, 11,
	59, 55, 50, 44, 37, 29, 22, 16,
	62, 59, 55, 50, 44, 37, 29, 22,
	64, 62, 59, 55, 50, 44, 37, 29,
};

#endif // !defined(BENTHOS_BITBOARD_H)
