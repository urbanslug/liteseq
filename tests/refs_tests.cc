#include <gtest/gtest.h>

#include <array>

#include <liteseq/refs.h>

template <typename T, std::size_t MAX_COLUMNS>
constexpr std::array<T, MAX_COLUMNS> pad_to_max(const std::initializer_list<T>& input) {
  std::array<T, MAX_COLUMNS> padded = {};
  auto it = input.begin();

  // Fill the array with input values
  for (std::size_t i = 0; i < input.size() && i < MAX_COLUMNS; ++i) {
    padded[i] = *it;
    ++it;
  }

  // Pad the remainder with NULL_ID
  for (std::size_t i = input.size(); i < MAX_COLUMNS; ++i) {
    padded[i] = static_cast<T>(NULL_ID); // Ensure type consistency
  }
  return padded;
}

TEST(Ref, CreateBlank) {
  ref *r = create_blank_ref();
  ASSERT_NE(r, nullptr);
  destroy_ref(&r);
  ASSERT_EQ(r, nullptr);
}

TEST(Ref, CreateWithData) {
  ref_data *rd = NULL;
  ref_name rn = create_ref_name_coarse(W_LINE, strdup("chm13#0#Chr1"));
  ref *r = create_ref(rn, rd);
  ASSERT_NE(r, nullptr);
  destroy_ref(&r);
  ASSERT_EQ(r, nullptr);
}

TEST(Tag, PanSN) {

  pansn_ref_name pn;
  pn.contig_name = strdup("Chr1");
  pn.sample_name = strdup("Chm13");
  pn.hap_id = 0;
  ref_name rn = create_ref_name_pansn(P_LINE, &pn);

  const char *name = "Chm13#0#Chr1";
  ASSERT_STREQ(get_ref_name_tag(&rn), name);
}

TEST(Tag, Coarse) {
  const char *name = "chm13#0#Chr1";
  ref_name rn = create_ref_name_coarse(P_LINE, strdup(name));

  ASSERT_STREQ(get_ref_name_tag(&rn), name);
}

TEST(HapLen, SetsHapLen) {
  ref_data *rd = create_ref_data(5);
  rd->hap_len = 42;

  ref_name rn = create_ref_name_coarse(W_LINE, strdup("sample#1#contig"));
  ref *r = create_ref(rn, rd);

  ASSERT_EQ(get_hap_len(r), 42);
  destroy_ref(&r);
  ASSERT_EQ(r, nullptr);
}

// Demonstrate some basic assertions.
TEST(ParseRefName, Valid) {
  pansn_ref_name pn;
  const char *name = "chm13#0#Chr1";
  const char HASH = '#';
  status_t res = try_parse_pansn((char *)name, HASH, &pn);

  ASSERT_EQ(res, 0);
  ASSERT_STREQ(pn.sample_name, "chm13");
  ASSERT_STREQ(pn.contig_name, "Chr1");
  ASSERT_EQ(pn.hap_id, 0);

  const idx_t N = 3;
  const char *names[] = {
    "chm13#0#Chr1",
    "chm13#1#ChrX",
    "sampleA#2#contig_5"
  };

  const char *expected_samples[] = {"chm13", "chm13", "sampleA"};

  const char *expected_contigs[] = {"Chr1", "ChrX", "contig_5"};
  const id_t expected_hap_ids[] = {0, 1, 2};

  for (idx_t i = 0; i < N; i++) {
    res = try_parse_pansn((char *)names[i], HASH, &pn);
    ASSERT_EQ(res, 0);
    ASSERT_STREQ(pn.sample_name, expected_samples[i]);
    ASSERT_STREQ(pn.contig_name, expected_contigs[i]);
    ASSERT_EQ(pn.hap_id, expected_hap_ids[i]);
  }
}

TEST(ParseRefName, Invalid) {

  const char *names[] = {
    "chm13__LPA__tig00000001",
    "HG002__LPA__tig00000001",
    "HG002__LPA__tig00000005",
    "#1#contig",
    "invalid_name_no_delim",
    "too#many#delims#here"
 };

  const char *expected_sample = NULL;
  const char *expected_contig = NULL;
  const id_t expected_hap_id = NULL_ID;

  printf("exp hap id %d\n", expected_hap_id);

  pansn_ref_name pn;

  const idx_t N = 6;
  status_t res;

  for (idx_t i = 0; i < N; i++) {
    char *name = strdup(names[i]);

    if (name == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(EXIT_FAILURE);
    }

    res = try_parse_pansn(name, HASH_CHAR, &pn);
    ASSERT_EQ(res, -1);
    ASSERT_STREQ(pn.sample_name, expected_sample);
    ASSERT_STREQ(pn.contig_name, expected_contig);
    ASSERT_EQ(pn.hap_id, expected_hap_id);
  }
}

TEST(StepCount, W_LINE) {
  const char *w_line_data_strs[] = {
    ">1>4>5>6>7>9",
    ">1>2>4>5>6>8>9",
    ">1>14>235>634>7>9",
    "",
    ">343",
  };

  const idx_t expected_step_counts[] = {6, 7, 6, 0, 1};

  const idx_t N = 5;
  idx_t res;
  for (idx_t i = 0; i < N; i++) {
    res = count_steps(W_LINE, w_line_data_strs[i]);
    ASSERT_EQ(res, expected_step_counts[i]);
  }
}

TEST(StepCount, P_LINE) {
  const char *p_line_data_strs[] = {
    "3+,4+,6+,8+,9+",
    "612+,614+,615+,617+,619+,621+,623+,624+,626+,628+,630+,631+,633+,634+",
    "",
    "123+",
    "11+,13+,15+,16+,18+,20+,21+,22+,23+,25+,15+,16+,26+,27+,15+,16+,28+",
  };

  const idx_t expected_step_counts[] = {5, 14, 0, 1, 17};

  const idx_t N = 5;
  idx_t res;
  for (idx_t i = 0; i < N; i++) {
    res = count_steps(P_LINE, p_line_data_strs[i]);
    ASSERT_EQ(res, expected_step_counts[i]);
  }
}

TEST(ParseDataLine, P_LINE) {
  const char *p_line_data_strs[] = {
    "3+,4+,6+,8+,9+",
    "612+,614+,615+,617+,619+,621+,623+,624+,626+,628+,630+,631+,633+,634+",
    "",
    "123+",
    "11+,13+,15+,16+,18+,20+,21+,22+,23+,25+,15+,16+,26+,27+,15+,16+,28+",
  };

  const idx_t expected_step_counts[] = {5, 14, 0, 1, 17};


  //#define MAX_COLUMNS 17 // Maximum row length
  constexpr idx_t MAX_COLUMNS = 17; // Maximum row length
  const std::array<std::array<id_t, MAX_COLUMNS>, 5> p_v_ids = {
    pad_to_max<id_t, MAX_COLUMNS>({3, 4, 6, 8, 9, 9}),
    pad_to_max<id_t, MAX_COLUMNS>({612, 614, 615, 617, 619, 621, 623, 624, 626, 628, 630, 631, 633, 634}),
    pad_to_max<id_t, MAX_COLUMNS>({}),
    pad_to_max<id_t, MAX_COLUMNS>({123}),
    pad_to_max<id_t, MAX_COLUMNS>({11, 13, 15, 16, 18, 20, 21, 22, 23, 25, 15, 16, 26, 27, 15, 16, 28}),
  };

  const idx_t N = 5;
  idx_t step_count;
  for (idx_t i = 0; i < N; i++) {
    const char *data_str = p_line_data_strs[i];
    step_count = count_steps(P_LINE, data_str);
    ASSERT_EQ(step_count, expected_step_counts[i]);
    ref_data *rd = create_ref_data(step_count);

    EXPECT_NE(rd, nullptr);
    status_t res = parse_data_line_p(data_str, rd);

    ASSERT_EQ(SUCCESS, res);

    const id_t *v_id_ptr = &p_v_ids[i][0];
    for (idx_t j = 0; j < rd->step_count; j++, v_id_ptr++) {
      ASSERT_EQ(*v_id_ptr, rd->v_id[j]);
    }
    destroy_ref_data(rd);
  }
}

TEST(ParseDataLine, W_LINE) {
  const char *w_line_data_strs[] = {
    ">1>4>5>6>7>9",
    ">1>2>4>5>6>8>9",
    ">1>14>235>634>7>9",
    "",
    ">343",
  };

  const idx_t expected_step_counts[] = {6, 7, 6, 0, 1};

  constexpr idx_t MAX_COLUMNS = 7; // Maximum row length
  const std::array<std::array<id_t, MAX_COLUMNS>, 5> w_v_ids = {
    pad_to_max<id_t, MAX_COLUMNS>({1, 4,   5,  6,  7, 9}),
    pad_to_max<id_t, MAX_COLUMNS>({1, 2,   4,  5,  6, 8, 9}),
    pad_to_max<id_t, MAX_COLUMNS>({1, 14, 235, 634, 7, 9}),
    pad_to_max<id_t, MAX_COLUMNS>({}),
    pad_to_max<id_t, MAX_COLUMNS>({343}),
  };

  const idx_t N = 5;
  idx_t computed_step_count, expected_step_count;
  for (idx_t i = 0; i < N; i++) {
    const char *data_str = w_line_data_strs[i];
    computed_step_count = count_steps(W_LINE, data_str);
    expected_step_count = expected_step_counts[i];

    ASSERT_EQ(computed_step_count, expected_step_count);
    ref_data *rd = create_ref_data(computed_step_count);

    EXPECT_NE(rd, nullptr);
    status_t res = parse_data_line_w(data_str, rd);

    ASSERT_EQ(SUCCESS, res);

    const id_t *v_id_ptr = w_v_ids[i].data();
    for (idx_t j = 0; j < rd->step_count; j++, v_id_ptr++) {
      ASSERT_EQ(*v_id_ptr, rd->v_id[j]);
    }
    destroy_ref_data(rd);
  }
}

TEST(ParseWLines, Valid) {

  const char *w_lines[] = {
    "W\tshort\t1\tchr\t0\t8\t>1>4>5<6>7>9",
    "W\talt\t0\tchr\t0\t9\t>1>2>4>5>6>8>9",
    "W\tshort\t2\tchr\t0\t8\t>1>2<5>6>7<9",
  };


  const id_t hap_ids[] = { 1, 0, 2 };
  const char *sample_names[] = { "short", "alt", "short"};
  const char *expected_tags[] = {"short#1#chr", "alt#0#chr", "short#2#chr"};

  const id_t expected_v_ids[][7] = {
    {1, 4, 5, 6, 7, 9, NULL_ID},
    {1, 2, 4, 5, 6, 8, 9},
    {1, 2, 5, 6, 7, 9, NULL_ID},
  };

  const strand_e expected_strands[][7] = {
      {FORWARD, FORWARD, FORWARD, REVERSE, FORWARD, FORWARD, FORWARD},
      {FORWARD, FORWARD, FORWARD, FORWARD, FORWARD, FORWARD, FORWARD},
      {FORWARD, FORWARD, REVERSE, FORWARD, FORWARD, REVERSE, FORWARD},
  };

  const char *expected_contig_name = "chr";

  const idx_t N = 3;
  for (idx_t i = 0; i < N; i++) {
    char *w_line = strdup(w_lines[i]);

    if (w_line == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(EXIT_FAILURE);
    }

    ref *r = parse_ref_line(W_LINE, w_line);
    ASSERT_NE(r, nullptr);

    ASSERT_EQ(r->name.line_type, W_LINE);
    ASSERT_EQ(r->name.type, PANSN);
    ASSERT_STREQ(r->name.pansn_value.contig_name, expected_contig_name);
    ASSERT_EQ(r->name.pansn_value.hap_id, hap_ids[i]);

    ASSERT_STREQ(get_tag(r), expected_tags[i]);
    ASSERT_STREQ(get_ref_name_tag(&r->name), expected_tags[i]);
    ASSERT_STREQ(r->name.pansn_value.sample_name, sample_names[i]);
    ASSERT_STREQ(get_sample_name(r), sample_names[i]);

    for (idx_t j = 0; j < get_step_count(r); j++) {
      ASSERT_EQ(r->data->v_id[j], expected_v_ids[i][j]);
      ASSERT_EQ(r->data->s[j], expected_strands[i][j]);
    }
    destroy_ref(&r);
    ASSERT_EQ(r, nullptr);
  }
}


TEST(ParsePLines, Valid) {

  const char *p_lines[] = {
    "P\tchm13__LPA__tig00000001\t3+,4+,6-,8+,9+",
    "P\tchm13#0#tig00000001\t3+,4+,6-,8+,9+",
  };

  const char *expected_tags[] = {
    "chm13__LPA__tig00000001",
    "chm13#0#tig00000001" };

  const id_t expected_v_ids[][7] = {
    {3, 4, 6, 8, 9, 9},
    {3, 4, 6, 8, 9, 9},
  };

  const ref_name_type_e expected_ref_name_types[] = {COARSE, PANSN};

  const char *expected_sample_names[] = {"chm13__LPA__tig00000001", "chm13"};

  const strand_e expected_strands[][7] = {
      {FORWARD, FORWARD, REVERSE, FORWARD, FORWARD},
      {FORWARD, FORWARD, REVERSE, FORWARD, FORWARD},
  };

  const idx_t N = 2;
  for (idx_t i = 0; i < N; i++) {
    char *p_line = strdup(p_lines[i]);
    if (p_line == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(EXIT_FAILURE);
    }

    ref *r = parse_ref_line(P_LINE, p_line);
    ASSERT_NE(r, nullptr);

    ASSERT_EQ(r->name.line_type, P_LINE);
    ASSERT_EQ(r->name.type, expected_ref_name_types[i]);
    ASSERT_STREQ(get_sample_name(r), expected_sample_names[i]);
    if (r->name.type == COARSE) {
      ASSERT_STREQ(r->name.coarse_name, expected_tags[i]);
    }
    else if (r->name.type == PANSN) {
      ASSERT_STREQ(r->name.pansn_value.sample_name, expected_sample_names[i]);
      // In this case we cannot parse hap_id and contig_name from the name
      ASSERT_EQ(r->name.pansn_value.hap_id, 0);
      ASSERT_STREQ(r->name.pansn_value.contig_name, "tig00000001");
    }
    ASSERT_STREQ(get_tag(r), expected_tags[i]);
    ASSERT_STREQ(get_ref_name_tag(&r->name), expected_tags[i]);

    for (idx_t j = 0; j < get_step_count(r); j++) {
      ASSERT_EQ(r->data->v_id[j], expected_v_ids[i][j]);
      ASSERT_EQ(r->data->s[j], expected_strands[i][j]);
    }
    destroy_ref(&r);
    ASSERT_EQ(r, nullptr);
  }
}
