/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */

/*
 * This file is the default initialization of algortihms
 * and objects for the vtspr
 */
#include "vtspr.h"
#include "_vtspr_private.h"

#if defined(VTSP_ENABLE_GAMRNB_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
#include <dsp.h>
#endif
/*
 * Global parameters.
 */
#define _VTSPR_P0DB (4004)
GLOBAL_Params _VTSPR_globals = {
    _VTSPR_P0DB,
    _VTSPR_P0DB
};


/*
 * VTSP needs to determine the dBm level, from 0 to -36 dBm, based on RMS.
 * The following constants are calculated from the P0DB value, then used
 * to create a table, which can be used by a search method.
 */
#define _VTSPR_P1DB  ((_VTSPR_P0DB * 114) >> 7)
#define _VTSPR_P2DB  ((_VTSPR_P0DB * 102) >> 7)
#define _VTSPR_P3DB  ((_VTSPR_P0DB * 91) >> 7)
#define _VTSPR_P4DB  ((_VTSPR_P0DB * 81) >> 7)
#define _VTSPR_P5DB  ((_VTSPR_P0DB * 72) >> 7)
#define _VTSPR_P6DB  ((_VTSPR_P0DB * 64) >> 7)
#define _VTSPR_P7DB  ((_VTSPR_P0DB * 57) >> 7)
#define _VTSPR_P8DB  ((_VTSPR_P0DB * 51) >> 7)
#define _VTSPR_P9DB  ((_VTSPR_P0DB * 45) >> 7)
#define _VTSPR_P10DB ((_VTSPR_P0DB * 40) >> 7)
#define _VTSPR_P11DB ((_VTSPR_P0DB * 36) >> 7)
#define _VTSPR_P12DB ((_VTSPR_P0DB * 32) >> 7)
#define _VTSPR_P13DB ((_VTSPR_P0DB * 29) >> 7)
#define _VTSPR_P14DB ((_VTSPR_P0DB * 26) >> 7)
#define _VTSPR_P15DB ((_VTSPR_P0DB * 23) >> 7)
#define _VTSPR_P16DB ((_VTSPR_P0DB * 20) >> 7)
#define _VTSPR_P17DB ((_VTSPR_P0DB * 18) >> 7)
#define _VTSPR_P18DB ((_VTSPR_P0DB * 16) >> 7)
#define _VTSPR_P19DB ((_VTSPR_P0DB * 29) >> 8)
#define _VTSPR_P20DB ((_VTSPR_P0DB * 26) >> 8)
#define _VTSPR_P21DB ((_VTSPR_P0DB * 23) >> 8)
#define _VTSPR_P22DB ((_VTSPR_P0DB * 20) >> 8)
#define _VTSPR_P23DB ((_VTSPR_P0DB * 18) >> 8)
#define _VTSPR_P24DB ((_VTSPR_P0DB * 16) >> 8)
#define _VTSPR_P25DB ((_VTSPR_P0DB * 29) >> 9)
#define _VTSPR_P26DB ((_VTSPR_P0DB * 26) >> 9)
#define _VTSPR_P27DB ((_VTSPR_P0DB * 23) >> 9)
#define _VTSPR_P28DB ((_VTSPR_P0DB * 20) >> 9)
#define _VTSPR_P29DB ((_VTSPR_P0DB * 18) >> 9)
#define _VTSPR_P30DB ((_VTSPR_P0DB * 16) >> 9)
#define _VTSPR_P31DB ((_VTSPR_P0DB * 29) >> 10)
#define _VTSPR_P32DB ((_VTSPR_P0DB * 26) >> 10)
#define _VTSPR_P33DB ((_VTSPR_P0DB * 23) >> 10)
#define _VTSPR_P34DB ((_VTSPR_P0DB * 20) >> 10)
#define _VTSPR_P35DB ((_VTSPR_P0DB * 18) >> 10)
#define _VTSPR_P36DB ((_VTSPR_P0DB * 16) >> 10)


vint _VTSPR_dbTable[36] = {
    _VTSPR_P1DB, _VTSPR_P2DB, _VTSPR_P3DB, _VTSPR_P4DB, _VTSPR_P5DB,
    _VTSPR_P6DB, _VTSPR_P7DB, _VTSPR_P8DB, _VTSPR_P9DB, _VTSPR_P10DB,
    _VTSPR_P11DB, _VTSPR_P12DB, _VTSPR_P13DB, _VTSPR_P14DB, _VTSPR_P15DB,
    _VTSPR_P16DB, _VTSPR_P17DB, _VTSPR_P18DB, _VTSPR_P19DB, _VTSPR_P20DB,
    _VTSPR_P21DB, _VTSPR_P22DB, _VTSPR_P23DB, _VTSPR_P24DB, _VTSPR_P25DB,
    _VTSPR_P26DB, _VTSPR_P27DB, _VTSPR_P28DB, _VTSPR_P29DB, _VTSPR_P30DB,
    _VTSPR_P31DB, _VTSPR_P32DB, _VTSPR_P33DB, _VTSPR_P34DB, _VTSPR_P35DB,
    _VTSPR_P36DB
};

/*
 * TONE lookup table for tone generation
 */
int _VTSPR_toneDtmfFreq[16][2] = {
    {941, 1336}, {697, 1209}, {697, 1336},  /* 0 1 2 */
    {697, 1477}, {770, 1209}, {770, 1336},  /* 3 4 5 */
    {770, 1477}, {852, 1209}, {852, 1336},  /* 6 7 8 */
    {852, 1477}, {941, 1209}, {941, 1477},  /* 9 * # */
    {697, 1633}, {770, 1633}, {852, 1633},  /* A B C */
    {941, 1633}                             /* D     */
};

/*
 * Ring tables.
 *
 * Time is in milliseconds
 */

_VTSP_RingTemplate _VTSPR_ringTemplate0 = {
    {
        1,                              /* Cadences */
        -1,   0,                        /* On time, Off time Cad #1 */
        0,    0,                        /* On time, Off time Cad #2 */
        0,    0,                        /* On time, Off time Cad #3 */
        1                               /* Repeats */
    },
    1                                   /* which cadence to insert FSKS */

};

_VTSP_RingTemplate _VTSPR_ringTemplate1 = {
    {
        1,
        200, 400,
        0,    0,
        0,    0,
        1
    },
    1
};

_VTSP_RingTemplate _VTSPR_ringTemplate2 = {
    {
        2,
        80,  40,
        80,  400,
        0,    0,
        1
    },
    2
};

_VTSP_RingTemplate _VTSPR_ringTemplate3 = {
    {
        3,
        40,  20,
        40,  20,
        80,  400,
        1
    },
    3
};

_VTSP_RingTemplate _VTSPR_ringTemplate4 = {
    {
        3,
        30,  20,
        100, 20,
        30,  400,
        1
    },
    3
};

_VTSP_RingTemplate _VTSPR_ringTemplate5 = {
    {
        1,
        50, 550,
        0,   0,
        0,   0,
        1
    },
    1
};

_VTSP_RingTemplate _VTSPR_ringTemplate6 = {
    {
        1,
        30, 500,
        0,   0,
        0,   0,
        1
    },
    1
};

#ifndef VTSP_ENABLE_MP_LITE
/*
 * PSTN Simulated Dial Tone
 */
TONE_Params _VTSPR_toneVoipDialtone = {
    350,                            /* F1 */
    -21 * 2,                        /* pwr1 */
    440,                            /* F2 */
    -21 * 2,                        /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    1000,0,60,                      /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    1                               /* tone repeat */
};

/*
 * VoIP Busy
 */
TONE_Params _VTSPR_toneVoipBusy = {
    480,                            /* F1 */
    -21 * 2,                        /* pwr1 */
    620,                            /* F2 */
    -21 * 2,                        /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    500, 500, 1,                    /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    60                              /* tone repeat */
};

/*
 * VoIP Ringback
 */
TONE_Params _VTSPR_toneVoipRingback = {
    440,                            /* F1 */
    -18 * 2,                        /* pwr1 */
    480,                            /* F2 */
    -18 * 2,                        /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    1000, 3000, 1,                  /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    60/4                            /* tone repeat */
};

/*
 * VoIP Reorder
 */
TONE_Params _VTSPR_toneVoipReorder = {
    480,                            /* F1 */
    -18 * 2,                        /* pwr1 */
    620,                            /* F2 */
    -18 * 2,                        /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    250, 250, 1,                    /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    -1                              /* tone repeat */
};

/*
 * Howler Tone
 */
TONE_Params _VTSPR_toneHowler = {
    1800,                           /* F1 */
    0,                              /* pwr1 */
    2500,                           /* F2 */
    0,                              /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    100,100,1000,                   /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    1                               /* tone repeat */
};
/*
 * DTMF ACK for CID
 */
TONE_Params _VTSPR_toneCidAck = {
    941,                            /* F1 */
    -12,                            /* pwr1 */
    1633,                           /* F2 */
    -12,                            /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    4000, 0, 1,                     /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    1                               /* tone repeat */
};

/*
 * VoIP Call Waiting Alert (fake "SAS")
 */
TONE_Params _VTSPR_toneVoipCWAlert = {
    400,                            /* F1 */
    -15 * 2,                        /* pwr1 */
    0,                              /* F2 */
    0,                              /* pwr2 */
    TONE_MONO,                      /* type */
    1,                              /* # cads */
    150, 11500, 1,                  /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    1                               /* tone repeat */
};

/*
 * Test0 tone, -0.5db 1Khz Sine
 */
TONE_Params _VTSPR_toneTest0 = {
    1000,                           /* F1 */
    -1,                             /* pwr1 */
    0,                              /* F2 */
    0,                              /* pwr2 */
    TONE_MONO,                      /* type */
    1,                              /* # cads */
    1000, 0, 1,                     /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    60                              /* tone repeat */
};

/*
 * Test1 tone, -3db 1Khz Sine
 */
TONE_Params _VTSPR_toneTest1 = {
    1000,                           /* F1 */
    -3 * 2,                         /* pwr1 */
    0,                              /* F2 */
    0,                              /* pwr2 */
    TONE_MONO,                      /* type */
    1,                              /* # cads */
    1000, 0, 1,                     /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    60                              /* tone repeat */
};

/*
 * Test2 tone, -6db 1Khz Sine
 */
TONE_Params _VTSPR_toneTest2 = {
    1000,                           /* F1 */
    -6 * 2,                         /* pwr1 */
    0,                              /* F2 */
    0,                              /* pwr2 */
    TONE_MONO,                      /* type */
    1,                              /* # cads */
    1000, 0, 1,                     /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    60                              /* tone repeat */
};

TONE_Params _VTSPR_toneDtmf = {
    350,                            /* F1 */
    -7 * 2,                         /* pwr1 */
    440,                            /* F2 */
    -7 * 2,                         /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    4000,0,3,                       /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    1                               /* tone repeat */
};

TONE_Params _VTSPR_toneVoipReady = {
    440,                            /* F1 */
    -5 * 2,                         /* pwr1 */
    0,                              /* F2 */
    0,                              /* pwr2 */
    TONE_MONO,                      /* type */
    1,                              /* # cads */
    1000,0,60,                      /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    1                               /* tone repeat */
};
TONE_Params _VTSPR_toneVoipNotReady = {
    350,                            /* F1 */
    -7 * 2,                         /* pwr1 */
    440,                            /* F2 */
    -7 * 2,                         /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    750, 250, 1,                    /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    1                               /* tone repeat */
};
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
/*
 * _VTSPR_toneQuad1 and _VTSPR_toneQuad2 are defined as IIT tone
 * which is specified in NTT spec.
 */
/*
 * IIT tone sequence 1/2
 */
GENF_Params _VTSPR_toneQuad1 = {
        GENF_MOD,  /* ctrlWord, set to quad tone */
        {
            {
                400,        /* carrier */
                16,         /* signal  */
                -25,        /* power   */
                85          /* mIndex  */
            }
        },
        1,          /* numCads, generate one on/off pair */
        500,         /* tMake1, set make time to 500 ms */
        500,         /* tBreak1, set break time to 500 ms */
        1,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        1,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};
/*
 * IIT tone sequence 2/2
 */
GENF_Params _VTSPR_toneQuad2 = {
        GENF_MONO,  /* ctrlWord, set to quad tone */
        {
            {
                400,        /* quad.tone1, set tone1 frequency */
                0,          /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
               -25,         /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        2,          /* numCads, generate one on/off pair */
        50,         /* tMake1, set make time to 80 ms */
        450,         /* tBreak1, set break time to 80 ms */
        2,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        1000,       /* 3000 tBreak2 */
        1,          /* 1 tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* 0 decay, no decay required */
        80          /* blockSize */
};
GENF_Params _VTSPR_toneQuad3 = {
        GENF_QUAD,  /* ctrlWord, set to quad tone */
        {
            {
                697,        /* quad.tone1, set tone1 frequency */
                1209,       /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
                0,          /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        1,          /* numCads, generate one on/off pair */
        80,         /* tMake1, set make time to 80 ms */
        80,         /* tBreak1, set break time to 80 ms */
        1,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        1,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};
GENF_Params _VTSPR_toneQuad4 = {
        GENF_QUAD,  /* ctrlWord, set to quad tone */
        {
            {
                697,        /* quad.tone1, set tone1 frequency */
                1209,       /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
                0,          /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        1,          /* numCads, generate one on/off pair */
        80,         /* tMake1, set make time to 80 ms */
        80,         /* tBreak1, set break time to 80 ms */
        1,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        1,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};
GENF_Params _VTSPR_toneQuad5 = {
        GENF_QUAD,  /* ctrlWord, set to quad tone */
        {
            {
                697,        /* quad.tone1, set tone1 frequency */
                1209,       /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
                0,          /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        1,          /* numCads, generate one on/off pair */
        80,         /* tMake1, set make time to 80 ms */
        80,         /* tBreak1, set break time to 80 ms */
        1,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        1,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};
GENF_Params _VTSPR_toneQuad6 = {
        GENF_QUAD,  /* ctrlWord, set to quad tone */
        {
            {
                697,        /* quad.tone1, set tone1 frequency */
                1209,       /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
                0,          /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        1,          /* numCads, generate one on/off pair */
        80,         /* tMake1, set make time to 80 ms */
        80,         /* tBreak1, set break time to 80 ms */
        1,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        1,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};
GENF_Params _VTSPR_toneQuad7 = {
        GENF_QUAD,  /* ctrlWord, set to quad tone */
        {
            {
                697,        /* quad.tone1, set tone1 frequency */
                1209,       /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
                0,          /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        1,          /* numCads, generate one on/off pair */
        80,         /* tMake1, set make time to 80 ms */
        80,         /* tBreak1, set break time to 80 ms */
        1,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        1,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};
GENF_Params _VTSPR_toneQuad8 = {
        GENF_QUAD,  /* ctrlWord, set to quad tone */
        {
            {
                697,        /* quad.tone1, set tone1 frequency */
                1209,       /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
                0,          /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        1,          /* numCads, generate one on/off pair */
        80,         /* tMake1, set make time to 80 ms */
        80,         /* tBreak1, set break time to 80 ms */
        1,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        1,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};
GENF_Params _VTSPR_toneQuad9 = {
        GENF_QUAD,  /* ctrlWord, set to quad tone */
        {
            {
                697,        /* quad.tone1, set tone1 frequency */
                1209,       /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
                0,          /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        1,          /* numCads, generate one on/off pair */
        80,         /* tMake1, set make time to 80 ms */
        80,         /* tBreak1, set break time to 80 ms */
        1,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        1,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        1,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};

#endif

#if defined(VTSP_ENABLE_DTMFR) && !defined(VTSP_ENABLE_MP_LITE)
/*
 * VoIP DTMF Relay Tone
 */
TONE_Params _VTSPR_drDecodeTone = {
    0,                              /* F1 */
    0,                              /* pwr1 */
    0,                              /* F2 */
    0,                              /* pwr2 */
    TONE_DUAL,                      /* type */
    1,                              /* # cads */
    1000, 0, 10,  /* 10 sec max. */ /* mak1, brk1, repeat1 */
    0, 0, 0,                        /* mak2, brk2, repeat2 */
    0, 0, 0,                        /* mak3, brk3, repeat3 */
    1                               /* tone repeat */
};

#ifdef VTSP_ENABLE_TONE_QUAD
GENF_Params _VTSPR_drDecodeToneQuad = {
        GENF_QUAD,  /* ctrlWord, set to quad tone */
        {
            {
                0,        /* quad.tone1, set tone1 frequency */
                0,       /* quad.tone2, set tone2 frequency */
                0,          /* quad.tone3, set tone3 frequency */
                0,          /* quad.tone4, set tone4 frequency */
                0,          /* quad.t1Pw, set tone1 power to 0 dBm */
                0,          /* quad.t2Pw, set tone2 power to 0 dBm */
                0,          /* quad.t3Pw, set tone3 power to 0 dBm */
                0           /* quad.t4Pw, set tone4 power to 0 dBm */
            }
        },
        1,          /* numCads, generate one on/off pair */
        1000,         /* tMake1, set make time to 80 ms */
        0,         /* tBreak1, set break time to 80 ms */
        10,          /* tRepeat1, do the first cadence once */
        0,          /* tMake2 */
        0,          /* tBreak2 */
        0,          /* tRepeat2 */
        0,          /* tMake3 */
        0,          /* tBreak3 */
        0,          /* tRepeat3 */
        1,          /* sRepeat, do the cadence sequence once */
        0,          /* deltaF, no change in frequencies required */
        0,          /* decay, no decay required */
        80          /* blockSize */
};
#endif
#endif

#ifdef VTSP_ENABLE_CIDR
/*
 * FSKR local params
 */
FSKR_Params _VTSPR_fskrParams = {
    FSKR_MINSEZ_DEF,
    FSKR_MINMRK_DEF,
    FSKR_MAXSMRK_DEF,
    VTSPR_NSAMPLES_10MS_8K >> 1
};
#endif
#ifdef VTSP_ENABLE_NFE
/*
 * NFE local params
 */
NFE_Local _VTSPR_nfeParams = {
    -60,                           /* min noise floor, dBm */
    -35,                           /* max noise floor, dBm */
    VTSPR_NSAMPLES_10MS_8K         /* # samples to process */
};
#endif
#ifdef VTSP_ENABLE_ECSR
/*
 * ECSR local params
 */
EC_Params _VTSPR_ecsrParams = {
    VTSPR_NSAMPLES_10MS_8K,         /* # samples to process */
    VTSPR_EC_TAILLENGTH,            /* tail length */
    0                               /* isr # samples to process */
};

/*
 * NLP local params
 */
NLP_Params _VTSPR_nlpParams = {
    VTSPR_NSAMPLES_10MS_8K,    /* # samples to process */
    0,                         /* isr # samples to process */
    -60,                       /* Min voice power to consider */
    NLP_VOICE_HNG_CNT_DEF      /* Voice hangover time */
};
#endif /* VTSP_ENABLE_ECSR */

#ifndef VTSP_ENABLE_MP_LITE
/*
 * DCRM for Near end params.
 * This default changes at run time
 */
DCRM_Params _VTSPR_dcrmNearParams = {
   VTSPR_NSAMPLES_10MS_MAX          /* # samples to process */
};
#endif

#ifdef VTSP_ENABLE_AEC
/*
 * AEC parameters
 * !!! Important Note !!!
 * The parameter tNEAR_END_DELAY, was measured at SVN revision 2489,
 * using _MEASURE_IMPULSE in _vtspr_audio.c, and _VTSP_RTPLOG in vtspr_task.c.
 * Delay between rout and sin is 2 ms.
 */

/*
 * These parameters are used to configure AEC for cancelling echo
 * in the handset mode.  This mode will generally have less echo and will
 * use a higher sidetone gain.  Also the AGC might be more subtle.
 */
#ifndef VTSP_ENABLE_AEC_HALF_DUPLEX
/* Full duplex generic settings */
AEC_Params _VTSPR_aecHandsetParams = {
    AEC_SAMPLE_RATE_16K,
    8,
    42,
    AEC_pNEAR_ERL_DEF,
    18000,
    0,
    0,
    -6,
    AEC_pSIDETONE_GAIN_DISABLE,
    -45,
    0,
    5,
    -40,
    0,
    7,
};
#else
/* Half duplex settings */
AEC_Params _VTSPR_aecHandsetParams = {
    AEC_SAMPLE_RATE_16K,
    8,
    0,
    AEC_pNEAR_ERL_DEF,
    18000,
    0,
    0,
    0,
    AEC_pSIDETONE_GAIN_DISABLE,
    -45,
    0,
    10,
    -40,
    0,
    10,
};
#endif
/*
 * These parameters are used to configure AEC for cancelling echo
 * in the hands free (speaker phone) mode.  This mode will generally
 * a large echo and will disable the sidetone gain.
 */
#ifndef VTSP_ENABLE_AEC_HALF_DUPLEX
AEC_Params _VTSPR_aecHandsFreeParams = {
    AEC_SAMPLE_RATE_16K,
    8,
    48,
    AEC_pNEAR_ERL_DEF,
    20000,
    AEC_pROUT_AGC_MAX_DBL_DEF,
    AEC_pSOUT_AGC_MAX_DBL_DEF,
    -35,
    AEC_pSIDETONE_GAIN_DISABLE,
    -45,
    0,
    5,
    -40,
    0,
    7,
};
#else
/* Half duplex settings */
AEC_Params _VTSPR_aecHandsFreeParams = {
    AEC_SAMPLE_RATE_16K,
    8,
    0,
    AEC_pNEAR_ERL_DEF,
    20000,
    AEC_pROUT_AGC_MAX_DBL_DEF,
    AEC_pSOUT_AGC_MAX_DBL_DEF,
    -60,
    AEC_pSIDETONE_GAIN_DISABLE,
    -45,
    0,
    5,
    -40,
    0,
    7,
};
#endif
#else
#ifndef VTSP_ENABLE_MP_LITE
/*
 * BND Near parameters
 */
BND_Params _VTSPR_bndParams = {
    VTSPR_NSAMPLES_AUDIO,
    BND_pNOISE_THRESH_DEF,
    0,
    BND_tHOLDOVER_DEF
};
#endif
#endif /* VTSP_ENABLE_AEC */


#ifndef VTSP_ENABLE_MP_LITE
/*
 * DCRM Peer params.
 * This default changes at run time
 */
DCRM_Params _VTSPR_dcrmPeerParams = {
   VTSPR_NSAMPLES_10MS_MAX     /* # samples to process */
};
#endif

#ifdef VTSP_ENABLE_DTMF
/*
 * DTMF local params
 */
DTMF_Params _VTSPR_dtmfParams = {
    DTMF_pDTMFPWR_DEF,         /* Default -45 dBm per frequency */
    5,  /* Faster detect to remove double-detect on far side; bug823 */
    DTMF_tDTMFSIL_DEF,         /* Default 3 blocks (24 ms) min off dur*/
    DTMF_pDTMFHITWIST_DEF,     /* Def 2.5/3.5 accept/reject freq dev */
    DTMF_pDTMFLOTWIST_DEF,     /* Default 8 dB maximum low twist */
    DTMF_fDTMFFDEV_DEF,        /* Default 8 dB msximum high twist */
    DTMF_tDTMFEFRAMES_DEF      /* Default good frames */
};
#endif

#ifndef VTSP_ENABLE_MP_LITE
/*
 * NSE local params
 * Default Blocksize changes at run time
 */
NSE_Params _VTSPR_nseParams = {
    0,                         /* seed: auto-select */
    VTSPR_NSAMPLES_AUDIO    /* # samples to process */
};
#endif

#ifdef VTSP_ENABLE_FMTD
/*
 * FMTD local parameters.
 */
FMTD_Params _VTSPR_fmtdParams = {
    FMTD_MINMAK_1100,          /* FAX calling tone parameters */
    FMTD_MAXMAK_1100,
    FMTD_MINBRK_1100,
    FMTD_MAXBRK_1100,
    FMTD_MINMAK_1300,          /* MODEM calling tone parameters */
    FMTD_MAXMAK_1300,
    FMTD_MINBRK_1300,
    FMTD_MAXBRK_1300,
    FMTD_MINMAK_2025,          /* minimum duration for 2025 Hz tone */
    FMTD_MINMAK_2225,          /* minimum duration for 2225 Hz tone */
    FMTD_MINMAK_PHASE,         /* phase change parameters */
    FMTD_MAXMAK_PHASE,
    VTSPR_NSAMPLES_10MS_8K     /* number of samples per user data block */
};
#endif

#ifdef VTSP_ENABLE_T38
FR38V3_GLOBAL_Params _VTSPR_fr38GlobalParams = {
    _VTSPR_FR38V3_P0DB,
    _VTSPR_FR38V3_P0DB,
    _VTSPR_FR38V3_CED_LENGTH
};

FR38_Params _VTSPR_fr38Params = {
    VTSPR_NSAMPLES_10MS_8K,     /* number of samples per user data block */
    MAX_RATE_14400,             /* Max rate */
    200,                        /* FR38_DEF_JITTER */  /* ms */
    CONNECT_TYPE_UDP,           /* Use UDP packets */
    ECC_TYPE_REDUNDANCY,        /* Redundancy */
    3,                          /* Redundancy on v.21 packets */
    1,                          /* Redundancy on image packets */
    RATE_MGT_TYPE_2,            /* Passing TCF is mandatory in UDP mode */
    T38_VERSION_0,              /* Support T38 Version 3 */
    FR38_FALSE,                 /* NSF disabled */
    FR38_TRUE,                  /* Allow ECM calls */
    FR38_FALSE,                 /* V34 Disabled */
    T38_OP_UNKNOWN              /* Set to 3 seconds */
};
#endif

#ifdef VTSP_ENABLE_UTD
/*
 * Define US Call Progress Tones
 */
UTD_Tonedef _VTSP_utdToneDialUS = {
    VTSP_TEMPL_UTD_TONE_TYPE_DIAL,
    VTSP_TEMPL_UTD_TONE_DUAL,
    8,    /* return value: modified at runtime */
    1,                      /* num_cads */
    350,                    /* frequency1 */
    11,                     /* bandwidth1 */
    440,                    /* frequency2 */
    11,                     /* bandwidth2 */
    500,                    /* min_make1 */
    0,                      /* max_make1 */
    0,                      /* min_break1 */
    0,                      /* max_break1 */
    0,                      /* min_make2 */
    0,                      /* max_make2 */
    0,                      /* min_break2 */
    0,                      /* max_break2 */
    0,                      /* min_make3 */
    0,                      /* max_make3 */
    0,                      /* min_break3 */
    0,                      /* max_break3 */
    -39                     /* power */
};
UTD_Tonedef _VTSP_utdToneRingbackUS = {
    VTSP_TEMPL_UTD_TONE_TYPE_RING,
    VTSP_TEMPL_UTD_TONE_DUAL,
    3,    /* return value: modified at runtime */
    0,                      /* num_cads */
    440,                    /* frequency1 */
    11,                     /* bandwidth1 */
    480,                    /* frequency2 */
    12,                     /* bandwidth2 */
    1800,                   /* min_make1 */
    2200,                   /* max_make1 */
    3600,                   /* min_break1 */
    4400,                   /* max_break1 */
    0,                      /* min_make2 */
    0,                      /* max_make2 */
    0,                      /* min_break2 */
    0,                      /* max_break2 */
    0,                      /* min_make3 */
    0,                      /* max_make3 */
    0,                      /* min_break3 */
    0,                      /* max_break3 */
    -39                     /* power */
};
UTD_Tonedef _VTSP_utdToneBusyUS = {
    VTSP_TEMPL_UTD_TONE_TYPE_BUSY,
    VTSP_TEMPL_UTD_TONE_DUAL,
    4,    /* return value: modified at runtime */
    1,                      /* num_cads */
    480,                    /* frequency1 */
    12,                     /* bandwidth1 */
    620,                    /* frequency2 */
    16,                     /* bandwidth2 */
    450,                    /* min_make1 */
    550,                    /* max_make1 */
    450,                    /* min_break1 */
    550,                    /* max_break1 */
    0,                      /* min_make2 */
    0,                      /* max_make2 */
    0,                      /* min_break2 */
    0,                      /* max_break2 */
    0,                      /* min_make3 */
    0,                      /* max_make3 */
    0,                      /* min_break3 */
    0,                      /* max_break3 */
    -39                     /* power */
};
UTD_Tonedef _VTSP_utdToneReorderUS = {
    VTSP_TEMPL_UTD_TONE_TYPE_REOR,
    VTSP_TEMPL_UTD_TONE_DUAL,
    5,    /* return value: modified at runtime */
    2,                      /* num_cads */
    480,                    /* frequency1 */
    12,                     /* bandwidth1 */
    620,                    /* frequency2 */
    16,                     /* bandwidth2 */
    225,                    /* min_make1 */
    275,                    /* max_make1 */
    225,                    /* min_break1 */
    275,                    /* max_break1 */
    0,                      /* min_make2 */
    0,                      /* max_make2 */
    0,                      /* min_break2 */
    0,                      /* max_break2 */
    0,                      /* min_make3 */
    0,                      /* max_make3 */
    0,                      /* min_break3 */
    0,                      /* max_break3 */
    -39                     /* power */
};
UTD_SITTonedef _VTSP_utdToneSIT1US = {
    VTSP_TEMPL_UTD_TONE_TYPE_SIT,
    VTSP_TEMPL_UTD_SIT_US,
    7,    /* return value: modified at runtime */
    914,                    /* tone1_lowfreq */
    23,                     /* tone1_lowfreq_bw */
    986,                    /* tone1_highfreq */
    25,                     /* tone1_highfreq_bw */
    1371,                   /* tone2_loqfreq */
    28,                     /* tone2_lowfreq_bw */
    1429,                   /* tone2_highfreq */
    28,                     /* tone2_highfreq_bw */
    1777,                   /* tone3_freq */
    45,                     /* tone3_bw */
    260,                    /* short_min_duration */
    288,                    /* short_max_duration */
    361,                    /* long_min_duration */
    399,                    /* long_max_duration */
    -39                     /* power */
};
#endif

/*
 * Jitter buffer local parameters.
 */
JB_Params _VTSPR_jbDefParams = {
    JB_LEAKRATE_DEF,                /* Leak rate */
    JB_ACCMRATE_DEF,                /* Accumulate rate */
    0,                              /* initLevel (new) */
    _VTSPR_JB_MAXLEVEL,             /* Maximum level in samples */
    JB_DECAY_RATE_DEF,              /* Time constant for shrinking the size */
    JB_DTRLY_ENABLE,                /* DTMF relay state */
    JB_ADAPT_RUN                    /* Normal mode, not frozen */
};


#ifdef VTSP_ENABLE_CIDS
CIDS_Params VTSPR_CIDSendParamJP = {
    NULL,                              /* fsksObj_ptr */
    NULL,                              /* toneObj_ptr */
    NULL,                              /* msg_ptr */
    0,                                 /* msgSz */
    CIDS_DATA_FSK            ,         /* dataType */
    CIDS_LOCALS_BATTREV_JP   ,         /*  battRev;                */
    CIDS_LOCALS_STARTTIME_JP ,         /*  startTime;  10ms timing */
    CIDS_LOCALS_SIGNALTIME_JP,         /*  signalTime; 10ms timing */
    CIDS_LOCALS_PRESEZTIME_JP,         /*  preSezTime; 10ms timing */
    CIDS_LOCALS_SEZLEN_JP    ,         /*  sezLen;     FSKS params */
    CIDS_LOCALS_MARKLEN_JP   ,         /*  markLen;    FSKS params */
    CIDS_LOCALS_ENDTIME_JP   ,         /*  endTime;    10ms timing */
    CIDS_LOCALS_OFFTIMEOUT_JP,         /*  offTimeout; 10ms timing */
    CIDS_LOCALS_ONTIMEOUT_JP ,         /*  onTimeout;  10ms timing */
    CIDS_LOCALS_ONSILTIME_JP ,         /*  onSilTime;  10ms timing */
    CIDS_LOCALS_SPACEFREQ_JP ,         /*  spaceFreq;  FSKS params */
    CIDS_LOCALS_MARKFREQ_JP  ,         /*  markFreq;   FSKS params */
    CIDS_LOCALS_MARKPWR_JP   ,         /*  spacePwr;   FSKS params */
    CIDS_LOCALS_SPACEPWR_JP  ,         /*  markPwr;    FSKS params */
    CIDS_LOCALS_TONE1FREQ_JP ,         /*  tone1Freq;  FSKS params */
    CIDS_LOCALS_TONE2FREQ_JP ,         /*  tone2Freq;  FSKS params */
    CIDS_LOCALS_TONE1PWR_JP  ,         /*  tone1Pwr;   FSKS params */
    CIDS_LOCALS_TONE2PWR_JP  ,         /*  tone2Pwr;   FSKS params */
    CIDS_LOCALS_CTYPE_JP     ,         /*  cType;      CID  param  */
    CIDS_LOCALS_SIGNALTYPE_JP,         /*  signalType; New  JCID   */
    CIDS_LOCALS_RINGTIME_JP
};

CIDS_Params VTSPR_CIDSendParamUS = {
    NULL,                              /* fsksObj_ptr */
    NULL,                              /* toneObj_ptr */
    NULL,                              /* msg_ptr */
    0,                                 /* msgSz */
    CIDS_DATA_FSK,                     /* dataType */
    CIDS_LOCALS_BATTREV_DEF,
    CIDS_LOCALS_STARTTIME_DEF,
    CIDS_LOCALS_SIGNALTIME_DEF,
    CIDS_LOCALS_PRESEZTIME_DEF,
    CIDS_LOCALS_SEZLEN_DEF,
    CIDS_LOCALS_MARKLEN_DEF,
    CIDS_LOCALS_ENDTIME_DEF,
    CIDS_LOCALS_OFFTIMEOUT_DEF,
    CIDS_LOCALS_ONTIMEOUT_DEF,
    CIDS_LOCALS_ONSILTIME_DEF,
    CIDS_LOCALS_SPACEFREQ_DEF,
    CIDS_LOCALS_MARKFREQ_DEF,
    CIDS_LOCALS_MARKPWR_DEF,
    CIDS_LOCALS_SPACEPWR_DEF,
    CIDS_LOCALS_TONE1FREQ_DEF,
    CIDS_LOCALS_TONE2FREQ_DEF,
    CIDS_LOCALS_TONE1PWR_DEF,
    CIDS_LOCALS_TONE2PWR_DEF,
    CIDS_LOCALS_CTYPE_DEF,
    CIDS_LOCALS_SIGNALTYPE_DEF,
    CIDS_LOCALS_RINGTIME_DEF
};

CIDS_Params VTSPR_CIDSendParamUKFSK = {
    NULL,           /* fsksObj_ptr */
    NULL,           /* toneObj_ptr */
    NULL,           /* msg_ptr */
    0,              /* msgSz */
    CIDS_DATA_FSK,  /* dataType */
    CIDS_LOCALS_BATTREV_UK,
    CIDS_LOCALS_STARTTIME_UK,
    CIDS_LOCALS_SIGNALTIME_UK,
    CIDS_LOCALS_PRESEZTIME_UK,
    CIDS_LOCALS_SEZLEN_UK,
    CIDS_LOCALS_MARKLEN_UK,
    CIDS_LOCALS_ENDTIME_UK,
    CIDS_LOCALS_OFFTIMEOUT_UK,
    CIDS_LOCALS_ONTIMEOUT_UK,
    CIDS_LOCALS_ONSILTIME_UK,
    CIDS_LOCALS_SPACEFREQ_UK,
    CIDS_LOCALS_MARKFREQ_UK,
    CIDS_LOCALS_MARKPWR_UK,
    CIDS_LOCALS_SPACEPWR_UK,
    CIDS_LOCALS_TONE1FREQ_UK,
    CIDS_LOCALS_TONE2FREQ_UK,
    CIDS_LOCALS_TONE1PWR_UK,
    CIDS_LOCALS_TONE2PWR_UK,
    CIDS_LOCALS_CTYPE_UK,
    CIDS_LOCALS_SIGNALTYPE_UK,
    CIDS_LOCALS_RINGTIME_UK
};

CIDS_Params VTSPR_CIDSendParamUKDTMF = {
    NULL,           /* fsksObj_ptr */
    NULL,           /* toneObj_ptr */
    NULL,           /* msg_ptr */
    0,              /* msgSz */
    CIDS_DATA_DTMF, /* dataType */
    CIDS_LOCALS_BATTREV_UK,
    CIDS_LOCALS_STARTTIME_UK,
    CIDS_LOCALS_SIGNALTIME_UK,
    CIDS_LOCALS_PRESEZTIME_UK,
    CIDS_LOCALS_SEZLEN_UK,
    CIDS_LOCALS_MARKLEN_UK,
    CIDS_LOCALS_ENDTIME_UK_DTMF,
    CIDS_LOCALS_OFFTIMEOUT_UK,
    CIDS_LOCALS_ONTIMEOUT_UK,
    CIDS_LOCALS_ONSILTIME_UK,
    CIDS_LOCALS_SPACEFREQ_UK,
    CIDS_LOCALS_MARKFREQ_UK,
    CIDS_LOCALS_MARKPWR_UK,
    CIDS_LOCALS_SPACEPWR_UK,
    CIDS_LOCALS_TONE1FREQ_UK,
    CIDS_LOCALS_TONE2FREQ_UK,
    CIDS_LOCALS_TONE1PWR_UK,
    CIDS_LOCALS_TONE2PWR_UK,
    CIDS_LOCALS_CTYPE_UK,
    CIDS_LOCALS_SIGNALTYPE_UK_DTMF,
    CIDS_LOCALS_RINGTIME_UK
};

CIDCWS_Params VTSPR_CIDCWSendParam = {
    NULL,           /* fsksObj_ptr */
    NULL,           /* toneObj_ptr */
#ifdef VTSP_ENABLE_TONE_QUAD
    NULL,           /* genfObj_ptr */
#endif
    NULL,           /* msg_ptr */
    0,              /* msgSz */
    CIDCWS_LOCALS_STARTTIME_BC,
    CIDCWS_LOCALS_SASTIME_BC,
    CIDCWS_LOCALS_INTRTIME_BC,
    CIDCWS_LOCALS_CASTIME_BC,
    CIDCWS_LOCALS_ACKTIMEOUT_BC,
    CIDCWS_LOCALS_PREXMITTIME_BC,
    CIDCWS_LOCALS_SEZLEN_BC,
    CIDCWS_LOCALS_MARKLEN_BC,
    CIDCWS_LOCALS_ENDTIME_BC,
    CIDCWS_LOCALS_SPACEFREQ_BC,
    CIDCWS_LOCALS_MARKFREQ_BC,
    CIDCWS_LOCALS_SASFREQ_BC,
    CIDCWS_LOCALS_SPACEPWR_BC,
    CIDCWS_LOCALS_MARKPWR_BC,
    CIDCWS_LOCALS_CASPWR_BC,
    CIDCWS_LOCALS_SASPWR_BC,
    CIDCWS_LOCALS_ACKTYPE_BC,
    CIDCWS_LOCALS_CTYPE_BC,
    CIDCWS_LOCALS_SIGNALTYPE_BC,
    0
};

CIDCWS_Params VTSPR_CIDCWSendParamJP = {
    NULL,           /* fsksObj_ptr */
    NULL,           /* toneObj_ptr */
#ifdef VTSP_ENABLE_TONE_QUAD
    NULL,           /* genfObj_ptr */
#endif
    NULL,           /* msg_ptr */
    0,              /* msgSz */
    CIDCWS_LOCALS_STARTTIME_NTT,
    CIDCWS_LOCALS_SASTIME_NTT,
    CIDCWS_LOCALS_INTRTIME_NTT,
    CIDCWS_LOCALS_CASTIME_NTT,
    CIDCWS_LOCALS_ACKTIMEOUT_NTT,
    CIDCWS_LOCALS_PREXMITTIME_NTT,
    CIDCWS_LOCALS_SEZLEN_NTT,
    CIDCWS_LOCALS_MARKLEN_NTT,
    CIDCWS_LOCALS_ENDTIME_NTT,
    CIDCWS_LOCALS_SPACEFREQ_NTT,
    CIDCWS_LOCALS_MARKFREQ_NTT,
    CIDCWS_LOCALS_SASFREQ_NTT,
    CIDCWS_LOCALS_SPACEPWR_NTT,
    CIDCWS_LOCALS_MARKPWR_NTT,
    CIDCWS_LOCALS_CASPWR_NTT,
    CIDCWS_LOCALS_SASPWR_NTT,
    CIDCWS_LOCALS_ACKTYPE_NTT,
    CIDCWS_LOCALS_CTYPE_NTT,
    CIDCWS_LOCALS_SIGNALTYPE_NTT,
    0
};
#endif

/*
 * ======== _VTSPR_defaults() ========
 *
 * Set default parameters for DSP object.
 */
void _VTSPR_defaults(
    VTSPR_DSP *dsp_ptr)
{
    VTSPR_ChanObj    *chan_ptr;
    VTSPR_StreamObj  *stream_ptr;
#if _VTSP_INFC_FXS_NUM > 0
    VTSPR_FxsInfcObj *fxs_ptr;
#endif
#ifdef VTSP_ENABLE_CIDS
    _VTSPR_CidsObj   *cids_ptr;
#endif
#ifdef VTSP_ENABLE_ECSR
    _VTSPR_EcObj     *ec_ptr;
    EC_Obj           *ecsr_ptr;
    NLP_Obj          *nlp_ptr;
#endif /* VTSP_ENABLE_ECSR */
#ifdef VTSP_ENABLE_AEC
    _VTSPR_AecObj    *aec_ptr;
    AEC_Obj          *aecObj_ptr;
    vint              chan;
#else /* VTSP_ENABLE_AEC */
#ifndef VTSP_ENABLE_MP_LITE
    BND_Obj          *bndObj_ptr;
#endif
#endif /* !VTSP_ENABLE_AEC */
#ifdef VTSP_ENABLE_UTD
    _VTSPR_UtdObj    *utd_ptr;
#endif /* VTSP_ENABLE_UTD */
#ifdef VTSP_ENABLE_DTMF
    _VTSPR_DtmfObj   *dtmf_ptr;
#endif
#ifdef VTSP_ENABLE_FMTD
    _VTSPR_FmtdObj   *fmtd_ptr;
    FMTD_Obj         *fmtdObj_ptr;
#endif /* VTSP_ENABLE_FMTD */
#ifdef VTSP_ENABLE_NFE
    NFE_Object       *nfe_ptr;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    DCRM_Obj         *dcrm_ptr;
#endif
    vint              streamId;      /* must be signed */
    vint              infc;          /* must be signed */
    void             *mem_ptr;
    vint              tId;
#ifdef VTSP_ENABLE_T38
    _VTSPR_FR38Obj   *fr38_ptr;
    FR38_Config       fr38Config;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    VTSPR_ToneSeq    *toneSeq_ptr;
#endif
    TIC_Obj          *tic_ptr;
    TIC_Status        nSamples10ms;
#if defined(VTSP_ENABLE_STREAM_16K) || defined(VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
    vint              lowRate;
    vint              highRate;
#endif
#endif
    /*
     * Set local/global params in object
     */
    dsp_ptr->globals_ptr = &_VTSPR_globals;

    dsp_ptr->curActiveInfcs = 0;
    dsp_ptr->curActiveStreams = 0;

    /*
     * Default minimum detection power set to -33 dB
     * Bug 2073
     */
    dsp_ptr->fmtdGlobalPowerMinInfc = VTSPR_FMTD_POWER_MIN_DETECT_DEF;
    dsp_ptr->fmtdGlobalPowerMinPeer = VTSPR_FMTD_POWER_MIN_DETECT_DEF;

    /*
     * Point to table of dB levels for DTMF relay and other potential uses
     */
    dsp_ptr->dbTable_ptr = _VTSPR_dbTable;

    /*
     * Ring Templates
     */
    dsp_ptr->ringTemplate_ptr[0] = &_VTSPR_ringTemplate0;
    dsp_ptr->ringTemplate_ptr[1] = &_VTSPR_ringTemplate1;
    dsp_ptr->ringTemplate_ptr[2] = &_VTSPR_ringTemplate2;
    dsp_ptr->ringTemplate_ptr[3] = &_VTSPR_ringTemplate3;
    dsp_ptr->ringTemplate_ptr[4] = &_VTSPR_ringTemplate4;
    dsp_ptr->ringTemplate_ptr[5] = &_VTSPR_ringTemplate5;
    dsp_ptr->ringTemplate_ptr[6] = &_VTSPR_ringTemplate6;
    for (tId = 7; tId < _VTSP_NUM_RING_TEMPL; tId++) {
        mem_ptr = OSAL_memCalloc(1, sizeof(_VTSP_RingTemplate), 0);
        if (NULL == mem_ptr) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
        else {
            dsp_ptr->ringTemplate_ptr[tId] = (_VTSP_RingTemplate *)mem_ptr;
        }
    }

#ifndef VTSP_ENABLE_MP_LITE
    /*
     * Tone Templates
     */
    dsp_ptr->toneParams_ptr[0]  = NULL;      /*  undef, "silence tone" */
    dsp_ptr->toneParams_ptr[1]  = &_VTSPR_toneVoipDialtone;
    dsp_ptr->toneParams_ptr[2]  = &_VTSPR_toneVoipBusy;
    dsp_ptr->toneParams_ptr[3]  = &_VTSPR_toneVoipReorder;
    dsp_ptr->toneParams_ptr[4]  = &_VTSPR_toneHowler;
    dsp_ptr->toneParams_ptr[5]  = &_VTSPR_toneVoipRingback;
    dsp_ptr->toneParams_ptr[6]  = &_VTSPR_toneVoipCWAlert;
    dsp_ptr->toneParams_ptr[7]  = &_VTSPR_toneTest0;
    dsp_ptr->toneParams_ptr[8]  = &_VTSPR_toneTest1;
    dsp_ptr->toneParams_ptr[9]  = &_VTSPR_toneTest2;

    dsp_ptr->toneParams_ptr[10] = &_VTSPR_toneVoipReady;
    dsp_ptr->toneParams_ptr[11] = &_VTSPR_toneVoipNotReady;
    for (tId = 12; tId < VTSPR_TONE_LIST_SZ; tId++) {
        mem_ptr = OSAL_memCalloc(1, sizeof(TONE_Params), 0);
        if (NULL == mem_ptr) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
        else {
            dsp_ptr->toneParams_ptr[tId] = (TONE_Params *)mem_ptr;
        }
    }
    /* VTSPR_TONE_ID_CIDACK is for CIDR DTMF ack generation */
    dsp_ptr->toneParams_ptr[VTSPR_TONE_ID_CIDACK] = &_VTSPR_toneCidAck;
#endif
    /*
     * Tone Quad Templates
     */
#ifdef VTSP_ENABLE_TONE_QUAD
    dsp_ptr->toneQuadParams_ptr[0]  = NULL;      /*  undef, "silence tone" */
    dsp_ptr->toneQuadParams_ptr[1]  = &_VTSPR_toneQuad1;
    dsp_ptr->toneQuadParams_ptr[2]  = &_VTSPR_toneQuad2;
    dsp_ptr->toneQuadParams_ptr[3]  = &_VTSPR_toneQuad3;
    dsp_ptr->toneQuadParams_ptr[4]  = &_VTSPR_toneQuad4;
    dsp_ptr->toneQuadParams_ptr[5]  = &_VTSPR_toneQuad5;
    dsp_ptr->toneQuadParams_ptr[6]  = &_VTSPR_toneQuad6;
    dsp_ptr->toneQuadParams_ptr[7]  = &_VTSPR_toneQuad7;
    dsp_ptr->toneQuadParams_ptr[8]  = &_VTSPR_toneQuad8;
    dsp_ptr->toneQuadParams_ptr[9]  = &_VTSPR_toneQuad9;
    for (tId = 10; tId < _VTSP_NUM_TONE_TEMPL; tId++) {
        mem_ptr = OSAL_memCalloc(1, sizeof(GENF_Params), 0);
        if (NULL == mem_ptr) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
        else {
            dsp_ptr->toneQuadParams_ptr[tId] = (GENF_Params *)mem_ptr;
            /* Set blocksize of template; not overwritten later */
            dsp_ptr->toneQuadParams_ptr[tId]->block = VTSPR_NSAMPLES_10MS_8K;
        }
    }
#endif

#ifndef VTSP_ENABLE_MP_LITE
    /* dtmfToneParams is for DTMF tone generation (not detection) */
    dsp_ptr->dtmfToneParams_ptr = &_VTSPR_toneDtmf;
#endif
    /*
     * Others
     */
    /* Initialize Fax Silence DB to default */
    dsp_ptr->faxSilRxDB     = VTSPR_FAX_SIL_DB;
    dsp_ptr->faxSilTxDB     = VTSPR_FAX_SIL_DB;
#ifdef VTSP_ENABLE_NFE
    dsp_ptr->nfeParams_ptr  = &_VTSPR_nfeParams;
#endif
#ifdef VTSP_ENABLE_ECSR
    dsp_ptr->ecsrParams_ptr = &_VTSPR_ecsrParams;
    dsp_ptr->nlpParams_ptr  = &_VTSPR_nlpParams;
#endif
#ifdef VTSP_ENABLE_DTMF
    dsp_ptr->dtmfParams_ptr = &_VTSPR_dtmfParams;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    dsp_ptr->nseParams_ptr  = &_VTSPR_nseParams;
#endif
#ifdef VTSP_ENABLE_FMTD
    dsp_ptr->fmtdParams_ptr = &_VTSPR_fmtdParams;
#endif
#ifdef VTSP_ENABLE_CIDR
    dsp_ptr->fskrParams_ptr = &_VTSPR_fskrParams;
#endif
    dsp_ptr->jbDefParams_ptr   = &_VTSPR_jbDefParams;
#ifndef VTSP_ENABLE_MP_LITE
    dsp_ptr->dcrmNearParams_ptr = &_VTSPR_dcrmNearParams;
    dsp_ptr->dcrmPeerParams_ptr = &_VTSPR_dcrmPeerParams;
#endif
#ifdef VTSP_ENABLE_T38
    dsp_ptr->fr38Params_ptr = &_VTSPR_fr38Params;
    dsp_ptr->fr38Global_ptr = &_VTSPR_fr38GlobalParams;
#endif
#ifdef VTSP_ENABLE_AEC
    dsp_ptr->aecNearParams_ptr = &_VTSPR_aecHandsFreeParams;
#else
#ifndef VTSP_ENABLE_MP_LITE
    dsp_ptr->bndParams_ptr = &_VTSPR_bndParams;
#endif
#endif
#ifdef VTSP_ENABLE_UTD
    /*
     * Assign UTD Tone Parameter templates.
     *
     * UTD has 2 parameter objects:  the human readable version and the encoded
     * DSP version.  The translate function converts the readable version to
     * the encoded version.
     */
    dsp_ptr->utdParamNum = 0;

    tId = 0;
    _VTSP_utdToneDialUS.retVal = tId;
    dsp_ptr->utdParams_ptr[tId] = &_VTSP_utdToneDialUS;
    dsp_ptr->utdParamNum++;
    tId = 1;
    _VTSP_utdToneRingbackUS.retVal = tId;
    dsp_ptr->utdParams_ptr[tId] = &_VTSP_utdToneRingbackUS;
    dsp_ptr->utdParamNum++;
    tId = 2;
    _VTSP_utdToneBusyUS.retVal = tId;
    dsp_ptr->utdParams_ptr[tId] = &_VTSP_utdToneBusyUS;
    dsp_ptr->utdParamNum++;
    tId = 3;
    _VTSP_utdToneReorderUS.retVal = tId;
    dsp_ptr->utdParams_ptr[tId] = &_VTSP_utdToneReorderUS;
    dsp_ptr->utdParamNum++;
    tId = 4;
    _VTSP_utdToneSIT1US.retVal = tId;
    dsp_ptr->utdParams_ptr[tId] = &_VTSP_utdToneSIT1US;
    dsp_ptr->utdParamNum++;

    /* Alloc & assign blank templates not already assigned above */
    for (tId = 5; tId < _VTSP_NUM_UTD_TEMPL; tId++) {
        mem_ptr = OSAL_memCalloc(1, sizeof(_VTSPR_utdParamUnion), 0);
        if (NULL == mem_ptr) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
        else {
            dsp_ptr->utdParams_ptr[tId] = mem_ptr;
        }
        /* Zero memory for undefined UTD tone specs  */
        COMM_octetFill(mem_ptr, 0, sizeof(_VTSPR_utdParamUnion));
    }

    /*
     * Alloc all translate templates, 0 .. _VTSP_NUM_UTD_TEMPL
     * Note: Read the UTD man page prior to modifying this code.
     */
    for (tId = 0; tId < _VTSP_NUM_UTD_TEMPL; tId++) {
        mem_ptr = OSAL_memCalloc(1,
                sizeof(_VTSPR_utdTransDataUnion), /* See man page; = 28 */
                0);
        if (NULL == mem_ptr) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
        else {
            dsp_ptr->utdTransData_ptr[tId] = mem_ptr;
        }
        /* Zero memory for undefined UTD tone specs  */
        COMM_octetFill(mem_ptr, 0, sizeof(_VTSPR_utdTransDataUnion));
    }

    _VTSPR_utdTranslate(dsp_ptr);

#endif

    /*
     * Do for all Foreign Exchange Station (FXS) Physical Interfaces.
     * TIC object and associated params will be initialized at run time.
     */
    _VTSPR_FOR_ALL_FXS(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

#if _VTSP_INFC_FXS_NUM > 0
        fxs_ptr  = &dsp_ptr->fxsInfc[infc];
        /* Initialize Interface events
         */
        fxs_ptr->hookEvent = -1;        /* Generates event on first loop */
        fxs_ptr->flashEvent = TIC_NO_FLASH;
#endif

#ifdef VTSP_ENABLE_CIDS
        /* Initialize for caller id send
         *
         * XXX when tone is active on channel, caller-id will overwrite the
         * tone.  VTSPR must generate TONE HALTED event when this happens.
         */
        if (NULL == (chan_ptr->cids_ptr = OSAL_memCalloc(1,
                sizeof(_VTSPR_CidsObj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        cids_ptr = chan_ptr->cids_ptr;
        cids_ptr->cidsParam_ptr              = &VTSPR_CIDSendParamUS;
        cids_ptr->cidsParam_ptr->fsksObj_ptr = &cids_ptr->fsksObj;
        cids_ptr->cidsParam_ptr->toneObj_ptr = &chan_ptr->toneObj;
        cids_ptr->cidsParam_ptr->msg_ptr     = chan_ptr->cidsData.data;
        cids_ptr->cidsObj.dst_ptr            = chan_ptr->audioOut_ary;
        cids_ptr->cidsObj.stat               = CIDS_STAT_IDLE;

        cids_ptr->cidcwsParam_ptr              = &VTSPR_CIDCWSendParam;
        cids_ptr->cidcwsParam_ptr->fsksObj_ptr = &cids_ptr->fsksObj;
        cids_ptr->cidcwsParam_ptr->toneObj_ptr = &chan_ptr->toneObj;
        cids_ptr->cidcwsParam_ptr->msg_ptr     = chan_ptr->cidsData.data;
        cids_ptr->cidcwsObj.dst_ptr            = chan_ptr->audioOut_ary;
#endif
    }

    /*
     * Do for all Foreign Exchange Office (FXO) Physical Interfaces.
     * TIC object and associated params will be initialized at run time.
     */
    _VTSPR_FOR_ALL_FXO(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#ifdef VTSP_ENABLE_UTD
        if (NULL == (chan_ptr->utd_ptr = OSAL_memCalloc(1,
                sizeof(_VTSPR_UtdObj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        /* Default to detect all tones */
        utd_ptr = chan_ptr->utd_ptr;
        utd_ptr->utdObj.detectionMask = -1;
#endif
#ifdef VTSP_ENABLE_CIDR
        if (NULL == (chan_ptr->cidr_ptr = OSAL_memCalloc(1,
                sizeof(_VTSPR_CidrObj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        /* XXX dont init anything CIDR???? */
#endif
    }

    /*
     * Do for all Foreign Exchange Physical Interfaces (FXS & FXO).
     */
    _VTSPR_FOR_ALL_FX(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        /*
         * TONE, DTMF, FMTD will be initialized at run time.
         * Init ECSR, NFE, NLP, DCRM.
         */
        /*
         * ecsr, nlp, and nfe all require 8K data rate, from sout_ptr.
         * Their pointers are set up at init time, but the 8K data must be
         * valid at run time. Therefore, call _get8ReadPtr() at run time before
         * ECSR, NLP, or NFE.
         */
#ifdef VTSP_ENABLE_ECSR
        if (NULL == (chan_ptr->ec_ptr = OSAL_memCalloc(1,
                sizeof(_VTSPR_EcObj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        ec_ptr = chan_ptr->ec_ptr;
        ecsr_ptr = &ec_ptr->ecsrObj;
        ecsr_ptr->control = EC_ERL;
        ecsr_ptr->dlyBuf_ptr  = ec_ptr->ecDly_ary;
        ecsr_ptr->rinFull_ptr = ec_ptr->ecFull_ary;
        ECSR_init(ecsr_ptr, dsp_ptr->globals_ptr->p0DBIN,
                dsp_ptr->ecsrParams_ptr, EC_COLDSTART);
        ecsr_ptr->sin_ptr  = chan_ptr->audioIn_ary;
        ecsr_ptr->rin_ptr  = ec_ptr->ecRin_ary[0];
        ecsr_ptr->sout_ptr = chan_ptr->ecSout_ary;
#ifdef VTSP_ENABLE_FXO
        if (VTSPR_INFC_TYPE_FXS == _VTSPR_infcToType(infc)) {
            ec_ptr->nAllignBufs = VTSPR_NALLIGNMENT_BUFS_FXS;
        }
        else {
            ec_ptr->nAllignBufs = VTSPR_NALLIGNMENT_BUFS_FXO;
        }
#else
        ec_ptr->nAllignBufs = VTSPR_NALLIGNMENT_BUFS_MAX;
#endif

        nlp_ptr = &ec_ptr->nlpObj;
        NLP_init(nlp_ptr, dsp_ptr->globals_ptr,
                dsp_ptr->nlpParams_ptr);
        nlp_ptr->control = NLP_CNTR_NFE;

        nlp_ptr->dst_ptr   = chan_ptr->ecNlp_ary;
        nlp_ptr->src_ptr   = ecsr_ptr->sout_ptr;
#endif /* VTSP_ENABLE_ECSR */

#ifdef VTSP_ENABLE_FMTD
        if (NULL == (chan_ptr->fmtd_ptr = OSAL_memCalloc(1,
                sizeof(_VTSPR_FmtdObj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        fmtd_ptr = chan_ptr->fmtd_ptr;
        fmtdObj_ptr = &fmtd_ptr->fmtdInfcObj;
        fmtdObj_ptr->control = (
                (1 << FMTD_FC_BIT) |     /* FAX Calling Tone */
                (1 << FMTD_MC_BIT) |     /* Modem Calling Tone */
                (1 << FMTD_ANSWER_BIT) | /* Modem or FAX Answer Tone */
                (1 << FMTD_PHASE_BIT) |  /* Modem Answer Tone */
                (1 << FMTD_V21_BIT) |    /* (ITU V.21) FAX low-speed signal */
                (1 << FMTD_BA1_BIT) |    /* (Bell-208) 2225 Hz tone */
                (1 << FMTD_BA2_BIT));    /* (Bell-208) 2025 Hz tone */
        fmtdObj_ptr = &fmtd_ptr->fmtdPeerObj;
        fmtdObj_ptr->control = (
                (1 << FMTD_FC_BIT) |     /* FAX Calling Tone */
                (1 << FMTD_MC_BIT) |     /* Modem Calling Tone */
                (1 << FMTD_ANSWER_BIT) | /* Modem or FAX Answer Tone */
                (1 << FMTD_PHASE_BIT) |  /* Modem Answer Tone */
                (1 << FMTD_V21_BIT) |    /* (ITU V.21) FAX low-speed signal */
                (1 << FMTD_BA1_BIT) |    /* (Bell-208) 2225 Hz tone */
                (1 << FMTD_BA2_BIT));    /* (Bell-208) 2025 Hz tone */
        /* Initialize Fax FMTD/EC Bypass timeout to default */
        fmtd_ptr->fmtdSilenceTimeMax = VTSPR_FAX_TIMEOUT;
#endif /* VTSP_ENABLE_FMTD */

#ifdef VTSP_ENABLE_DTMF
        /*
         * Initialize DTMF events
         */
        if (NULL == (chan_ptr->dtmf_ptr = OSAL_memCalloc(1,
                sizeof(_VTSPR_DtmfObj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        dtmf_ptr = chan_ptr->dtmf_ptr;
        dtmf_ptr->dtmfDigit = -1;
        dtmf_ptr->dtmfLe    = -1;
        dtmf_ptr->dtmfTe    = -1;
        /*
         * Set by default to run DTMF remove
         */
        dtmf_ptr->dtmfRemove = 1;
        dtmf_ptr->earlyCount = 0;
#endif
        /* Initialize EC events */
        chan_ptr->echoCancEvent = VTSP_EVENT_ECHO_CANC_OFF;
        chan_ptr->echoCancEventLast = chan_ptr->echoCancEvent;

#ifndef VTSP_ENABLE_MP_LITE
        /* Initialize NEAR TONE Sequence and NEAR GENF Sequence */
        toneSeq_ptr = &chan_ptr->toneSeq;
        toneSeq_ptr->toneObj_ptr   = &chan_ptr->toneObj;
        toneSeq_ptr->toneEventType = VTSP_EVENT_TYPE_TONE_BASIC;
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
        toneSeq_ptr = &chan_ptr->toneQuadSeq;
        toneSeq_ptr->toneObj_ptr   = &chan_ptr->toneQuadObj;
        toneSeq_ptr->toneEventType = VTSP_EVENT_TYPE_TONE_QUAD;
#endif


#ifdef VTSP_ENABLE_T38
            if (NULL == (chan_ptr->fr38_ptr = OSAL_memCalloc(1,
                    sizeof(_VTSPR_FR38Obj), 0))) {
                _VTSP_TRACE(__FILE__,__LINE__);
            }
            fr38_ptr = chan_ptr->fr38_ptr;
            if (NULL == (fr38_ptr->fr38Obj.internal = OSAL_memCalloc(
                    1, sizeof(_FR38_Internal), 0))) {
                _VTSP_TRACE(__FILE__, __LINE__);
            }
            fr38_ptr->fr38Obj.fax_packetIn_ptr  = &fr38_ptr->t38ObjIn;
            fr38_ptr->fr38Obj.fax_packetOut_ptr = &fr38_ptr->t38ObjOut;
            fr38_ptr->fr38Obj.status_ptr        = &fr38_ptr->t38Status;
        fr38_ptr->fr38Obj.fax_packetOut_ptr->numBits = FR38_MAX_T38_PACKET << 3;
            fr38_ptr->fr38Event     = VTSP_EVENT_INACTIVE;
#endif
        /*
         * Init streams.
         */
        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
#ifdef VTSP_ENABLE_T38
            stream_ptr->fr38Obj_ptr = &fr38_ptr->fr38Obj;
            stream_ptr->fr38Jb_ptr  = fr38_ptr->fr38Jb;
            stream_ptr->fr38Mdm_ptr = fr38_ptr->fr38MdmContext;

            /* The following initializes both Encode/Decode directions */
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_ENCODER,
                    VTSP_CODER_T38);
            /* BUG 1873, check for internal context size */
            FR38_getConfig(&fr38Config);
            if (FR38_CONTEXT_SIZE < fr38Config.cInternalSize) {
                OSAL_logMsg("%s:%d FATAL ERROR:\n FR38 context size of %d"
                        " shorts\n is less than the required %d shorts\n",
                    __FILE__, __LINE__, FR38_CONTEXT_SIZE,
                    fr38Config.cInternalSize);
            }
#endif
        }
    }

#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRNB_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
    if (DSP_ERROR == DSP_init()) {
        OSAL_logMsg("%s:%d DSP ACCELERATOR Initialization Error!",
                __FILE__, __LINE__);
    }
#endif

    /*
     * Do for all Foreign Exchange Physical Interfaces (FXS & FXO).
     */
    _VTSPR_FOR_ALL_INFC(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);

        nSamples10ms = TIC_getStatus(tic_ptr, TIC_GET_STATUS_NSAMPLES_10MS);
        switch (nSamples10ms) {
            case TIC_NSAMPLES_10MS_8K:
                chan_ptr->numSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                break;
            case TIC_NSAMPLES_10MS_16K:
                chan_ptr->numSamples10ms = VTSPR_NSAMPLES_10MS_16K;
                break;
            case TIC_NSAMPLES_10MS_32K:
                chan_ptr->numSamples10ms = VTSPR_NSAMPLES_10MS_32K;
                break;
            case TIC_NSAMPLES_10MS_48K:
                chan_ptr->numSamples10ms = VTSPR_NSAMPLES_10MS_48K;
                break;
            default:
                OSAL_logMsg("%s:%d numSamples10ms %d is not supported\n",
                        __FILE__, __LINE__, nSamples10ms);
        }
        chan_ptr->curActive = 0;

        /*
         * TONE, DTMF, FMTD will be initialized at run time.
         * Init ECSR, NFE, NLP, DCRM.
         */
        /*
         * ecsr, nlp, and nfe all require 8K data rate, from sout_ptr.
         * Their pointers are set up at init time, but the 8K data must be
         * valid at run time. Therefore, call _get8ReadPtr() at run time before
         * ECSR, NLP, or NFE.
         */
#ifdef VTSP_ENABLE_NFE
        nfe_ptr = &chan_ptr->nfeInfcObj;
        NFE_init(nfe_ptr, dsp_ptr->globals_ptr,
                dsp_ptr->nfeParams_ptr, NFE_COLD_START);
        nfe_ptr->src_ptr   = chan_ptr->ecSout_ary;
        nfe_ptr->xmit_ptr  = NULL;

        nfe_ptr = &chan_ptr->nfePeerObj;
        NFE_init(nfe_ptr, dsp_ptr->globals_ptr,
                dsp_ptr->nfeParams_ptr, NFE_COLD_START);
        nfe_ptr->src_ptr   = chan_ptr->audioOut_ary;
        nfe_ptr->xmit_ptr  = NULL;
#endif
#ifndef VTSP_ENABLE_MP_LITE
        /*
         * Set DCRM Blocksize dynamically
         */
        dsp_ptr->dcrmNearParams_ptr->blockSz = chan_ptr->numSamples10ms;
        dsp_ptr->dcrmPeerParams_ptr->blockSz = chan_ptr->numSamples10ms;

        /*
         * DCRM for Near end (to voice chipset)
         * src and dst are assigned during real time processing.
         */
        dcrm_ptr = &chan_ptr->dcrmNearObj;
        DCRM_init(dcrm_ptr, dsp_ptr->dcrmNearParams_ptr);
        /* DCRM for Peer end after decode */
        dcrm_ptr = &chan_ptr->dcrmPeerObj;
        DCRM_init(dcrm_ptr, dsp_ptr->dcrmPeerParams_ptr);
        dcrm_ptr->src_ptr = chan_ptr->audioOut_ary;
        dcrm_ptr->dst_ptr = chan_ptr->audioOut_ary;
#endif
        /* Initialize EC events */
        chan_ptr->echoCancEvent = VTSP_EVENT_ECHO_CANC_OFF;
        chan_ptr->echoCancEventLast = chan_ptr->echoCancEvent;

#if defined(VTSP_ENABLE_STREAM_16K) || defined(VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
        if (chan_ptr->numSamples10ms < VTSPR_NSAMPLES_STREAM)
        {
            lowRate = chan_ptr->numSamples10ms;
            highRate = VTSPR_NSAMPLES_STREAM;
        }
        else {
            lowRate = VTSPR_NSAMPLES_STREAM;
            highRate = VTSPR_NSAMPLES_10MS_MAX;
        }
        lowRate = _VTSPR_samples10msToUdsMode(lowRate);
        highRate = _VTSPR_samples10msToUdsMode(highRate);

        UDS_init(&chan_ptr->udsAudioUp, (UDS_SampleRate)lowRate,
                (UDS_SampleRate)highRate);
        UDS_init(&chan_ptr->udsAudioDown, (UDS_SampleRate)lowRate,
                (UDS_SampleRate)highRate);
#endif
#endif /* end VTSP_ENABLE_STREAM_16K || VTSP_ENABLE_AUDIO_SRC */

#ifndef VTSP_ENABLE_MP_LITE
        /* Initialize NEAR TONE Sequence and NEAR GENF Sequence */
        toneSeq_ptr = &chan_ptr->toneSeq;
        toneSeq_ptr->toneObj_ptr   = &chan_ptr->toneObj;
        toneSeq_ptr->toneEventType = VTSP_EVENT_TYPE_TONE_BASIC;
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
        toneSeq_ptr = &chan_ptr->toneQuadSeq;
        toneSeq_ptr->toneObj_ptr   = &chan_ptr->toneQuadObj;
        toneSeq_ptr->toneEventType = VTSP_EVENT_TYPE_TONE_QUAD;
#endif

#if defined(VTSP_ENABLE_DTMFR) && !defined(VTSP_ENABLE_MP_LITE)
        /* Initialize for DTMF Relay
         */
        chan_ptr->drDecodeObj.toneParam_ptr = &_VTSPR_drDecodeTone;
        chan_ptr->drDecodeObj.toneObj_ptr   = &chan_ptr->toneObj;
#ifdef VTSP_ENABLE_TONE_QUAD
        chan_ptr->drDecodeObj.toneQuadParam_ptr = &_VTSPR_drDecodeToneQuad;
        chan_ptr->drDecodeObj.toneQuadObj_ptr   = &chan_ptr->toneQuadObj;
#endif
        chan_ptr->drDecodeObj.globals_ptr = dsp_ptr->globals_ptr;
        /*
         * Init DR event object
         */
        chan_ptr->drEventObj.event    = 0;
        chan_ptr->drEventObj.newDigit  = 0;
        chan_ptr->drEventObj.newPower = 0;
        chan_ptr->drEventObj.newPlayTime = DR_MAX_PLAYTIME_8K;
        chan_ptr->drEventObj.stop     = DR_STOP_CLEAR;
#endif
        /*
         * Init streams.
         */
        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            /*
             * Set default algorithm masks and coder types
             */
            stream_ptr->lastEncoder = VTSP_CODER_UNAVAIL;
            stream_ptr->lastExtension = 0;

            stream_ptr->curActive = 0;

#ifdef VTSP_ENABLE_DTMFR
            /*
             * Initialize for DTMF Relay
             * Decode
             */
            stream_ptr->drDecodeObj_ptr = &chan_ptr->drDecodeObj;

            /* Encode */
            stream_ptr->drEncodeObj.status     = 0;
            stream_ptr->drEncodeObj.redundancy = 2;  /* extra "E" payloads */
            stream_ptr->drEncodeObj.newEvent   = 0;
            stream_ptr->drEncodeObj.lastEvent  = 0;
            stream_ptr->drEncodeObj.dur        = 0;
            stream_ptr->drEncodeObj.digit      = 0;
            stream_ptr->drEncodeObj.playTime   = DR_MAX_PLAYTIME_8K;
            stream_ptr->drEncodeObj.power      = 0;
            stream_ptr->drEncodeObj.totalTime  = 0;
            stream_ptr->drEncodeObj.sampleRate = DR_SAMPLE_RATE_8K;

#ifdef VTSP_ENABLE_DTMF
            /* detectTime is in 8kHz units.
             * tDTMFDUR is in 5ms units.  Acts as neg offset to timestamp.
             * tDTMFSIL is in 5ms units.  Acts as neg offset to duration.
             */
            stream_ptr->drEncodeObj.detectLeTime =
                   (_VTSPR_dtmfParams.tDTMFDUR * 40);
#else
            stream_ptr->drEncodeObj.detectLeTime = 0;
#endif
            /*
             * stream_ptr->drEncodeObj.detectTeTime =
             *      (_VTSPR_dtmfParams.tDTMFSIL << 4);
             */
            stream_ptr->drEncodeObj.detectTeTime = 0;
#endif
            /*
             * Coders, init only stateful coders.
             */
#ifdef VTSP_ENABLE_G729
#ifndef VTSP_ENABLE_G729_ACCELERATOR
           /*
            * Init G729 Coder
            */
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_ENCODER,
                    VTSP_CODER_G729);
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_DECODER,
                    VTSP_CODER_G729);
#else
            /*
             * Initialize DSP accelerator coders and decoders
             * Place instances in the stream.
             */
           stream_ptr->multiEncObj.encG729Instance = DSP_getInstance(
                   DSP_CODER_TYPE_G729, DSP_ENCODE);
           stream_ptr->multiDecObj.decG729Instance = DSP_getInstance(
                   DSP_CODER_TYPE_G729, DSP_DECODE);
#endif // VTSP_ENABLE_G729_ACCELERATOR
#endif // VTSP_ENABLE_G729

#ifdef VTSP_ENABLE_G726
#ifndef VTSP_ENABLE_G726_ACCELERATOR
           /*
            * Init G726 Coder
            */
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_ENCODER,
                    VTSP_CODER_G726_32K);
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_DECODER,
                    VTSP_CODER_G726_32K);
#else
            /*
             * Bug 3252 initialize DSP accelerator coders and decoders
             * Place instances in the stream.
             */
           stream_ptr->multiEncObj.encG726Instance = DSP_getInstance(
                   DSP_CODER_TYPE_G726, DSP_ENCODE);
           stream_ptr->multiDecObj.decG726Instance = DSP_getInstance(
                   DSP_CODER_TYPE_G726, DSP_DECODE);
#endif // VTSP_ENABLE_G726_ACCELERATOR
#endif // VTSP_ENABLE_G726

#ifdef VTSP_ENABLE_G722
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_ENCODER,
                    VTSP_CODER_G722);
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_DECODER,
                    VTSP_CODER_G722);
#endif

#ifdef VTSP_ENABLE_GAMRNB_ACCELERATOR
            stream_ptr->multiEncObj.encGamrnbInstance = DSP_getInstance(
                    DSP_CODER_TYPE_AMRNB_OA, DSP_ENCODE);
            stream_ptr->multiDecObj.decGamrnbInstance = DSP_getInstance(
                    DSP_CODER_TYPE_AMRNB_OA, DSP_DECODE);
#endif

#ifdef VTSP_ENABLE_GAMRWB_ACCELERATOR
            stream_ptr->multiEncObj.encGamrwbInstance = DSP_getInstance(
                    DSP_CODER_TYPE_AMRWB_OA, DSP_ENCODE);
            stream_ptr->multiDecObj.decGamrwbInstance = DSP_getInstance(
                    DSP_CODER_TYPE_AMRWB_OA, DSP_DECODE);
#endif

#ifndef VTSP_ENABLE_MP_LITE
            /*
             * PLC, NFE, NSE. TONE will be initialized at run time.
             */
            PLC_init(&stream_ptr->plcObj);

            /*
             * Set CN Power Attenuation to default.
             */
            dsp_ptr->cnPwrAttenDb = VTSPR_CN_PWR_ATTEN_DEF;

            /*
             * Set NSE blockSz dynamically
             */
            dsp_ptr->nseParams_ptr->blockSz = chan_ptr->numSamples10ms;
            NSE_init(&stream_ptr->nseObj, dsp_ptr->globals_ptr,
                    dsp_ptr->nseParams_ptr);
#endif
#ifdef VTSP_ENABLE_STREAM_16K
#ifndef VTSP_ENABLE_MP_LITE
            UDS_init(&stream_ptr->udsStreamUp, UDS_SAMPLERATE_8K,
                    UDS_SAMPLERATE_16K);
            UDS_init(&stream_ptr->udsStreamDown, UDS_SAMPLERATE_8K,
                    UDS_SAMPLERATE_16K);
#endif
#endif /* end VTSP_ENABLE_STREAM_16K */
#ifdef VTSP_ENABLE_NFE
            stream_ptr->nfe_ptr = &chan_ptr->nfePeerObj;
#endif
#ifndef VTSP_ENABLE_MP_LITE
            /* Initialize Stream TONE Sequence and Stream GENF Sequence */
            toneSeq_ptr = &stream_ptr->toneSeq;
            toneSeq_ptr->toneObj_ptr   = &stream_ptr->toneObj;
            toneSeq_ptr->toneEventType = VTSP_EVENT_TYPE_TONE_BASIC;
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
            toneSeq_ptr = &stream_ptr->toneQuadSeq;
            toneSeq_ptr->toneObj_ptr   = &stream_ptr->genfObj;
            toneSeq_ptr->toneEventType = VTSP_EVENT_TYPE_TONE_QUAD;
#endif
            /*
             * Init the jitter buffer and RTP params.
             */
            OSAL_memCpy(&stream_ptr->jbParams, dsp_ptr->jbDefParams_ptr,
                    sizeof(stream_ptr->jbParams));
            JB_init(&stream_ptr->jbObj, &stream_ptr->jbParams);
        }
    }

    /*
     * Do for all Foreign Exchange Physical Interfaces (FXS & FXO).
     */
    _VTSPR_FOR_ALL_AUDIO(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        /*
         * TONE, DTMF, FMTD will be initialized at run time.
         * Init ECSR, NFE, NLP, DCRM.
         */
#ifdef VTSP_ENABLE_AEC
        chan     = _VTSPR_infcToChan(infc);
        /*
         * Initialize AEC
         * Rin and Rout buffers are set during audio processing, for correct
         * time allignment.
         */
        if (NULL == (chan_ptr->aec_ptr = OSAL_memCalloc(1,
                sizeof(_VTSPR_AecObj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        aec_ptr                 = chan_ptr->aec_ptr;
        aecObj_ptr              = &aec_ptr->aecNearObj;
        aecObj_ptr->sin_ptr     = chan_ptr->audioIn_ary;   /* input */
        aecObj_ptr->sout_ptr    = chan_ptr->ecNlp_ary;     /* output */
        aecObj_ptr->rin_ptr     = aec_ptr->aecRin_ary[0];  /* input */
        aecObj_ptr->rout_ptr    = aec_ptr->aecRout_ary[0]; /* output */
        aecObj_ptr->filter_ptr  = aec_ptr->aecfilt_ary;    /* internal buf */
        aecObj_ptr->rinHist_ptr = aec_ptr->aecRinHist_ary; /* internal buf */
        aec_ptr->nAllignBufs    = VTSPR_NALLIGNMENT_BUFS;

        if (VTSPR_NSAMPLES_10MS_16K == chan_ptr->numSamples10ms) {
            _VTSPR_aecHandsetParams.pSAMPLE_RATE = AEC_SAMPLE_RATE_16K;
            _VTSPR_aecHandsFreeParams.pSAMPLE_RATE = AEC_SAMPLE_RATE_16K;
        }

        AEC_init(aecObj_ptr, dsp_ptr->globals_ptr, dsp_ptr->aecNearParams_ptr,
                AEC_INIT_COLD);
        /* By default, enable AEC on all channels */
        _VTSPR_algStateChan(dsp_ptr, chan, 0, VTSPR_ALG_CHAN_AEC);

#ifdef VTSP_ENABLE_AEC_HALF_DUPLEX
        aecObj_ptr->control = AEC_CONTROL_AEC_HALF_DUPLEX;
#endif

        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            stream_ptr->aec_ptr = aecObj_ptr;
        }
#else  /* VTSP_ENABLE_AEC */
#ifndef VTSP_ENABLE_MP_LITE
        if (NULL == (chan_ptr->bndNear_ptr = OSAL_memCalloc(1,
                sizeof(BND_Obj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        bndObj_ptr          = chan_ptr->bndNear_ptr;
        bndObj_ptr->src_ptr = chan_ptr->audioIn_ary;   /* input */

        BND_init(bndObj_ptr, dsp_ptr->globals_ptr, dsp_ptr->bndParams_ptr,
                BND_COLD_START);
        if (NULL == (chan_ptr->bndPeer_ptr = OSAL_memCalloc(1,
                sizeof(BND_Obj), 0))) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        bndObj_ptr          = chan_ptr->bndPeer_ptr;
        bndObj_ptr->src_ptr = chan_ptr->audioOut_ary;   /* input */
        BND_init(bndObj_ptr, dsp_ptr->globals_ptr, dsp_ptr->bndParams_ptr,
                BND_COLD_START);
        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            stream_ptr->bnd_ptr = chan_ptr->bndPeer_ptr;
        }
#endif
#endif /* !VTSP_ENABLE_AEC */
    }
}

void _VTSPR_cidSetCountryCode(
    VTSPR_ChanObj *chan_ptr,
    vint           infc,
    uvint          code)
{
#ifdef VTSP_ENABLE_CIDS
    _VTSPR_CidsObj *cids_ptr;
    cids_ptr = chan_ptr->cids_ptr;

    if (VTSPR_INFC_TYPE_FXS == _VTSPR_infcToType(infc)) {
        /* First, set FSK/DTMF Params */
        switch (code) {
            case VTSP_TEMPL_CID_FORMAT_JP:
            case VTSP_TEMPL_CID_FORMAT_DATA_JP:
                cids_ptr->cidsParam_ptr = &VTSPR_CIDSendParamJP;
                cids_ptr->cidcwsParam_ptr = &VTSPR_CIDCWSendParamJP;
                break;
            case VTSP_TEMPL_CID_FORMAT_UK_FSK:
                cids_ptr->cidsParam_ptr = &VTSPR_CIDSendParamUKFSK;
                break;
            case VTSP_TEMPL_CID_FORMAT_UK_DTMF:
                cids_ptr->cidsParam_ptr = &VTSPR_CIDSendParamUKDTMF;
            /* Default to US parameters.  */
            case VTSP_TEMPL_CID_FORMAT_US:
            case VTSP_TEMPL_CID_FORMAT_DATA_US:
            default:
                cids_ptr->cidsParam_ptr = &VTSPR_CIDSendParamUS;
                cids_ptr->cidcwsParam_ptr = &VTSPR_CIDCWSendParam;

        }

        /* Second, set extension word option */
        switch (code) {
            case VTSP_TEMPL_CID_FORMAT_DATA_JP:
            case VTSP_TEMPL_CID_FORMAT_DATA_US:
                cids_ptr->cidsParam_ptr->extension |= CIDS_LOCALS_EXT_DATA_ONLY;
                cids_ptr->cidcwsParam_ptr->extension |=
                      CIDS_LOCALS_EXT_DATA_ONLY;
                break;
        }
    }
#endif
}


#ifndef VTSP_ENABLE_T38  /* ======== Stubs for T38 ======== */
#define FR38_Handle void *
#define FR38_Params void *
#define FR38V3_GLOBAL_Params void *
void FR38_init(
    FR38_Handle     fr38Obj_ptr, /* pointer to FR38 object */
    GLOBAL_Params  *global_ptr,  /* pointer to global parameters */
    FR38_Params    *params_ptr,  /* pointer to FR38 parameters */
    void           *jitter_ptr,  /* pointer to jitter buffer memory */
    unsigned short *stack_ptr,   /* Ptr to VP Open stack or NULL */
    unsigned short *b1_ptr)      /* Ptr to VP Open B1 memory or
                                    Modem Context ptr for Non=Embedded
                                    modem context */
{
    return;
}

int FR38_run(
    FR38_Handle    fr38Obj_ptr,  /* pointer to FR38 object */
    int            packetAvailable)
{
    return 0;
}

int FR38_putPacket(
    FR38_Handle fr38Obj_ptr)     /* pointer to FR38 object */
{
    return 0;
}
#endif

