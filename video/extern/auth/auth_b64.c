/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: $ $Date: $
 *
 * Name:        Plain Text base 64 encoder/decoder 
 *
 * File:        auth_b64.h
 *
 * Description: Encoder and decoder functions for converting strings to base64
 *
 * Author:      
 */

 
/*---------------------------------------------------------------------------*/
/* base64                                                                    */
/* ======                                                                    */
/*                                                                           */
/* Base64 is a stand-alone C program to encode 7-Bit ASCII strings into      */
/* base64 encoded strings and decode base64 encoded strings back into 7 bit  */
/* ASCII strings.                                                            */
/*                                                                    PROTO_LIST       */
/* Base64 encoding is sometimes used for simple HTTP authentication. That is */
/* when strong encryption isn't necessary, Base64 encryption is used to      */
/* authenticate User-ID's and Passwords.                                     */
/*                                                                           */
/* Base64 processes a string by octets (3 Byte blocks).  For every octet in  */
/* the decoded string, four byte blocks are generated in the encoded string. */
/* If the decoded string length is not a multiple of 3, the Base64 algorithm */
/* pads the end of the encoded string with equal signs '='.                  */
/*                                                                           */
/* An example taken from RFC 2617 (HTTP Authentication):                     */
/*                                                                           */
/* Resource (URL) requires basic authentication (Authorization: Basic) for   */
/* access, otherwise a HTTP 401 Unauthorized response is returned.           */
/*                                                                           */
/* User-ID:Password string  = "Aladdin:open sesame"                          */
/* Base64 encoded   string  = "QWxhZGRpbjpvcGVuIHNlc2FtZQ=="                 */
/*                                                                           */
#include "auth_b64.h"
#include "auth_port.h"

static  
const                                       /* Base64 Index into encoding */
uint8  pIndex[]     =   {                      /* and decoding table. */
                        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
                        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                        0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
                        0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
                        0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
                        0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
                        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f
                        };

static
const                                       /* Base64 encoding and decoding */
uint8   pBase64[]   =   {                      /* table. */
                        0x3e, 0x7f, 0x7f, 0x7f, 0x3f, 0x34, 0x35, 0x36,
                        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x7f,
                        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x01,
                        0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                        0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
                        0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x1a, 0x1b,
                        0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
                        0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
                        0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
                        };

/*---------------------------------------------------------------------------*/
/* b64encode - Encode a 7-Bit ASCII string to a Base64 string.               */
/* ===========================================================               */
/*                                                                           */
/* Call with:   char *  - The 7-Bit ASCII string to encode.                  */
/*                                                                           */
/* Returns:     bool    - True (!0) if the operation was successful.         */
/*                        False (0) if the operation was unsuccessful.       */
/*---------------------------------------------------------------------------*/
int b64encode(char *s, char *t, int size)
{
    int     l   = size;        /* Get the length of the string. */
    int     x   = 0;                     /* General purpose integers. */
    //size = 0;
    while   (x < l)                         /* Validate each byte of string */
    {                                       /* to ensure it's 7-Bit ASCII */
        if (!b64is7bit((uint8) *(s + x)))
        {
            /* is not a 7-Bit ASCII string */
            return 0;                   /* Return false if it's not */
        }
        x++;                                /* Next byte. */
    }

    /* memset(t, 0x3d, b64blocks(l) - 1); */     /* Initialize it to "=". */
    memset(t, 0x3d, b64blocks(l) - 1);      /* Initialize it to "=". */

    x = 0;                                  /* Initialize string index. */

    while   (x < (l - (l % 3)))             /* encode each 3 byte octet. */
    {
        *t++   = pIndex[  s[x]             >> 2];
        *t++   = pIndex[((s[x]     & 0x03) << 4) + (s[x + 1] >> 4)];
        *t++   = pIndex[((s[x + 1] & 0x0f) << 2) + (s[x + 2] >> 6)];
        *t++   = pIndex[  s[x + 2] & 0x3f];
         x    += 3;                         /* Next octet. */
    }

    if (l - x)                              /* Partial octet remaining? */
    {
        *t++        = pIndex[s[x] >> 2];    /* Yes, encode it. */

        if  (l - x == 1)                    /* End of octet? */
            *t      = pIndex[ (s[x] & 0x03) << 4];
        else
        {                                   /* No, one more part. */
            *t++    = pIndex[((s[x]     & 0x03) << 4) + (s[x + 1] >> 4)];
            *t      = pIndex[ (s[x + 1] & 0x0f) << 2];
        }
    }

    return 1;                            /* Return to caller with success. */
}

typedef enum
{
    step_a, step_b, step_c, step_d
} base64_decodestep;

typedef struct
{
    base64_decodestep step;
    char plainchar;
} base64_decodestate;

int base64_decode_value(char value_in)
{
    static const char decoding[] =
{62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51};
    static const char decoding_size = sizeof(decoding);
    value_in -= 43;
    if (value_in < 0 || value_in > decoding_size) return -1;
    return decoding[(int)value_in];
}

void base64_init_decodestate(base64_decodestate* state_in)
{
    state_in->step = step_a;
    state_in->plainchar = 0;
}

int base64_decode_block(const char* code_in, const int length_in, char* plaintext_out, base64_decodestate* state_in)
{
    const char* codechar = code_in;
    char* plainchar = plaintext_out;
    char fragment;

    *plainchar = state_in->plainchar;

    switch (state_in->step)
    {
        while (1)
        {
    case step_a:
            do {
                if (codechar == code_in+length_in)
                {
                    state_in->step = step_a;
                    state_in->plainchar = *plainchar;
                    return plainchar - plaintext_out;
                }
                fragment = (char)base64_decode_value(*codechar++);
            } while (fragment < 0);
            *plainchar    = (fragment & 0x03f) << 2;
    case step_b:
            do {
                if (codechar == code_in+length_in)
                {
                    state_in->step = step_b;
                    state_in->plainchar = *plainchar;
                    return plainchar - plaintext_out;
                }
                fragment = (char)base64_decode_value(*codechar++);
            } while (fragment < 0);
            *plainchar++ |= (fragment & 0x030) >> 4;
            *plainchar    = (fragment & 0x00f) << 4;
    case step_c:
            do {
                if (codechar == code_in+length_in)
                {
                    state_in->step = step_c;
                    state_in->plainchar = *plainchar;
                    return plainchar - plaintext_out;
                }
                fragment = (char)base64_decode_value(*codechar++);
            } while (fragment < 0);
            *plainchar++ |= (fragment & 0x03c) >> 2;
            *plainchar    = (fragment & 0x003) << 6;
    case step_d:
            do {
                if (codechar == code_in+length_in)
                {
                    state_in->step = step_d;
                    state_in->plainchar = *plainchar;
                    return plainchar - plaintext_out;
                }
                fragment = (char)base64_decode_value(*codechar++);
            } while (fragment < 0);
            *plainchar++   |= (fragment & 0x03f);
        }
    }
    /* control should not reach here */
    return plainchar - plaintext_out;
}

/*---------------------------------------------------------------------------*/
/* b64decode - Decode a Base64 string to a 7-Bit ASCII string.               */
/* ===========================================================               */
/*                                                                           */
/* Call with:   char *  - The Base64 string to decode.                       */
/*                                                                           */
/* Returns:     int     - decoded size.                                      */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int b64decode(char *s, int size, char *t)
{
    base64_decodestate decState;

    base64_init_decodestate(&decState);
    return (base64_decode_block(s, size, t, &decState));
}

