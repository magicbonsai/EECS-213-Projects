#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define TMin INT_MIN
#define TMax INT_MAX

#include "btest.h"
#include "bits.h"

test_rec test_set[] = {





 {"bitAnd", (funct_t) bitAnd, (funct_t) test_bitAnd, 2, "| ~", 8, 1,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},
 {"getByte", (funct_t) getByte, (funct_t) test_getByte, 2,
    "! ~ & ^ | + << >>", 6, 2,
  {{TMin, TMax},{0,3},{TMin,TMax}}},
 {"conditional", (funct_t) conditional, (funct_t) test_conditional, 3, "! ~ & ^ | << >>", 16, 3,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},
 {"bitCount", (funct_t) bitCount, (funct_t) test_bitCount, 1, "! ~ & ^ | + << >>", 40, 4,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},
 {"bang", (funct_t) bang, (funct_t) test_bang, 1,
    "~ & ^ | + << >>", 12, 4,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},
 {"tmin", (funct_t) tmin, (funct_t) test_tmin, 0, "! ~ & ^ | + << >>", 4, 1,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},
 {"absVal", (funct_t) absVal, (funct_t) test_absVal, 1, "! ~ & ^ | + << >>", 10, 4,
  {{-TMax, TMax},{TMin,TMax},{TMin,TMax}}},
 {"fitsShort", (funct_t) fitsShort, (funct_t) test_fitsShort, 1,
    "! ~ & ^ | + << >>", 8, 1,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},
 {"divpwr2", (funct_t) divpwr2, (funct_t) test_divpwr2, 2,
    "! ~ & ^ | + << >>", 15, 2,
  {{TMin, TMax},{0,30},{TMin,TMax}}},
 {"negate", (funct_t) negate, (funct_t) test_negate, 1,
    "! ~ & ^ | + << >>", 5, 2,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},
 {"isPositive", (funct_t) isPositive, (funct_t) test_isPositive, 1,
    "! ~ & ^ | + << >>", 8, 3,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},
 {"ilog2", (funct_t) ilog2, (funct_t) test_ilog2, 1, "! ~ & ^ | + << >>", 90, 4,
  {{1, TMax},{TMin,TMax},{TMin,TMax}}},
 {"float_neg", (funct_t) float_neg, (funct_t) test_float_neg, 1,
    "$", 10, 2,
     {{1, 1},{1,1},{1,1}}},
 {"float_abs", (funct_t) float_abs, (funct_t) test_float_abs, 1,
    "$", 10, 2,
     {{1, 1},{1,1},{1,1}}},
 {"float_twice", (funct_t) float_twice, (funct_t) test_float_twice, 1,
    "$", 30, 4,
     {{1, 1},{1,1},{1,1}}},
  {"", NULL, NULL, 0, "", 0, 0,
   {{0, 0},{0,0},{0,0}}}
};
