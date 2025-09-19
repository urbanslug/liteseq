#ifndef LQ_REFS_H
#define LQ_REFS_H

#include <stddef.h>
#include <stdint.h> // uint8_t, uint32_t
//#include <pthread.h> // multithreading
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" { // Ensure the function has C linkage
#endif

typedef uint8_t byte;
typedef uint32_t idx_t;
typedef uint32_t id_t;

typedef int8_t status_t;

/*Status type used for returns. Generally 0 is success, -1 is failure*/
const char HASH_CHAR = '#';
const char NULL_CHAR = '\0';
  //const char TAB_CHAR = '\t';

#define COMMA_CHAR ','
#define P_LINE_FORWARD_SYMBOL '+'
#define P_LINE_REVERSE_SYMBOL '-'
#define W_LINE_FORWARD_SYMBOL '>'
#define W_LINE_REVERSE_SYMBOL '<'

// Maximum number of digits we expect in a number
#define MAX_DIGITS 12

const id_t NULL_ID = (id_t)(-1);
const idx_t NULL_IDX = (idx_t)(-1);
const idx_t INVALID_LEN = (idx_t)(-1);

/* always make sure this is less than any value for expected_tokens */
#define MAX_TOKENS 10 // the max number of tokens to extract from a single line

#define READ_P_LINE_TOKENS 3 // the number of tokens expected in a P line
#define READ_W_LINE_TOKENS 7 // the number of tokens expected in a P line

extern const char NEWLINE;
extern const char TAB_CHAR;
extern const char NULL_TERM;
extern const byte NULL_BYTE;
extern const char *TAB_STR;
extern const char *COMMA_STR;

typedef enum {
    ERROR_CODE_SUCCESS = 0,          // Operation completed successfully
    ERROR_CODE_FAILURE = -1,         // General failure
    ERROR_CODE_NULL_POINTER = -2,    // Null pointer encountered
    ERROR_CODE_OUT_OF_MEMORY = -3,   // Memory allocation failed
    ERROR_CODE_OUT_OF_BOUNDS = -4,   // Array or index out of bounds
    ERROR_CODE_INVALID_ARGUMENT = -5,// Invalid input argument
    ERROR_CODE_OVERFLOW = -6,        // Numeric or data overflow
    ERROR_CODE_UNDERFLOW = -7,       // Numeric or data underflow
    ERROR_CODE_NOT_FOUND = -8,       // Data or element not found
    ERROR_CODE_ALREADY_EXISTS = -9,  // Item already exists
    ERROR_CODE_TIMEOUT = -10,        // Operation timed out
    ERROR_CODE_IO_ERROR = -11,       // Input/output error
    ERROR_CODE_PERMISSION_DENIED = -12, // Insufficient permissions
    ERROR_CODE_NOT_IMPLEMENTED = -13,  // Feature not implemented
    ERROR_CODE_CONFLICT = -14,       // Data or state conflict
    ERROR_CODE_UNKNOWN = -15         // Unknown error
} ERROR_CODE;

#define SUCCESS ERROR_CODE_SUCCESS

typedef int8_t status_t;

// Represents the two directions of a DNA sequence
typedef enum {
  FORWARD, // other name is positive
  REVERSE  // other name is negative
} strand_e;

/* the actual ref data */
typedef struct {
  strand_e *s;
  id_t *v_id;
  idx_t *locus;
  idx_t step_count; // the number of steps
  idx_t hap_len; // the length of the haplotype
} ref_data;

typedef enum {
  P_LINE,
  W_LINE
} ref_line_type_e;

/*
 =====
 ref name
  =====
 */

/**
In pansn the ref name is as follows
  sample_name : string
  delim : char
  haplotype_id : number
  contig_or_scaffold_name : string

in the W walk the ref name is as follows
  SampleId : string
  HapIndex : number
  SeqId : string
they essentially represent the same thing

*/
typedef struct {
  char *sample_name;
  char *contig_name;
  id_t hap_id; // haplotype id
} pansn_ref_name;

/**
  * @brief The type of reference name
  * PANSN - the name is in the pansn format
  * COARSE - the name is a coarse name, just a string
*/
typedef enum {
  PANSN,
  COARSE
} ref_name_type_e;

typedef struct {
  ref_line_type_e line_type;
  ref_name_type_e type; // call this name type
  union {
    pansn_ref_name pansn_value;
    char *coarse_name;
  };
  char *tag;
} ref_name;

typedef struct {
  ref_data *data;
  ref_name name;
} ref;

ref *parse_ref_line(ref_line_type_e line_type, char *line);
const char *get_tag(const ref *r);
const char *get_sample_name(const ref *r);
idx_t get_hap_len(const ref *r);
idx_t get_step_count(const ref *r);
void destroy_ref(ref **r);

// fns I want to expose only for testing
#ifdef TESTING
ref_name create_ref_name_pansn(ref_line_type_e lt, pansn_ref_name *pn);
ref_name create_ref_name_coarse(ref_line_type_e lt, char *cn);
const char *get_ref_name_tag(const ref_name *rn);
ref *create_blank_ref();
ref *create_ref(ref_name rn, ref_data *rd);
ref_data *create_ref_data(idx_t len);
status_t try_parse_pansn(char *name, const char delim, pansn_ref_name *pn);
ref* parse_w_line(char *line);
idx_t count_steps(ref_line_type_e line_type, const char *str);
status_t parse_data_line_w(const char *str, ref_data *r);
status_t parse_data_line_p(const char *str, ref_data *r);
void destroy_ref_data(ref_data *r);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LQ_REFS_H */
