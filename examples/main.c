#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/gfa.h"

int main() {
  const char *fp = "./test_data/LPA.gfa";
  const char *ref1 = "chm13__LPA__tig00000001";
  const char *ref2 = "HG002__LPA__tig00000001";
  idx_t ref_count = 2;
  const char *refs[] = {ref1,ref2};

  gfa_config conf ={
    .fp = fp,
    .inc_vtx_labels = true,
    .inc_refs = true,
    .ref_count = ref_count,
    .ref_names = refs
  };

  gfa_props *gfa = gfa_new(&conf);

  /* confirm the refs parsed */
  printf("refs:\n");
  for (size_t i = 0; i < gfa->ref_count; i++) {
    printf("%s\n", gfa->refs[i].name);
  }

  gfa_free(gfa);

  return 0;
}

