;;// for asm code:
;;// MP3_DEC_ASM_LITTLE_ENDIAN = 1; little endian mode
;;// Mp3_DEC_ASM_LITTLE_ENDIAN = 0; big endian mode
MP3_DEC_ASM_LITTLE_ENDIAN EQU 1

;;// for c code
;;// if define MP3_DEC_WORDS_BIGENDIAN macro, mean big endian mode
;;// if close macro MP3_DEC_WORDS_BIGENDIAN, mean little endian mode
INTERLEAVE_PCM EQU 0