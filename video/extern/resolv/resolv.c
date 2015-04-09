#include "resolv.h"
#include <osal.h>

static void    setsection(ns_msg *msg, ns_sect sect);

static const char    digits[] = "0123456789";

/* These need to be in the same order as the nres.h:ns_flag enum. */
struct _ns_flagdata _ns_flagdata[16] = {
    { 0x8000, 15 },       /* qr. */
    { 0x7800, 11 },       /* opcode. */
    { 0x0400, 10 },       /* aa. */
    { 0x0200, 9 },        /* tc. */
    { 0x0100, 8 },        /* rd. */
    { 0x0080, 7 },        /* ra. */
    { 0x0040, 6 },        /* z. */
    { 0x0020, 5 },        /* ad. */
    { 0x0010, 4 },        /* cd. */
    { 0x000f, 0 },        /* rcode. */
    { 0x0000, 0 },        /* expansion (1/6). */
    { 0x0000, 0 },        /* expansion (2/6). */
    { 0x0000, 0 },        /* expansion (3/6). */
    { 0x0000, 0 },        /* expansion (4/6). */
    { 0x0000, 0 },        /* expansion (5/6). */
    { 0x0000, 0 },        /* expansion (6/6). */
};
uint16
ns_get16(const uint8 *src) {
    int32 dst;

    NS_GET16(dst, src);
    return (dst);
}

uint32
ns_get32(const uint8 *src) {
    int32 dst;

    NS_GET32(dst, src);
    return (dst);
}
int
ns_initparse(const uint8 *msg, int msglen, ns_msg *handle) {
    const uint8 *eom = msg + msglen;
    int i;

    OSAL_memSet(handle, 0x5e, sizeof *handle);
    handle->_msg = msg;
    handle->_eom = eom;
    if (msg + sizeof(uint16) > eom)
        RETERR(EMSGSIZE);
    NS_GET16(handle->_id, msg);
    if (msg + sizeof(uint16) > eom)
        RETERR(EMSGSIZE);
    NS_GET16(handle->_flags, msg);
    for (i = 0; i < ns_s_max; i++) {
        if (msg + sizeof(uint16) > eom)
            RETERR(EMSGSIZE);
        NS_GET16(handle->_counts[i], msg);
    }
    for (i = 0; i < ns_s_max; i++)
        if (handle->_counts[i] == 0)
            handle->_sections[i] = NULL;
        else {
            int b = ns_skiprr(msg, eom, (ns_sect)i,
                      handle->_counts[i]);

            if (b < 0)
                return (-1);
            handle->_sections[i] = msg;
            msg += b;
        }
    if (msg != eom)
        RETERR(EMSGSIZE);
    setsection(handle, ns_s_max);
    return (0);
}

int
ns_parserr(ns_msg *handle, ns_sect section, int rrnum, ns_rr *rr) {
    int b;

    /* Make section right. */
    if (section < 0 || section >= ns_s_max)
        RETERR(ENODEV);
    if (section != handle->_sect)
        setsection(handle, section);

    /* Make rrnum right. */
    if (rrnum == -1)
        rrnum = handle->_rrnum;
    if (rrnum < 0 || rrnum >= handle->_counts[(int)section])
        RETERR(ENODEV);
    if (rrnum < handle->_rrnum)
        setsection(handle, section);
    if (rrnum > handle->_rrnum) {
        b = ns_skiprr(handle->_ptr, handle->_eom, section,
                  rrnum - handle->_rrnum);

        if (b < 0)
            return (-1);
        handle->_ptr += b;
        handle->_rrnum = rrnum;
    }

    /* Do the parse. */
    b = dn_expand(handle->_msg, handle->_eom,
              handle->_ptr, rr->name, NS_MAXDNAME);
    if (b < 0)
        return (-1);
    handle->_ptr += b;
    if (handle->_ptr + sizeof(uint16) + sizeof(uint16) > handle->_eom)
        RETERR(EMSGSIZE);
    NS_GET16(rr->type, handle->_ptr);
    NS_GET16(rr->rr_class, handle->_ptr);
    if (section == ns_s_qd) {
        rr->ttl = 0;
        rr->rdlength = 0;
        rr->rdata = NULL;
    } else {
        if (handle->_ptr + sizeof(uint32) + sizeof(uint16) > handle->_eom)
            RETERR(EMSGSIZE);
        NS_GET32(rr->ttl, handle->_ptr);
        NS_GET16(rr->rdlength, handle->_ptr);
        if (handle->_ptr + rr->rdlength > handle->_eom)
            RETERR(EMSGSIZE);
        rr->rdata = handle->_ptr;
        handle->_ptr += rr->rdlength;
    }
    if (++handle->_rrnum > handle->_counts[(int)section])
        setsection(handle, (ns_sect)((int)section + 1));

    /* All done. */
    return (0);
}

int
ns_skiprr(const uint8 *ptr, const uint8 *eom, ns_sect section, int count) {
    const uint8 *optr = ptr;

    for ((void)NULL; count > 0; count--) {
        int b, rdlength;

        b = dn_skipname(ptr, eom);
        if (b < 0)
            RETERR(EMSGSIZE);
        ptr += b/*Name*/ + sizeof(uint16)/*Type*/ + sizeof(uint16)/*Class*/;
        if (section != ns_s_qd) {
            if (ptr + sizeof(uint32) + sizeof(uint16) > eom)
                RETERR(EMSGSIZE);
            ptr += sizeof(uint32)/*TTL*/;
            NS_GET16(rdlength, ptr);
            ptr += rdlength/*RData*/;
        }
    }
    if (ptr > eom)
        RETERR(EMSGSIZE);
    return (ptr - optr);
}

/*
 * Expand compressed domain name 'comp_dn' to full domain name.
 * 'msg' is a pointer to the begining of the message,
 * 'eomorig' points to the first location after the message,
 * 'exp_dn' is a pointer to a buffer of size 'length' for the result.
 * Return size of compressed name or -1 if there was an error.
 */
int
dn_expand(const uint8 *msg, const uint8 *eom, const uint8 *src,
      char *dst, int dstsiz)
{
    int n = ns_name_uncompress(msg, eom, src, dst, (size_t)dstsiz);

    if (n > 0 && dst[0] == '.')
        dst[0] = '\0';
    return (n);
}

/*
 * Skip over a compressed domain name. Return the size or -1.
 */
int
dn_skipname(const uint8 *ptr, const uint8 *eom) {
    const uint8 *saveptr = ptr;

    if (ns_name_skip(&ptr, eom) == -1)
        return (-1);
    return (ptr - saveptr);
}

/*
 * ns_name_uncompress(msg, eom, src, dst, dstsiz)
 *    Expand compressed domain name to presentation format.
 * return:
 *    Number of bytes read out of `src', or -1 (with errno set).
 * note:
 *    Root domain returns as "." not "".
 */
int
ns_name_uncompress(const uint8 *msg, const uint8 *eom, const uint8 *src,
           char *dst, size_t dstsiz)
{
    uint8 tmp[NS_MAXCDNAME];
    int n;

    if ((n = ns_name_unpack(msg, eom, src, tmp, sizeof tmp)) == -1)
        return (-1);
    if (ns_name_ntop(tmp, dst, dstsiz) == -1)
        return (-1);
    return (n);
}

/*
 * special(ch)
 *    Thinking in noninternationalized USASCII (per the DNS spec),
 *    is this characted special ("in need of quoting") ?
 * return:
 *    boolean.
 */
static int
special(int ch) {
    switch (ch) {
    case 0x22: /* '"' */
    case 0x2E: /* '.' */
    case 0x3B: /* ';' */
    case 0x5C: /* '\\' */
    /* Special modifiers in zone files. */
    case 0x40: /* '@' */
    case 0x24: /* '$' */
        return (1);
    default:
        return (0);
    }
}

/*
 * printable(ch)
 *    Thinking in noninternationalized USASCII (per the DNS spec),
 *    is this character visible and not a space when printed ?
 * return:
 *    boolean.
 */
static int
printable(int ch) {
    return (ch > 0x20 && ch < 0x7f);
}

/*
 * ns_name_ntop(src, dst, dstsiz)
 *    Convert an encoded domain name to printable ascii as per RFC1035.
 * return:
 *    Number of bytes written to buffer, or -1 (with errno set)
 * notes:
 *    The root is returned as "."
 *    All other domains are returned in non absolute form
 */
int
ns_name_ntop(const uint8 *src, char *dst, size_t dstsiz) {
    const uint8 *cp;
    char *dn, *eom;
    uint8 c;
    uint32 n;
    uint32 u;

    cp = src;
    dn = dst;
    eom = dst + dstsiz;

    while ((n = *cp++) != 0) {
        if ((n & NS_CMPRSFLGS) != 0 && n != 0x41) {
            /* Some kind of compression pointer. */
            __set_errno (EMSGSIZE);
            return (-1);
        }
        if (dn != dst) {
            if (dn >= eom) {
                __set_errno (EMSGSIZE);
                return (-1);
            }
            *dn++ = '.';
        }

        if (n == 0x41) {
            n = *cp++ / 8;
            if (dn + n * 2 + 4 >= eom) {
                __set_errno (EMSGSIZE);
                return (-1);
            }
            *dn++ = '\\';
            *dn++ = '[';
            *dn++ = 'x';

            while (n-- > 0) {
                c = *cp++;
                u = c >> 4;
                *dn++ = u > 9 ? 'a' + u - 10 : '0' + u;
                u = c & 0xf;
                *dn++ = u > 9 ? 'a' + u - 10 : '0' + u;
            }

            *dn++ = ']';
            continue;
        }

        if (dn + n >= eom) {
            __set_errno (EMSGSIZE);
            return (-1);
        }
        for ((void)NULL; n > 0; n--) {
            c = *cp++;
            if (special(c)) {
                if (dn + 1 >= eom) {
                    __set_errno (EMSGSIZE);
                    return (-1);
                }
                *dn++ = '\\';
                *dn++ = (char)c;
            } else if (!printable(c)) {
                if (dn + 3 >= eom) {
                    __set_errno (EMSGSIZE);
                    return (-1);
                }
                *dn++ = '\\';
                *dn++ = digits[c / 100];
                *dn++ = digits[(c % 100) / 10];
                *dn++ = digits[c % 10];
            } else {
                if (dn >= eom) {
                    __set_errno (EMSGSIZE);
                    return (-1);
                }
                *dn++ = (char)c;
            }
        }
    }
    if (dn == dst) {
        if (dn >= eom) {
            __set_errno (EMSGSIZE);
            return (-1);
        }
        *dn++ = '.';
    }
    if (dn >= eom) {
        __set_errno (EMSGSIZE);
        return (-1);
    }
    *dn++ = '\0';
    return (dn - dst);
}

/*
 * ns_name_unpack(msg, eom, src, dst, dstsiz)
 *    Unpack a domain name from a message, source may be compressed.
 * return:
 *    -1 if it fails, or consumed octets if it succeeds.
 */
int
ns_name_unpack(const uint8 *msg, const uint8 *eom, const uint8 *src,
           uint8 *dst, size_t dstsiz)
{
    const uint8 *srcp, *dstlim;
    uint8 *dstp;
    int n, len, checked;

    len = -1;
    checked = 0;
    dstp = dst;
    srcp = src;
    dstlim = dst + dstsiz;
    if (srcp < msg || srcp >= eom) {
        __set_errno (EMSGSIZE);
        return (-1);
    }
    /* Fetch next label in domain name. */
    while ((n = *srcp++) != 0) {
        /* Check for indirection. */
        switch (n & NS_CMPRSFLGS) {
        case 0x40:
            if (n == 0x41) {
                if (dstp + 1 >= dstlim) {
                    __set_errno (EMSGSIZE);
                    return (-1);
                  }
                *dstp++ = 0x41;
                n = *srcp++ / 8;
                ++checked;
            } else {
                __set_errno (EMSGSIZE);
                return (-1);        /* flag error */
            }
            /* FALLTHROUGH */
        case 0:
            /* Limit checks. */
            if (dstp + n + 1 >= dstlim || srcp + n >= eom) {
                __set_errno (EMSGSIZE);
                return (-1);
            }
            checked += n + 1;
            *dstp++ = n;
            OSAL_memCpy(dstp, srcp, n);
            dstp += n;
            srcp += n;
            break;

        case NS_CMPRSFLGS:
            if (srcp >= eom) {
                __set_errno (EMSGSIZE);
                return (-1);
            }
            if (len < 0)
                len = srcp - src + 1;
            srcp = msg + (((n & 0x3f) << 8) | (*srcp & 0xff));
            if (srcp < msg || srcp >= eom) {  /* Out of range. */
                __set_errno (EMSGSIZE);
                return (-1);
            }
            checked += 2;
            /*
             * Check for loops in the compressed name;
             * if we've looked at the whole message,
             * there must be a loop.
             */
            if (checked >= eom - msg) {
                __set_errno (EMSGSIZE);
                return (-1);
            }
            break;

        default:
            __set_errno (EMSGSIZE);
            return (-1);            /* flag error */
        }
    }
    *dstp = '\0';
    if (len < 0)
        len = srcp - src;
    return (len);
}

/*
 * ns_name_skip(ptrptr, eom)
 *    Advance *ptrptr to skip over the compressed name it points at.
 * return:
 *    0 on success, -1 (with errno set) on failure.
 */
int
ns_name_skip(const uint8 **ptrptr, const uint8 *eom) {
    const uint8 *cp;
    uint32 n;

    cp = *ptrptr;
    while (cp < eom && (n = *cp++) != 0) {
        /* Check for indirection. */
        switch (n & NS_CMPRSFLGS) {
        case 0:            /* normal case, n == len */
            cp += n;
            continue;
        case NS_CMPRSFLGS:    /* indirection */
            cp++;
            break;
        default:        /* illegal type */
            __set_errno (EMSGSIZE);
            return (-1);
        }
        break;
    }
    if (cp > eom) {
        __set_errno (EMSGSIZE);
        return (-1);
    }
    *ptrptr = cp;
    return (0);
}


static void
setsection(ns_msg *msg, ns_sect sect) {
    msg->_sect = sect;
    if (sect == ns_s_max) {
        msg->_rrnum = -1;
        msg->_ptr = NULL;
    } else {
        msg->_rrnum = 0;
        msg->_ptr = msg->_sections[(int)sect];
    }
}

#ifndef OSAL_KERNEL_EMULATION
EXPORT_SYMBOL(ns_initparse);
EXPORT_SYMBOL(ns_parserr);
EXPORT_SYMBOL(ns_get16);
EXPORT_SYMBOL(ns_get32);
#endif
