/* $ZEL: plx8311reg.h,v 1.2 2008/02/06 19:31:18 wuestner Exp $ */

/*
 * struct plx8311reg is identical to struct plx9045reg
 * thus plx9054reg is used instead regardless of the actual chip
 */
struct plx8311reg {
    u_int32_t LAS0RR;                 /*  00 */
    u_int32_t LAS0BA;                 /*  04 */
    u_int32_t MARBR;                  /*  08 */
    u_int32_t BIGEND_LMISC_PROT_AREA; /*  0c */
    /*
        u_int8_t BIGEND;              /*  0c */
        u_int8_t LMISC;               /*  0d */
        u_int8_t PROT_AREA;           /*  0e */
        u_int8_t LMISC2;              /*  0f */
    */
    u_int32_t EROMRR;                 /*  10 */
    u_int32_t EROMBA;                 /*  14 */
    u_int32_t LBRD0;                  /*  18 */
    u_int32_t DMRR;                   /*  1c */
    u_int32_t DMLBAM;                 /*  20 */
    u_int32_t DMLBAI;                 /*  24 */ /* not DMLDAI */
    u_int32_t DMPBAM;                 /*  28 */
    u_int32_t DMCFGA;                 /*  2c */
    u_int32_t OPQIS;                  /*  30 */
    u_int32_t OPQIM;                  /*  34 */
    u_int32_t _dummy1;                /*  38 */
    u_int32_t _dummy2;                /*  3c */
    u_int32_t IQP;                    /*  40 */ /* ==MBOX0 if I2O is not enabled */
    u_int32_t OQP;                    /*  44 */ /* ==MBOX1 if I2O is not enabled */
    u_int32_t MBOX2;                  /*  48 */
    u_int32_t MBOX3;                  /*  4c */
    u_int32_t MBOX4;                  /*  50 */
    u_int32_t MBOX5;                  /*  54 */
    u_int32_t MBOX6;                  /*  58 */
    u_int32_t MBOX7;                  /*  5c */
    u_int32_t P2LDBELL;               /*  60 */
    u_int32_t L2PDBELL;               /*  64 */
    u_int32_t INTCSR;                 /*  68 */
    u_int32_t CNTRL;                  /*  6c */
    u_int32_t PCIHIDR;                /*  70 */
    u_int32_t PCIHREV;                /*  74 */
    u_int32_t MBOX0;                  /*  78 */
    u_int32_t MBOX1;                  /*  7c */
    u_int32_t DMAMODE0;               /*  80 */
    u_int32_t DMAPADR0;               /*  84 */
    u_int32_t DMALADR0;               /*  88 */
    u_int32_t DMASIZ0;                /*  8c */
    u_int32_t DMADPR0;                /*  90 */
    u_int32_t DMAMODE1;               /*  94 */
    u_int32_t DMAPADR1;               /*  98 */
    u_int32_t DMALADR1;               /*  9c */
    u_int32_t DMASIZ1;                /*  a0 */
    u_int32_t DMADPR1;                /*  a4 */
    u_int32_t DMACSR0_DMACSR1;        /*  a8 */
    u_int32_t DMAARB;                 /*  ac */
    u_int32_t DMATHR;                 /*  b0 */
    u_int32_t DMADAC0;                /*  b4 */
    u_int32_t DMADAC1;                /*  b8 */
    u_int32_t _dummy3;                /*  bc */
    u_int32_t MQCR;                   /*  c0 */
    u_int32_t QBAR;                   /*  c4 */
    u_int32_t IFHPR;                  /*  c8 */
    u_int32_t IFTPR;                  /*  cc */
    u_int32_t IPHPR;                  /*  d0 */
    u_int32_t IPTPR;                  /*  d4 */
    u_int32_t OFHPR;                  /*  d8 */
    u_int32_t OFTPR;                  /*  dc */
    u_int32_t OPHPR;                  /*  e0 */
    u_int32_t OPTPR;                  /*  e4 */
    u_int32_t QSR;                    /*  e8 */
    u_int32_t _dummy4;                /*  ec */
    u_int32_t LAS1RR;                 /*  f0 */
    u_int32_t LAS1BA;                 /*  f4 */
    u_int32_t LBRD1;                  /*  f8 */
    u_int32_t DMDAC;                  /*  fc */
    u_int32_t PCIARB;                 /* 100 */
    u_int32_t PABTADR;                /* 104 */
};
