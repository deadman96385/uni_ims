/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2003-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 2115  Date: 2007-08-01 15:15:20 -0700 (Wed, 01 Aug 2007) 
 * +D2Tech+ Release Version: PENT0_A_1_1-comm_1_6
 */

#ifndef _COMM_H_
#define _COMM_H_

#include <d2types.h>

/* 
 * Macros
 */

#define COMM_5MS_LEN   (40) /* Block length for 5 ms processing */
#define COMM_10MS_LEN  (80) /* Block length for 10 ms processing */

/* 
 * Macros for min, max and abs inline functions
 */
#define COMM_ABS(a)    ((a) < 0 ? -(a) : (a))
#define COMM_MAX(a, b) ((a) > (b) ? (a) : (b))
#define COMM_MIN(a, b) ((a) < (b) ? (a) : (b))

/*
 * Define global parameters
 */
typedef struct {
    vint p0DBIN;  /* Reference input level for 0 dBm */
    vint p0DBOUT; /* Reference output level for 0 dBm */
} GLOBAL_Params;

/*
 * Fills the buffer pointed by 'dst_ptr' of length 'length' with pattern 
 * 'fill'.
 * Returns:
 *  None
 */
void COMM_fill(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint   fill,     /* Fill pattern */
    uvint  length);  /* length */

/*
 * Copies the buffer pointed by 'src_ptr' of length 'length' to buffer
 * pointed by 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_copy(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length);  /* length */

/*
 * Right shifts each element of the buffer pointed by 'src_ptr' of length 
 * 'length' and places the result in 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_shiftRight(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    uvint  value,    /* number of bits to shift */
    uvint  length);  /* length */

/*
 * Left shifts each element of the buffer pointed by 'src_ptr' of length 
 * 'length' and places the result in 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_shiftLeft(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    uvint  value,    /* number of bits to shift */
    uvint  length);  /* length */

/*
 * Adds each element of the buffer pointed by 'src_ptr' of 'length' to dst_ptr
 * of 'length' and places the result in 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_sum(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    uvint  length);  /* length */

/*
 * Attenuates each element of the buffer pointed by 'src_ptr' of length 
 * 'length' and places the result in 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_attn(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    vint   attn,     /* Attn value in dBm, 1/2 dB steps (0 to -159) */
    uvint  length);  /* length */

/*
 * Scales each element of the buffer pointed by 'src_ptr' of length 'length'
 * and places the result in the buffer pointed by 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_scale(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    vint   value,    /* linear scale value */
    uvint  length);  /* length */

/*
 * Finds the square root of the number 'num'.
 * Returns:
 *  Square root of 'num'
 */
uvint COMM_sqrt(
    uint32 num);      /* number */

/*
 * Calculates the RMS value of the data in buffer pointer by 'src_ptr' of
 * RMS length 'length'.
 * Returns:
 *  RMS of the data.
 */
uvint COMM_lrms(
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length);  /* length */

/*
 * Calculates the linear value of the dBm 'db'.
 * 'db' is (dBm value - 20) dBm in 0.5 dB units.
 * Minimum dBm value is -20 dBm (db=0) and maximum value is +20 dBm (db=80)
 * Returns:
 *  Linear value of 'db-40' shifted left by 11 bits.
 */
uvint COMM_db2lin(
    uvint db);       /* (dBm value - 20) dBm in 0.5 dB units */ 

/*
 * Calculates the dB value of the linear 'lin'.
 * 'lin' is the linear value in range 1 to 32767.
 * Performs bitlog function: B(x) = 256*(b-1)+n
 * b = most-significant non zero bit location.
 * n = 8 most significant bits after the most significant non zero bit.
 * 
 * Returns:
 *  200 * log (lin), lin >= 1
 *  Large negative number, lin < 1
 */
vint COMM_lin2db(
    uvint lin);      /* the linear value, in range 1 to 32767 */

/*
 * Converts each element of the buffer pointed by 'src_ptr' of length 
 * 'length' from muLaw to linear and places the result in the buffer 
 * pointed by 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_mu2lin(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length);  /* length */

/*
 * Converts each element of the buffer pointed by 'src_ptr' of length 
 * 'length' from aLaw to linear and places the result in the buffer 
 * pointed by 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_a2lin(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length);  /* length */

/*
 * Converts each element of the buffer pointed by 'src_ptr' of length 
 * 'length' from linear to mulaw and places the result in the buffer 
 * pointed by 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_lin2mu(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length);  /* length */

/*
 * Converts each element of the buffer pointed by 'src_ptr' of length 
 * 'length' from linear to alaw and places the result in the buffer 
 * pointed by 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_lin2a(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length);  /* length */

/*
 * Packs data pointed by src_ptr in 8-bits per value to 32-bit packed data,
 * pointed by dst_ptr.
 * 'length' is the number of data elements to pack, minimum length=4. 
 * Returns:
 *  None
 */
void COMM_pack32(
    uint32 *dst_ptr,  /* destination buffer pointer */
    vint   *src_ptr,  /* source buffer pointer */
    uvint   length);  /* length */

/*
 * Unpacks data pointed by src_ptr in 32-bits per value to 8-bit unpacked data,
 * pointed by dst_ptr.
 * 'length' is the number of data elements to unpack, minimum length=4.
 * Returns:
 *  None
 */
void COMM_unpack32(
    vint   *dst_ptr,  /* destination buffer pointer */
    uint32 *src_ptr,  /* source buffer pointer */
    uvint   length);  /* length */

/*
 * Copies the buffer pointed by 'src_ptr' of length 'length' octets to buffer
 * pointed by 'dst_ptr' of length 'length'.
 * Returns:
 *  None
 */
void COMM_octetCopy(
    int8  *dst_ptr,  /* destination buffer pointer */
    int8  *src_ptr,  /* source buffer pointer */
    uvint  length);  /* length in octets */

/*
 * Fills the buffer pointed by 'dst_ptr' of length 'length' octets
 * with pattern 'fill'.
 * Returns:
 *  None
 */
void COMM_octetFill(
    int8  *dst_ptr,  /* destination buffer pointer */
    int8   fill,     /* fill pattern */
    uvint  length);  /* length in octets */

#endif
