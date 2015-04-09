#ifndef _RESOLVE_H_
#define _RESOLVE_H_

#include <errno.h>
#include <osal_types.h>

/*
 * Inline versions of get short/long.  Pointer is advanced.
 */
#define NS_GET16(s, cp) do { \
    register uint8 *t_cp = (uint8 *)(cp); \
    (s) = ((uint16)t_cp[0] << 8) \
        | ((uint16)t_cp[1]) \
        ; \
    (cp) += sizeof(uint16); \
} while (0)

#define NS_GET32(l, cp) do { \
    register uint8 *t_cp = (uint8 *)(cp); \
    (l) = ((uint32)t_cp[0] << 24) \
        | ((uint32)t_cp[1] << 16) \
        | ((uint32)t_cp[2] << 8) \
        | ((uint32)t_cp[3]) \
        ; \
    (cp) += sizeof(uint32); \
} while (0)




#define size_t unsigned int
#define ns_get16               __ns_get16
#define ns_get32               __ns_get32
#define ns_parserr             __ns_parserr
#define ns_skiprr              __ns_skiprr
#define ns_initparse           __ns_initparse
#define dn_expand              __dn_expand
#define dn_skipname            __dn_skipname
#define    ns_name_uncompress  __ns_name_uncompress
#define    ns_name_unpack      __ns_name_unpack
#define    ns_name_ntop        __ns_name_ntop
#define    ns_name_skip        __ns_name_skip


#ifndef __THROW
# define __THROW
#endif

#ifndef _HURD_ERRNO
#define _HURD_ERRNO(n)    ((0x10 << 26) | ((n) & 0x3fff))
#endif

#define NS_MAXDNAME    1025    /* maximum domain name */
#define NS_MAXCDNAME   255     /* maximum compressed domain name */
#define NS_MAXLABEL    63      /* maximum length of domain label */
#define NS_CMPRSFLGS   0xc0    /* Flag bits indicating name compression. */

#define __BEGIN_DECLS
#define __END_DECLS


#ifndef __set_errno
#define __set_errno(e) (errno = (e))
#endif

#define RETERR(err) do { __set_errno (err); return (-1); } while (0)
struct _ns_flagdata {  int mask, shift;  };
extern struct _ns_flagdata _ns_flagdata[];

/* Accessor macros - this is part of the public interface. */
#define ns_msg_getflag(handle, flag) ( \
((handle)._flags & _ns_flagdata[flag].mask) \
 >> _ns_flagdata[flag].shift \
)

/*
 * This is a parsed record.  It is caller allocated and has no dynamic data.
 */
typedef    struct __ns_rr {
    char        name[NS_MAXDNAME];
    uint16      type;
    uint16      rr_class;
    uint32      ttl;
    uint16      rdlength;
    const uint8 *   rdata;
} ns_rr;

/* Accessor macros - this is part of the public interface. */
#define ns_rr_name(rr)  (((rr).name[0] != '\0') ? (rr).name : ".")
#define ns_rr_type(rr)  ((ns_type)((rr).type + 0))
#define ns_rr_class(rr) ((ns_class)((rr).rr_class + 0))
#define ns_rr_ttl(rr)   ((rr).ttl + 0)
#define ns_rr_rdlen(rr) ((rr).rdlength + 0)
#define ns_rr_rdata(rr) ((rr).rdata + 0)

/*
 * These can be expanded with synonyms, just keep ns_parse.c:ns_parserecord()
 * in synch with it.
 */
typedef enum __ns_sect {
    ns_s_qd = 0,        /* Query: Question. */
    ns_s_zn = 0,        /* Update: Zone. */
    ns_s_an = 1,        /* Query: Answer. */
    ns_s_pr = 1,        /* Update: Prerequisites. */
    ns_s_ns = 2,        /* Query: Name servers. */
    ns_s_ud = 2,        /* Update: Update. */
    ns_s_ar = 3,        /* Query|Update: Additional records. */
    ns_s_max = 4
} ns_sect;
/*
 * This is a message handle.  It is caller allocated and has no dynamic data.
 * This structure is intended to be opaque to all but ns_parse.c, thus the
 * leading _'s on the member names.  Use the accessor functions, not the _'s.
 */
typedef struct __ns_msg {
    const uint8    *_msg, *_eom;
    uint16          _id, _flags, _counts[ns_s_max];
    const uint8    *_sections[ns_s_max];
    ns_sect         _sect;
    int             _rrnum;
    const uint8    *_ptr;
} ns_msg;

/* Accessor macros - this is part of the public interface. */

#define ns_msg_id(handle) ((handle)._id + 0)
#define ns_msg_base(handle) ((handle)._msg + 0)
#define ns_msg_end(handle) ((handle)._eom + 0)
#define ns_msg_size(handle) ((handle)._eom - (handle)._msg)
#define ns_msg_count(handle, section) ((handle)._counts[section] + 0)

/*
 * These don't have to be in the same order as in the packet flags word,
 * and they can even overlap in some cases, but they will need to be kept
 * in synch with ns_parse.c:ns_flagdata[].
 */
typedef enum __ns_flag {
    ns_f_qr,        /*Question/Response. */
    ns_f_opcode,    /* Operation code. */
    ns_f_aa,        /* Authoritative Answer. */
    ns_f_tc,        /* Truncation occurred. */
    ns_f_rd,        /* Recursion Desired. */
    ns_f_ra,        /* Recursion Available. */
    ns_f_z,         /* MBZ. */
    ns_f_ad,        /* Authentic Data (DNSSEC). */
    ns_f_cd,        /* Checking Disabled (DNSSEC). */
    ns_f_rcode,     /* Response code. */
    ns_f_max
} ns_flag;

/*
 * Currently defined opcodes.
 */
typedef enum __ns_opcode {
    ns_o_query = 0,     /* Standard query. */
    ns_o_iquery = 1,    /* Inverse query (deprecated/unsupported). */
    ns_o_status = 2,    /* Name server status query (unsupported). */
                /* Opcode 3 is undefined/reserved. */
    ns_o_notify = 4,    /* Zone change notification. */
    ns_o_update = 5,    /* Zone update message. */
    ns_o_max = 6
} ns_opcode;

/*%
 * Currently defined response codes.
 */
typedef    enum __ns_rcode {
    ns_r_noerror = 0,    /* No error occurred. */
    ns_r_formerr = 1,    /* Format error. */
    ns_r_servfail = 2,   /* Server failure. */
    ns_r_nxdomain = 3,   /* Name error. */
    ns_r_notimpl = 4,    /* Unimplemented. */
    ns_r_refused = 5,    /* Operation refused. */
    /* these are for BIND_UPDATE */
    ns_r_yxdomain = 6,   /* Name exists */
    ns_r_yxrrset = 7,    /* RRset exists */
    ns_r_nxrrset = 8,    /* RRset does not exist */
    ns_r_notauth = 9,    /* Not authoritative for zone */
    ns_r_notzone = 10,   /* Zone of record different from zone section */
    ns_r_max = 11,
    /* The following are EDNS extended rcodes */
    ns_r_badvers = 16,
    /* The following are TSIG errors */
    ns_r_badsig = 16,
    ns_r_badkey = 17,
    ns_r_badtime = 18
} ns_rcode;

/*
 * Currently defined type values for resources and queries.
 */
typedef enum __ns_type {
    ns_t_invalid = 0,   /* Cookie. */
    ns_t_a = 1,         /* Host address. */
    ns_t_ns = 2,        /* Authoritative server. */
    ns_t_md = 3,        /* Mail destination. */
    ns_t_mf = 4,        /* Mail forwarder. */
    ns_t_cname = 5,     /* Canonical name. */
    ns_t_soa = 6,       /* Start of authority zone. */
    ns_t_mb = 7,        /* Mailbox domain name. */
    ns_t_mg = 8,        /* Mail group member. */
    ns_t_mr = 9,        /* Mail rename name. */
    ns_t_null = 10,     /* Null resource record. */
    ns_t_wks = 11,      /* Well known service. */
    ns_t_ptr = 12,      /* Domain name pointer. */
    ns_t_hinfo = 13,    /* Host information. */
    ns_t_minfo = 14,    /* Mailbox information. */
    ns_t_mx = 15,       /* Mail routing information. */
    ns_t_txt = 16,      /* Text strings. */
    ns_t_rp = 17,       /* Responsible person. */
    ns_t_afsdb = 18,    /* AFS cell database. */
    ns_t_x25 = 19,      /* X_25 calling address. */
    ns_t_isdn = 20,     /* ISDN calling address. */
    ns_t_rt = 21,       /* Router. */
    ns_t_nsap = 22,     /* NSAP address. */
    ns_t_nsap_ptr = 23, /* Reverse NSAP lookup (deprecated). */
    ns_t_sig = 24,      /* Security signature. */
    ns_t_key = 25,      /* Security key. */
    ns_t_px = 26,       /* X.400 mail mapping. */
    ns_t_gpos = 27,     /* Geographical position (withdrawn). */
    ns_t_aaaa = 28,     /* Ip6 Address. */
    ns_t_loc = 29,      /* Location Information. */
    ns_t_nxt = 30,      /* Next domain (security). */
    ns_t_eid = 31,      /* Endpoint identifier. */
    ns_t_nimloc = 32,   /* Nimrod Locator. */
    ns_t_srv = 33,      /* Server Selection. */
    ns_t_atma = 34,     /* ATM Address */
    ns_t_naptr = 35,    /* Naming Authority PoinTeR */
    ns_t_kx = 36,       /* Key Exchange */
    ns_t_cert = 37,     /* Certification record */
    ns_t_a6 = 38,       /* IPv6 address (deprecated, use ns_t_aaaa) */
    ns_t_dname = 39,    /* Non-terminal DNAME (for IPv6) */
    ns_t_sink = 40,     /* Kitchen sink (experimentatl) */
    ns_t_opt = 41,      /* EDNS0 option (meta-RR) */
    ns_t_apl = 42,      /* Address prefix list (RFC3123) */
    ns_t_tkey = 249,    /* Transaction key */
    ns_t_tsig = 250,    /* Transaction signature. */
    ns_t_ixfr = 251,    /* Incremental zone transfer. */
    ns_t_axfr = 252,    /* Transfer zone of authority. */
    ns_t_mailb = 253,   /* Transfer mailbox records. */
    ns_t_maila = 254,   /* Transfer mail agent records. */
    ns_t_any = 255,     /* Wildcard match. */
    ns_t_zxfr = 256,    /* BIND-specific, nonstandard. */
    ns_t_max = 65536
} ns_type;

__BEGIN_DECLS
uint16        ns_get16 (const uint8 *) __THROW;
uint32        ns_get32 (const uint8 *) __THROW;
int        ns_initparse (const uint8 *, int, ns_msg *) __THROW;
int        ns_parserr (ns_msg *, ns_sect, int, ns_rr *) __THROW;
int        ns_skiprr (const uint8 *, const uint8 *, ns_sect, int)
     __THROW;
int        dn_expand (const uint8 *, const uint8 *, const uint8 *,
            char *, int) __THROW;
int        dn_skipname (const uint8 *, const uint8 *) __THROW;   
int        ns_name_uncompress (const uint8 *, const uint8 *,
                    const uint8 *, char *, size_t) __THROW;  
int        ns_name_ntop (const uint8 *, char *, size_t) __THROW;
int        ns_name_unpack (const uint8 *, const uint8 *,
                const uint8 *, uint8 *, size_t) __THROW;                    
int        ns_name_skip (const uint8 **, const uint8 *) __THROW;
__END_DECLS
#endif /* !_RESOLVE_H_ */
