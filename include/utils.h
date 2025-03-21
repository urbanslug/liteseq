#ifndef LQ_UTILS_H
#define LQ_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * A 32 bit bitstring where each bit corresponds to a character in the alphabet
 * Bitstring for 'A' to 'Z' (32 bits) which are 26 but padded to 32
 * Map characters relative to 'A', so bit_position = char - 'A'.
 * 'A' → Bit 0, 'C' → Bit 2, 'G' → Bit 6, 'T' → Bit 19, 'N' → Bit 13.
 */
#define ALPHABET_MASK                                                          \
  ((uint32_t)((1 << ('A' - 'A')) | /* 'A' */                                   \
              (1 << ('C' - 'A')) | /* 'C' */                                   \
              (1 << ('G' - 'A')) | /* 'G' */                                   \
              (1 << ('T' - 'A')) | /* 'T' */                                   \
              (1 << ('N' - 'A'))   /* 'N' */                                   \
              ))

/**
 * mark a variable as unused
 */
#define UNUSED(x) (void)(x)

/**
 * Likely and unlikely macros to wrap around __builtin_expect
 */
#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

/**
 * Validates that the character is in the alphabet
 * if not it will print an error message and exit the program
 */
void validate_character(char c);

/**
   Function to check if a character is in the alphabet
*/
bool in_alphabet(char c);

/**
 * Encodes a base character to a 3-bit representation.
 * Map the character to a 3-bit encoding
 */
uint8_t encodeBase(char base);

#endif /* LQ_UTILS_H */
