#ifndef LQ_GFA_H
#define LQ_GFA_H

#include <pthread.h> // multithreading
#include <threads.h> // for pthreads

#include "./io.h"
#include "./types.h"
#include "./utils.h"


#ifdef __cplusplus
extern "C" {

namespace liteseq {

#endif

/*
 * Enums
 * -----
 */

// Represents the two sides of a vertex in a bidirected graph
typedef enum {
  LEFT, // other names are positive or forward
  RIGHT // other name is negative or reverse
} vtx_side_e;

// Represents the two directions of a DNA sequence
typedef enum {
  FORWARD, // other name is positive
  REVERSE  // other name is negative
} strand_e;

/*
 * Structs
 * --------
 *
 */

// a line in the GFA file
typedef  struct {
  char *start;
  idx_t line_idx;
  idx_t len;
} line;


// TODO
//  - handle vertex ids that go beyond the vertex count
//  - support vertex names that are not positive integers
typedef struct {
  char *seq; // the label (or sequence) of the vertex
  id_t id;   // the identifier of the vertex in the GFA file
} vtx;

typedef struct {
  id_t v1_id;         // the id of the first vertex
  id_t v2_id;         // the id of the second vertex
  vtx_side_e v1_side; // the side of the first vertex
  vtx_side_e v2_side; // the side of the second vertex
} edge;

typedef struct {
  id_t v_id;
  strand_e s;
} step;

typedef struct {
  char *name;
  step *steps;
  idx_t step_count;
} ref;


// This struct holds metadata about the GFA file
// for internal use
// TODO: rename to gfa_meta
typedef struct {
  bool inc_vtx_labels;
  bool inc_refs;
  idx_t ref_count; // unify ref count and P line count
  char **ref_names;

  char *start; // pointer to the start of the memory mapped file
  char *end; // pointer to the end of the memory mapped file
  size_t file_size; // size of the memory mapped file
  status_t status;

  line *s_lines;
  line *l_lines;
  line *p_lines;

  vtx *v;  // the array of vertices
  edge *e; // the array of edges
  ref *refs; // the reference sequences

  /* number of S, L and P lines in the file */
  idx_t s_line_count; // TODO replace with vtx count
  idx_t l_line_count; // TODO replace with edge count
  idx_t p_line_count; // TODO replace with ref count
} gfa_props;


typedef struct {
  const char* fp;
  bool inc_vtx_labels;
  bool inc_refs;
  idx_t ref_count; // unify ref count and P line count
  const char **ref_names;
} gfa_config;

gfa_props *gfa_new(const gfa_config *conf);

void gfa_free(gfa_props *c);

#ifdef __cplusplus

} // liteseq

}
#endif

#endif /* LQ_GFA_H */
