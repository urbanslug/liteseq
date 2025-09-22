#ifndef LQ_REFS_H
#define LQ_REFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h> // uint8_t, uint32_t

#include "./types.h"

#ifdef __cplusplus
extern "C" { // Ensure the function has C linkage
namespace liteseq
{
#endif

#define P_LINE_ID_TOKEN_COUNT 1
#define W_LINE_ID_TOKEN_COUNT 3

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

struct ref_walk {
	// the actual walk
	enum strand *strands;
	id_t *v_ids;
	idx_t *loci;
	// walk metadata
	idx_t step_count; // the number of steps
	idx_t hap_len;	  // the length of the haplotype in bases
};

struct pansn {
	char *sample_name;
	id_t hap_id;
	char *contig_name;
};

enum ref_id_type {
	REF_ID_PANSN,
	REF_ID_RAW,
};

// ensure that every kind of ref id has a way of implementing tag
union ref_id_value {
	struct pansn *id_value;
	char *raw;
};

struct ref_id {
	enum ref_id_type type;
	union ref_id_value value;
	char *tag;
};

struct ref {
	enum gfa_line_prefix line_prefix;
	struct ref_walk *walk;
	struct ref_id *id;
};

struct ref *parse_ref_line(enum gfa_line_prefix line_type, const char *line);

const char *get_tag(const struct ref *r);
const char *get_sample_name(const struct ref *r);
idx_t get_hap_len(const struct ref *r);
enum gfa_line_prefix get_line_prefix(const struct ref *r);
status_t set_hap_len(struct ref *r, idx_t hap_len);
idx_t get_step_count(const struct ref *r);
void destroy_ref(struct ref **r);

idx_t get_hap_id(const struct ref *r);

const char *get_contig_name(const struct ref *r);

enum ref_id_type get_ref_id_type(const struct ref *r);
const id_t *get_walk_v_ids(const struct ref *r);
const enum strand *get_walk_strands(const struct ref *r);

// fns I want to expose only for testing
#ifdef TESTING
/* PanSN */
struct pansn *try_parse_pansn(const char *name, const char delim);
void destroy_pansn(struct pansn **pn);

struct ref_id *alloc_ref_id(char **tokens, idx_t token_count);
void destroy_ref_id(struct ref_id **r_id);
struct ref_walk *alloc_ref_walk(idx_t step_count);
void destroy_ref_walk(struct ref_walk **r_walk);

idx_t tokenize(char *str, const char delim, char **tokens, idx_t token_limit);
struct ref *alloc_ref(enum gfa_line_prefix line_prefix,
		      struct ref_walk **r_walk, struct ref_id **id);

struct ref *parse_w_line(char *line);
idx_t count_steps(enum gfa_line_prefix line_type, const char *str);
status_t parse_data_line_w(const char *str, struct ref_walk **empty_r_walk);
status_t parse_data_line_p(const char *str, struct ref_walk **empty_r_walk);

/**
 * Function to retrieve metadata for a given line prefix.
 * This function provides access to line-specific metadata, which includes
 * required token counts, token indices, and parsing logic. It is primarily
 * used for testing purposes or when controlled access to metadata is needed.
 *
 * @param prefix The line prefix for which metadata is requested (e.g., W_LINE,
 * P_LINE).
 * @return A pointer to the corresponding metadata, or NULL if the prefix is
 * unsupported.
 */
const struct line_metadata *get_line_metadata(enum gfa_line_prefix prefix);
#endif

#ifdef __cplusplus
} // namespace liteseq
} // extern "C"
#endif

#endif /* LQ_REFS_H */
