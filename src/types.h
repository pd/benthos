#ifndef BENTHOS_TYPES_H
#define BENTHOS_TYPES_H

// redefinitions of the various integer types that are needed.
// this should avoid any annoying naming clashes.
typedef signed   char      int8;
typedef unsigned char      uint8;
typedef signed   short     int16;
typedef unsigned short     uint16;
typedef signed   long      int32;
typedef unsigned long      uint32;
typedef signed   long long int64;
typedef unsigned long long uint64;
typedef uint64 bitboard_t;
typedef uint64 hashkey_t;
typedef uint32 move_t;
typedef uint8  square_t;
typedef uint8  piece_t;
#define ULL(x) x##LL

#endif // !defined(BENTHOS_TYPES_H)
