#include "./utils.h"

uint8_t encodeBase(char base) {
  switch (base) {
  case 'A': return 0b000;
  case 'T': return 0b001;
  case 'C': return 0b010;
  case 'G': return 0b011;
  case 'N': return 0b100;
  case 'I': return 0b101; // Invalid base. Internal to far. May indicate end of sequence
  default:
    fprintf(stderr, "Invalid base character: %c\n", base);
    exit(EXIT_FAILURE); // Exit the program on invalid input
  }
}

#ifdef NDEBUG
inline bool in_alphabet(char c) {
    if (c < 'A' || c > 'Z') return false; // Out of range for uppercase letters
    return (ALPHABET_MASK & (1 << (c - 'A'))) != 0;
}
#else
bool in_alphabet(char c) {
    if (c < 'A' || c > 'Z') return false; // Out of range for lowercase letters
    return (ALPHABET_MASK & (1 << (c - 'A'))) != 0;
}
# endif

void validate_character(char c) {
  if (!in_alphabet(c)) {
    fprintf(stderr, "character (%c), ASCII (%d)", c, c);
    perror("Invalid character in sequence\n");
    exit(EXIT_FAILURE); // Exit on invalid input
  }
}
