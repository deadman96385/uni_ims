
#ifndef VP8_POSTPROC_H
#define VP8_POSTPROC_H

struct postproc_state
{
    int           last_q;
    int           last_noise;
    char          noise[3072];
    char blackclamp[16];
    char whiteclamp[16];
    char bothclamp[16];
};

#endif //VP8_POSTPROC_H