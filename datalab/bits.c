/* 
 * CS:APP Data Lab 
 * 
 * Student Name: Cheng Zhang
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* 
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int evenBits(void) {
    /*If it is a 4 bit number x, eve-numbered bits set to 1 means x = 0x55
     *it's bit presentation is 0101
     * Now for 32 bit number, it should be 0x55555555
     * Therefore, we can just use << and | to return this value.
     */
    int result = (0x55 << 8) | 0x55;
    result = (result << 16) | result;
    return result;
}

/* 
 * isEqual - return 1 if x == y, and 0 otherwise 
 *   Examples: isEqual(5,5) = 1, isEqual(4,5) = 0
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int isEqual(int x, int y) {
    /*
     *if x equals y, x^y should be 1, otherwise, x^y should be 0.
     *Therefore !(x^y) can return the right answer
     */
    return !(x^y);
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
    /*
     *  Firstly, get the nth byte: byte_n and the mth byte_m
     *It could be done by right shifting x of n*8 and m*8 bits respectively,
     *Then use x & 0xFF to get the 0th byte
     *  Secondly, make a mask number to set nth and mth byte to 0
     *  Thirdly, shift nth byte to mth byte and mth byte to nth byte
     */
    int shift_n = n << 3;
    int shift_m = m << 3;
    int byte_n = (x >> shift_n) & 0xFF;
    int byte_m = (x >> shift_m) & 0xFF;
    int mask = (0xFF << shift_m) | (0xFF << shift_n);
    return (x & (~mask)) | (byte_m << shift_n) | (byte_n << shift_m);
}
/* 
 * rotateRight - Rotate x to the right by n
 *   Can assume that 0 <= n <= 31
 *   Examples: rotateRight(0x87654321,4) = 0x18765432
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 3 
 */
int rotateRight(int x, int n) {
    /* Firstly, calculate shift_left, which is the times shift_number should move from right to left
     * Secondly, use mask to pick shift_number out
     * Thirdly, move other bits from left to right
     * Last, add shift_number to the right position
     */
    int shift_left = ((~n) & 0x1F) + 1;
    int mask = (1 << n) + ~1 + 1;   //(~1 + 1 = -1)
    int shift_number = x & mask;
    x = (x >> n) & (~(mask << shift_left)); //artithematic right shift may result 1
    return x | (shift_number << shift_left);

}
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
    /*
     *Do or opeartion x and -x, only when x = 0 will (x | -x)'s MSB be 0
     */
    int minus_x = ~x + 1;
    int result = ((x | minus_x) >> 31);
    return  (result & 1) ^ 1;

}
/* 
 * TMax - return maximum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmax(void) {
    /* It shold be 0x7FFFFFFF
     */
    int x = ~(1 << 31);
    return x;
}
/* 
 * sign - return 1 if positive, 0 if zero, and -1 if negative
 *  Examples: sign(130) = 1
 *            sign(-23) = -1
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 10
 *  Rating: 2
 */
int sign(int x) {
    /* Firstly, distingusth with 0 and others use !(!x), only x=0 result in 0, others is 1
     * Secondly, right shift x, if x < 0, it will become 0xFFFFFFFF, otherwise it will become 0
     * Last, use | to make right answer
     */
    int mask = x >> 31;
    return (!(!x)) | mask;
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
    /* x will be greater than y when:
     * 1. x > 0 and y < 0 (use x_MSB an y_MSB to determine)
     * 2. x and y are both positive or negitive and x != y and x - y > 0
     * ,which means that x - y - 1 >= 0(that is x + ~y + 1 - 1)
     */
    int x_MSB = (x >> 31) & 1;
    int y_MSB = (y >> 31) & 1;
    int posX_negY = (!x_MSB) & y_MSB;   //x_MSB = 0, y_MSB = 1
    int same_sign = !(x_MSB ^ y_MSB);
    int result = x + ~y;
    
    return posX_negY | (same_sign & !(result >> 31));

}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
    /* Only when x be different sign with y and
     * x - y be different sign with x will overflow occur
     */
    int x_MSB = x >> 31;
    int y_MSB = y >> 31;
    
    int diff_sign = x_MSB ^ y_MSB;
    
    int result = x + ~y + 1;
    
    int result_overflow = (x_MSB ^ (result >> 31)) & 1; // 1 be overflow
    
    return !(diff_sign & result_overflow);
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          maximum possible value, and when negative overflow occurs,
 *          it returns minimum possible value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y) {
    /* If x and y are of different sign, return x + y
     * If x and y are of same sign, define if it exits overflow.
     * if yes, return T_min or T_max, if no, return x + y
     */
    
    int result = x + y;
    int result_MSB = result >> 31;
    
    int diff_sign = (x ^ y) >> 31;                  //different sign will be 0xfffffff,same be 0
    int if_overflow = (~diff_sign) & ((x ^ result) >> 31);           //0xFFFFFF be overflow, 0 be not overflow
    
    int overflow_result = result_MSB ^ (1 << 31);  //if result_MSB be 0xFFFFFFFF, it will be 0x7FFFFFFF, if result_MSB is 0x0000000, it will be 0x10000000
    
    
    return ((~if_overflow) & result) | (if_overflow & overflow_result);
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
    /* First, turn x to ~x if x < 0
     * THen, Check the first 16bits, then first 8bits of the rest 16bits if the first 16 bits are all 0,
     * or the first 8bits in the 16bits if it != 0. Do this iteration and find how many bits
     */
    int x_MSB = x >> 31;
    int unsigned_x = (x & (~x_MSB)) | ((~x) & x_MSB);
    int count = 0;
    
    int not_move1 = 0;
    int not_move2 = 0;
    int not_move4 = 0;
    int not_move8 = 0;
    int not_move16 = 0;
    
    int move1 = 0;
    int move2 = 0;
    int move4 = 0;
    int move8 = 0;
    int move16 = 0;
    
    not_move16 = !(unsigned_x & ((0xFF << 24) | (0xFF << 16)));//1 means all 0 in first 16th bits
    move16 = !not_move16;
    move16 <<= 4;                                 //0, shift right 0 0;1, shft right 16
    count |= move16;
    unsigned_x = unsigned_x >> move16;
    
    not_move8 =! (unsigned_x & (0xFF << 8) );//1 means all 0 in first 8th bits
    move8 = !not_move8;
    move8 <<= 3;
    count |= move8;
    unsigned_x = unsigned_x >> move8;
    
    not_move4 = !(unsigned_x & 0xF0);
    move4 = !not_move4;
    move4 <<= 2;
    count |= move4;
    unsigned_x = unsigned_x >> move4;
    
    not_move2 = !(unsigned_x & 0xC);
    move2 = !not_move2;
    move2 <<= 1;
    count |= move2;
    unsigned_x = unsigned_x >> move2;
    
    not_move1 = !(unsigned_x & 0x2);
    move1 = !not_move1;
    count |= move1;
    unsigned_x = unsigned_x >> move1;
    
    count += 1 + unsigned_x;  //if x = 0, unsigned_x be 0
    
    return count;
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
    /* First, split uf into s, e and f
     * Second, calculate result upon the following codition:
     *  1. e is all 1s(use !(e^0xFF)), it is infinity or NaN, return uf
     *  2. e is 1 or 0(use !(e >> 1)), in this case, e should be 0 and f should 
     * left shift 1 bit (and carry should be considered). I find that in this
     * case you can left shift e and f together no matter e is 0 or 1
     *  3. e > 1 (use e >> 1), so e should minus 1
     */
    unsigned result = 0;
    unsigned rest = 0;
    
    unsigned s = uf >> 31;
    unsigned e = uf >> 23 & 0xFF;
    
    unsigned minus_1 = ~1 + 1;
    unsigned low23 = (1 << 23) + minus_1;
    
    unsigned f = uf & low23;
    unsigned carry = (f >> 1) & f & 1;
    
    
    if (!(e ^ 0xFF)){
        return uf;
    }
    if (!(e >> 1)){
        rest = (((e << 23) | f) >> 1) + carry;
    }
    if (e >> 1){
        rest = (((e + minus_1) & 0xFF) << 23) | f;
    }
    result = (s << 31) | rest;
    return result;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
    unsigned s = uf >> 31;
    int e = uf >> 23 & 0xFF;
    int E = 0;
    int E_vs_23 = 0;
    int tmp;
    
    int minus_1 = ~1 + 1;
    int bit24 = 1 << 23;
    int low23 = bit24 + minus_1;
    int M = (uf & low23) | bit24; //M = 1 + f
    
    E = e + (~0x9E) + 1; // e - 127 - 31
    if (E >> 31){ // E < 31
        E = E + 8;  //e - 127  - 23
        E_vs_23 = E >> 31;
        if (E_vs_23){ // e - 127 < 23
            tmp = E + 23; // e - 127
            if (tmp >> 31) // e < 127
                return 0;
            tmp = ~E + 1;
            M = M >> tmp;
        }
        if (!E_vs_23){
            M = M << E;
        }
        if (s){
            return ~M + 1;
        }
        return M;
    }
    return 1 << 31;

}
