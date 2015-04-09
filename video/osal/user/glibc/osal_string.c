/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <osal_string.h>
#include <ctype.h>

/* 
 * ======== OSAL_strcpy() ========
 * This function is a C library strcpy implementation.
 *
 * Note: This routine is under investigation for deprecation.
 *       It may be replaced by strncpy().  Use strncpy() 
 *       whenever possible.
 *
 * Return:
 *  See help for strcpy.
 */
char* OSAL_strcpy(
    char        *dst_ptr,
    const char  *src_ptr)
{
    return (strcpy(dst_ptr, src_ptr));
}

/*
 * ======== OSAL_strncpy() ========
 * This function is a !!!custom!!! implementation similiar to 
 * strncpy() but with differnt behavior. strncpy() from the C
 * library will NOT NULL terminate a string if the max length 
 * of the destination buffer has been reached.  Furthermore,
 * the C Library implementation will not return a value that can
 * indicate that this error has happened.  Therefore, strncpy()
 * is deemed to be unsafe. This custom implementation ensures
 * that the destination buffer will be NULL terminated by writting
 * a NULL character at the very end if the max number of bytes has
 * been written.
 *
 * It is realized that this behavior is dangerous.  However it will 
 * be replaced when a suitable solution has been found.
 *
 * Returns:
 *  A pointer to dst_ptr.
 */
char* OSAL_strncpy(
    char       *dst_ptr,
    const char *src_ptr,
    vint        size)
{
    char *dest_ptr = dst_ptr;
    if (0 >= size) {
        return (dest_ptr);
    }
    /*
     * Decrement size by one so we are guaranteed we can write
     * NULL termination.
     */
    size--;
    while (*src_ptr && size--) {
        *dst_ptr++ = *src_ptr++;
    }
    /* 
     * This function always NULL terminates.  This is different
     * then the behavior of clib's strncpy.
     */
    *dst_ptr = 0;
    /* 
     * Return the pointer to the destination as specified in
     * libc strncpy man page.
     */
    return (dest_ptr);
}

/* 
 * ======== OSAL_strcmp() ========
 * This function is a C library strcmp implementation.
 *
 * Note: This routine is under investigation for deprecation.
 *       It may be replaced by strncmp().  Use strncmp() 
 *       whenever possible.
 *
 * Returns:
 *  See help for strcmp.
 */
vint OSAL_strcmp(
    const char *str1_ptr,
    const char *str2_ptr)
{
    return (strcmp(str1_ptr, str2_ptr));
}

/* 
 * ======== OSAL_strcasecmp() ========
 * This function is similiar to a C library strcmp implementation with
 * the exception that the string comparison is case insensitive.
 *
 * Note: This routine is under investigation for deprecation.
 *       It may be replaced by strncasecmp().  Use strncasecmp() 
 *       whenever possible.
 *
 * Returns:
 *  See help for strcasecmp.
 */
vint OSAL_strcasecmp(
    const char *str1_ptr, 
    const char *str2_ptr)
{
    return (strcasecmp(str1_ptr, str2_ptr));
}


/* 
 * ======== OSAL_strncmp() ========
 * This function is a C library strncmp implementation.
 *
 * Returns:
 *  See help for strncmp.
 */   
vint OSAL_strncmp(
    const char *str1_ptr,
    const char *str2_ptr,
    vint        size)
{
    return (strncmp(str1_ptr, str2_ptr, size));
}

/* 
 * ======== OSAL_strncasecmp() ========
 * This function is similiar to a C library strcmp implementation with
 * the exception that the string comparison is case insensitive
 * and that the comparison will NOT exceed the number of bytes
 * specified in 'size'.
 * 
 * Note , size should never be zero ('0').
 *
 * Returns:
 *  See help for strncasecmp.
 */
vint OSAL_strncasecmp(
    const char *str1_ptr, 
    const char *str2_ptr,
    vint        size)
{
    return (strncasecmp(str1_ptr, str2_ptr, size));
}

/* 
 * ======== OSAL_strlen() ========
 * This function is a C library strlen implementation.
 *
 * Returns:
 *  The length in bytes of the string specified in str_ptr.
 */
vint OSAL_strlen(
    const char *str_ptr)
{
    return (strlen(str_ptr));
}

/* 
 * ======== OSAL_snprintf() ========
 * This function is a C library snprintf implementation.
 *
 * Returns:
 *  Number of chars printed.
 */   
vint OSAL_snprintf(
    char       *buf_ptr,
    vint        size,
    const char *format_ptr,
    ...)
{
    va_list ap;
    vint    n;
    
    va_start(ap, format_ptr);
    
    n = vsnprintf(buf_ptr, size, format_ptr, ap);
    
    va_end(ap);
    
    /* Return value of vsnprintf() would be size of (format_ptr),
     * however we need to return the real printed size.
     */
    if (n > size) {
        n = size;
    }
    return (n);
}

/* 
 * ======== OSAL_strchr() ========
 * This function is a C library strchr implementation.
 *
 * Returns:
 *  char* : A pointer to the location of the char.
 *  NULL : If 'c' was not found in str_ptr.
 */   
char* OSAL_strchr(
    const char *str_ptr,
    vint        c)
{
    return (strchr(str_ptr, c));
}

/* 
 * ======== OSAL_strtok() ========
 * Finds token in a string. See help of strtok.
 *
 * Returns:
 *  char* : The location of match. 
 *  NULL  : No match was found.
 */   
char* OSAL_strtok(
    char *str_ptr,
    const char *sub_ptr)
{
    return (strtok(str_ptr, sub_ptr));
}

/* 
 * ======== OSAL_strscan() ========
 * Finds location of a sub-string in a string.
 *
 * Note: This routine is under investigation for deprecation.
 *       It may be replaced by strnscan().  Use strnscan() 
 *       whenever possible.
 *
 * Returns:
 *  char* : The location of match. 
 *  NULL  : No match was found.
 */   
char* OSAL_strscan(
    const char *str_ptr,
    const char *sub_ptr)
{
    return (strstr(str_ptr, sub_ptr));
}

/* 
 * ======== OSAL_strnscan() ========
 * Finds location of a sub-string in a string.
 * The search will not exceed 'size' number of bytes.
 *
 * Returns:
 *  char* : The location of match. 
 *  NULL  : No match was found.
 */   
char* OSAL_strnscan(
    const char *str_ptr,
    vint        size,
    const char *sub_ptr)
{
    vint  i;
    vint  len = strlen(sub_ptr);
    
    while (*str_ptr && size--) {
        i = 0;
        if (str_ptr[i] == sub_ptr[i]) {
            i++;
            while (i < len) {
                if (!str_ptr[i]) {
                    return (NULL);
                }
                
                if (str_ptr[i] != sub_ptr[i]) {
                    break;
                }

                i++;
            }
            if (i == len) {
                return ((char*)str_ptr);
            }
        }
        str_ptr++;
    }
    return (NULL);
}

/* 
 * ======== OSAL_strncasescan() ========
 * Finds location of a sub-string in a string while ignoring case.
 * The search will not exceed 'size' number of bytes.
 *
 * Returns:
 *  char* : The location of match.
 *  NULL  : No match was found.
 */
char* OSAL_strncasescan(
    const char *str_ptr,
    vint        size,
    const char *sub_ptr)
{
    vint  i;
    vint  len = strlen(sub_ptr);

    while (*str_ptr && size--) {
        i = 0;
        if (tolower(str_ptr[i]) == tolower(sub_ptr[i])) {
            i++;
            while (i < len) {
                if (!str_ptr[i]) {
                    return (NULL);
                }

                if (tolower(str_ptr[i]) != tolower(sub_ptr[i])) {
                    break;
                }

                i++;
            }
            if (i == len) {
                return ((char*)str_ptr);
            }
        }
        str_ptr++;
    }
    return (NULL);
}

/* 
 * ======== OSAL_atoi() ========
 * This function is a C library atoi implementation.
 *
 * Returns:
 *  See help for atoi.
 */   
vint OSAL_atoi(
    const char *num_ptr)
{
    return (atoi(num_ptr));
}

/* 
 * ======== OSAL_itoa() ========
 * This function converts a integer to a string.
 * This function will not overwrite 'strLen' number of bytes.
 * If 'strLen' is not large enough then the string will be truncated
 * and NULL terminated.
 * 
 * Returns:
 *  -1 : There was an error.
 *   vint : The number of bytes writen.  If str_len and strLen were not 
 *          large enough to accomidate writing the whole value then 
 *          the string in str_ptr will be truncated but still terminated
 *          with NULL/zero and the number of bytes that would have been writen 
 *          is returned.  Therefore developers should compare the value 
 *          provided in 'strLen' with the return value to determine if the 
 *          string was trunicated
 *   
 */
vint OSAL_itoa(
    vint    value,
    char   *str_ptr,
    vint    strLen)
{
    return (snprintf(str_ptr, strLen, "%d", value));
}

/* 
 * ======== OSAL_htoi() ========
 * This function is similar to atoi but parse the hexidecimal
 * representation of the integer. This is reverse to the itoh function.
 *
 * Returns:
 *  On success, the function returns the converted int value.
 *  If the value would be out of the int range, it is an undefined value.
 */
vint OSAL_htoi(
    const char *num_ptr)
{
    vint value;
    char ch;

    while (0 != (*num_ptr)) {
        ch = *num_ptr++;
        if (ch >= '0' && ch <= '9') {
            value = (value << 4) + (ch - '0');
        }
        else if (ch >= 'A' && ch <= 'F') {
            value = (value << 4) + (ch - 'A' + 10);
        }
        else if (ch >= 'a' && ch <= 'f') {
            value = (value << 4) + (ch - 'a' + 10);
        }
        else {
            break;
        }
    }
    return value;
}

/*
 * ======== OSAL_itoh() ========
 * This function converts an integer to a NULL terminated 
 * string containing the hexidecimal representation of the integer.
 * This function will not overwrite 'strLen' number of bytes.
 * If 'strLen' is not large enough then the string will be truncated
 * and null terminated.
 * 
 * Returns:
 *  -1 : There was an error.
 *   vint : The number of bytes writen.  If str_len and strLen were not 
 *          large enough to accomidate writing the whole value then 
 *          the string in str_ptr will be truncated but still terminated
 *          with NULL/zero and the number of bytes that would have been writen 
 *          is returned.  Therefore developers should compare the value 
 *          provided in 'strLen' with the return value to determine if the 
 *          string was trunicated
 *   
 */
vint OSAL_itoh(
    vint    value,
    char   *str_ptr,
    vint    strLen)
{
    return (snprintf(str_ptr, strLen, "%x", value));
}

/* 
 * ======== OSAL_strtoul() ========
 * This function is a C library strtoul implementation.
 *
 * Returns:
 *  See help for strtoul.
 */
uint32 OSAL_strtoul(
    const char *str_ptr, 
    char      **end_ptr, 
    vint        base) 
{
    return (strtoul(str_ptr, end_ptr, base));
}
