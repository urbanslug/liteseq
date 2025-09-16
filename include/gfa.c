#ifdef __linux__
#define _GNU_SOURCE
#elif defined(_WIN32)
#define strtok_r strtok_s strndup strdup
#endif

#include "./gfa.h"

/*
 * Utility Functions
 * -----------------
 */

/**
 * @brief Count the number of steps in a P line path; it is the number of commas + 1
 *
 * @param [in] str the path string
 * @return the number of steps in the path
 */
size_t count_steps(const char *str) {
  size_t steps = 0;
  const char *c = str;
  for(; *c; c++) {
    if(*c == COMMA_CHAR) { steps++; }
  }

  return ++steps;
}

void tokens_free(char **tokens) {
  for(size_t i = 0; i < MAX_TOKENS && tokens[i] != NULL; i++) {
    free(tokens[i]);
    tokens[i] = NULL;
  }
  // is a memset here faster or better suited in case of errors?
}

/* clean up previously allocated tokens and line_copy */
status_t cleanup_tokenise(char* buf, char **tokens) {
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
int tokenise(const char *str, size_t line_length, size_t expected_token_count, char **tokens) {
  char *fn_name = "[liteseq::gfa::tokenise]";
  /*
    Thread-safe tokenization state
    The saveptr argument ensures that tokenization state is isolated for each
    thread, avoiding interference.
    Each thread now maintains its own state, preventing out-of-sync tokenisation.
  */
  char *saveptr;
  size_t token_count = 0;
  char *line_copy = strndup(str, line_length); // buffer to store the current line
  if (!line_copy) {
    perror("[liteseq::gfa::tokenise] Failed to allocate memory for line_copy");
    return -1;
  }
  char *token = strtok_r(line_copy, TAB_STR, &saveptr); // extract the first token
  //char *token = strtok(line_copy, TAB_STR);

  // Extract the tokens and append them to tokens array
  while (token != NULL && token_count < MAX_TOKENS) {
    tokens[token_count] = strdup(token);
    if (!tokens[token_count]) {
      fprintf(stderr, "%s Failed to allocate memory for token", fn_name);
      cleanup_tokenise(line_copy, tokens);
      return -1;
    }

    token_count++;
    token = strtok_r(NULL, TAB_STR, &saveptr); // extract the next token
  }

  tokens[token_count] = NULL; // Null-terminate the tokens array


  if ((size_t) token_count < expected_token_count) {
    fprintf(stderr, "%s Error: Expected at least %ld tokens, but got %ld. Line: %.*s\n",
            fn_name, expected_token_count, token_count, (int)line_length, str);
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

status_t set_version(char *curr_char, char *newline, gfa_props *g) {
  idx_t line_length = (idx_t)(newline - curr_char);
  char *tokens[MAX_TOKENS] = {NULL};
  int token_count;
  const char *version_str = NULL;
  status_t result = -1; // assume failure

  token_count = tokenise(curr_char, line_length, EXPECTED_H_LINE_TOKENS, tokens);
  // TODO: check if token_count is -1
  version_str = tokens[1];

  // TODO: is this better than a chain of if statements?
  g->version = UNSUPPORTED;
  for (size_t i = 0; i < gfa_version_map_len; ++i) {
    if (strcmp(version_str, gfa_version_map[i].tag) == 0) {
      g->version = gfa_version_map[i].version;
      break;
    }
  }

  if (g->version == UNSUPPORTED) {
    fprintf(stderr, "[liteseq::gfa] Unsupported GFA version: %s\n", version_str);
  }
  else {
    result = 0; // success
  }

  tokens_free(tokens);
  return result;
}

/**
 * Process memory mapped file line by line and count the number of S, L, and P lines
 *
 * @param [in] gfa_props gfa metadata
 * @return 0 on success, -1 on failure
 */
status_t analyse_gfa_structure(gfa_props *g) {
  idx_t curr_line = 0;
  g->s_line_count = 0;
  g->l_line_count = 0;
  g->p_line_count = 0;

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
        fprintf(stderr, "[liteseq::gfa] Failed to set GFA version\n");
        return -1;
      }
      break;
    default: // unsupported line type
      fprintf(stderr, "[liteseq::gfa] Unsupported line type: [%c] on line: [%d]\n", curr_char[0], curr_line);
      return -2;
    }

    // Move to the next line
    curr_char = newline + 1; // Skip the newline character
    curr_line++;
  }

  if (g->inc_refs) {
    switch (g->version) {
    case GFA_1_0:
      g->ref_count = g->p_line_count;
      break;
    case GFA_1_1:
      g->ref_count = g->w_line_count;
      break;
    default:
      fprintf( stderr, "[liteseq::gfa] Unsupported GFA version for reference parsing  \n");
      return -3;
    }
  }

  return 0;
}


status_t index_lines(gfa_props *gfa) {
  idx_t s_idx = 0;
  idx_t l_idx = 0;
  idx_t p_idx = 0;
  idx_t w_idx = 0;
  idx_t line_count = 0; // reset line count
  char *curr_char = gfa->start;  // reset the current character

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

    line_length = (idx_t) (newline - curr_char);
    curr_line = (line) {.start = curr_char, .line_idx = line_count++, .len = line_length};

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
      fprintf(stderr, "Unsupported line type: [%c] on line: [%d]\n", curr_char[0], line_count);
      return -1;
    }

    // Move to the next line
    curr_char = newline + 1; // Skip the newline character
  }

  return 0;
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
status_t handle_l(const char *l_line, idx_t line_length, size_t idx, char **tokens, bool c, void *e) {
  UNUSED(c); // mark the config as unused. Suppress unused parameter warning
  edge *edges = (edge *) e;

  int token_count = tokenise(l_line, line_length, EXPECTED_L_LINE_TOKENS, tokens);
  if (token_count == -1) { return -1; }

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
              v1_id, v1_strand_symbol, v2_id, v2_strand_symbol);
      return -1;
    }
    else if (v1_strand_symbol == v2_strand_symbol) {
      v1_side = LEFT;
      v2_side = RIGHT;
    }
  }
  else {
    v1_side = (v1_strand_symbol == '+') ? RIGHT : LEFT;
    v2_side = (v2_strand_symbol == '+') ? LEFT : RIGHT;
  }

  // populate the edge
  edges[idx] = (edge) {.v1_id = v1_id, .v1_side = v1_side, .v2_id = v2_id, .v2_side = v2_side};

  tokens_free(tokens);

  return 0;
}

status_t handle_s(const char *s_line, idx_t line_length, size_t idx,
                  char **tokens, bool inc_vtx_labels, void *s) {
  vtx *vertices = (vtx *)s;

  int token_count = tokenise(s_line, line_length, EXPECTED_S_LINE_TOKENS, tokens);
  if (token_count == -1) {
    return -1;
  }

  size_t v_id = strtoul(tokens[1], NULL, 10);
  char *v_label = inc_vtx_labels ? strdup(tokens[2]) : NULL;
  vertices[idx] = (vtx){.id = v_id, .seq = v_label};

  tokens_free(tokens);

  return 0;
}

status_t parse_ref2(step *steps, const char *c) {
  idx_t step_count = 0;

  char str_num[MAX_DIGITS] = {0};
  size_t digit_pos = 0; // current position in str_num
  size_t v_id = 0;

  for (; *c; c++) {
    switch (*c) {
    case '+':
      v_id = strtoul(str_num, NULL, 10);
      steps[step_count] = (step){.v_id = v_id, .s = FORWARD};
      break;
    case '-':
      v_id = strtoul(str_num, NULL, 10);
      steps[step_count] = (step){.v_id = v_id, .s = REVERSE};
      break;
    case ',':
      step_count++;
      digit_pos = 0;
      memset(str_num, 0, sizeof(char) * MAX_DIGITS);
      break;
    default:
      str_num[digit_pos++] = *c;
      if (digit_pos >= MAX_DIGITS) {
        fprintf(stderr,
                "Error: Vertex ID exceeds maximum length of %d digits\n",
                MAX_DIGITS);
        return -1;
      }
    }
  }

  return 0;
}

status_t handle_p(const char *p_line, size_t line_length, size_t idx, char **tokens, bool c, void *p) {
  UNUSED(c); // mark the config as unused. Suppress unused parameter warning
  ref *paths = (ref *) p;

  int token_count = tokenise(p_line, line_length, EXPECTED_P_LINE_TOKENS, tokens);
  if (token_count == -1) { return -1; }

  char *path_name = tokens[1];
  char *path_str = tokens[2];

  idx_t step_count = count_steps(path_str);

  //paths[idx] = (ref){.name = NULL, .steps = NULL};

  step *steps = (step *)malloc(step_count * sizeof(step));

  paths[idx].name = strdup(path_name);
  paths[idx].steps = steps;
  paths[idx].step_count = step_count;

  parse_ref2(steps, path_str);

  tokens_free(tokens);

  return 0;
}


/**
 * @brief a wrapper function for handle_s_lines
 */
void *thread_handle_s_lines(void *arg) {
  void **args = (void **)arg;
  void *vertices = args[0];
  line *line_positions = (line *)args[1];
  idx_t line_count = *((idx_t *)args[2]);
  //bool cfg = args[3];
  bool inc_vtx_labels = *((bool *)args[3]);

  // temporary storage for the tokens extracted from a given line
  char *tokens[MAX_TOKENS] = {NULL};

  for (idx_t i = 0; i < line_count; i++) {
    handle_s(line_positions[i].start, line_positions[i].len, i, tokens, inc_vtx_labels, vertices);
  }

  return NULL;
}

/**
 * @brief a wrapper function for handle_l_lines
 */
void *thread_handle_l_lines(void *arg) {
  void **args = (void **)arg;
  void *edges = args[0];
  line *line_positions = (line *)args[1];
  idx_t line_count = *((idx_t *)args[2]);
  //bool cfg = args[3];

  // temporary storage for the tokens extracted from a given line
  char *tokens[MAX_TOKENS] = {NULL};


  for (idx_t i = 0; i < line_count; i++) {
    handle_l(line_positions[i].start, line_positions[i].len, i, tokens, false, edges);
  }

  return NULL;
}

/**
 * @brief a wrapper function for handle_p_lines
 */
void *thread_handle_p_lines(void *arg) {
  void **args = (void **)arg;
  void *refs = args[0];
  line *line_positions = (line *)args[1];
  idx_t line_count = *((idx_t *)args[2]);
  bool cfg = *((bool *)args[3]);

  // temporary storage for the tokens extracted from a given line
  char *tokens[MAX_TOKENS] = {NULL};

  for (idx_t i = 0; i < line_count; i++) {
    handle_p(line_positions[i].start, line_positions[i].len, i, tokens, cfg, refs);
  }

  return NULL;
}

status_t populate_gfa(gfa_props *gfa) {
  /* Create threads */
  pthread_t thread_s, thread_l, thread_p;

  /* Prepare arguments for threads */
  void *args_s[] = {(void *)gfa->v, gfa->s_lines, &gfa->s_line_count, &gfa->inc_vtx_labels};
  void *args_l[] = {(void *)gfa->e, gfa->l_lines, &gfa->l_line_count, &gfa->inc_vtx_labels};

  /* launch threads */
  if (pthread_create(&thread_s, NULL, thread_handle_s_lines, args_s) != 0) {
    return -1; // Failed to create thread for S lines
  }

  if (pthread_create(&thread_l, NULL, thread_handle_l_lines, args_l) != 0) {
    return -1; // Failed to create thread for L lines
  }

  if (gfa->inc_refs) {
    line *ref_lines = gfa->version == GFA_1_0 ? gfa->p_lines : gfa->w_lines;
    void *args_p[] = {(void *)gfa->refs, ref_lines, &gfa->ref_count, &gfa->inc_refs};
    if (pthread_create(&thread_p, NULL, thread_handle_p_lines, args_p) != 0) {
      return -1; // Failed to create thread for P lines
    }
  }

  /* Wait for threads to finish */
  pthread_join(thread_s, NULL);
  pthread_join(thread_l, NULL);
  if (gfa->inc_refs) {
    pthread_join(thread_p, NULL);
  }

  return 0;
}

/**  pre-allocate memory for vertices, edges, and maybe references
 */
status_t preallocate_gfa(gfa_props *p) {
  p->v = malloc(p->s_line_count * sizeof(vtx));
  if (p->inc_vtx_labels) {
    for (idx_t i = 0; i < p->s_line_count; i++) {
      p->v[i].seq = NULL;
    }
  }
  if (!p->v) { // Failed to allocate memory for vertices
    return -1;
  }

  // p->edge_count = gfa->l_line_count;
  p->e = malloc(p->l_line_count * sizeof(edge));
  if (!p->e) { // Failed to allocate memory for edges
    return -1;
  }

  if (p->inc_refs) { // Initialize references (paths)
    idx_t rc = p->ref_count == 0 ? p->p_line_count : p->ref_count;

    p->refs = (ref *)malloc(rc * sizeof(ref));
    if (!p->refs) {
      return -1;
    }

    for (idx_t i = 0; i < rc; i++) {
      p->refs[i].name = NULL;
      p->refs[i].steps = NULL;
    }
  }

  return 0;
}

gfa_props *init_gfa(const gfa_config *conf) {
  gfa_props *p = (gfa_props *)malloc(sizeof(gfa_props));

  p->fp = conf->fp;
  p->inc_vtx_labels = conf->inc_vtx_labels;
  p->inc_refs = conf->inc_refs;

  p->start = NULL;
  p->end = NULL;
  p->l_lines= NULL;
  p->s_lines = NULL;
  p->p_lines = NULL;
  p->w_lines = NULL;

  p->s_line_count = 0;
  p->l_line_count = 0;
  p->p_line_count = 0;
  p->w_line_count = 0;

  p->ref_count = 0;

  p->v = NULL;
  p->e = NULL;
  p->refs = NULL;

  p->file_size = 0;
  p->status = -1;

  return p;
}

gfa_props *gfa_new(const gfa_config *conf) {

  // set up the config
  gfa_props *p = init_gfa(conf);

  char *mapped; // pointer to the start of the memory mapped file
  char *end; // pointer to the end of the memory mapped file
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

  if (p->s_line_count == 0 && p->l_line_count == 0 && p->p_line_count == 0) {
    fprintf(stderr, "Error: GFA has no vertices edges or paths\n");
    return p;
  }

  index_lines(p);
  preallocate_gfa(p);
  p->status = populate_gfa(p);
  p->status = 0;

  return p;
}

void gfa_free(gfa_props *gfa) {
  close_mmap(gfa->start, gfa->file_size);

  if (gfa->inc_vtx_labels) {
    for (idx_t i = 0; i < gfa->s_line_count; i++) {
      if (gfa->v[i].seq != NULL) {
        free(gfa->v[i].seq);
      }
    }
  }

  if (gfa->v) { free(gfa->v); }
  if (gfa->e) { free(gfa->e); }

  if (gfa->refs) {
    for (idx_t i = 0; i < gfa->ref_count; i++) {
      free(gfa->refs[i].name);
      free(gfa->refs[i].steps);
    }
    free(gfa->refs);
  }

  if (gfa->s_lines) { free(gfa->s_lines); }
  if (gfa->l_lines) { free(gfa->l_lines); }
  if (gfa->p_lines) { free(gfa->p_lines); }
  if (gfa->w_lines) { free(gfa->w_lines); }

  free(gfa);
}
