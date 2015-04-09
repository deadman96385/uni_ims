/*
 * This file was sourced from "http://www.efgh.com/software/rijndael.htm".  
 * There is no license associated with this file and it is considered to have 
 * no restrictions and is part of the "Public Domain". Below is the original 
 * header of the file. 
 *
 * $D2Tech$ $Rev: 7735 $ $Date: 2008-09-30 10:59:43 -0500 (Tue, 30 Sep 2008) $
 */

/*-------------------------------------------------------------------
 *          Example algorithms f1, f1*, f2, f3, f4, f5, f5*
 *-------------------------------------------------------------------
 *
 *  A sample implementation of the example 3GPP authentication and
 *  key agreement functions f1, f1*, f2, f3, f4, f5 and f5*.  This is
 *  a byte-oriented implementation of the functions, and of the block
 *  cipher kernel function Rijndael.
 *
 *  This has been coded for clarity, not necessarily for efficiency.
 *
 *  The functions f2, f3, f4 and f5 share the same inputs and have
 *  been coded together as a single function.  f1, f1* and f5* are
 *  all coded separately.
 *
 *-----------------------------------------------------------------*/

#ifndef RIJNDAEL_H
#define RIJNDAEL_H

#ifdef __cplusplus
extern "C" {
#endif

void RijndaelKeySchedule( u8 key[16] );
void RijndaelEncrypt( u8 input[16], u8 output[16] );

#ifdef __cplusplus
}
#endif

#endif
