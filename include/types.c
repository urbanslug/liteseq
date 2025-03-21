#include "./types.h"


// universal constants
// -------------------

const unsigned int BUFFER_SIZE = 1024*1024;
const unsigned int EXPECTED_HEADER_LENGTH = 512;
const unsigned int EXPECTED_SEQ_LENGTH = (1 << 30); // 2^30 ≈ 10^9


const char NEWLINE = '\n';
const char TAB_CHAR = '\t';
const char COMMA_CHAR = ',';
const char NULL_TERM = '\0';
const byte NULL_BYTE = 0x00;
const char *TAB_STR = "\t";
const char *COMMA_STR = ",";

const char *ERR = "ERR";
const char *WARN = "WARN";

// size of a compressed run in a byte
// 0 to 31 therefore 32
const unsigned int MAX_COMPRESSED_RUN = 31;

