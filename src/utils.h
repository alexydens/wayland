/* Include guard */
#ifndef UTILS_H
#define UTILS_H

/* Types */
/* Unsigned Integers */
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
/* Signed Integers */
typedef signed char         i8;
typedef signed short        i16;
typedef signed int          i32;
typedef signed long long    i64;
/* Floating-Point Numbers */
typedef float               f32;
typedef double              f64;

/* Consts */
#define C_PI                3.141592653       /* Mathematical constant PI */
#define C_E                 2.718281828       /* Euler's number */
#define C_KiB               1024              /* One Kibibyte in Bytes */
#define C_MiB               1024*1024         /* One Mebibyte in Bytes */
#define C_GiB               1024*1024*1024    /* One Gibibyte in Bytes */

/* Utility macros */
/* Make into string */
#define STRING(A)     #A
/* Concatenate */
#define CONCAT(A,B)   A##B
/* The smallest value out of A or B */
#define MIN(A,B)      ((A) < (B) ? (A) : (B))
/* The largest value out of A or B */
#define MAX(A,B)      ((A) > (B) ? (A) : (B))
/* A value between A and B, where V is how much inbetween (e.g. 0.5 would be
 * halfway between A and B */
#define LERP(A,B,V)   (MIN(A,B) + V * (MAX(A,B)-MIN(A,B)))
/* Swap the values of A and B (A and B must be variables) */
#define SWAP(A,B)     do{__typeof__(A) TMP = A; A = B; B = TMP;}while(0)

#endif /* end of include guard: UTILS_H */
