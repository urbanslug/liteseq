#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <liteseq/gfa.h>

gfa_config gen_config() {
  const char *fp = "./tests/data/LPA.gfa";

  gfa_config conf_a = {
      .fp = fp,
      .inc_vtx_labels = false,
      .inc_refs = false,
  };

  gfa_config conf_b = {
      .fp = fp,
      .inc_vtx_labels = true,
      .inc_refs = true,
  };

  (void)conf_a; // to avoid unused variable warning
  (void)conf_b; // to avoid unused variable warning

  gfa_config conf_c = {
    .fp = fp,
    .inc_vtx_labels = false,
    .inc_refs = true
  };

  return conf_c;
}

int main() {
  gfa_config conf = gen_config();
  gfa_props *gfa = gfa_new(&conf);

  if(gfa->status != 0) {
    printf("GFA parsing failed. Status: %d\n", gfa->status);
    gfa_free(gfa);
    return -1;
  }

  printf("vtx count: %d\n", gfa->s_line_count);

  /* confirm the refs parsed */
  if (gfa->inc_refs) {
    printf("refs:\n");
    for (size_t i = 0; i < gfa->ref_count; i++) {
      printf("%s\n", gfa->refs[i].name);
    }
  }
  else {
    printf("No refs parsed\n");
  }

  gfa_free(gfa);

  return 0;
}
