#ifndef _ARPA_NAMESER_COMPAT_
#define _ARPA_NAMESER_COMPAT_
#define __BIND 19950621
#include<endian.h>
typedef struct{
	unsigned id:16;
	unsigned rd:1;
	unsigned tc:1;
	unsigned aa:1;
	unsigned opcode:4;
	unsigned qr:1;
	unsigned rcode:4;
	unsigned cd:1;
	unsigned ad:1;
	unsigned unused:1;
	unsigned ra:1;
	unsigned qdcount:16;
	unsigned ancount:16;
	unsigned nscount:16;
unsigned arcount:16;
}HEADER;
#define PACKETSZ        NS_PACKETSZ
#define MAXDNAME        NS_MAXDNAME
#define MAXCDNAME       NS_MAXCDNAME
#define MAXLABEL        NS_MAXLABEL
#define HFIXEDSZ        NS_HFIXEDSZ
#define QFIXEDSZ        NS_QFIXEDSZ
#define RRFIXEDSZ       NS_RRFIXEDSZ
#define INT32SZ         NS_INT32SZ
#define INT16SZ         NS_INT16SZ
#define INT8SZ          NS_INT8SZ
#define INADDRSZ        NS_INADDRSZ
#define IN6ADDRSZ       NS_IN6ADDRSZ
#define INDIR_MASK      NS_CMPRSFLGS
#define NAMESERVER_PORT NS_DEFAULTPORT
#define S_ZONE          ns_s_zn
#define S_PREREQ        ns_s_pr
#define S_UPDATE        ns_s_ud
#define S_ADDT          ns_s_ar
#define QUERY           ns_o_query
#define IQUERY          ns_o_iquery
#define STATUS          ns_o_status
#define NS_NOTIFY_OP    ns_o_notify
#define NS_UPDATE_OP    ns_o_update
#define NOERROR         ns_r_noerror
#define FORMERR         ns_r_formerr
#define SERVFAIL        ns_r_servfail
#define NXDOMAIN        ns_r_nxdomain
#define NOTIMP          ns_r_notimpl
#define REFUSED         ns_r_refused
#define YXDOMAIN        ns_r_yxdomain
#define YXRRSET         ns_r_yxrrset
#define NXRRSET         ns_r_nxrrset
#define NOTAUTH         ns_r_notauth
#define NOTZONE         ns_r_notzone
#define DELETE          ns_uop_delete
#define ADD             ns_uop_add
#define T_A             ns_t_a
#define T_NS            ns_t_ns
#define T_MD            ns_t_md
#define T_MF            ns_t_mf
#define T_CNAME         ns_t_cname
#define T_SOA           ns_t_soa
#define T_MB            ns_t_mb
#define T_MG            ns_t_mg
#define T_MR            ns_t_mr
#define T_NULL          ns_t_null
#define T_WKS           ns_t_wks
#define T_PTR           ns_t_ptr
#define T_HINFO         ns_t_hinfo
#define T_MINFO         ns_t_minfo
#define T_MX            ns_t_mx
#define T_TXT           ns_t_txt
#define T_RP            ns_t_rp
#define T_AFSDB         ns_t_afsdb
#define T_X25           ns_t_x25
#define T_ISDN          ns_t_isdn
#define T_RT            ns_t_rt
#define T_NSAP          ns_t_nsap
#define T_NSAP_PTR      ns_t_nsap_ptr
#define T_SIG           ns_t_sig
#define T_KEY           ns_t_key
#define T_PX            ns_t_px
#define T_GPOS          ns_t_gpos
#define T_AAAA          ns_t_aaaa
#define T_LOC           ns_t_loc
#define T_NXT           ns_t_nxt
#define T_EID           ns_t_eid
#define T_NIMLOC        ns_t_nimloc
#define T_SRV           ns_t_srv
#define T_ATMA          ns_t_atma
#define T_NAPTR         ns_t_naptr
#define T_A6            ns_t_a6
#define T_TSIG          ns_t_tsig
#define T_IXFR          ns_t_ixfr
#define T_AXFR          ns_t_axfr
#define T_MAILB         ns_t_mailb
#define T_MAILA         ns_t_maila
#define T_ANY           ns_t_any
#define C_IN            ns_c_in
#define C_CHAOS         ns_c_chaos
#define C_HS            ns_c_hs
#define C_NONE          ns_c_none
#define C_ANY           ns_c_any
#define GETSHORT        NS_GET16
#define GETLONG         NS_GET32
#define PUTSHORT        NS_PUT16
#define PUTLONG         NS_PUT32
#endif