#ifndef LQ_GFA_H
#define LQ_GFA_H

#include <pthread.h> // multithreading

#include "../src/internal/lq_types.h"
#include "../src/internal/lq_utils.h"
#include "../src/internal/lq_io.h"

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

typedef enum {
  GFA_1_0,
  GFA_1_1,
  UNSUPPORTED
} gfa_versions_e;

#define GFA_V_1_0 "VN:Z:1.0"
#define GFA_V_1_1 "VN:Z:1.1"

/* lookup table tying strings → enum */
static const struct {
  const char *tag;
  gfa_versions_e version;
} gfa_version_map[] = {
    {GFA_V_1_0, GFA_1_0},
    {GFA_V_1_1, GFA_1_1},
};
static const size_t gfa_version_map_len =
  sizeof(gfa_version_map) / sizeof(*gfa_version_map);

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
  const char *fp; // file path

  bool inc_vtx_labels;
  bool inc_refs;

  char *start; // pointer to the start of the memory mapped file
  char *end; // pointer to the end of the memory mapped file
  size_t file_size; // size of the memory mapped file
  status_t status;

  line *s_lines;
  line *l_lines;

  line *p_lines;
  line *w_lines;

  // is w the number of W lines in GFA 1.1
  idx_t ref_count;

  vtx *v;  // the array of vertices
  edge *e; // the array of edges
  ref *refs; // the reference sequences

  gfa_versions_e version; // version

  /* number of S, L and P lines in the file */
  idx_t s_line_count; // TODO replace with vtx count
  idx_t l_line_count; // TODO replace with edge count
  idx_t p_line_count; // TODO replace with ref count
  idx_t w_line_count;
} gfa_props;


typedef struct {
  const char* fp;
  bool inc_vtx_labels;
  bool inc_refs;
} gfa_config;

gfa_props *gfa_new(const gfa_config *conf);

void gfa_free(gfa_props *c);

#ifdef __cplusplus

// C++-only extension: constructor inside the same struct
// C++ wrapper for gfa_config specifically C++11 to C++20 which lack designated
// initializers https://cplusplus.com/forum/general/285267/
struct gfa_config_cpp : gfa_config {
  gfa_config_cpp(const char *fp_,
                 bool inc_vtx_labels_ = false,
                 bool inc_refs_ = false) {
    fp = fp_;
    inc_vtx_labels = inc_vtx_labels_;
    inc_refs = inc_refs_;
  }
};

} // liteseq

}
#endif

#endif /* LQ_GFA_H */
