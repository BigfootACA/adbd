#ifndef _ARPA_NAMESER_H_
#define _ARPA_NAMESER_H_
#ifndef _ARPA_NAMESER_H
#define _ARPA_NAMESER_H
#define BIND_4_COMPAT
#include<stdint.h>
#include<sys/types.h>
#include<sys/cdefs.h>
#define __NAMESER        19991006
#define NS_PACKETSZ      512       /*default UDP packet size*/
#define NS_MAXDNAME      1025      /*maximum domain name*/
#define NS_MAXMSG        65535     /*maximum message size*/
#define NS_MAXCDNAME     255       /*maximum compressed domain name*/
#define NS_MAXLABEL      63        /*maximum length of domain label*/
#define NS_HFIXEDSZ      12        /*#/bytes of fixed data in header*/
#define NS_QFIXEDSZ      4         /*#/bytes of fixed data in query*/
#define NS_RRFIXEDSZ     10        /*#/bytes of fixed data in r record*/
#define NS_INT32SZ       4         /*#/bytes of data in a uint32_t*/
#define NS_INT16SZ       2         /*#/bytes of data in a uint16_t*/
#define NS_INT8SZ        1         /*#/bytes of data in a uint8_t*/
#define NS_INADDRSZ      4         /*IPv4 T_A*/
#define NS_IN6ADDRSZ     16        /*IPv6 T_AAAA*/
#define NS_CMPRSFLGS     0xc0      /*Flag bits indicating name compression.*/
#define NS_DEFAULTPORT   53        /*For both TCP and UDP.*/
typedef enum __ns_sect{
        ns_s_qd=0,/*Query: Question.*/
        ns_s_zn=0,/*Update: Zone.*/
        ns_s_an=1,/*Query: Answer.*/
        ns_s_pr=1,/*Update: Prerequisites.*/
        ns_s_ns=2,/*Query: Name servers.*/
        ns_s_ud=2,/*Update: Update.*/
        ns_s_ar=3,/*Query|Update: Additional records.*/
        ns_s_max=4
}ns_sect;
typedef struct __ns_msg{
        const u_char*_msg,*_eom;
        uint16_t _id,_flags,_counts[ns_s_max];
        const u_char*_sections[ns_s_max];
        ns_sect _sect;
        int _rrnum;
        const u_char*_msg_ptr;
}ns_msg;
struct _ns_flagdata{int mask,shift; };
extern const struct _ns_flagdata _ns_flagdata[];
#define ns_msg_id(handle)            ((handle)._id+0)
#define ns_msg_base(handle)          ((handle)._msg+0)
#define ns_msg_end(handle)           ((handle)._eom+0)
#define ns_msg_size(handle)          ((size_t)((handle)._eom-(handle)._msg))
#define ns_msg_count(handle,section) ((handle)._counts[section]+0)
typedef        struct __ns_rr{
        char name[NS_MAXDNAME];
        uint16_t type,rr_class;
        uint32_t ttl;
        uint16_t rdlength;
        const u_char*rdata;
}ns_rr;
#define ns_rr_name(rr)  (((rr).name[0]!='\0')?(rr).name:".")
#define ns_rr_type(rr)  ((ns_type)((rr).type+0))
#define ns_rr_class(rr) ((ns_class)((rr).rr_class+0))
#define ns_rr_ttl(rr)   ((u_long)(rr).ttl+0)
#define ns_rr_rdlen(rr) ((size_t)(rr).rdlength+0)
#define ns_rr_rdata(rr) ((rr).rdata + 0)
typedef enum __ns_flag{
        ns_f_qr,           /*Question/Response.*/
        ns_f_opcode,       /*Operation code.*/
        ns_f_aa,           /*Authoritative Answer.*/
        ns_f_tc,           /*Truncation occurred.*/
        ns_f_rd,           /*Recursion Desired.*/
        ns_f_ra,           /*Recursion Available.*/
        ns_f_z,            /*MBZ.*/
        ns_f_ad,           /*Authentic Data (DNSSEC).*/
        ns_f_cd,           /*Checking Disabled (DNSSEC).*/
        ns_f_rcode,        /*Response code.*/
        ns_f_max
}ns_flag;
typedef enum __ns_opcode{
        ns_o_query=0,  /*Standard query.*/
        ns_o_iquery=1, /*Inverse query (deprecated/unsupported).*/
        ns_o_status=2, /*Name server status query (unsupported).*/
        ns_o_notify=4, /*Zone change notification.*/
        ns_o_update=5, /*Zone update message.*/
        ns_o_max=6
}ns_opcode;
typedef        enum __ns_rcode{
        ns_r_noerror=0, /*No error occurred.*/
        ns_r_formerr=1, /*Format error.*/
        ns_r_servfail=2,/*Server failure.*/
        ns_r_nxdomain=3,/*Name error.*/
        ns_r_notimpl=4, /*Unimplemented.*/
        ns_r_refused=5, /*Operation refused.*/
        ns_r_yxdomain=6,/*Name exists*/
        ns_r_yxrrset=7, /*RRset exists*/
        ns_r_nxrrset=8, /*RRset does not exist*/
        ns_r_notauth=9, /*Not authoritative for zone*/
        ns_r_notzone=10,/*Zone of record different from zone section*/
        ns_r_max=11,
        ns_r_badvers=16,
        ns_r_badsig=16,
        ns_r_badkey=17,
        ns_r_badtime=18
}ns_rcode;
typedef enum __ns_update_operation{
        ns_uop_delete=0,
        ns_uop_add=1,
        ns_uop_max=2
}ns_update_operation;
struct ns_tsig_key{
        char name[NS_MAXDNAME],alg[NS_MAXDNAME];
        unsigned char*data;
        int len;
};
typedef struct ns_tsig_key ns_tsig_key;
struct ns_tcp_tsig_state{
        int counter;
        struct dst_key*key;
        void*ctx;
        unsigned char sig[NS_PACKETSZ];
        int siglen;
};
typedef struct ns_tcp_tsig_state ns_tcp_tsig_state;
#define NS_TSIG_FUDGE          300
#define NS_TSIG_TCP_COUNT      100
#define NS_TSIG_ALG_HMAC_MD5   "HMAC-MD5.SIG-ALG.REG.INT"
#define NS_TSIG_ERROR_NO_TSIG  -10
#define NS_TSIG_ERROR_NO_SPACE -11
#define NS_TSIG_ERROR_FORMERR  -12
typedef enum __ns_type{
        ns_t_invalid=0,        /*Cookie.*/
        ns_t_a=1,              /*Host address.*/
        ns_t_ns=2,             /*Authoritative server.*/
        ns_t_md=3,             /*Mail destination.*/
        ns_t_mf=4,             /*Mail forwarder.*/
        ns_t_cname=5,          /*Canonical name.*/
        ns_t_soa=6,            /*Start of authority zone.*/
        ns_t_mb=7,             /*Mailbox domain name.*/
        ns_t_mg=8,             /*Mail group member.*/
        ns_t_mr=9,             /*Mail rename name.*/
        ns_t_null=10,          /*Null resource record.*/
        ns_t_wks=11,           /*Well known service.*/
        ns_t_ptr=12,           /*Domain name pointer.*/
        ns_t_hinfo=13,         /*Host information.*/
        ns_t_minfo=14,         /*Mailbox information.*/
        ns_t_mx=15,            /*Mail routing information.*/
        ns_t_txt=16,           /*Text strings.*/
        ns_t_rp=17,            /*Responsible person.*/
        ns_t_afsdb=18,         /*AFS cell database.*/
        ns_t_x25=19,           /*X_25 calling address.*/
        ns_t_isdn=20,          /*ISDN calling address.*/
        ns_t_rt=21,            /*Router.*/
        ns_t_nsap=22,          /*NSAP address.*/
        ns_t_nsap_ptr=23,      /*Reverse NSAP lookup (deprecated).*/
        ns_t_sig=24,           /*Security signature.*/
        ns_t_key=25,           /*Security key.*/
        ns_t_px=26,            /*X.400 mail mapping.*/
        ns_t_gpos=27,          /*Geographical position (withdrawn).*/
        ns_t_aaaa=28,          /*Ip6 Address.*/
        ns_t_loc=29,           /*Location Information.*/
        ns_t_nxt=30,           /*Next domain (security).*/
        ns_t_eid=31,           /*Endpoint identifier.*/
        ns_t_nimloc=32,        /*Nimrod Locator.*/
        ns_t_srv=33,           /*Server Selection.*/
        ns_t_atma=34,          /*ATM Address*/
        ns_t_naptr=35,         /*Naming Authority PoinTeR*/
        ns_t_kx=36,            /*Key Exchange*/
        ns_t_cert=37,          /*Certification record*/
        ns_t_a6=38,            /*IPv6 address (deprecates AAAA)*/
        ns_t_dname=39,         /*Non-terminal DNAME (for IPv6)*/
        ns_t_sink=40,          /*Kitchen sink (experimentatl)*/
        ns_t_opt=41,           /*EDNS0 option (meta-RR)*/
        ns_t_apl=42,           /*Address prefix list (RFC 3123)*/
        ns_t_tkey=249,         /*Transaction key*/
        ns_t_tsig=250,         /*Transaction signature.*/
        ns_t_ixfr=251,         /*Incremental zone transfer.*/
        ns_t_axfr=252,         /*Transfer zone of authority.*/
        ns_t_mailb=253,        /*Transfer mailbox records.*/
        ns_t_maila=254,        /*Transfer mail agent records.*/
        ns_t_any=255,          /*Wildcard match.*/
        ns_t_zxfr=256,         /*BIND-specific,nonstandard.*/
        ns_t_max=65536
}ns_type;
#define ns_t_qt_p(t)  (ns_t_xfr_p(t)||(t)==ns_t_any||(t)==ns_t_mailb||(t)==ns_t_maila)
#define ns_t_mrr_p(t) ((t)==ns_t_tsig||(t)==ns_t_opt)
#define ns_t_rr_p(t)  (!ns_t_qt_p(t)&&!ns_t_mrr_p(t))
#define ns_t_udp_p(t) ((t)!=ns_t_axfr&&(t)!=ns_t_zxfr)
#define ns_t_xfr_p(t) ((t)==ns_t_axfr||(t)==ns_t_ixfr||(t)==ns_t_zxfr)
typedef enum __ns_class{
        ns_c_invalid=0,   /*Cookie.*/
        ns_c_in=1,        /*Internet.*/
        ns_c_2=2,         /*unallocated/unsupported.*/
        ns_c_chaos=3,     /*MIT Chaos-net.*/
        ns_c_hs=4,        /*MIT Hesiod.*/
        ns_c_none=254,    /*for prereq. sections in update requests*/
        ns_c_any=255,     /*Wildcard match.*/
        ns_c_max=65536
}ns_class;
typedef enum __ns_key_types{
        ns_kt_rsa=1,      /*key type RSA/MD5*/
        ns_kt_dh =2,      /*Diffie Hellman*/
        ns_kt_dsa=3,      /*Digital Signature Standard (MANDATORY)*/
        ns_kt_private=254 /*Private key type starts with OID*/
}ns_key_types;
typedef enum __ns_cert_types{
        cert_t_pkix=1,    /*PKIX (X.509v3)*/
        cert_t_spki=2,    /*SPKI*/
        cert_t_pgp=3,     /*PGP*/
        cert_t_url=253,   /*URL private type*/
        cert_t_oid=254    /*OID private type*/
}ns_cert_types;
#define NS_KEY_TYPEMASK       0xC000 /*Mask for "type" bits*/
#define NS_KEY_TYPE_AUTH_CONF 0x0000 /*Key usable for both*/
#define NS_KEY_TYPE_CONF_ONLY 0x8000 /*Key usable for confidentiality*/
#define NS_KEY_TYPE_AUTH_ONLY 0x4000 /*Key usable for authentication*/
#define NS_KEY_TYPE_NO_KEY    0xC000 /*No key usable for either; no key*/
#define NS_KEY_NO_AUTH        0x8000 /*Key unusable for authentication*/
#define NS_KEY_NO_CONF        0x4000 /*Key unusable for confidentiality*/
#define NS_KEY_RESERVED2      0x2000 /*Security is*mandatory*if bit=0*/
#define NS_KEY_EXTENDED_FLAGS 0x1000 /*reserved - must be zero*/
#define NS_KEY_RESERVED4      0x0800 /*reserved - must be zero*/
#define NS_KEY_RESERVED5      0x0400 /*reserved - must be zero*/
#define NS_KEY_NAME_TYPE      0x0300 /*these bits determine the type*/
#define NS_KEY_NAME_USER      0x0000 /*key is assoc. with user*/
#define NS_KEY_NAME_ENTITY    0x0200 /*key is assoc. with entity eg host*/
#define NS_KEY_NAME_ZONE      0x0100 /*key is zone key*/
#define NS_KEY_NAME_RESERVED  0x0300 /*reserved meaning*/
#define NS_KEY_RESERVED8      0x0080 /*reserved - must be zero*/
#define NS_KEY_RESERVED9      0x0040 /*reserved - must be zero*/
#define NS_KEY_RESERVED10     0x0020 /*reserved - must be zero*/
#define NS_KEY_RESERVED11     0x0010 /*reserved - must be zero*/
#define NS_KEY_SIGNATORYMASK  0x000F /*key can sign RR's of same name*/
#define NS_KEY_RESERVED_BITMASK (\
	NS_KEY_RESERVED2|\
        NS_KEY_RESERVED4|\
        NS_KEY_RESERVED5|\
        NS_KEY_RESERVED8|\
        NS_KEY_RESERVED9|\
        NS_KEY_RESERVED10|\
        NS_KEY_RESERVED11)
#define NS_KEY_RESERVED_BITMASK2 0xFFFF    /*no bits defined here*/
#define NS_ALG_MD5RSA            1         /*MD5 with RSA*/
#define NS_ALG_DH                2         /*Diffie Hellman KEY*/
#define NS_ALG_DSA               3         /*DSA KEY*/
#define NS_ALG_DSS               NS_ALG_DSA
#define NS_ALG_EXPIRE_ONLY       253       /*No alg,no security*/
#define NS_ALG_PRIVATE_OID       254       /*Key begins with OID giving alg*/
#define NS_KEY_PROT_TLS          1
#define NS_KEY_PROT_EMAIL        2
#define NS_KEY_PROT_DNSSEC       3
#define NS_KEY_PROT_IPSEC        4
#define NS_KEY_PROT_ANY          255
#define NS_MD5RSA_MIN_BITS       512        /*Size of a mod or exp in bits*/
#define NS_MD5RSA_MAX_BITS       4096
#define NS_MD5RSA_MAX_BYTES      ((NS_MD5RSA_MAX_BITS+7/8)*2+3)
#define NS_MD5RSA_MAX_BASE64     (((NS_MD5RSA_MAX_BYTES+2)/3)*4)
#define NS_MD5RSA_MIN_SIZE       ((NS_MD5RSA_MIN_BITS+7)/8)
#define NS_MD5RSA_MAX_SIZE       ((NS_MD5RSA_MAX_BITS+7)/8)
#define NS_DSA_SIG_SIZE          41
#define NS_DSA_MIN_SIZE          213
#define NS_DSA_MAX_BYTES         405
#define NS_SIG_TYPE              0        /*Type flags*/
#define NS_SIG_ALG               2        /*Algorithm*/
#define NS_SIG_LABELS            3        /*How many labels in name*/
#define NS_SIG_OTTL              4        /*Original TTL*/
#define NS_SIG_EXPIR             8        /*Expiration time*/
#define NS_SIG_SIGNED            12       /*Signature time*/
#define NS_SIG_FOOT              16       /*Key footprint*/
#define NS_SIG_SIGNER            18       /*Domain name of who signed it*/
#define NS_NXT_BITS              8
#define NS_NXT_BIT_SET(n,p)      (p[(n)/NS_NXT_BITS]|=(0x80>>((n)%NS_NXT_BITS)))
#define NS_NXT_BIT_CLEAR(n,p)    (p[(n)/NS_NXT_BITS]&=~(0x80>>((n)%NS_NXT_BITS)))
#define NS_NXT_BIT_ISSET(n,p)    (p[(n)/NS_NXT_BITS]&(0x80>>((n)%NS_NXT_BITS)))
#define NS_NXT_MAX               127
#define NS_OPT_DNSSEC_OK         0x8000U
#define NS_GET16(s,cp) do{\
	const u_char*t_cp=(const u_char*)(cp);\
	(s)=((uint16_t)t_cp[0]<<8)|((uint16_t)t_cp[1]);\
	(cp)+=NS_INT16SZ;\
}while(0)
#define NS_GET32(l,cp) do{\
	const u_char*t_cp=(const u_char*)(cp);\
	(l)=((uint32_t)t_cp[0]<<24)|((uint32_t)t_cp[1]<<16)|((uint32_t)t_cp[2]<< 8)|((uint32_t)t_cp[3]);\
	(cp)+=NS_INT32SZ;\
}while(0)
#define NS_PUT16(s,cp) do{\
	uint32_t t_s=(uint32_t)(s);\
	u_char*t_cp=(u_char*)(cp);\
	*t_cp++=t_s>>8;\
	*t_cp=t_s;\
	(cp)+=NS_INT16SZ;\
}while(0)
#define NS_PUT32(l,cp) do{\
	uint32_t t_l=(uint32_t)(l);\
	u_char*t_cp=(u_char*)(cp);\
	*t_cp++=t_l>>24;\
	*t_cp++=t_l>>16;\
	*t_cp++=t_l>>8;\
	*t_cp=t_l;\
	(cp)+=NS_INT32SZ;\
}while(0)

#define ns_msg_getflag          __ns_msg_getflag
#define ns_get16                __ns_get16
#define ns_get32                __ns_get32
#define ns_put16                __ns_put16
#define ns_put32                __ns_put32
#define ns_initparse            __ns_initparse
#define ns_skiprr               __ns_skiprr
#define ns_parserr              __ns_parserr
#define ns_sprintrr             __ns_sprintrr
#define ns_sprintrrf            __ns_sprintrrf
#define ns_format_ttl           __ns_format_ttl
#define ns_parse_ttl            __ns_parse_ttl
#define ns_datetosecs           __ns_datetosecs
#define ns_name_ntol            __ns_name_ntol
#define ns_name_ntop            __ns_name_ntop
#define ns_name_pton            __ns_name_pton
#define ns_name_unpack          __ns_name_unpack
#define ns_name_pack            __ns_name_pack
#define ns_name_compress        __ns_name_compress
#define ns_name_uncompress      __ns_name_uncompress
#define ns_name_skip            __ns_name_skip
#define ns_name_rollback        __ns_name_rollback
#define ns_sign                 __ns_sign
#define ns_sign2                __ns_sign2
#define ns_sign_tcp             __ns_sign_tcp
#define ns_sign_tcp2            __ns_sign_tcp2
#define ns_sign_tcp_init        __ns_sign_tcp_init
#define ns_find_tsig            __ns_find_tsig
#define ns_verify               __ns_verify
#define ns_verify_tcp           __ns_verify_tcp
#define ns_verify_tcp_init      __ns_verify_tcp_init
#define ns_samedomain           __ns_samedomain
#define ns_subdomain            __ns_subdomain
#define ns_makecanon            __ns_makecanon
#define ns_samename             __ns_samename
__BEGIN_DECLS
int      ns_msg_getflag(ns_msg,int);
uint16_t ns_get16(const u_char*);
uint32_t ns_get32(const u_char*);
void     ns_put16(uint16_t,u_char*);
void     ns_put32(uint32_t,u_char*);
int      ns_initparse(const u_char*,int,ns_msg*);
int      ns_skiprr(const u_char*,const u_char*,ns_sect,int);
int      ns_parserr(ns_msg*,ns_sect,int,ns_rr*);
int      ns_sprintrr(const ns_msg*,const ns_rr*,const char*,const char*,char*,size_t);
int      ns_sprintrrf(const u_char*,size_t,const char*,ns_class,ns_type,u_long,const u_char*,size_t,const char*,const char*,char*,size_t);
int      ns_format_ttl(u_long,char*,size_t);
int      ns_parse_ttl(const char*,u_long*);
uint32_t ns_datetosecs(const char*cp,int*errp);
int      ns_name_ntol(const u_char*,u_char*,size_t);
int      ns_name_ntop(const u_char*,char*,size_t);
int      ns_name_pton(const char*,u_char*,size_t);
int      ns_name_unpack(const u_char*,const u_char*,const u_char*,u_char*,size_t);
int      ns_name_pack(const u_char*,u_char*,int,const u_char**,const u_char**);
int      ns_name_uncompress(const u_char*,const u_char*,const u_char*,char*,size_t);
int      ns_name_compress(const char*,u_char*,size_t,const u_char**,const u_char**);
int      ns_name_skip(const u_char**,const u_char*);
void     ns_name_rollback(const u_char*,const u_char**,const u_char**);
int      ns_sign(u_char*,int*,int,int,void*,const u_char*,int,u_char*,int*,time_t);
int      ns_sign2(u_char*,int*,int,int,void*,const u_char*,int,u_char*,int*,time_t,u_char**,u_char**);
int      ns_sign_tcp(u_char*,int*,int,int,ns_tcp_tsig_state*,int);
int      ns_sign_tcp2(u_char*,int*,int,int,ns_tcp_tsig_state*,int,u_char**,u_char**);
int      ns_sign_tcp_init(void*,const u_char*,int,ns_tcp_tsig_state*);
u_char*  ns_find_tsig(u_char*,u_char*);
int      ns_verify(u_char*,int*,void*,const u_char*,int,u_char*,int*,time_t*,int);
int      ns_verify_tcp(u_char*,int*,ns_tcp_tsig_state*,int);
int      ns_verify_tcp_init(void*,const u_char*,int,ns_tcp_tsig_state*);
int      ns_samedomain(const char*,const char*);
int      ns_subdomain(const char*,const char*);
int      ns_makecanon(const char*,char*,size_t);
int      ns_samename(const char*,const char*);
__END_DECLS
#ifdef BIND_4_COMPAT
#include "arpa_nameser_compat.h"
#endif
#define XLOG(...) do{}while(0)
#endif
#endif
