/* ACCEPT parameter list... */
 
typedef struct accept_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    short int   ipmsglim;       /* 0x04: message limit */
    char        filler1[10];    /* 0x06: unused */
    char        ipuser[16];     /* 0x10: user data */
    char        filler2[8];     /* 0x20: unused */
  } accept_pl;
 
/* CONNECT parameter list... */
 
typedef struct connect_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    short int   ipmsglim;       /* 0x04: message limit */
    char        filler1[2];     /* 0x06: unused */
    char        ipvmid[8];      /* 0x08: target VM userid */
    char        ipuser[16];     /* 0x10: user data */
    char        filler2[8];     /* 0x20: unused */
  } connect_pl;
 
/* DECLARE BUFFER parameter list... */
 
typedef struct dclbfr_pl
  {
    char        filler1[3];     /* 0x00: unused */
    char        iprcode;        /* 0x03: return code */
    char        filler2[8];     /* 0x04: unused */
    void        *ipbfadr1;      /* 0x0c: &xtrn int buffer */
    char        filler3[24];    /* 0x10: unused */
  } dclbfr_pl;
 
/* DESCRIBE parameter list... */
 
typedef struct describe_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    int         ipmsgid;        /* 0x04: message identification */
    int         iptrgcls;       /* 0x08: target class */
    int         iprmmsg1;       /* 0x0c: message data */
    int         ipbfln1f;       /* 0x10: message 1 length */
    char        filler1[12];    /* 0x14: unused */
    int         ipbfln2f;       /* 0x20: message 2 length */
    char        filler2[4];     /* 0x24: unused */
  } describe_pl;
 
/* PURGE parameter list... */
 
typedef struct purge_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    int         ipmsgid;        /* 0x04: message identification */
    short int   ipaudit;        /* 0x08: message audit trail */
    char        filler1[10];    /* 0x0a: unused */
    int         ipsrccls;       /* 0x14: message class */
    int         ipmsgtag;       /* 0x18: message tag */
    char        filler2[12];    /* 0x1c: unused */
  } purge_pl;
 
/* QUIESCE parameter list... */
 
typedef struct quiesce_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    char        filler1[12];    /* 0x04: unused */
    char        ipuser[16];     /* 0x10: user data */
    char        filler2[8];     /* 0x20: unused */
  } quiesce_pl;
 
/* RECEIVE parameter list... */
 
typedef struct receive_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    int         ipmsgid;        /* 0x04: message identification */
    int         iptrgcls;       /* 0x08: target class */
    void        *ipbfadr1;      /* 0x0c: &message text buffer */
    int         ipbfln1f;       /* 0x10: message length */
    char        filler1[12];    /* 0x14: unused */
    int         ipbfln2f;       /* 0x20: message length */
    char        filler2[4];     /* 0x24: unused */
  } receive_pl;
 
/* REJECT parameter list... */
 
typedef struct reject_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    int         ipmsgid;        /* 0x04: message identification */
    int         iptrgcls;       /* 0x08: target class */
    char        filler1[28];    /* 0x0c: unused */
  } reject_pl;
 
/* REPLY parameter list... */
 
typedef struct reply_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    int         ipmsgid;        /* 0x04: message identification */
    int         iptrgcls;       /* 0x08: target class */
    int         iprmmsg1;       /* 0x0c: message data */
    int         iprmmsg2;       /* 0x10: message data */
    char        filler1[8];     /* 0x14: unused */
    void        *ipbfadr2;      /* 0x1c: &message text buffer */
    int         ipbfln2f;       /* 0x20: message length */
    char        filler2[4];     /* 0x24: unused */
  } reply_pl;
 
/* RESUME parameter list... */
 
typedef struct resume_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    char        filler1[12];    /* 0x04: unused */
    char        ipuser[16];     /* 0x10: user data */
    char        filler2[8];     /* 0x20: unused */
  } resume_pl;
 
/* SEND parameter list... */
 
typedef struct send_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    int         ipmsgid;        /* 0x04: message identification */
    int         iptrgcls;       /* 0x08: target class */
    void        *ipbfadr1;      /* 0x0c: &input buffer */
    int         ipbfln1f;       /* 0x10: buffer length */
    int         ipsrccls;       /* 0x14: message class */
    int         ipmsgtag;       /* 0x18: message tag */
    void        *ipbfadr2;      /* 0x1c: &reply buffer */
    int         ipbfln2f;       /* 0x20: buffer length */
    char        filler1[4];     /* 0x24: unused */
  } send_pl;
 
/* SETCMASK parameter list... */
 
typedef struct setcmask_pl
  {
    char        ipcmask;        /* 0x00: xtrn int mask byte */
    char        filler1[39];    /* 0x01: unused */
  } setcmask_pl;
 
/* SETMASK parameter list... */
 
typedef struct setmask_pl
  {
    char        ipmask;         /* 0x00: xtrn int mask byte */
    char        filler1[39];    /* 0x01: unused */
  } setmask_pl;
 
/* SEVER parameter list... */
 
typedef struct sever_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    char        filler1[12];    /* 0x04: unused */
    char        ipuser[16];     /* 0x10: user data */
    char        filler2[8];     /* 0x20: unused */
  } sever_pl;
 
/* TESTCMPL parameter list... */
 
typedef struct testcmpl_pl
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iprcode;        /* 0x03: return code */
    int         ipmsgid;        /* 0x04: message identification */
    short int   ipaudit;        /* 0x08: message audit trail */
    char        filler1[2];     /* 0x0a: unused */
    int         iprmmsg1;       /* 0x0c: message data */
    int         iprmmsg2;       /* 0x10: message data */
    int         ipsrccls;       /* 0x14: message class */
    int         ipmsgtag;       /* 0x18: message tag */
    char        filler2[4];     /* 0x1c: unused */
    int         ipbfln2f;       /* 0x20: message length */
    char        filler3[4];     /* 0x24: unused */
  } testcmpl_pl;
 
/* pending connection external interrupt buffer... */
 
typedef struct pending_connection_xib
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iptype;         /* 0x03: interrupt code */
    char        ipmsglim;       /* 0x04: message limit */
    char        filler1[3];     /* 0x05: unused */
    char        ipvmid[8];      /* 0x08: target VM userid */
    char        ipuser[16];     /* 0x10: user data */
    char        filler2[8];     /* 0x20: unused */
  } pending_connection_xib;
 
/* connection complete external interrupt buffer... */
 
typedef struct connection_complete_xib
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iptype;         /* 0x03: interrupt code */
    char        ipmsglim;       /* 0x04: message limit */
    char        filler1[11];    /* 0x05: unused */
    char        ipuser[16];     /* 0x10: user data */
    char        filler2[8];     /* 0x20: unused */
  } connection_complete_xib;
 
/* sever, quiesce and resume external interrupt buffer... */
 
typedef struct sever_xib
  {
    short int   ippathid;       /* 0x00: path id */
    char        filler1;        /* 0x02: unused */
    char        iptype;         /* 0x03: interrupt code */
    char        filler2[12];    /* 0x04: unused */
    char        ipuser[16];     /* 0x10: user data */
    char        filler3[8];     /* 0x20: unused */
  } sever_xib;
 
#define quiesce_xib sever_xib
#define resume_xib sever_xib
 
/* message complete external interrupt buffer... */
 
typedef struct message_complete_xib
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iptype;         /* 0x03: interrupt code */
    int         ipmsgid;        /* 0x04: message identification */
    short int   ipaudit;        /* 0x08: message audit trail */
    char        filler1[2];     /* 0x0a: unused */
    int         iprmmsg1;       /* 0x0c: message data */
    int         iprmmsg2;       /* 0x10: message data */
    int         ipsrccls;       /* 0x14: message class */
    int         ipmsgtag;       /* 0x18: message tag */
    char        filler2[4];     /* 0x1c: unused */
    int         ipbfln2f;       /* 0x20: message length */
    char        filler3[4];     /* 0x24: unused */
  } message_complete_xib;
 
/* pending message external interrupt buffer... */
 
typedef struct pending_message_xib
  {
    short int   ippathid;       /* 0x00: path id */
    char        ipflags1;       /* 0x02: flags */
    char        iptype;         /* 0x03: interrupt code */
    int         ipmsgid;        /* 0x04: message identification */
    int         iptrgcls;       /* 0x08: target class */
    int         iprmmsg1;       /* 0x0c: message data */
    short int   ipbfln1;        /* 0x10: message length */
    short int   iprmmsg2;       /* 0x12: message data */
    char        filler1[12];    /* 0x14: unused */
    int         ipbfln2f;       /* 0x20: message length */
    char        filler2[4];     /* 0x24: unused */
  } pending_message_xib;
 
typedef union iparml
  {
    double                      align_pl;
    accept_pl                   ipl_ac; /* ACCEPT */
    connect_pl                  ipl_co; /* CONNECT */
    dclbfr_pl                   ipl_db; /* DECLARE BUFFER  */
    describe_pl                 ipl_de; /* DESCRIBE */
    purge_pl                    ipl_pu; /* PURGE */
    quiesce_pl                  ipl_qu; /* QUIESCE */
    receive_pl                  ipl_re; /* RECEIVE */
    reject_pl                   ipl_rj; /* REJECT */
    reply_pl                    ipl_rp; /* REPLY */
    resume_pl                   ipl_rs; /* RESUME */
    send_pl                     ipl_se; /* SEND */
    setcmask_pl                 ipl_sc; /* SETCMASK */
    setmask_pl                  ipl_sm; /* SETMASK */
    sever_pl                    ipl_sv; /* SEVER */
    testcmpl_pl                 ipl_tc; /* TESTCMPL */
    pending_connection_xib      ipl_pc; /* pending connection */
    connection_complete_xib     ipl_cc; /* connection complete */
    sever_xib                   ipl_sr; /* sever */
    message_complete_xib        ipl_mc; /* message complete */
    pending_message_xib         ipl_pm; /* pending message */
  } iparml;
 
/* mask values for ipflags1... */
#define IPF1_ALL        0x80    /* quiesce, resume, sever all */
#define IPF1_PARMDATA   0x80    /* message is in plist */
#define IPF1_QUIESCE    0x40    /* connect in quiesce mode */
#define IPF1_BUFLST     0x40    /* buffer list option */
#define IPF1_PRIORITY   0x20    /* priority message/reply */
#define IPF1_NOREPLY    0x10    /* one-way protocol */
#define IPF1_ANSLST     0x08    /* answer list option */
#define IPF1_MSGID      0x04    /* message id specified */
#define IPF1_PATHID     0x02    /* pathid specified */
#define IPF1_MSGCLASS   0x01    /* message class specified */
 
/* mask values for ipmask... */
#define IPM_SEND_NONPR  0x80    /* enable non-priority messages */
#define IPM_SEND_PR     0x40    /* ... priority messages */
#define IPM_REPLY_NONPR 0x20    /* ... non-priority replies */
#define IPM_REPLY_PR    0x10    /* ... priority replies */
#define IPM_CTRL        0x08    /* ... IUCV control interrupts */
 
/* mask values for ipcmask... */
#define IPCM_PENDINGCON 0x80    /* enable pending connection */
#define IPCM_COMPLETECON 0x40   /* ... complete connection */
#define IPCM_SEVER      0x20    /* ... sever */
#define IPCM_QUIESCE    0x10    /* ... quiesce */
#define IPCM_RESUME     0x08    /* ... resume */
 
/* mask values for ipaudit... */
#define IPAD_REPLY_LEN  0x8000  /* reply too long for buffer */
#define IPAD_SEND_PROTX 0x4000  /* send buffer protection exception */
#define IPAD_SEND_ADDRX 0x2000  /* send buffer addressing exception */
#define IPAD_ANS_PROTX  0x1000  /* answer buffer protection exception */
#define IPAD_ANS_ADDRX  0x0800  /* answer buffer addressing exception */
#define IPAD_REJECT     0x0400  /* message was rejected */
#define IPAD_PLDATA     0x0200  /* reply sent in parameter list */
 
#define IPAD_RECV_PROTX 0x0080  /* receive buffer protection except'n */
#define IPAD_RECV_ADDRX 0x0040  /* receive buffer addressing except'n */
#define IPAD_REPLY_PROTX 0x0020 /* reply buffer protection exception */
#define IPAD_REPLY_ADDRX 0x0010 /* reply buffer addressing exception */
#define IPAD_SEVERED    0x0008  /* path was severed */
#define IPAD_INVRLIST   0x0004  /* invalid receive/reply list */
 
/* values for iptype... */
#define IPTYP_PENDING_CON       1
#define IPTYP_CON_COMPLETE      2
#define IPTYP_SEVERED           3
#define IPTYP_QUIESCED          4
#define IPTYP_RESUMED           5
#define IPTYP_INPRIOR_REPLY     6
#define IPTYP_INREPLY           7
#define IPTYP_INPRIOR_MSG       8
#define IPTYP_INMSG             9
 
/* values for iucv_fncode... */
#define IUCVFC_ACCEPT   10
#define IUCVFC_CONNECT  11
#define IUCVFC_DCLBFR   12
#define IUCVFC_DESCRIBE 3
#define IUCVFC_PURGE    9
#define IUCVFC_QUIESCE  13
#define IUCVFC_RECEIVE  5
#define IUCVFC_REJECT   8
#define IUCVFC_REPLY    6
#define IUCVFC_RESUME   14
#define IUCVFC_SEND     4
#define IUCVFC_SETCMASK 17
#define IUCVFC_SETMASK  16
#define IUCVFC_SEVER    15
#define IUCVFC_TESTCMPL 7
#define IUCVFC_QUERY    0
#define IUCVFC_RTRVBFR  2
#define IUCVFC_TESTMSG  1
 
#define iaccept(iucvpl)  iucv(IUCVFC_ACCEPT,(iucvpl))
#define iconnect(iucvpl) iucv(IUCVFC_CONNECT,(iucvpl))
#define idclbfr(iucvpl)  iucv(IUCVFC_DCLBFR,(iucvpl))
#define idescribe(iucvpl) iucv(IUCVFC_DESCRIBE,(iucvpl))
#define ipurge(iucvpl)   iucv(IUCVFC_PURGE,(iucvpl))
#define iquiesce(iucvpl) iucv(IUCVFC_QUIESCE,(iucvpl))
#define ireceive(iucvpl) iucv(IUCVFC_RECEIVE,(iucvpl))
#define ireply(iucvpl)   iucv(IUCVFC_REPLY,(iucvpl))
#define ireject(iucvpl)  iucv(IUCVFC_REJECT,(iucvpl))
#define iresume(iucvpl)  iucv(IUCVFC_RESUME,(iucvpl))
#define isend(iucvpl)    iucv(IUCVFC_SEND,(iucvpl))
#define isetcmask(iucvpl) iucv(IUCVFC_SETCMASK,(iucvpl))
#define isetmask(iucvpl) iucv(IUCVFC_SETMASK,(iucvpl))
#define isever(iucvpl)   iucv(IUCVFC_SEVER,(iucvpl))
#define itestcmpl(iucvpl) iucv(IUCVFC_TESTCMPL,(iucvpl))
#define iquery()         iucv(IUCVFC_QUERY,0)
#define irtrvbfr()       iucv(IUCVFC_RTRVBFR,0)
#define itestmsg()       iucv(IUCVFC_TESTMSG,0)
 
static inline int iucv (int iucv_fncode, void *iucvpl)
{
 int rc;
 
	asm volatile (
		"L	r0,%1;\n"
		"LA	r1,%2;\n"
		"EX	r0,=X'B2F01000';\n"
		"IPM	r1;\n"
		"SRL	r1,28;n"
		"LR	%0,r1;\n"
		:"=r" (rc)
		:"=m" (iucv_fncode), "m" (*iucvpl)
		:"r0", "r1", "memory");
	return (rc);
}
#endif
