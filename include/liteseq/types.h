#ifndef LQ_TYPES_H
#define LQ_TYPES_H

#include <stddef.h>
#include <stdint.h> // uint8_t, uint32_t

#ifdef __cplusplus
extern "C" {

namespace liteseq
{

#endif

typedef uint8_t byte;
typedef uint32_t idx_t;
typedef uint32_t id_t;
typedef int8_t status_t;

// for null values use the max value of the type
#define NULL_ID UINT32_MAX
#define NULL_IDX UINT32_MAX
#define INVALID_LEN UINT32_MAX

typedef enum {
	ERROR_CODE_SUCCESS = 0,
	ERROR_CODE_FAILURE = -1,
	ERROR_CODE_NULL_POINTER = -2,
	ERROR_CODE_OUT_OF_MEMORY = -3,
	ERROR_CODE_MEMORY_ALLOCATE_FAIL = -4,
	ERROR_CODE_MEMORY_RELEASE_FAIL = -5,
	ERROR_CODE_OUT_OF_BOUNDS = -6,
	ERROR_CODE_INVALID_ARGUMENT = -7,
	ERROR_CODE_OVERFLOW = -8,
	ERROR_CODE_UNDERFLOW = -9,
	ERROR_CODE_NOT_FOUND = -10,
	ERROR_CODE_ALREADY_EXISTS = -11,
	ERROR_CODE_TIMEOUT = -12,
	ERROR_CODE_IO_ERROR = -13,
	ERROR_CODE_PERMISSION_DENIED = -14,
	ERROR_CODE_NOT_IMPLEMENTED = -15,
	ERROR_CODE_CONFLICT = -16, // Data or state conflict
	ERROR_CODE_UNKNOWN = -17
} ERROR_CODE;

#define SUCCESS ERROR_CODE_SUCCESS

#define COMMA_CHAR ','
#define COMMA_STR ","
#define NEWLINE '\n'
#define NEWLINE '\n'
#define TAB_CHAR '\t'
#define NULL_TERM '\0'
#define NULL_BYTE 0x00
#define TAB_STR "\t"
#define COMMA_STR ","

#define BUFFER_SIZE (1024 * 1024)
#define EXPECTED_HEADER_LENGTH 512
#define EXPECTED_SEQ_LENGTH (1 << 30) // 2^30 ≈ 10^9
#define MAX_COMPRESSED_RUN 31

#define HASH_CHAR '#'
#define NULL_CHAR '\0'

// Maximum number of digits we expect in a number
#define MAX_DIGITS 12
#define MAX_PATH_LEN 4096 // Maximum path length
// the max number of tokens to extract from a single line
// always make sure this is less than any value for expected_tokens
#define MAX_TOKENS 10

#define ERR "ERR"
#define WARN "WARN"

/* extern const char *ERR; */
/* extern const char *WARN; */

// size of a compressed run in a byte
// 0 to 31 therefore 32
// extern const unsigned int MAX_COMPRESSED_RUN;

// TODO: merge this and the define
// TODO: use same line kind as rest of GFA
/* Single-character prefix (e.g., 'P', 'W') */
enum gfa_line_prefix {
	P_LINE,
	W_LINE,
};

enum strand {
	STRAND_FWD, // aka '+'
	STRAND_REV  // aka '-'
};

/* GFA related constants */
// TODO: rename to GFA_P_LINE_IDENTIFIER etc
// using define instead of const to make them usable in switch statements
#define GFA_S_LINE 'S' // segment line (for vertices)
#define GFA_L_LINE 'L' // link line (or edges)
#define GFA_P_LINE 'P' // path line
#define GFA_H_LINE 'H' // header line

// GFA v1.1
#define GFA_W_LINE 'W' // walk line

#define EXPECTED_P_LINE_TOKENS 3 // the number of tokens expected in a P line
#define EXPECTED_L_LINE_TOKENS 4 // the number of tokens expected in a L line
#define EXPECTED_S_LINE_TOKENS 3 // the number of tokens expected in a S line
#define EXPECTED_H_LINE_TOKENS 2 // the number of tokens expected in a H line

#ifdef __cplusplus

} // liteseq
}
#endif

#endif /* LQ_TYPES_H */
