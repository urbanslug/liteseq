#if defined(__linux__)
#define _GNU_SOURCE // Enable GNU extensions for Linux-specific behavior
#elif defined(_WIN32)
// Provide portable macros for missing functionality on Windows
#define strtok_r strtok_s
#define strndup _strndup
#define strdup _strdup
#elif defined(__APPLE__)
// No additional definitions required for macOS
#else
#error "Platform not supported"
#endif

#include "../include/liteseq/gfa.h"
#include "../include/liteseq/types.h"
#include "../src/internal/lq_io.h"
#include "../src/internal/lq_utils.h"
#include "liteseq/refs.h"

#include <log.h>

#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
/*
 * Utility Functions
 * -----------------
 */

#define EXPECTED_S_LINE_TOKENS 3 // the number of tokens expected in a S line
#define H_LINE_VERSION_IDX 1	 // the index of the version token in the H line

void tokens_free(char **tokens)
{
	for (size_t i = 0; i < MAX_TOKENS && tokens[i] != NULL; i++) {
		free(tokens[i]);
		tokens[i] = NULL;
	}
	// is a memset here faster or better suited in case of errors?
}

/* clean up previously allocated tokens and line_copy */
status_t cleanup_tokenise(char *buf, char **tokens)
{
	tokens_free(tokens);
	free(buf); // free the line copy buffer
	return 0;
}

/**
 * @brief Using tabs as the separator, extracts up to MAX_TOKENS tokens from
 * the input string and stores them in the tokens array
 *
 * @param [in] str the string to tokenise
 * @param [in] line_length the length of the input string
 * @param [in] expected_token_count the expected number of tokens
 * @param [out] tokens array to store the extracted tokens
 * @return the number of tokens extracted
 */
int tokenise(const char *str, size_t line_length, size_t expected_token_count,
	     char **tokens)
{
	char *fn_name = "[liteseq::gfa::tokenise]";
	/*
	  Thread-safe tokenization state
	  The saveptr argument ensures that tokenization state is isolated for
	  each thread, avoiding interference. Each thread now maintains its own
	  state, preventing out-of-sync tokenisation.
	*/
	char *saveptr;
	size_t token_count = 0;
	char *line_copy =
		strndup(str, line_length); // buffer to store the current line
	if (!line_copy) {
		log_error("%s Failed to allocate memory for line_copy",
			  fn_name);
		return ERROR_CODE_OUT_OF_MEMORY;
	}
	char *token = strtok_r(line_copy, TAB_STR,
			       &saveptr); // extract the first token
	// char *token = strtok(line_copy, TAB_STR);

	// Extract the tokens and append them to tokens array
	while (token != NULL && token_count < MAX_TOKENS) {
		tokens[token_count] = strdup(token);
		if (!tokens[token_count]) {
			fprintf(stderr,
				"%s Failed to allocate memory for token",
				fn_name);
			cleanup_tokenise(line_copy, tokens);
			return -1;
		}

		token_count++;
		token = strtok_r(NULL, TAB_STR,
				 &saveptr); // extract the next token
	}

	tokens[token_count] = NULL; // Null-terminate the tokens array

	if ((size_t)token_count < expected_token_count) {
		fprintf(stderr,
			"%s Error: Expected at least %ld tokens, but got %ld. "
			"Line: %.*s\n",
			fn_name, expected_token_count, token_count,
			(int)line_length, str);
		cleanup_tokenise(line_copy, tokens);
		return -1;
	}

	free(line_copy); // free the line copy buffer
	return (int)token_count;
}

/*
 * GFA Parse Functions
 * -------------------
 */

status_t set_version(const char *h_line, gfa_props *g)
{
	char *tokens[EXPECTED_H_LINE_TOKENS] = {NULL};
	struct split_str_params p = {
		.str = h_line,
		.delimiter = TAB_CHAR,
		.fallbacks = "",
		.fallback_chars_count = 0,
		.max_splits = EXPECTED_H_LINE_TOKENS,

		.tokens_found = 0,
		.tokens = tokens,
		.end = NULL,
	};

	status_t res = split_str(&p);
	if (res != SUCCESS) {
		log_fatal("Could not parse H line");
		return res;
	}

	const char *version_str = tokens[H_LINE_VERSION_IDX];
	g->version = from_string_gfa_version(version_str);
	if (g->version == gfa_version_INVALID) {
		log_fatal("Unsupported GFA version: %s", version_str);
	}

	tokens_free(tokens);

	return SUCCESS;
}

/**
 * @brief used to extract the v id from an S line
 *
 * @param [in] str the start of the S line
 * @return the vertex id
 */
u32 get_num_vid(const char *str, u32 linum)
{
	char *start = NULL;
	char *end = NULL;
	int count = 0;
	do {
		start = end;
		end = strchr(str, TAB_CHAR);
		if (end == NULL)
			log_fatal("Badly formatted S Line on line %u", linum);
		count++;
	} while (count < 2);

	u32 num = strtoull(start, &end, 10);

	return num;
}

void set_v_id_bounds(const char *curr_char, gfa_props *g, idx_t linum)
{
	u32 curr_v_id = get_num_vid(curr_char, linum);
	if (curr_v_id > g->max_v_id)
		g->max_v_id = curr_v_id;
	if (curr_v_id < g->min_v_id)
		g->min_v_id = curr_v_id;
}

/**
 * Process memory mapped file line by line and count the number of S, L, and P
 * lines
 *
 * @param [in] gfa_props gfa metadata
 * @return 0 on success, -1 on failure
 */
status_t analyse_gfa_structure(gfa_props *g)
{
	idx_t curr_line = 0;
	g->s_line_count = 0;
	g->l_line_count = 0;
	g->p_line_count = 0;
	g->w_line_count = 0;

	char *curr_char = g->start;
	while (curr_char < g->end) {
		// Find the next newline
		char *newline = memchr(curr_char, NEWLINE, g->end - curr_char);
		// If no newline is found, process the remainder of the file
		if (!newline) {
			newline = g->end;
		}

		switch (curr_char[0]) {
		case GFA_S_LINE:
			g->s_line_count++;
			set_v_id_bounds(curr_char, g, curr_line);
			break;
		case GFA_L_LINE:
			g->l_line_count++;
			break;
		case GFA_P_LINE:
			g->p_line_count++;
			break;
		case GFA_W_LINE:
			g->w_line_count++;
			break;
		case GFA_H_LINE:
			if (set_version(curr_char, newline, g) != 0) {
				fprintf(stderr, "[liteseq::gfa] Failed to set "
						"GFA version\n");
				return -1;
			}
			break;
		default: // unsupported line type
			fprintf(stderr,
				"[liteseq::gfa] Unsupported line type: [%c] on "
				"line: [%d]\n",
				curr_char[0], curr_line);
			return -2;
		}

		// Move to the next line
		curr_char = newline + 1; // Skip the newline character
		curr_line++;
	}

	return 0;
}

status_t index_lines(gfa_props *gfa)
{
	idx_t s_idx = 0;
	idx_t l_idx = 0;
	idx_t p_idx = 0;
	idx_t w_idx = 0;
	idx_t line_count = 0;	      // reset line count
	char *curr_char = gfa->start; // reset the current character

	char *newline;
	line curr_line;
	idx_t line_length;

	/*  allocate ... */
	gfa->s_lines = (line *)malloc(gfa->s_line_count * sizeof(line));
	gfa->l_lines = (line *)malloc(gfa->l_line_count * sizeof(line));
	gfa->p_lines = (line *)malloc(gfa->p_line_count * sizeof(line));
	gfa->w_lines = (line *)malloc(gfa->w_line_count * sizeof(line));

	if (!gfa->s_lines || !gfa->l_lines || !gfa->p_lines) {
		perror("Failed to allocate memory for line indices");
		return -1;
	}

	while (curr_char < gfa->end) {
		// Find the next newline
		newline = memchr(curr_char, NEWLINE, gfa->end - curr_char);
		// If no newline is found, process the remainder of the file
		if (!newline) {
			newline = gfa->end;
		}

		line_length = (idx_t)(newline - curr_char);
		curr_line = (line){.start = curr_char,
				   .line_idx = line_count++,
				   .len = line_length};

		switch (curr_char[0]) {
		case GFA_S_LINE:
			gfa->s_lines[s_idx++] = curr_line;
			break;
		case GFA_L_LINE:
			gfa->l_lines[l_idx++] = curr_line;
			break;
		case GFA_W_LINE:
			gfa->w_lines[w_idx++] = curr_line;
			break;
		case GFA_P_LINE:
			gfa->p_lines[p_idx++] = curr_line;
			break;
		case GFA_H_LINE:
			line_count++;
			break;
		default: // unsupported line type
			fprintf(stderr,
				"Unsupported line type: [%c] on line: [%d]\n",
				curr_char[0], line_count);
			return -1;
		}

		// Move to the next line
		curr_char = newline + 1; // Skip the newline character
	}

	return 0;
}

status_t handle_s(const char *s_line, idx_t line_length, size_t idx,
		  char **tokens, bool inc_vtx_labels, void *s)
{
	vtx *vertices = (vtx *)s;

	struct split_str_params p = {
		.str = s_line,
		.delimiter = TAB_CHAR,
		.fallbacks = "",
		.fallback_chars_count = 0,
		.max_splits = 3,

		.tokens_found = 0,
		.tokens = tokens,
		.end = NULL,
	};

	status_t res = split_str(&p);
	if (res != SUCCESS) {
		log_fatal("could not parse S line");
		return res;
	}

	vtx v = {.id = strtoul(tokens[1], NULL, 10),
		 .seq = inc_vtx_labels ? tokens[2] : NULL};
	vertices[v.id] = v;

	free(tokens[0]); // free the line type token
	free(tokens[1]); // free the vertex ID token

	// free the sequence token only if vertex labels are not included
	if (!inc_vtx_labels)
		free(tokens[2]);

	return 0;
}

/**
 * @brief a wrapper function for handle_s_lines
 */
void *thread_handle_s_lines(void *arg)
{
	void **args = (void **)arg;
	void *vertices = args[0];
	line *line_positions = (line *)args[1];
	idx_t line_count = *((idx_t *)args[2]);
	// bool cfg = args[3];
	bool inc_vtx_labels = *((bool *)args[3]);

	// temporary storage for the tokens extracted from a given line
	char *tokens[EXPECTED_S_LINE_TOKENS] = {NULL};

	for (idx_t i = 0; i < line_count; i++) {
		handle_s(line_positions[i].start, line_positions[i].len, i,
			 tokens, inc_vtx_labels, vertices);
	}

	return NULL;
}

/**
 * In a self loop the source (src) and sink (snk) are the same value
 * A self loop can be in the forward, reverse, or mixed strand
 *
 * Examples:
 * L 1 + 1 + is a forward self loop
 * L 1 - 1 - is a reverse self loop
 *
 * L 1 + 1 -  and L 1 - 1 + are mixed self loops that aren't
 * representable in a bidirected graph without node duplication
 *
 * @param [in] l_line the line to parse
 * @param [in] line_length the length of the line
 * @param [in] idx the index of the line
 * @param [in] tokens the tokens to parse
 * @param [in] c the config
 * @param [out] e the edges to populate
 * @return 0 on success, -1 on failure
 */
status_t handle_l(const char *l_line, idx_t line_length, size_t idx,
		  char **tokens, bool c, void *e)
{
	UNUSED(c); // mark the config as unused. Suppress unused parameter
		   // warning
	edge *edges = (edge *)e;

	int token_count =
		tokenise(l_line, line_length, EXPECTED_L_LINE_TOKENS, tokens);
	if (token_count == -1) {
		return -1;
	}

	// Parse vertex IDs
	size_t v1_id = strtoul(tokens[1], NULL, 10);
	size_t v2_id = strtoul(tokens[3], NULL, 10);

	// parse strand symbols. A strand symbol is either + or -
	char v1_strand_symbol = tokens[2][0];
	char v2_strand_symbol = tokens[4][0];

	// Determine vertex sides based on strand symbols
	vtx_side_e v1_side, v2_side;
	if (unlikely(v1_id == v2_id)) { // check for self loop
		if (v1_strand_symbol != v2_strand_symbol) {
			fprintf(stderr,
				"Error: Invalid self loop: %ld %c and %ld %c\n",
				v1_id, v1_strand_symbol, v2_id,
				v2_strand_symbol);
			return -1;
		} else if (v1_strand_symbol == v2_strand_symbol) {
			v1_side = LEFT;
			v2_side = RIGHT;
		}
	} else {
		v1_side = (v1_strand_symbol == '+') ? RIGHT : LEFT;
		v2_side = (v2_strand_symbol == '+') ? LEFT : RIGHT;
	}

	// populate the edge
	edges[idx] = (edge){.v1_id = v1_id,
			    .v1_side = v1_side,
			    .v2_id = v2_id,
			    .v2_side = v2_side};

	tokens_free(tokens);

	return 0;
}

/**
 * @brief a wrapper function for handle_l_lines
 */
void *thread_handle_l_lines(void *arg)
{
	void **args = (void **)arg;
	void *edges = args[0];
	line *line_positions = (line *)args[1];
	idx_t line_count = *((idx_t *)args[2]);

	// temporary storage for the tokens extracted from a given line
	char *tokens[MAX_TOKENS] = {NULL};

	for (idx_t i = 0; i < line_count; i++) {
		handle_l(line_positions[i].start, line_positions[i].len, i,
			 tokens, false, edges);
	}

	return NULL;
}

struct ref_thread_data {
	struct ref **refs;
	line *p_lines; // metadata for a P line
	line *w_lines; // metadata for a W line
	idx_t p_line_count;
	idx_t w_line_count;
};

/**
 * @brief a wrapper function for handle_p_lines
 */
void *thread_handle_p_lines(void *ref_metadata)
{
	struct ref_thread_data *data = (struct ref_thread_data *)ref_metadata;
	line *p_lines = data->p_lines;
	line *w_lines = data->w_lines;
	idx_t p_line_count = data->p_line_count;
	idx_t w_line_count = data->w_line_count;
	struct ref **refs = data->refs;
	idx_t ref_idx = 0;

	for (idx_t i = 0; i < p_line_count; i++)
		refs[ref_idx++] = parse_ref_line(P_LINE, p_lines[i].start);

	for (idx_t i = 0; i < w_line_count; i++)
		refs[ref_idx++] = parse_ref_line(W_LINE, w_lines[i].start);

	return NULL;
}

status_t set_ref_loci(gfa_props *gfa)
{
	vtx *vs = gfa->v;
	if (vs == NULL) {
		return ERROR_CODE_INVALID_ARGUMENT;
	}

	for (int i = 0; i < gfa->ref_count; i++) {
		struct ref *r = gfa->refs[i];
		struct ref_walk *rw = r->walk;
		idx_t locus = 1; // DNA is 1 indexed
		for (int j = 0; j < rw->step_count; j++) {
			rw->loci[j] = locus;
			idx_t v_id = rw->v_ids[j];
			vtx vv = vs[v_id];
			const char *s = vv.seq;
			idx_t l = strlen(s);
			locus += l;
		}
		set_hap_len(r, locus - 1);
	}

	return SUCCESS;
}

status_t populate_gfa(gfa_props *gfa)
{
	/* Create threads */
	pthread_t thread_s, thread_l, thread_p;

	/* Prepare arguments for threads */
	void *args_s[] = {(void *)gfa->v, gfa->s_lines, &gfa->s_line_count,
			  &gfa->inc_vtx_labels};
	void *args_l[] = {(void *)gfa->e, gfa->l_lines, &gfa->l_line_count,
			  &gfa->inc_vtx_labels};

	/* launch threads */
	if (pthread_create(&thread_s, NULL, thread_handle_s_lines, args_s) !=
	    0) {
		return -1; // Failed to create thread for S lines
	}

	if (pthread_create(&thread_l, NULL, thread_handle_l_lines, args_l) !=
	    0) {
		return -1; // Failed to create thread for L lines
	}

	if (gfa->inc_refs && gfa->inc_vtx_labels) {
		struct ref_thread_data ref_data = {
			.refs = gfa->refs,
			.p_lines = gfa->p_lines,
			.w_lines = gfa->w_lines,
			.p_line_count = gfa->p_line_count,
			.w_line_count = gfa->w_line_count};
		if (pthread_create(&thread_p, NULL, thread_handle_p_lines,
				   (void *)&ref_data) != 0) {
			return -1; // Failed to create thread for P lines
		}
	}

	/* Wait for threads to finish */
	pthread_join(thread_s, NULL);
	pthread_join(thread_l, NULL);
	if (gfa->inc_refs)
		pthread_join(thread_p, NULL);

	/* Set reference loci if references are included */
	if (gfa->inc_refs) {
		gfa->status = set_ref_loci(gfa);
		if (gfa->status != 0) {
			log_fatal("Failed to set reference loci");
			return gfa->status;
		}
	}

	return SUCCESS;
}

/**  pre-allocate memory for vertices, edges, and maybe references
 */
status_t preallocate_gfa(gfa_props *p)
{
	u32 vtx_arr_size = p->max_v_id + 1;
	p->v = malloc(sizeof(vtx) * vtx_arr_size);
	if (!p->v)
		return ERROR_CODE_OUT_OF_MEMORY;
	// init with NULLs makes freeing more straightforward
	// memset sssumes seq pointers are at offset 0 in the vtx structure
	if (p->inc_vtx_labels) {
		memset(p->v, 0, sizeof(vtx) * vtx_arr_size);
	}

	p->e = malloc(p->l_line_count * sizeof(edge));
	if (!p->e)
		return ERROR_CODE_OUT_OF_MEMORY;

	if (p->inc_refs) { // Initialize references (paths)
		p->ref_count = p->p_line_count + p->w_line_count;
		p->refs = malloc(sizeof(struct ref *) * p->ref_count);
		if (!p->refs)
			return ERROR_CODE_OUT_OF_MEMORY;
	}

	return SUCCESS;
}

gfa_props *init_gfa(const gfa_config *conf)
{
	gfa_props *p = (gfa_props *)malloc(sizeof(gfa_props));

	p->fp = conf->fp;
	p->inc_vtx_labels = conf->inc_vtx_labels;
	p->inc_refs = conf->inc_refs;

	p->start = NULL;
	p->end = NULL;
	p->l_lines = NULL;
	p->s_lines = NULL;
	p->p_lines = NULL;
	p->w_lines = NULL;

	p->s_line_count = 0;
	p->l_line_count = 0;
	p->p_line_count = 0;
	p->w_line_count = 0;

	p->ref_count = 0;

	p->min_v_id = UINT32_MAX;
	p->max_v_id = 0;

	p->v = NULL;
	p->e = NULL;
	p->refs = NULL;

	p->file_size = 0;
	p->status = -1;

	return p;
}

gfa_props *gfa_new(const gfa_config *conf)
{
	gfa_props *p = init_gfa(conf); // set up the config
	if (p == NULL) {
		log_fatal("init gfa failed");
		return NULL;
	}

	char *mapped;	      // pointer to the start of the memory mapped file
	char *end;	      // pointer to the end of the memory mapped file
	size_t file_size = 0; // size of the memory mapped file

	p->status = -1; // status of a given operation

	open_mmap(p->fp, &mapped, &file_size);
	if (mapped == NULL) { // Failed to mmap file
		return p;
	}
	end = mapped + file_size;

	p->start = mapped;
	p->end = end;
	p->file_size = file_size;

	p->status = analyse_gfa_structure(p);
	if (p->status != 0) {
		fprintf(stderr, "Error: GFA file structure analysis failed\n");
		return p;
	}

	if (p->s_line_count == 0 && p->l_line_count == 0 &&
	    p->p_line_count == 0) {
		fprintf(stderr, "Error: GFA has no vertices edges or paths\n");
		return p;
	}

	index_lines(p);
	preallocate_gfa(p);
	p->status = populate_gfa(p);
	p->status = 0;

	return p;
}

void gfa_free(gfa_props *gfa)
{
	close_mmap(gfa->start, gfa->file_size);

	if (gfa->s_lines)
		free(gfa->s_lines);

	if (gfa->l_lines)
		free(gfa->l_lines);

	if (gfa->p_lines)
		free(gfa->p_lines);

	if (gfa->w_lines)
		free(gfa->w_lines);

	if (gfa->e)
		free(gfa->e);

	if (gfa->inc_vtx_labels)
		for (idx_t i = 0; i < (gfa->max_v_id + 1); i++)
			if (gfa->v[i].seq != NULL)
				free(gfa->v[i].seq);

	if (gfa->v)
		free(gfa->v);

	if (gfa->refs) {
		for (idx_t i = 0; i < gfa->ref_count; i++)
			destroy_ref(&(gfa->refs[i]));
		if (gfa->refs)
			free(gfa->refs);
	}

	free(gfa);
}
