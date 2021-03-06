/* ANSI-C code produced by gperf version 3.0.3 */
/* Command-line: gperf -LANSI-C -t -c -C -l -E -o -k'1,$' -N spki_gperf  */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif


/* Automatically generated by process-types,
 *  lör 9 mar 2013 09:21:07 CET
 * Do not edit! */

struct spki_assoc { const char *name; enum spki_type id; };
/* maximum key range = 43, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 15,
      45, 45, 45,  0, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 10, 45, 15,
       0,  0, 45,  0, 15,  0, 45, 45,  5,  0,
       0,  0,  5, 45, 15,  0, 10, 45, 15, 45,
      45,  0, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#ifdef __GNUC_STDC_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct spki_assoc *
spki_gperf (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 27,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 14,
      MIN_HASH_VALUE = 2,
      MAX_HASH_VALUE = 44
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  2,  3,  4,  5,  0,  7,  8,  9, 10, 11, 12,  3,
       9, 10, 11,  7,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,
      13,  4,  0,  0,  7,  0,  4,  0,  0,  0,  0,  0,  0,  0,
       0,  0, 14
    };
  static const struct spki_assoc wordlist[] =
    {
      {"", 0}, {"", 0},
      {"do", SPKI_TYPE_DO},
      {"md5", SPKI_TYPE_MD5},
      {"name", SPKI_TYPE_NAME},
      {"entry", SPKI_TYPE_ENTRY},
      {"", 0},
      {"display", SPKI_TYPE_DISPLAY},
      {"sequence", SPKI_TYPE_SEQUENCE},
      {"signature", SPKI_TYPE_SIGNATURE},
      {"not-before", SPKI_TYPE_NOT_BEFORE},
      {"issuer-info", SPKI_TYPE_ISSUER_INFO},
      {"subject-info", SPKI_TYPE_SUBJECT_INFO},
      {"tag", SPKI_TYPE_TAG},
      {"propagate", SPKI_TYPE_PROPAGATE},
      {"public-key", SPKI_TYPE_PUBLIC_KEY},
      {"private-key", SPKI_TYPE_PRIVATE_KEY},
      {"subject", SPKI_TYPE_SUBJECT},
      {"acl", SPKI_TYPE_ACL},
      {"sha1", SPKI_TYPE_SHA1},
      {"valid", SPKI_TYPE_VALID},
      {"issuer", SPKI_TYPE_ISSUER},
      {"version", SPKI_TYPE_VERSION},
      {"dsa-sha1", SPKI_TYPE_DSA_SHA1},
      {"not-after", SPKI_TYPE_NOT_AFTER},
      {"", 0}, {"", 0}, {"", 0},
      {"rsa-pkcs1-md5", SPKI_TYPE_RSA_PKCS1_MD5},
      {"cert", SPKI_TYPE_CERT},
      {"", 0}, {"", 0},
      {"comment", SPKI_TYPE_COMMENT},
      {"", 0},
      {"hash", SPKI_TYPE_HASH},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"rsa-pkcs1-sha1", SPKI_TYPE_RSA_PKCS1_SHA1}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key].name;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &wordlist[key];
          }
    }
  return 0;
}
