#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/gfa.h"

int main() {
  // const char *fp = "./test_data/LPA.gfa";
  const char *fp = "./test_data/gfa_with_w_lines.gfa";
  const char *ref1 = "chm13__LPA__tig00000001";
  const char *ref2 = "HG002__LPA__tig00000001";
  idx_t ref_count = 2;
  const char *refs[] = {ref1,ref2};

  gfa_config conf_a ={
    .fp = fp,
    .inc_vtx_labels = false,
    .inc_refs = false,
    .ref_count = 0,
    .ref_names = refs
  };

  gfa_config conf_b = {
    .fp = fp,
    .inc_vtx_labels = true,
    .inc_refs = true,
    .ref_count = ref_count,
    .ref_names = refs
  };

  gfa_props *gfa = gfa_new(&conf_a);

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
