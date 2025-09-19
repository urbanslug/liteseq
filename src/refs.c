#ifdef __linux__
#define _GNU_SOURCE
#elif defined(_WIN32)
#define strtok_r strtok_s strndup strdup
#endif


#include "../include/liteseq/refs.h"

ref *create_blank_ref() {
  ref *r = malloc(sizeof(ref));
  r->data = NULL;
  ref_name n = {
    .line_type = W_LINE,
    .type = COARSE,
    .coarse_name = NULL
  };
  r->name = n;

  return r;
}

ref *create_ref(ref_name rn, ref_data *rd) {
  ref *r = malloc(sizeof(ref));
  r->data = rd;
  r->name = rn;

  return r;
}

ref_name create_ref_name_pansn(ref_line_type_e lt, pansn_ref_name *pn) {
  ref_name rn;
  rn.line_type = lt;
  rn.type = PANSN;
  rn.pansn_value = *pn;

  char hap_id_str[MAX_DIGITS];
  snprintf(hap_id_str, sizeof hap_id_str, "%u", pn->hap_id);

  size_t need = strlen(pn->sample_name) + strlen(hap_id_str) +
                strlen(pn->contig_name) + 1;

  rn.tag = (char *)malloc(need);
  if (!rn.tag) {
    return rn;
  }

  sprintf(rn.tag, "%s#%u#%s", pn->sample_name, pn->hap_id, pn->contig_name);

  return rn;
}

ref_name create_ref_name_coarse(ref_line_type_e lt, char *cn) {
  ref_name rn;
  rn.line_type = lt;
  rn.type = COARSE;
  rn.coarse_name = strdup(cn);
  rn.tag = NULL;

  return rn;
}

const char *get_ref_name_tag(const ref_name *rn) {
  if (!rn) {
    return NULL;
  }

  if (rn->type == COARSE) {
    return rn->coarse_name;
  }

  return rn->tag;
}

const char *get_tag(const ref *r) {
  if (!r) {
    return NULL;
  }

  return get_ref_name_tag(&r->name);
}

const char *get_sample_name(const ref *r) {
  if (!r) {
    return NULL;
  }

  return (r->name.type == PANSN) ? r->name.pansn_value.sample_name : r->name.coarse_name;
}

idx_t get_step_count(const ref *r) { return r->data->step_count; }

idx_t get_hap_len(const ref *r) {
  return r->data->hap_len;
}

ref_data* create_ref_data(idx_t step_count) {
  ref_data *r = malloc(sizeof(ref_data));

  if (!r) {
    fprintf(stderr, "Error: Failed to allocate memory for ref_data\n");
    return NULL;
  }

  r->s = malloc(sizeof(strand_e) * step_count);
  r->v_id = malloc(sizeof(id_t) * step_count);
  r->locus = malloc(sizeof(idx_t) * step_count);

  if (!r->s || !r->v_id || !r->locus) {
    fprintf(stderr, "Error: Failed to allocate memory for ref_data arrays\n");
    free(r->s); // Free any partially allocated resources
    free(r->v_id);
    free(r->locus);
    free(r);
    return NULL;
  }

  r->step_count = step_count;
  r->hap_len = 0;

  return r;
}

/** returns the token count */
idx_t tokenize(char *str, const char delim, char **tokens, idx_t token_limit) {

  if (str == NULL) {
    fprintf(stderr, "Error: Input string is NULL\n");
    return 0;
  }

  char *saveptr;
  char *token = strtok_r(str, &delim, &saveptr);
  idx_t count = 0;

  while (count < token_limit && token != NULL) {
    tokens[count++] = token;
    token = strtok_r(NULL, &delim, &saveptr);
  }

  return count;
}

status_t try_parse_pansn(char *name, const char delim, pansn_ref_name *pn) {
  char *name_copy = strdup(name);
  const idx_t PANSN_MAX_TOKENS = 3;
  char *tokens[MAX_TOKENS] = {NULL};
  int num_tokens = tokenize(name_copy, delim, tokens, MAX_TOKENS);


  if (num_tokens != PANSN_MAX_TOKENS) {
    pn->sample_name = NULL;
    pn->contig_name = NULL;
    pn->hap_id = NULL_ID;
    return -1;
  }

  const idx_t SAMPLE_COL = 0;
  const idx_t HAP_ID_COL = 1;
  const idx_t CONTIG_NAME_COL = 2;

  pn->sample_name = strdup(tokens[0]);
  pn->contig_name = strdup(tokens[2]);
  pn->hap_id = atol(tokens[1]);

  free(name_copy);
  name_copy = NULL;
  return  0;
}

static inline strand_e parse_direction(ref_line_type_e line_type, const char c) {
  switch (line_type) {
  case W_LINE: return (c == W_LINE_FORWARD_SYMBOL) ? FORWARD : REVERSE;
  case P_LINE: return (c == P_LINE_FORWARD_SYMBOL) ? FORWARD : REVERSE;
  default: exit(1);  // Default case
  }
}

static inline bool is_step_sep(ref_line_type_e line_type, const char c) {
  switch (line_type) {
  case P_LINE: return c == P_LINE_FORWARD_SYMBOL || c == P_LINE_REVERSE_SYMBOL;
  case W_LINE: return c == W_LINE_FORWARD_SYMBOL || c == W_LINE_REVERSE_SYMBOL;
  default: exit(1);  // Default case
  }
}

/**
 * @brief Count the number of steps in a P line path; it is the number of commas
 * + 1
 *
 * @param [in] str the path string
 * @return the number of steps in the path
 */
idx_t count_steps(ref_line_type_e line_type, const char *str) {
  idx_t steps = 0;
  for (; *str; str++) {
    if (is_step_sep(line_type, *str)) {
      steps++;
    }
  }

  return steps;
}

status_t parse_data_line_w(const char *str, ref_data *r) {
  if (str == NULL) {
    fprintf(stderr, "Error: Input string is NULL\n");
    return ERROR_CODE_INVALID_ARGUMENT;
  }

  #define MAX_DIGITS 12 // Maximum number of digits we expect in a vertex ID
  char str_num[MAX_DIGITS] = {0};
  size_t digit_pos = 0; // current position in str_num
  size_t v_id = 0;
  idx_t step_count = 0;
  strand_e curr_or;

  for (; *str; str++) {
    switch (*str) {
    case W_LINE_FORWARD_SYMBOL:
    case W_LINE_REVERSE_SYMBOL:
      if (step_count > 0) {
        id_t v_id = strtoul(str_num, NULL, 10);
        r->v_id[step_count-1] = strtoul(str_num, NULL, 10);
        r->s[step_count-1] = curr_or;
        r->locus[step_count-1] = -1;
      }
      curr_or = (*str == W_LINE_FORWARD_SYMBOL) ? FORWARD : REVERSE;
      step_count++;
      digit_pos = 0;
      memset(str_num, 0, sizeof(char) * MAX_DIGITS);
      break;
    default: // accumulate the str
      if (digit_pos >= MAX_DIGITS) {
        fprintf(stderr, "Error: Vertex ID exceeds maximum length of %d digits\n", MAX_DIGITS);
        return ERROR_CODE_OUT_OF_BOUNDS;
      }
      str_num[digit_pos++] = *str;
    }
  }

  // Handle trailing token if valid
  if (digit_pos > 0) {
    r->v_id[step_count-1] = strtoul(str_num, NULL, 10);
    r->s[step_count-1] = curr_or;
  }

  return SUCCESS;
}

status_t parse_data_line_p(const char *str, ref_data *r) {
  if (str == NULL) {
    fprintf(stderr, "Error: Input string is NULL\n");
    return ERROR_CODE_INVALID_ARGUMENT;
  }

  char str_num[MAX_DIGITS] = {0};
  size_t digit_pos = 0; // current position in str_num
  size_t v_id = 0;
  idx_t step_count = 0;

  for (; *str; str++) {
    switch (*str) {
    case P_LINE_FORWARD_SYMBOL:
      v_id = strtoul(str_num, NULL, 10);
      r->v_id[step_count] = v_id;
      r->s[step_count] = FORWARD;
      break;
    case P_LINE_REVERSE_SYMBOL:
      v_id = strtoul(str_num, NULL, 10);
      r->v_id[step_count] = v_id;
      r->s[step_count] = REVERSE;
      break;
    case COMMA_CHAR:
      step_count++;
      digit_pos = 0;
      memset(str_num, 0, sizeof(char) * MAX_DIGITS);
      break;
    default: // accumulate the str
      str_num[digit_pos++] = *str;
      if (digit_pos >= MAX_DIGITS) {
        fprintf(stderr, "Error: Vertex ID exceeds maximum length of %d digits\n", MAX_DIGITS);
        return -1;
      }
    }
  }

  return SUCCESS;
}

ref* parse_w_line(char *line) {
  char *tokens[MAX_TOKENS] = {NULL};
  idx_t line_token_count = tokenize(line, TAB_CHAR, tokens, READ_W_LINE_TOKENS);

  if (line_token_count < READ_W_LINE_TOKENS) {
    printf("Tokenisation failed. Found %u tokens", line_token_count);
  }

  const idx_t SAMPLE_COL = 1;
  const idx_t HAP_ID_COL = 2;
  const idx_t CONTIG_NAME_COL = 3;

  const idx_t DATA_COL = 6;

  // W line will always have names in PanSN format
  pansn_ref_name pn;
  pn.sample_name = strdup(tokens[SAMPLE_COL]);
  pn.contig_name = strdup(tokens[CONTIG_NAME_COL]);
  pn.hap_id = atol(tokens[HAP_ID_COL]);

  ref_name rn = create_ref_name_pansn(W_LINE, &pn);

  const char *data_str = tokens[DATA_COL];
  idx_t step_count = count_steps(W_LINE, data_str);
  ref_data *rd = create_ref_data(step_count);
  parse_data_line_w(data_str, rd);

  return create_ref(rn, rd);
}

ref *parse_p_line(char *line) {
  char *tokens[MAX_TOKENS] = {NULL};
  idx_t line_token_count = tokenize(line, TAB_CHAR, tokens, READ_P_LINE_TOKENS);

  if (line_token_count < READ_P_LINE_TOKENS) {
    printf("Failed to tokenise P Line. Found %u tokens.\n", line_token_count);
    return NULL;
  }

  const idx_t REF_NAME_COL = 1;
  const idx_t DATA_LINE_COL = 2;

  char *ref_tag = tokens[REF_NAME_COL];
  if (ref_tag == NULL) {
    perror("Could not\n");
    return NULL;
  }

  pansn_ref_name pn;
  status_t res_parse_name = try_parse_pansn(ref_tag, HASH_CHAR, &pn);
  ref_name rn = (res_parse_name == 0)
    ? create_ref_name_pansn(P_LINE, &pn)
    : create_ref_name_coarse(P_LINE, tokens[REF_NAME_COL]);




  const char *data_str = tokens[DATA_LINE_COL];
  idx_t step_count = count_steps(P_LINE, data_str);
  ref_data *rd = create_ref_data(step_count);

  status_t res_parse_data = parse_data_line_p(data_str, rd);

  return create_ref(rn, rd);
}

ref* parse_ref_line(ref_line_type_e line_type, char *line) {
  switch (line_type) {
  case (P_LINE): return parse_p_line(line);
  case (W_LINE): return parse_w_line(line);
  default: return NULL;
  }
}

void destroy_ref_data(ref_data *r) {
  if (r == NULL) {
    return;
  }

  if (r->v_id != NULL) {
    free(r->v_id);
    r->v_id = NULL;
  }

  if (r->s != NULL) {
    free(r->s);
    r->s = NULL;
  }

  if (r->locus != NULL) {
    free(r->locus);
    r->locus = NULL;
  }

  if (r != NULL) {
    free(r);
    r = NULL;
  }
}

void destroy_ref_name(ref_name *rn) {
  if (rn == NULL) {
    return;
  }

  if (rn->type == COARSE) {
    if (rn->coarse_name != NULL) {
      free(rn->coarse_name);
      rn->coarse_name = NULL;
    }
  } else if (rn->type == PANSN) {
    if (rn->pansn_value.sample_name != NULL) {
      free(rn->pansn_value.sample_name);
      rn->pansn_value.sample_name = NULL;
    }
    if (rn->pansn_value.contig_name != NULL) {
      free(rn->pansn_value.contig_name);
      rn->pansn_value.contig_name = NULL;
    }
    if (rn->tag != NULL) {
      free(rn->tag);
      rn->tag = NULL;
    }
  }
}

void destroy_ref(ref **r) {
  if (r == NULL || *r == NULL) {
    return;
  }

  destroy_ref_data((*r)->data);
  destroy_ref_name(&(*r)->name);
  if ((*r) != NULL) {
    free(*r);
    *r = NULL;
  }
}
