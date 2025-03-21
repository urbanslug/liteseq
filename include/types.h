#ifndef LQ_TYPES_H
#define LQ_TYPES_H

#include <stdint.h> // uint8_t, uint32_t
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {

namespace liteseq {

#endif

/*
  typedefs
  --------
 */

typedef uint8_t byte;
typedef uint32_t idx_t;
typedef uint32_t id_t;

/*Status type used for returns. Generally 0 is success, -1 is failure*/
typedef int8_t status_t;


/*
  Constants
  ---------
 */

extern const unsigned int BUFFER_SIZE;
extern const unsigned int EXPECTED_HEADER_LENGTH;
extern const unsigned int EXPECTED_SEQ_LENGTH;

extern const char NEWLINE;
extern const char TAB_CHAR;
extern const char COMMA_CHAR;
extern const char NULL_TERM;
extern const byte NULL_BYTE;
extern const char *TAB_STR;
extern const char *COMMA_STR;

extern const char *ERR;
extern const char *WARN;


// size of a compressed run in a byte
// 0 to 31 therefore 32
extern const unsigned int MAX_COMPRESSED_RUN;

/* GFA related constants */

// using define instead of const to make them usable in switch statements
#define GFA_S_LINE 'S' // segment line (for vertices)
#define GFA_L_LINE 'L' // link line (or edges)
#define GFA_P_LINE 'P' // path line
#define GFA_H_LINE 'H' // header line

#define EXPECTED_P_LINE_TOKENS 3 // the number of tokens expected in a P line
#define EXPECTED_L_LINE_TOKENS 4 // the number of tokens expected in a L line
#define EXPECTED_S_LINE_TOKENS 3 // the number of tokens expected in a S line
#define EXPECTED_H_LINE_TOKENS 2 // the number of tokens expected in a H line

/* always make sure this is less than any value for expected_tokens */
#define MAX_TOKENS 10 // the max number of tokens to extract from a single line

#define MAX_DIGITS 12 // Maximum number of digits we expect in a vertex ID
#define MAX_PATH_LEN 4096 // Maximum path length
#ifdef __cplusplus

} // liteseq

}
#endif

#endif /* LQ_TYPES_H */
