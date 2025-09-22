#ifdef __linux__
#define _GNU_SOURCE // Enable GNU extensions on Linux
#elif defined(_WIN32)
#define strtok_r strtok_s // Map strtok_r to strtok_s on Windows
#define strndup _strndup  // Map strndup to _strndup
#define strdup _strdup	  // Map strdup to _strdup
#elif defined(__APPLE__)
/* On macOS, strndup and strdup exist, but no mappings are needed. */
#endif

#include <assert.h>
#include <log.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/liteseq/refs.h"
#include "../include/liteseq/types.h"
#include "../src/internal/lq_utils.h"

#define P_LINE_FORWARD_SYMBOL '+'
#define P_LINE_REVERSE_SYMBOL '-'
#define W_LINE_FORWARD_SYMBOL '>'
#define W_LINE_REVERSE_SYMBOL '<'

#define PANSN_ID_PARTS_COUNT 3
/* #define P_LINE_ID_TOKEN_COUNT 1 */
/* #define W_LINE_ID_TOKEN_COUNT 3 */

#define READ_P_LINE_TOKENS 3 // the number of tokens expected in a P line
#define READ_W_LINE_TOKENS 7 // the number of tokens expected in a P line

/* Define constants globally or within the file */
#define PANSN_MAX_TOKENS 3
#define PANSN_PARSER_BUF_SIZE PANSN_MAX_TOKENS + 1

// how PanSN fields are organised in a W line
#define PANSN_SAMPLE_COL 1
#define PANSN_HAP_ID_COL 2
#define PANSN_CONTIG_NAME_COL 3

// the column of the W line that contains the walk data
#define W_LINE_WALK_COL 6

#define P_LINE_NAME_COL 1
#define P_LINE_WALK_COL 2

#define DEFAULT_HAP_LEN 0

struct pansn *alloc_pansn(char **tokens)
{
	struct pansn *pn = malloc(sizeof(struct pansn));
	if (!pn)
		return NULL;

	idx_t offset = 1;
	int sample_idx = PANSN_SAMPLE_COL - offset;
	int contig_idx = PANSN_CONTIG_NAME_COL - offset;
	int hap_id_idx = PANSN_HAP_ID_COL - offset;

	// clang-format off
	pn->sample_name = tokens[sample_idx] ? strdup(tokens[sample_idx]) : NULL;
	pn->contig_name = tokens[contig_idx] ? strdup(tokens[contig_idx]) : NULL;
	pn->hap_id = tokens[hap_id_idx] ? atol(tokens[hap_id_idx]) : NULL_ID;
	// clang-format on

	if (!pn->sample_name || !pn->contig_name || pn->hap_id == NULL_ID) {
		destroy_pansn(&pn);
		return NULL;
	}

	return pn;
}

void destroy_pansn(struct pansn **pn)
{
	if (pn == NULL || *pn == NULL)
		return;

	if ((*pn)->sample_name != NULL) {
		free((*pn)->sample_name);
		(*pn)->sample_name = NULL;
	}

	if ((*pn)->contig_name != NULL) {
		free((*pn)->contig_name);
		(*pn)->contig_name = NULL;
	}

	if (*pn != NULL) {
		free(*pn);
		*pn = NULL;
	}
}

idx_t count_digits(idx_t num)
{
	if (num == 0)
		return 1;
	return (idx_t)log10(num) + 1;
}

char *alloc_pansn_tag(const struct pansn *pn)
{
	// idx_t hap_id_len = (idx_t)log10(pn->hap_id);
	idx_t hap_id_len = count_digits(pn->hap_id);

	log_info("sample len  %u hap_id_len %u ctg %u", strlen(pn->sample_name),
		 hap_id_len, strlen(pn->contig_name));

	char hap_id_str[MAX_DIGITS];
	snprintf(hap_id_str, sizeof hap_id_str, "%u", pn->hap_id);

	size_t need =
		strlen(pn->sample_name) + hap_id_len + strlen(pn->contig_name);
	need += 2; // +2 for the two '#' characters
	need += 1; // +1 for the null terminator

	char *tag = (char *)malloc(need);
	if (!tag)
		return NULL;

	sprintf(tag, "%s#%u#%s", pn->sample_name, pn->hap_id, pn->contig_name);

	return tag;
}

struct pansn *free_pansn_buf(char **out_tokens)
{
	for (idx_t i = 0; i < PANSN_MAX_TOKENS; i++) {
		if (out_tokens[i] != NULL) {
			free(out_tokens[i]);
			out_tokens[i] = NULL;
		}
	}
	return NULL;
}

/* used for PanSN in P lines */
struct pansn *try_parse_pansn(const char *name, const char delim)
{
	const char *fn = "[liteseq::refs::try_parse_pansn]";

	char *out_tokens[PANSN_MAX_TOKENS] = {NULL};
	struct split_str_params p = {
		// input
		.str = name,
		.delimiter = delim,
		.max_splits = PANSN_MAX_TOKENS,
		.fallbacks = (const char[]){NEWLINE, NULL_CHAR},
		.fallback_chars_count = 2,
		// output
		.tokens_found = 0,
		.tokens = out_tokens,
	};
	status_t res = split_str(&p);
	if (res != SUCCESS)
		return free_pansn_buf(out_tokens);

	if (p.tokens_found != PANSN_MAX_TOKENS) {
		return free_pansn_buf(out_tokens);
	}

	idx_t offset = 1;
	int sample_idx = PANSN_SAMPLE_COL - offset;
	int contig_idx = PANSN_CONTIG_NAME_COL - offset;
	int hap_id_idx = PANSN_HAP_ID_COL - offset;

	for (int i = 0; i < PANSN_MAX_TOKENS; i++) {
		if (strlen(out_tokens[i]) == 0) {
			/* log_warn("%s PanSN like format could be a badly " */
			/*	 "formatted " */
			/*	 "ref: %s", */
			/*	 fn, name); */
			return free_pansn_buf(out_tokens);
		}
	}

	char *endptr;

	strtoul(out_tokens[hap_id_idx], &endptr, 10);
	if (*endptr != NULL_CHAR) {
		return free_pansn_buf(out_tokens);
	}

	if (strchr(out_tokens[contig_idx], delim) != NULL) {
		return free_pansn_buf(out_tokens);
	}

	struct pansn *pn = alloc_pansn(out_tokens);
	free_pansn_buf(out_tokens);

	return pn;
}

struct ref_id *alloc_ref_id(char **id_tokens, idx_t token_count)
{
	struct ref_id *r_id = malloc(sizeof(struct ref_id));
	if (!r_id)
		return NULL;

	struct pansn *pn = NULL;
	const char *str = id_tokens[0];

	switch (token_count) {
	case 3:
		pn = alloc_pansn(id_tokens);
		break;
	case 1:
		pn = try_parse_pansn(str, HASH_CHAR);
		break;
	default:
		return NULL;
	}

	if (pn) {
		r_id->type = REF_ID_PANSN;
		r_id->value.id_value = pn;

		// create the tag
		r_id->tag = alloc_pansn_tag(pn);
		if (!r_id->tag) {
			free(r_id);
			destroy_pansn(&pn);
			return NULL;
		}
	} else {
		r_id->type = REF_ID_RAW;
		r_id->value.raw = strdup(str);
		if (!r_id->value.raw) {
			free(r_id);
			return NULL;
		}
		// for raw ids, tag is the raw string
		r_id->tag = r_id->value.raw;
	}

	return r_id;
}

void destroy_ref_id(struct ref_id **r_id)
{
	if (r_id == NULL || *r_id == NULL)
		return;

	if ((*r_id)->type == REF_ID_PANSN) {
		if ((*r_id)->value.id_value->sample_name != NULL) {
			free((*r_id)->value.id_value->sample_name);
			(*r_id)->value.id_value->sample_name = NULL;
		}
		if ((*r_id)->value.id_value->contig_name != NULL) {
			free((*r_id)->value.id_value->contig_name);
			(*r_id)->value.id_value->contig_name = NULL;
		}
		if ((*r_id)->tag != NULL) {
			free((*r_id)->tag);
			(*r_id)->tag = NULL;
		}

		// free the pansn struct itself
		if ((*r_id)->value.id_value != NULL) {
			free((*r_id)->value.id_value);
			(*r_id)->value.id_value = NULL;
		}

	} else if ((*r_id)->type == REF_ID_RAW) {
		if ((*r_id)->value.raw != NULL) {
			free((*r_id)->value.raw);
			(*r_id)->value.raw = NULL;
		}
	}

	if (*r_id != NULL) {
		free(*r_id);
		*r_id = NULL;
	}
}

const char *get_tag(const struct ref *r)
{
	if (!r)
		return NULL;

	return r->id->tag;
}

id_t get_hap_id(const struct ref *r)
{
	if (get_ref_id_type(r) == REF_ID_RAW)
		return NULL_ID;

	return r->id->value.id_value->hap_id;
}

const char *get_contig_name(const struct ref *r)
{
	if (get_ref_id_type(r) == REF_ID_RAW)
		return NULL;

	return r->id->value.id_value->contig_name;
}

const char *get_sample_name(const struct ref *r)
{
	if (!r)
		return NULL;

	switch (r->id->type) {
	case REF_ID_PANSN:
		return r->id->value.id_value->sample_name;
	case REF_ID_RAW:
		return get_tag(r);
	}

	return NULL;
}

idx_t get_step_count(const struct ref *r)
{
	return r->walk->step_count;
}

const id_t *get_walk_v_ids(const struct ref *r)
{
	return r->walk->v_ids;
}

const enum strand *get_walk_strands(const struct ref *r)
{
	return r->walk->strands;
}

idx_t get_hap_len(const struct ref *r)
{
	return r->walk->hap_len;
}

enum ref_id_type get_ref_id_type(const struct ref *r)
{
	return r->id->type;
}

enum gfa_line_prefix get_line_prefix(const struct ref *r)
{
	if (!r)
		return (enum gfa_line_prefix) - 1;

	return r->line_prefix;
}

status_t set_hap_len(struct ref *r, idx_t hap_len)
{
	if (!r)
		return ERROR_CODE_INVALID_ARGUMENT;

	r->walk->hap_len = hap_len;

	return SUCCESS;
}

void destroy_ref_walk(struct ref_walk **w)
{

	if (w == NULL || *w == NULL)
		return;

	if ((*w)->strands != NULL) {
		free((*w)->strands);
		(*w)->strands = NULL;
	}

	if ((*w)->v_ids != NULL) {
		free((*w)->v_ids);
		(*w)->v_ids = NULL;
	}

	if ((*w)->loci != NULL) {
		free((*w)->loci);
		(*w)->loci = NULL;
	}

	if (*w != NULL) {
		free(*w);
		*w = NULL;
	}
}

struct ref_walk *alloc_ref_walk(idx_t step_count)
{
	struct ref_walk *w = malloc(sizeof(struct ref_walk));
	if (!w)
		return NULL;

	w->strands = NULL;
	w->v_ids = NULL;
	w->loci = NULL;

	w->strands = malloc(sizeof(enum strand) * step_count);
	if (!w->strands) {
		destroy_ref_walk(&w);
		return NULL;
	}

	w->v_ids = malloc(sizeof(id_t) * step_count);
	if (!w->v_ids) {
		destroy_ref_walk(&w);
		return NULL;
	}

	w->loci = malloc(sizeof(idx_t) * step_count);
	if (!w->loci) {
		destroy_ref_walk(&w);
		return NULL;
	}

	w->step_count = step_count;
	w->hap_len = 0; // default to 0

	return w;
}

struct ref *alloc_ref(enum gfa_line_prefix line_prefix,
		      struct ref_walk **r_walk, struct ref_id **id)
{
	struct ref *r = malloc(sizeof(struct ref));
	if (!r)
		return NULL;

	r->line_prefix = line_prefix;
	r->walk = *r_walk;
	r->id = *id;

	return r;
}

void destroy_ref(struct ref **r)
{
	if (r == NULL || *r == NULL) {
		return;
	}

	destroy_ref_id(&(*r)->id);
	destroy_ref_walk(&(*r)->walk);

	if (*r != NULL) {
		free(*r);
		*r = NULL;
	}
}

/** returns the token count */
idx_t tokenize(char *str, const char delim, char **tokens, idx_t token_limit)
{
	char *fn = "[liteseq::refs::tokenize]";
	if (str == NULL) {
		log_error("%s Input string is NULL", fn);
		return 0;
	}

	char *saveptr = malloc(sizeof(char *) * strlen(str));
	char *token = strtok_r(str, &delim, &saveptr);
	idx_t count = 0;

	while (count < token_limit && token != NULL) {
		tokens[count++] = token;
		token = strtok_r(NULL, &delim, &saveptr);
	}

	return count;
}

static inline bool is_step_sep(enum gfa_line_prefix line_prefix, const char c)
{
	switch (line_prefix) {
	case P_LINE:
		return c == P_LINE_FORWARD_SYMBOL || c == P_LINE_REVERSE_SYMBOL;
	case W_LINE:
		return c == W_LINE_FORWARD_SYMBOL || c == W_LINE_REVERSE_SYMBOL;
	default:
		log_fatal("Invalid line prefix in is_step_sep");
		exit(1);
	}
}

/**
 * @brief Count the number of steps in a P line path; it is the number
 * of commas
 * + 1
 *
 * @param [in] str the path string
 * @return the number of steps in the path
 */
idx_t count_steps(enum gfa_line_prefix line_prefix, const char *str)
{
	idx_t steps = 0;
	for (; *str; str++)
		if (is_step_sep(line_prefix, *str))
			steps++;

	return steps;
}

status_t parse_data_line_w(const char *str, struct ref_walk **empty_r_walk)
{
	if (str == NULL) {
		fprintf(stderr, "Error: Input string is NULL\n");
		return ERROR_CODE_INVALID_ARGUMENT;
	}

	// we assume the payload is already allocated and empty
	// we just fill it
	struct ref_walk *w = *empty_r_walk;

	char str_num[MAX_DIGITS] = {0};
	size_t digit_pos = 0; // current position in str_num
	size_t v_id = 0;
	idx_t step_count = 0;
	enum strand curr_or;

	for (; *str; str++) {
		switch (*str) {
		case W_LINE_FORWARD_SYMBOL:
		case W_LINE_REVERSE_SYMBOL:
			// clang-format off
			if (step_count > 0) {
				id_t v_id = strtoul(str_num, NULL, 10);
				w->v_ids[step_count - 1] = strtoul(str_num, NULL, 10);
				w->strands[step_count - 1] = curr_or;
			}
			curr_or = (*str == W_LINE_FORWARD_SYMBOL) ? STRAND_FWD : STRAND_REV;
			// clang-format on

			step_count++;
			digit_pos = 0;
			memset(str_num, 0, sizeof(char) * MAX_DIGITS);
			break;
		default: // accumulate the str
			if (digit_pos >= MAX_DIGITS) {
				fprintf(stderr,
					"Error: Vertex ID exceeds "
					"maximum "
					"length of %d digits\n",
					MAX_DIGITS);
				return ERROR_CODE_OUT_OF_BOUNDS;
			}
			str_num[digit_pos++] = *str;
		}
	}

	// Handle trailing token if valid
	if (digit_pos > 0) {
		w->v_ids[step_count - 1] = strtoul(str_num, NULL, 10);
		w->strands[step_count - 1] = curr_or;
	}

	return SUCCESS;
}

status_t parse_data_line_p(const char *str, struct ref_walk **empty_r_walk)
{
	if (str == NULL) {
		fprintf(stderr, "Error: Input string is NULL\n");
		return ERROR_CODE_INVALID_ARGUMENT;
	}

	// we assume the payload is already allocated and empty
	// we just fill it
	struct ref_walk *w = *empty_r_walk;

	char str_num[MAX_DIGITS] = {0};
	size_t digit_pos = 0; // current position in str_num
	size_t v_id = 0;
	idx_t step_count = 0;

	for (; *str; str++) {
		switch (*str) {
		case P_LINE_FORWARD_SYMBOL:
			v_id = strtoul(str_num, NULL, 10);
			w->v_ids[step_count] = v_id;
			w->strands[step_count] = STRAND_FWD;
			break;
		case P_LINE_REVERSE_SYMBOL:
			v_id = strtoul(str_num, NULL, 10);
			w->v_ids[step_count] = v_id;
			w->strands[step_count] = STRAND_REV;
			break;
		case COMMA_CHAR:
			step_count++;
			digit_pos = 0;
			memset(str_num, 0, sizeof(char) * MAX_DIGITS);
			break;
		default: // accumulate the str
			str_num[digit_pos++] = *str;
			if (digit_pos >= MAX_DIGITS) {
				fprintf(stderr,
					"Error: Vertex ID exceeds "
					"maximum "
					"length of %d digits\n",
					MAX_DIGITS);
				return EXIT_FAILURE;
			}
		}
	}

	return SUCCESS;
}

struct line_metadata {
	idx_t required_tokens;
	idx_t id_token_count;
	const idx_t *id_token_indices;
	idx_t data_col_index;
	enum gfa_line_prefix line_prefix;
	status_t (*parse_data_line)(const char *data_str, struct ref_walk **w);
};

// Define metadata for W_LINE and P_LINE
static const struct line_metadata metadata[] = {
	[W_LINE] = {.required_tokens = READ_W_LINE_TOKENS,
		    .id_token_count = W_LINE_ID_TOKEN_COUNT,
		    .id_token_indices =
			    (const idx_t[]){PANSN_SAMPLE_COL, PANSN_HAP_ID_COL,
					    PANSN_CONTIG_NAME_COL},
		    .data_col_index = W_LINE_WALK_COL,
		    .line_prefix = W_LINE,
		    .parse_data_line = parse_data_line_w},
	[P_LINE] = {.required_tokens = READ_P_LINE_TOKENS,
		    .id_token_count = P_LINE_ID_TOKEN_COUNT,
		    .id_token_indices = (const idx_t[]){P_LINE_NAME_COL},
		    .data_col_index = P_LINE_WALK_COL,
		    .line_prefix = P_LINE,
		    .parse_data_line = parse_data_line_p}};

// for testing
const struct line_metadata *get_line_metadata(enum gfa_line_prefix prefix)
{
	if (prefix == W_LINE || prefix == P_LINE) {
		return &metadata[prefix];
	}
	return NULL;
}

// Consolidated line parsing logic using metadata
struct ref *parse_line_generic(const char *line,
			       const struct line_metadata *meta)
{
	const char *fn = "[liteseq::refs::parse_line_generic]";
	char *tokens[MAX_TOKENS] = {NULL};
	struct split_str_params p = {
		// input
		.str = line,
		.delimiter = TAB_CHAR,
		.max_splits = meta->required_tokens,
		.fallbacks = (const char[]){NEWLINE, NULL_CHAR},
		.fallback_chars_count = 2,

		// output
		.tokens_found = 0,
		.tokens = tokens,
	};

	status_t s = split_str(&p);
	if (p.tokens_found < meta->required_tokens) {
		log_fatal("%s Failed to split %d-line. Found %u tokens.", fn,
			  p.delimiter, p.tokens_found);
		return NULL;
	}

	// Extract ID tokens
	char *id_tokens[meta->id_token_count];
	for (idx_t i = 0; i < meta->id_token_count; i++) {
		id_tokens[i] = tokens[meta->id_token_indices[i]];
	}

	struct ref_id *r_id = alloc_ref_id(id_tokens, meta->id_token_count);
	if (!r_id) {
		log_fatal("Failed to allocate ref_id for %d-line.",
			  meta->line_prefix);
		return NULL;
	}

	// Parse the data string
	const char *data_str = tokens[meta->data_col_index];
	idx_t step_count = count_steps(meta->line_prefix, data_str);
	struct ref_walk *w = alloc_ref_walk(step_count);
	if (!w) {
		log_fatal("%s Failed to allocate walk for %d-line.", fn,
			  meta->line_prefix);
		destroy_ref_id(&r_id);
		exit(1);
	}

	status_t res = meta->parse_data_line(data_str, &w);
	if (res != SUCCESS) {
		log_error("%s Failed to parse data for %d-line.", fn,
			  meta->line_prefix);
		return NULL;
	}

	for (size_t i = 0; i < MAX_TOKENS; i++) {
		if (tokens[i] != NULL)
			free(tokens[i]);
		tokens[i] = NULL;
	}
	/* struct ref_walk *w = alloc_ref_walk(0); */

	return alloc_ref(meta->line_prefix, &w, &r_id);
}

struct ref *parse_ref_line(enum gfa_line_prefix prefix, const char *line)
{
	const char *fn = "[liteseq::refs::parse_ref_line]";
	switch (prefix) {
	case (P_LINE):
		return parse_line_generic(line, &metadata[P_LINE]);
	case (W_LINE):
		return parse_line_generic(line, &metadata[W_LINE]);
	default:
		log_error("%s Unsupported line prefix.", fn);
		return NULL;
	}
}
