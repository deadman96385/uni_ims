/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30256 $ $Date: 2014-12-08 17:30:17 +0800 (Mon, 08 Dec 2014) $
 */

#include "sip_sip.h"
#include "sip_voipnet.h"
#include "sip_abnfcore.h"
#include "sip_token.h"
#include "sip_clib.h"
#include "sip_mem.h"
#include "sip_sdp_msg.h"
#include "sip_sdp_dec.h"
#include "sip_mem_pool.h"

#include "_sip_sdp_syntax.h"

/* internal targets for SDP Message decoding */
static vint _SDPDEC_ProtVersDecoder (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_OriginDecoder   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_SessNameDecoder (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

static vint _SDPDEC_CxDecoder       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_AddrInDecoder   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_AddrAtmDecoder  (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_AddrLocalDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_BwDecoder       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_TimeDecoder     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

static vint _SDPDEC_AttrDecoder     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_SilenceSupp     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_RtpMapDecoder   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_PTimeDecoder    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_FmtpDecoder     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_ExtMapDecoder   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_MediaDecoder    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_MediaSetDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle  hIntMsg, tSdpMedia **ppSdpMedia);

/* RTCP and fax support */
static vint _SDPDEC_RtcpDecoder                 (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38FaxVersionDecoder        (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38MaxBitRateDecoder        (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38FaxFillBitRemovalDecoder (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38FaxTranscodingMMRDecoder (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38FaxTranscodingJBIGDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38FaxRateManagementDecoder (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38FaxMaxBufferDecoder      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38FaxMaxDatagramDecoder    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_T38FaxUdpECDecoder          (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_CryptoDecoder               (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_EncryptionDecoder           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_PathDecoder                 (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_AcceptTypesDecoder          (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_AcceptWrappedTypesDecoder   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_FileSelectorDecoder         (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_FileDispositionDecoder      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_FileTransferIdDecoder       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_SetupDecoder                (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_FingerprintDecoder          (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_FramesizeDecoder            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_TcapDecoder                 (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_PcfgDecoder                 (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_RtcpfbDecoder               (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

/* Precondition */
static vint _SDPDEC_CurrDecoder                 (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_DesDecoder                  (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_ConfDecoder                 (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPDEC_FramerateDecoder            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

static vint _SDPDEC_Skip(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

static tTokenizer  _SdpDecFieldsTable[] = 
{
   { SDP_FIELD_PVERSION    , eSdpProtVersion            , _SDPDEC_ProtVersDecoder   },
   { SDP_FIELD_ORIGIN      , eSdpOrigin                 , _SDPDEC_OriginDecoder     },
   { SDP_FIELD_SESSNAME    , eSdpName                   , _SDPDEC_SessNameDecoder   },
   { SDP_FIELD_INFO        , eSdpInfo                   , _SDPDEC_Skip              },
   { SDP_FIELD_URI         , eSdpURI                    , _SDPDEC_Skip              },
   { SDP_FIELD_EMAIL       , eSdpEMail                  , _SDPDEC_Skip              },
   { SDP_FIELD_PHONE       , eSdpPhone                  , _SDPDEC_Skip              },
   { SDP_FIELD_CONN        , eSdpConnInfo               , _SDPDEC_CxDecoder         },
   { SDP_FIELD_BANDWIDTH   , eSdpBWInfo                 , _SDPDEC_BwDecoder         },
   { SDP_FIELD_TIME        , eSdpTime                   , _SDPDEC_TimeDecoder       },
   { SDP_FIELD_KEY         , eSdpEncriptKey             , _SDPDEC_Skip              },
   { SDP_FIELD_ATTR        , eSdpAttr                   , _SDPDEC_AttrDecoder       },
   { SDP_FIELD_MEDIA       , eSdpMedia                  , _SDPDEC_MediaDecoder      }
};

static tTokenizer  _SdpDecNetTypesTable[] = 
{
   { SDP_NETTYPE_IN        , eNetworkIN                 , _SDPDEC_AddrInDecoder     },
   { SDP_NETTYPE_ATM       , eNetworkATM                , _SDPDEC_AddrAtmDecoder    },
   { SDP_NETTYPE_LOCAL     , eNetworkLocal              , _SDPDEC_AddrLocalDecoder  }
};

static tTokenizer  _SdpDecMediaTypesTable[] = 
{
   { SDP_MEDIA_AUDIO       , eSdpMediaAudio                               , 0       }, 
   { SDP_MEDIA_IMAGE       , eSdpMediaImage                               , 0       }, 
   { SDP_MEDIA_VIDEO       , eSdpMediaVideo                               , 0       }, 
   { SDP_MEDIA_APPL        , eSdpMediaApplication                         , 0       },
   { SDP_MEDIA_MSG         , eSdpMediaMsMessage                           , 0       }
};

static tTokenizer  _SdpDecTpTypesTable[] = 
{
   { SDP_TP_LOCAL          , eTransportLocal                              , 0       }, 
   { SDP_TP_RTPAVP         , eTransportRtpAvp                             , 0       },
   { SDP_TP_RTPSAVP        , eTransportRtpSavp                            , 0       },  
   { SDP_TP_RTPAVPF        , eTransportRtpAvpf                            , 0       },
   { SDP_TP_RTPSAVPF       , eTransportRtpSavpf                           , 0       },  
   { SDP_TP_ATMAVP         , eTransportAtmAvp                             , 0       },
   { SDP_TP_UDP            , eTransportUdp                                , 0       },
   { SDP_TP_TCP            , eTransportTcp                                , 0       },
   { SDP_TP_UDPTL          , eTransportUdptl                              , 0       },
   { SDP_TP_SIP            , eTransportSip                                , 0       },
   { SDP_TP_MSRPTCP        , eTransportMsrpTcp                            , 0       },
   { SDP_TP_MSRPTLS        , eTransportMsrpTls                            , 0       }
};

static tTokenizer _SdpDecPrecTypeTable[] =
{
   { SDP_PREC_QOS         , ePrecTypeQos         , 0    }
};

static tTokenizer _SdpDecPrecStatusTypeTable[] =
{
   { SDP_PREC_E2E         , ePrecStatusTypeE2e   , 0    },
   { SDP_PREC_LOCAL       , ePrecStatusTypeLocal , 0    },
   { SDP_PREC_REMOTE      , ePrecStatusTypeRemote, 0    }
};

static tTokenizer _SdpDecPrecDirTable[] =
{
   { SDP_PREC_NONE        , ePrecDirNone         , 0    },
   { SDP_PREC_SEND        , ePrecDirSend         , 0    },
   { SDP_PREC_RECV        , ePrecDirRecv         , 0    },
   { SDP_PREC_SENDRECV    , ePrecDirSendRecv     , 0    }
};

static tTokenizer _SdpDecPrecStrengthTable[] =
{
   { SDP_PREC_MANDATORY   , ePrecStrengthMandatory, 0    },
   { SDP_PREC_OPTIONAL    , ePrecStrengthOptional , 0    },
   { SDP_PREC_NONE        , ePrecStrengthNone     , 0    },
   { SDP_PREC_FAILURE     , ePrecStrengthFailure  , 0    },
   { SDP_PREC_UNKNOWN     , ePrecStrengthUnknown  , 0    }
};

static tTokenizer _SdpDecHashTable[] =
{
   { SDP_HASH_SHA1        , eSdpHashSha1   , 0    },
   { SDP_HASH_SHA224      , eSdpHashSha224 , 0    },
   { SDP_HASH_SHA256      , eSdpHashSha256 , 0    },
   { SDP_HASH_SHA384      , eSdpHashSha384 , 0    },
   { SDP_HASH_SHA512      , eSdpHashSha512 , 0    },
   { SDP_HASH_MD5         , eSdpHashMd5    , 0    },
   { SDP_HASH_MD2         , eSdpHashMd2    , 0    }
};

static const tTokenizer  _SdpDecSetupRoleTable[] =
{
   { SDP_SETUP_ACTIVE      , eSdpSetupActive , 0     },
   { SDP_SETUP_ACTPASS     , eSdpSetupActpass, 0     },
   { SDP_SETUP_PASSIVE     , eSdpSetupPassive, 0     }
};

static tTokenizer  _SdpDecMediaSetTable[] = 
{
   { SDP_FIELD_INFO        , eSdpInfo                   , _SDPDEC_Skip              },
   { SDP_FIELD_CONN        , eSdpConnInfo               , _SDPDEC_CxDecoder         },
   { SDP_FIELD_BANDWIDTH   , eSdpBWInfo                 , _SDPDEC_BwDecoder         },
   { SDP_FIELD_KEY         , eSdpEncriptKey             , _SDPDEC_Skip              },
   { SDP_FIELD_ATTR        , eSdpAttr                   , _SDPDEC_AttrDecoder       }
};

static tTokenizer  _SdpDecAttrSetTable[] = 
{
   { SDP_ATTR_RTPMAP       , eSdpAttrRtpMap             , _SDPDEC_RtpMapDecoder     },
   { SDP_ATTR_CAT          , eSdpAttrCat                , _SDPDEC_Skip              },
   { SDP_ATTR_KEYWDS       , eSdpAttrKeywds             , _SDPDEC_Skip              },
   { SDP_ATTR_TOOL         , eSdpAttrTool               , _SDPDEC_Skip              },
   { SDP_ATTR_PTIME        , eSdpAttrPTime              , _SDPDEC_PTimeDecoder      },
   { SDP_ATTR_MAXPTIME     , eSdpAttrMaxPTime           , _SDPDEC_PTimeDecoder      },
   { SDP_ATTR_MSRPACM      , eSdpAttrMsrpAcm            , 0                         },
   { SDP_ATTR_RECVONLY     , eSdpAttrRecvOnly           , 0                         },
   { SDP_ATTR_SENDRECV     , eSdpAttrSendRecv           , 0                         },
   { SDP_ATTR_SENDONLY     , eSdpAttrSendOnly           , 0                         },
   { SDP_ATTR_INACTIVE     , eSdpAttrInactive           , 0                         },
   { SDP_ATTR_ORIENT       , eSdpAttrOrient             , _SDPDEC_Skip              },
   { SDP_ATTR_TYPE         , eSdpAttrType               , _SDPDEC_Skip              },
   { SDP_ATTR_CHARSET      , eSdpAttrCharset            , _SDPDEC_Skip              },
   { SDP_ATTR_SDPLANG      , eSdpAttrSdpLang            , _SDPDEC_Skip              },
   { SDP_ATTR_LANG         , eSdpAttrLang               , _SDPDEC_Skip              },
   { SDP_ATTR_FRAMERATE    , eSdpAttrFrameRate          , _SDPDEC_FramerateDecoder  },
   { SDP_ATTR_QUALITY      , eSdpAttrQuality            , _SDPDEC_Skip              },
   { SDP_ATTR_FMTP         , eSdpAttrFmtp               , _SDPDEC_FmtpDecoder       },
   { SDP_ATTR_EXTMAP       , eSdpAttrExtMap             , _SDPDEC_ExtMapDecoder     },
   { SDP_ATTR_SILENCESUPP  , eSdpAttrSilenceSupp        , _SDPDEC_SilenceSupp       },
   { SDP_ATTR_RTCP                  , eSdpAttrRtcp                  , _SDPDEC_RtcpDecoder                  },
   { SDP_ATTR_T38FAXVERSION         , eSdpAttrT38FaxVersion         , _SDPDEC_T38FaxVersionDecoder         },
   { SDP_ATTR_T38MAXBITRATE         , eSdpAttrT38MaxBitRate         , _SDPDEC_T38MaxBitRateDecoder         },
   { SDP_ATTR_T38FAXFILLBITREMOVAL  , eSdpAttrT38FaxFillBitRemoval  , _SDPDEC_T38FaxFillBitRemovalDecoder  },
   { SDP_ATTR_T38FAXTRANSCODINGMMR  , eSdpAttrT38FaxTranscodingMMR  , _SDPDEC_T38FaxTranscodingMMRDecoder  },
   { SDP_ATTR_T38FAXTRANSCODINGJBIG , eSdpAttrT38FaxTranscodingJBIG , _SDPDEC_T38FaxTranscodingJBIGDecoder },
   { SDP_ATTR_T38FAXRATEMANAGEMENT  , eSdpAttrT38FaxRateManagement  , _SDPDEC_T38FaxRateManagementDecoder  },
   { SDP_ATTR_T38FAXMAXBUFFER       , eSdpAttrT38FaxMaxBuffer       , _SDPDEC_T38FaxMaxBufferDecoder       },
   { SDP_ATTR_T38FAXMAXDATAGRAM     , eSdpAttrT38FaxMaxDatagram     , _SDPDEC_T38FaxMaxDatagramDecoder     },
   { SDP_ATTR_T38FAXUDPEC           , eSdpAttrT38FaxUdpEC           , _SDPDEC_T38FaxUdpECDecoder           },
   { SDP_ATTR_CRYPTO                , eSdpAttrCrypto                , _SDPDEC_CryptoDecoder                },
   { SDP_ATTR_ENCRYPTION            , eSdpAttrEncryption            , _SDPDEC_EncryptionDecoder            },
   { SDP_ATTR_PATH                  , eSdpAttrPath                  , _SDPDEC_PathDecoder                  },
   { SDP_ATTR_ACCEPT_TYPES          , eSdpAttrAcceptTypes           , _SDPDEC_AcceptTypesDecoder           },
   { SDP_ATTR_ACCEPT_WRAPPED_TYPES  , eSdpAttrAcceptWrappedTypes    , _SDPDEC_AcceptWrappedTypesDecoder    },
   { SDP_ATTR_FILE_SELECTOR         , eSdpAttrFileSelector          , _SDPDEC_FileSelectorDecoder          },
   { SDP_ATTR_FILE_DISPOSITION      , eSdpAttrFileDisposition       , _SDPDEC_FileDispositionDecoder       },
   { SDP_ATTR_FILE_TRANSFER_ID      , eSdpAttrFileTransferId        , _SDPDEC_FileTransferIdDecoder        },
   { SDP_ATTR_CURR                  , eSdpAttrCurr                  , _SDPDEC_CurrDecoder                  },
   { SDP_ATTR_DES                   , eSdpAttrDes                   , _SDPDEC_DesDecoder                   },
   { SDP_ATTR_CONF                  , eSdpAttrConf                  , _SDPDEC_ConfDecoder                  },
   { SDP_ATTR_SETUP                 , eSdpAttrSetup                 , _SDPDEC_SetupDecoder                 },
   { SDP_ATTR_FINGERPRINT           , eSdpAttrFingerprint           , _SDPDEC_FingerprintDecoder           },
   { SDP_ATTR_FRAMESIZE             , eSdpAttrFramesize             , _SDPDEC_FramesizeDecoder             },
   { SDP_ATTR_FINGERPRINT           , eSdpAttrFingerprint           , _SDPDEC_FingerprintDecoder           },
   { SDP_ATTR_TCAP                  , eSdpAttrTcap                  , _SDPDEC_TcapDecoder                  },
   { SDP_ATTR_PCFG                  , eSdpAttrPcfg                  , _SDPDEC_PcfgDecoder                  },
   { SDP_ATTR_RTCPFB                , eSdpAttrRtcpfb                , _SDPDEC_RtcpfbDecoder                }
};


/*================================ CVDC_SdpExec ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
vint SDPDEC_Exec(tFSM *pFSM, tL4Packet *pBuff, tSdpMsg *pIntMsg)
{
    tToken *pT;
    uint32     idx;
    uint32     TableSize;

    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "SDPDEC_Exec ", 0, 0, 0);
    pT = &pFSM->CurrToken;
    for(;;)
    {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "SDPDEC_Exec currToken:%X, length=%d.", (int)pT, (int)pT->length, 0);

        /* get next token */
        if ( pFSM->pfGetToken(pFSM, pBuff, "=\r\n}") != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "Failed hSdpMsg:%X", (int)pIntMsg, 0, 0);
            RETURN_ERROR(pFSM, 0);
        }

        /* if the target has reached the end of message then break out.
         * Remember that you must check for the end of packet before you attempt
         * to operate on the pDmtr of the Current Token*/
        if (pFSM->isEndOfPacket) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "SDPDEC_Exec end of packet", 0, 0, 0);
            break;
        }

        /* we have some emty line there - the end of parameters set */
        if ( (ABNF_ISEOL(pT->pDmtr)||ABNF_ISRBRKT(pT->pDmtr[0])) && !pT->length ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "SDPDEC_Exec eol, or bracket", 0, 0, 0);
            break;
        }

        /* fields table lookup */
        TableSize = sizeof(_SdpDecFieldsTable)/sizeof(_SdpDecFieldsTable[0]);
        if ( TOKEN_ExtLookup(pT, _SdpDecFieldsTable, TableSize, &idx) != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "Failed dont understand sdp field %s hSdpMsg:%X", 
                    (int)pIntMsg, 0, 0);
            RETURN_ERROR(pFSM, 0);
        }

        /* set the appropriate to this field bit in presence bit mask */
        SDP_SET_PARM_PRESENT(pIntMsg, _SdpDecFieldsTable[idx].Int);

        pFSM->hCurrBlock = NULL;

        /* check if we have the first mandatory Protocol Version field */
        if ( !SDP_IS_PARM_PRESENT(pIntMsg, eSdpProtVersion) ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "Failed bad SDP protocol version hSdpMsg:%X", 
                    (int)pIntMsg, 0, 0);
            RETURN_ERROR(pFSM, 0);
        }

        /* set the corresponding to this field target */         
        if (NULL == (pFSM->pfHandler = (tpfTokenHndlr)_SdpDecFieldsTable[idx].pfHandler) ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "Failed the field handler is NULL for field:%s hSdpMsg:%X", 
                (int)_SdpDecFieldsTable[idx].pExt, (int)pIntMsg, 0);
            RETURN_ERROR(pFSM, 0);
        }

        /* and launch the target */         
        if ( pFSM->pfHandler(pFSM, pBuff, pIntMsg) != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "Failed Field handler function failed for for field:%s hSdpMsg:%X", 
                (int)_SdpDecFieldsTable[idx].pExt, (int)pIntMsg, 0);
            RETURN_ERROR(pFSM, 0);
        }

        /* if the target has reached the end of message or the piggy-backing */
        if (pFSM->isEndOfPacket) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "SDPDEC_Exec end of packet", 0, 0, 0);
            break;
        }
    }
   
    return SIP_OK;
}
/* !END CVDC_SdpExec */


/*============================== _SDPDEC_ProtVersDecoder =================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_ProtVersDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tSdpMsg    *pIntMsg = (tSdpMsg *)hIntMsg;
   char *pEnd;
   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   /* compare this token with Protocol Version */
   if ( !TOKEN_iCmpToken(&pFSM->CurrToken, SDP_VERSION) )
      RETURN_ERROR(pFSM, 0);

   pIntMsg->version = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
   
   return SIP_OK;
}
/* !END _SDPDEC_ProtVersDecoder */


/*=============================== _SDPDEC_OriginDecoder ==================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_OriginDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   
    tSdpMsg *pSdp = (tSdpMsg*)hIntMsg;
    uint16 cnt = 0;
    tAttribute *pAttr = NULL;
    tSdpOrigin *pO = NULL;
    uint8 loopFlag = 1;
    char *pEnd;
    while(cnt < 6 && loopFlag) {
        if (pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK ) {
            RETURN_ERROR(pFSM, 0);
        }
        
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            if (!pFSM->CurrToken.length)
                break;
            else
                loopFlag = 0;
        }
        
        if (cnt == 0) {
            pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE);
            if (!pAttr) break;
            pAttr->id = eSdpPAttrOrigin;
            pO = &pAttr->value.x.origin;
            if (TOKEN_iCmpToken(&pFSM->CurrToken, "-")) {
                /* peer doesn't support a user id */
                pO->userName[0] = 0;
            }
            else {
                /* this must be a name */
                TOKEN_copyToBuffer(pO->userName, sizeof(pO->userName), &pFSM->CurrToken);
            }
        }
        else if (cnt == 1) {
            pO->sessId = (tSdpSessId)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10); 
        }
        else if (cnt == 2) {
            pO->sessVersion = (tSdpSessVersion)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10); 
        }
        else if (cnt == 3) {
            if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_NETTYPE_IN)) 
                pO->nwType = eNetworkIN;
            else if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_NETTYPE_LOCAL)) 
                pO->nwType = eNetworkLocal;
            else
                pO->nwType = eNetworkATM;
        }
        else if (cnt == 4) {
             if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_ADDRTYPE_IP4))
                 pO->nwAddress.addressType = eNwAddrIPv4;
             else if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_ADDRTYPE_IP6))
                 pO->nwAddress.addressType = eNwAddrIPv6;
             else
                 pO->nwAddress.addressType = eNwAddrIPv4;
        }
        else {
            VoIP_IpAddrExt2Int(pFSM->CurrToken.pStart, pFSM->CurrToken.length, 0, &pO->nwAddress);
        }

        cnt++;
    }

    if (pAttr) {
        SDP_InsertAttr(&pSdp->pParmAttr, pAttr);
    }

    return SIP_OK;
}


/*============================== _SDPDEC_SessNameDecoder =================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_SessNameDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   UNUSED(hIntMsg);

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   return SIP_OK;
}
/* !END _SDPDEC_SessNameDecoder */


/*================================ _SDPDEC_TimeDecoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_TimeDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   UNUSED(hIntMsg);
   
   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   return SIP_OK;
}
/* !END _SDPDEC_TimeDecoder */


/*================================== _SDPDEC_CxDecoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_CxDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg          *pIntMsg = (tSdpMsg *)hIntMsg;
    tSdpConnInfo     *pConnInfo;
    tNetworkAddress  *pNetAddr;
    uint32            idx;
    char              ipStr_ptr[MAX_IPV6_STR_LEN];
    OSAL_NetAddress   addrObj;
    vint              rval;

    if ( !pFSM->hCurrBlock )
        pConnInfo = &pIntMsg->connInfo;
    else
        pConnInfo = &((tSdpMedia *)pFSM->hCurrBlock)->connInfo;

    pNetAddr = &pConnInfo->nwAddress;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        RETURN_ERROR(pFSM, 0);
    }

    /* Network Types table lookup */
    {
        uint32 TableSize = sizeof(_SdpDecNetTypesTable)/sizeof(_SdpDecNetTypesTable[0]);
      
        if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecNetTypesTable, TableSize, &idx) != SIP_OK ) {
            RETURN_ERROR(pFSM, 0);
        }
    }
    /* set the appropriate to this field bit in presence bit mask */
    pConnInfo->nwType = (tNetworkType)_SdpDecNetTypesTable[idx].Int;

    pFSM->hCurrBlock = (tSipHandle)pNetAddr;

    /* set the corresponding to this field target */         
    if (NULL == (pFSM->pfHandler = (tpfTokenHndlr)_SdpDecNetTypesTable[idx].pfHandler) ) {
        OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
        RETURN_ERROR(pFSM, 0);
    }
   
    /* and launch the target */         
    if ( pFSM->pfHandler(pFSM, pBuff, pIntMsg) != SIP_OK ) {
        RETURN_ERROR(pFSM, 0);
    }

    if (eNwAddrIPv6 == pNetAddr->addressType) {
        addrObj.type = OSAL_NET_SOCK_UDP_V6;
        OSAL_netIpv6Hton(addrObj.ipv6, pNetAddr->x.ip.v6);
        rval = OSAL_netAddressToString((int8 *)ipStr_ptr, &addrObj);
        if (OSAL_SUCCESS != rval) {
            /* XXX Need to handle multicast cast? To be done here */
            OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
        }
        else if ((!pFSM->isEndOfPacket) && pFSM->CurrToken.pDmtr[0] == '/' ) {
            OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
            RETURN_ERROR(pFSM, 0);
        }
        return (SIP_OK);
    }

    addrObj.type = OSAL_NET_SOCK_UDP;
    addrObj.ipv4 = OSAL_netHtonl(pNetAddr->x.ip.v4.ul);
    rval = OSAL_netAddressToString((int8 *)ipStr_ptr, &addrObj);
    if ((rval != OSAL_SUCCESS) || (OSAL_strcmp("224.", ipStr_ptr) == 0)) {
         /* Address is 224.x.x.x or invalid */
        if (pFSM->isEndOfPacket || pFSM->CurrToken.pDmtr[0] != '/' ) {
            OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
            RETURN_ERROR(pFSM, 0);
        }
        else {
            char *pEnd;
            /* get next token */
            if (pFSM->pfGetToken(pFSM, pBuff, "/\r\n") != SIP_OK) {
                OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
                RETURN_ERROR(pFSM, 0);
            }
         
            pConnInfo->x.extMCast.ttl = (uint8)OSAL_strtoul(pFSM->CurrToken.pStart,
                     &pEnd, 10);
         
            if ((!pFSM->isEndOfPacket) && pFSM->CurrToken.pDmtr[0] == '/') {
                char *pEnd;
                /* get next token */
                if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK) {
                    OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
                    RETURN_ERROR(pFSM, 0);
                }
            
                pConnInfo->x.extMCast.num =
                        (uint8)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);

            }      
        }
    }
    else if ((!pFSM->isEndOfPacket) && pFSM->CurrToken.pDmtr[0] == '/' ) {
        OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
        RETURN_ERROR(pFSM, 0);
    }

    return SIP_OK;
}
/* !END _SDPDEC_CxDecoder */


/*=============================== _SDPDEC_AddrInDecoder ==================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_AddrInDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tNetworkAddress  *pNetAddr = (tNetworkAddress *)pFSM->hCurrBlock;
   OSAL_NetAddress   addrObj;
   char              str_ptr[MAX_IPV6_STR_LEN + 1];
   UNUSED(hIntMsg);
   
   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   /* compare this token with AddrType expression constant */
   if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_ADDRTYPE_IP6)) {
       pNetAddr->addressType = eNwAddrIPv6;
       addrObj.type = OSAL_NET_SOCK_UDP_V6;
   }
   else if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_ADDRTYPE_IP4)) {
       pNetAddr->addressType = eNwAddrIPv4;
       addrObj.type = OSAL_NET_SOCK_UDP;
   }
   else {
       RETURN_ERROR(pFSM, 0);
   }

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   /* Allow for CHHOSE */
   if ( (*pFSM->CurrToken.pStart=='$') && (pFSM->CurrToken.length==1) )
      return SIP_OK;

   /* Protect from memory overrun */
   if (pFSM->CurrToken.length > (MAX_IPV6_STR_LEN - 1)) {
       return (SIP_FAILED);
   }
   TOKEN_copyToBuffer(str_ptr, sizeof(str_ptr), &pFSM->CurrToken);

   if (OSAL_SUCCESS != OSAL_netStringToAddress((int8 *)str_ptr, &addrObj)) { 
       return (SIP_FAILED);
   }
   
   if (eNwAddrIPv6 == pNetAddr->addressType) {
       OSAL_netIpv6Hton(pNetAddr->x.ip.v6, addrObj.ipv6);
   }
   else {
       pNetAddr->x.ip.v4.ul = OSAL_netHtonl(addrObj.ipv4);
   }

   return SIP_OK;
}
/* !END _SDPDEC_AddrInDecoder */


/*=============================== _SDPDEC_AddrAtmDecoder =================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_AddrAtmDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   UNUSED(pFSM);
   UNUSED(pBuff);
   UNUSED(hIntMsg);

   return SIP_OK;
}
/* !END _SDPDEC_AddrAtmDecoder */


/*============================= _SDPDEC_AddrLocalDecoder =================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_AddrLocalDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tNetworkAddress  *pNetAddr = (tNetworkAddress *)pFSM->hCurrBlock;
   int len;
   
   UNUSED(hIntMsg);

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   /* compare this token with AddrType expression constant */
   if ( !TOKEN_iCmpToken(&pFSM->CurrToken, SDP_ADDRTYPE_EPN) )
      RETURN_ERROR(pFSM, 0);

   pNetAddr->addressType = eNwAddrLocal;

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   len = CALC_MIN(pFSM->CurrToken.length, (MAX_DOMAIN_NAME_LEN - 1));
   
   OSAL_memCpy(pNetAddr->x.epName, pFSM->CurrToken.pStart, len);
   pNetAddr->x.epName[len] = '\0';

   return SIP_OK;
}
/* !END _SDPDEC_AddrLocalDecoder */


/*================================= _SDPDEC_BwDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_BwDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg      *pSdp = (tSdpMsg *)hIntMsg;
    tSdpMedia    *pMedia  = (tSdpMedia *)pFSM->hCurrBlock;
    tSdpBw       *pBw;
    tSdpBw        bwInfo;
    tAttribute   *pBwInfo;
    char         *pEnd;

    pBw = &bwInfo;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, ":") != SIP_OK ) {
        RETURN_ERROR(pFSM, 0);
    }

    /* compare this token with BandwidthType expression constant */
    if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_BW_CT)) {
        pBw->bwModifier = eSdpBwCT;
    }
    else if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_BW_AS)) {
        pBw->bwModifier = eSdpBwAS;
    }
    else if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_BW_RS)) {
        pBw->bwModifier = eSdpBwRS;
    }
    else if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_BW_RR)) {
        pBw->bwModifier = eSdpBwRR;
    }
    else if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_BW_TIAS)) {
        pBw->bwModifier = eSdpBwTIAS;
    }
    else {
        RETURN_ERROR(pFSM, 0);
    }

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK) {
        RETURN_ERROR(pFSM, 0);
    } 
    pBw->bw = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
   
    if (NULL == (pBwInfo = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
        RETURN_ERROR(pFSM, 0);
    }

    pBwInfo->id = eSdpPAttrBwInfo;
    pBwInfo->value.valType = SIP_VAL_DATA;
    pBwInfo->value.x.bwInfo = bwInfo;
    /* Insert the attribute to parameter list. */
    if (!pMedia) {
        /* BW in SDP. */
        SDP_InsertAttr(&pSdp->pParmAttr, pBwInfo);
    }
    else {
        /* BW in media. */
        SDP_InsertAttr(&pMedia->pParmAttr, pBwInfo);
    }

    return (SIP_OK);
}
/* !END _SDPDEC_BwDecoder */

/*=============================== _SDPDEC_MediaDecoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_MediaDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg    *pIntMsg    = (tSdpMsg *)hIntMsg;
    tSdpMedia  *pSdpMedia  = &pIntMsg->media;
    uint32         idx;
    char       *pEnd;
    uint32         TableSize;

    while (pSdpMedia) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_MediaDecoder: hSdpMsg:%X", (int)pIntMsg, 0, 0);
   
        /* get next token */
        if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
           SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed getting token for hSdpMsg:%X currToken:%c", 
               (int)pIntMsg, (int)(*pBuff->pCurr), 0);
           RETURN_ERROR(pFSM, 0);
        }

        /* MediaTypes table lookup */
        TableSize = sizeof(_SdpDecMediaTypesTable)/sizeof(_SdpDecMediaTypesTable[0]);
      
        if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecMediaTypesTable, TableSize, &idx) != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed dont understand media type hSdpMsg:%X", 
                    (int)pIntMsg, 0, 0);
            RETURN_ERROR(pFSM, 0);
        }
        
        pSdpMedia->mediaType = (tSdpMediaType)_SdpDecMediaTypesTable[idx].Int;

        /* get next token */
        if ( pFSM->pfGetToken(pFSM, pBuff, " /") != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed getting token or hSdpMsg:%X", 
                  (int)pIntMsg, 0, 0);
            RETURN_ERROR(pFSM, 0);
        }

        pSdpMedia->port = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10); 

        /* Convert port allowing for CHOOSE */
        if ( !((*pFSM->CurrToken.pStart=='$') && (pFSM->CurrToken.length==1)) && (pEnd == pFSM->CurrToken.pStart)) {
           SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed handling '$' choose logic for hSdpMsg:%X", 
                   (int)pIntMsg, 0, 0);
           RETURN_ERROR(pFSM, 0);
        }

        if ((!pFSM->isEndOfPacket) && pFSM->CurrToken.pDmtr[0] == '/' ) {
            /* get next token */
            if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
                SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed getting token for hSdpMsg:%X currToken:%c", 
                        (int)pIntMsg, (int)(*pBuff->pCurr), 0);
                RETURN_ERROR(pFSM, 0);
            }
            pSdpMedia->nPorts = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        }

        /* get next token */
        if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed getting token for hSdpMsg:%X currToken:%c", 
                    (int)pIntMsg, (int)(*pBuff->pCurr), 0);
            RETURN_ERROR(pFSM, 0);
        }
        /* TransportTypes table lookup */
        TableSize = sizeof(_SdpDecTpTypesTable)/sizeof(_SdpDecTpTypesTable[0]);
        if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecTpTypesTable, TableSize, &idx) != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed dont understand transport type hSdpMsg:%X", 
                    (int)pIntMsg, 0, 0);
            RETURN_ERROR(pFSM, 0);
        }
        
        pSdpMedia->transpType = (tTransportType)_SdpDecTpTypesTable[idx].Int;

        if ((!pFSM->isEndOfPacket) && pFSM->CurrToken.pDmtr[0] == ' ' ) {
            for(;;) {
                /* get next token */
                if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK ) {
                    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed getting token for hSdpMsg:%X currToken:%c", 
                            (int)pIntMsg, (int)(*pBuff->pCurr), 0);
                    RETURN_ERROR(pFSM, 0);
                }

                if (!pFSM->CurrToken.length)
                    break;

                if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_NULL_STR) == TRUE) {
                    break;
                }

                if ( (pSdpMedia->nFormats + 1) > MAX_SDP_FORMATS ) {
                    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: Failed exceeded max media coder types for hSdpMsg:%X", 
                            (int)pIntMsg, 0, 0);
                    RETURN_ERROR(pFSM, 0);
                }

                pSdpMedia->formats[pSdpMedia->nFormats] = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
         
                pSdpMedia->nFormats++;
         
                if (pFSM->isEndOfPacket || pFSM->CurrToken.pDmtr[0] != ' ' )
                    break;
            }
        }

        if (!pFSM->isEndOfPacket) {
            pFSM->hCurrBlock = (tSipHandle)pSdpMedia;     
            if ( _SDPDEC_MediaSetDecoder(pFSM, pBuff, hIntMsg, &pSdpMedia) != SIP_OK ) {
                SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaDecoder: _SDPDEC_MediaSetDecoder Failed for hSdpMsg:%X", 
                        (int)pIntMsg, 0, 0);
                RETURN_ERROR(pFSM, 0);
            }
            else {
                if (pSdpMedia) {
                    /* then there are more media sets */
                    pSdpMedia->next = (tSdpMedia *)SIP_memPoolAlloc(eSIP_OBJECT_SDP_MEDIA);
                    pSdpMedia = pSdpMedia->next;
                }
            }
        }
        else {
            break; /* out of the pSdpMedia loop */
        }
    } /* end of while */        
    return SIP_OK;
}

/*=============================== _SDPDEC_MediaSetDecoder ================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_MediaSetDecoder(
    tFSM       *pFSM, 
    tL4Packet  *pBuff, 
    tSipHandle  hIntMsg, 
    tSdpMedia **ppSdpMedia)
{
    tSdpMedia *pSdpMedia  = *ppSdpMedia;
    tToken    *pT         = &pFSM->CurrToken;
    uint32        idx;
    uint32        TableSize;

    /* assume that this is the last media set */
    *ppSdpMedia = NULL;
   
    if (!pSdpMedia) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaSetDecoder: Failed pSdpMedia was NULL", 0, 0, 0);
        return (SIP_BADPARM);
    }

    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_MediaSetDecoder: hMediaMsg:%X", (int)pSdpMedia, 0, 0);

    for(;;) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_MediaSetDecoder: next token", 0, 0, 0);

        /* get next token */
        if ( pFSM->pfGetToken(pFSM, pBuff, "=\r\n}") != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaSetDecoder: Failed getting token for hMediaMsg:%X currToken:%c", 
                    (int)pSdpMedia, (int)(*pBuff->pCurr), 0);
            RETURN_ERROR(pFSM, 0);
        }

        if (pFSM->isEndOfPacket) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "SDPDEC_Exec end of packet", 0, 0, 0);
            break;
        }

        /* we have some emty line there - the end of parameters set */
        if ( (ABNF_ISEOL(pT->pDmtr)||ABNF_ISRBRKT(pT->pDmtr[0])) && !pFSM->CurrToken.length ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "SDPDEC_Exec eol, or bracket, or length=0", 0, 0, 0);
            break;
        }

        /* if we have the next Media Descriptor - the end of the current one */
        if ( TOKEN_iCmpToken(&pFSM->CurrToken, SDP_FIELD_MEDIA) ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "SDPDEC_Exec not media field", 0, 0, 0);
            *ppSdpMedia = pSdpMedia;
            break;
        }

        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_MediaSetDecoder: token:%s", (int)pFSM->CurrToken.pStart, 0, 0);

        /* fields table lookup */
        TableSize = sizeof(_SdpDecMediaSetTable)/sizeof(_SdpDecMediaSetTable[0]);
        if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecMediaSetTable, TableSize, &idx) != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaSetDecoder: Failed media set doesn't exist hMediaMsg:%X", 
                   (int)pSdpMedia, 0, 0);
            RETURN_ERROR(pFSM, 0);
        }
      
        /* set the appropriate to this field bit in presence bit mask */
        SDP_SET_PARM_PRESENT(pSdpMedia, _SdpDecMediaSetTable[idx].Int);

        pFSM->hCurrBlock = (tSipHandle)pSdpMedia;

        /* set the corresponding to this field target */         
        if (NULL == (pFSM->pfHandler = (tpfTokenHndlr)_SdpDecMediaSetTable[idx].pfHandler) ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaSetDecoder: Failed handler for media set:%s is NULL for hMediaMsg:%X", 
                    (int)_SdpDecMediaSetTable[idx].pExt, (int)pSdpMedia, 0);
            RETURN_ERROR(pFSM, 0);
        }

        /* and launch the target */         
        if ( pFSM->pfHandler(pFSM, pBuff, hIntMsg) != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_MediaSetDecoder: Failed handler for media set:%s FAILED for hMediaMsg:%X", 
                    (int)_SdpDecMediaSetTable[idx].pExt, (int)pSdpMedia, 0);
            RETURN_ERROR(pFSM, 0);
        }
      
        /* if the target has reached the end of message or the piggy-backing */
        if (pFSM->isEndOfPacket)
            break;
    } /* end of for loop */
    return SIP_OK;
}
/* !END _SDPDEC_MediaSetDecoder */


/*================================ _SDPDEC_AttrDecoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_AttrDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg          *pIntMsg = (tSdpMsg *)hIntMsg;
    tSdpMedia        *pMedia  = (tSdpMedia *)pFSM->hCurrBlock;
    tAttribute       *pAttr, *pLastAttr = NULL;
    uint32               idx;
    uint32               TableSize;

    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_AttrDecoder: hMediaMsg:%X", (int)pMedia, 0, 0);
   
    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, ":\r\n") != SIP_OK )
        RETURN_ERROR(pFSM, 0);

    /* SDP Attributes table lookup */
    TableSize = sizeof(_SdpDecAttrSetTable)/sizeof(_SdpDecAttrSetTable[0]);
    /* Look up attribute and ignore unknown */
    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecAttrSetTable, TableSize, &idx) != SIP_OK ) {
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            return (SIP_OK);
        }
        return _SDPDEC_Skip( pFSM, pBuff, hIntMsg );
    }
    
    /* allocate new Attribute */         
    if (NULL == (pAttr = (tAttribute *)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE))) {
        RETURN_ERROR(pFSM, 0);
    }

    if ( pMedia ) {
        if ( !pMedia->pAttr ) 
            pMedia->pAttr = pAttr;
        else
            pLastAttr = pMedia->pAttr;
    }
    else {
        if ( !pIntMsg->pAttr )
            pIntMsg->pAttr = pAttr;
        else
            pLastAttr = pIntMsg->pAttr;
    }      

    if ( pLastAttr ) {
        while ( pLastAttr->next )
            pLastAttr = pLastAttr->next;
        pLastAttr->next = pAttr;
    }

    pAttr->id = _SdpDecAttrSetTable[idx].Int;

    /* set current decoding block */
    pFSM->hCurrBlock = (tSipHandle)pAttr;

    /* set the corresponding to this field target, if it's NULL then it's a no-op */         
    if (NULL != (pFSM->pfHandler = (tpfTokenHndlr)_SdpDecAttrSetTable[idx].pfHandler)) {
        /* and launch the target */         
        if ( pFSM->pfHandler(pFSM, pBuff, hIntMsg) != SIP_OK )
            RETURN_ERROR(pFSM, 0);
    }
   
    return SIP_OK;
}
/* !END _SDPDEC_AttrDecoder */
static vint _SDPDEC_FramesizeDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute       *pAttr = (tAttribute *)pFSM->hCurrBlock;
    tSdpAttrFramesize   *pFramesize;
    char *pEnd;

    UNUSED(hIntMsg);
    
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_RtpMapDecoder: hSdpMsg:%X hAttr:%X", (int)hIntMsg, (int)pAttr, 0);

    /* allocate new framesize value */
    pAttr->value.valType = SIP_VAL_DATA;
    pFramesize = &pAttr->value.x.framesize;
   
    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK )
        RETURN_ERROR(pFSM, 0);

    pFramesize->payloadType = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);


   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, "-") != SIP_OK )
       RETURN_ERROR(pFSM, 0);
   
   pFramesize->width = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, ",\r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   pFramesize->height = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
   
   return SIP_OK;
}
/*================================= _SDPDEC_FramerateDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_FramerateDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_FramerateDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        pAttr->value.x.uparm = 
            (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, NULL, 10);
    }
    return SIP_OK;
}


/*============================== _SDPDEC_RtpMapDecoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_RtpMapDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute       *pAttr = (tAttribute *)pFSM->hCurrBlock;
    tSdpAttrRtpMap   *pRtpMap;
    char *pEnd;

    UNUSED(hIntMsg);
    
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_RtpMapDecoder: hSdpMsg:%X hAttr:%X", (int)hIntMsg, (int)pAttr, 0);

    /* allocate new RtpMap value */
    pAttr->value.valType = SIP_VAL_DATA;
    pRtpMap = &pAttr->value.x.rtpMap;
   
    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK )
        RETURN_ERROR(pFSM, 0);

    pRtpMap->payloadType = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
   
    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, "/") != SIP_OK )
        RETURN_ERROR(pFSM, 0);

    /* fill Encoding string */
    TOKEN_copyToBuffer(pRtpMap->szEncodingName,
            sizeof(pRtpMap->szEncodingName), &pFSM->CurrToken);

    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_RtpMapDecoder: we have a %s Audio coder type -hSdpMsg:%X hAttr:%X", 
                    (int)pRtpMap->szEncodingName, (int)hIntMsg, (int)pAttr);

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, "/\r\n") != SIP_OK )
        RETURN_ERROR(pFSM, 0);

    pRtpMap->clockRate = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);

    if (!pFSM->isEndOfPacket) {
        if (pFSM->CurrToken.pDmtr[0] == '/' ) {
            /* get next token */
            if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK )
                RETURN_ERROR(pFSM, 0);
      
            pRtpMap->numChan = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);

        }
    }
    return SIP_OK;
}

/*================================= _SDPDEC_T38xxDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_T38FaxVersionDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16       num;
   char        *pEnd;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
  
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)num;

   return SIP_OK;
}

static vint _SDPDEC_T38MaxBitRateDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16       num;
   char        *pEnd;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
  
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)num;

   return SIP_OK;
}

static vint _SDPDEC_T38FaxFillBitRemovalDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16       num;
   char        *pEnd;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
  
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)num;

   return SIP_OK;
}

static vint _SDPDEC_T38FaxTranscodingMMRDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16       num;
   char        *pEnd;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
  
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)num;

   return SIP_OK;
}

static vint _SDPDEC_T38FaxTranscodingJBIGDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16       num;
   char        *pEnd;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
  
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)num;

   return SIP_OK;
}

static vint _SDPDEC_T38FaxRateManagementDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   TOKEN_copyToBuffer(pAttr->value.x.szStr,
               sizeof(pAttr->value.x.szStr), &pFSM->CurrToken);
   
   pAttr->value.valType = SIP_VAL_DATA;

   return SIP_OK;
}

static vint _SDPDEC_T38FaxMaxBufferDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16       num;
   char        *pEnd;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
  
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)num;

   return SIP_OK;
}

static vint _SDPDEC_T38FaxMaxDatagramDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16       num;
   char        *pEnd;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
  
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)num;

   return SIP_OK;
}

static vint _SDPDEC_T38FaxUdpECDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

   UNUSED( hIntMsg );

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);

   TOKEN_copyToBuffer(pAttr->value.x.szStr,
           sizeof(pAttr->value.x.szStr), &pFSM->CurrToken);
   
   pAttr->value.valType = SIP_VAL_DATA;

   return SIP_OK;
}

static vint _SDPDEC_CryptoDecoder(
        tFSM      *pFSM,
        tL4Packet *pBuff,
        tSipHandle hIntMsg)
{
    tAttribute      *pAttr   = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue       *pValue  = &pAttr->value;
    tSdpAttrCrypto  *pCrypto = &(pValue->x.crypto);
    char            *pEnd;

    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_CryptoDecoder:\n",0,0,0);  

    UNUSED(hIntMsg);

    pAttr->value.valType = SIP_VAL_DATA;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        RETURN_ERROR(pFSM, 0);
    }
    pCrypto->tag = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        RETURN_ERROR(pFSM, 0);
    }
    TOKEN_copyToBuffer(pCrypto->cryptoSuite, sizeof(pCrypto->cryptoSuite),
            &pFSM->CurrToken);

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, ":") != SIP_OK ) {
        RETURN_ERROR(pFSM, 0);
    }
            
    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK ) {
        RETURN_ERROR(pFSM, 0);
    }
    
    if (0 == OSAL_strncmp(pCrypto->cryptoSuite,"AES_CM_128_HMAC_SHA1_80",23)) {
        TOKEN_copyToBuffer(pCrypto->keyParamsAes80, sizeof(pCrypto->keyParamsAes80),
                &pFSM->CurrToken);
    }
    
    else if (0 == OSAL_strncmp(pCrypto->cryptoSuite,"AES_CM_128_HMAC_SHA1_32",23)) {
        TOKEN_copyToBuffer(pCrypto->keyParamsAes32, sizeof(pCrypto->keyParamsAes32),
                &pFSM->CurrToken);
    }
    
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "crypto.tag=%d\n",
            (int)pCrypto->tag,0,0);
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "crypto.cryptoSuite=%s\n",
            (int)pCrypto->cryptoSuite, 0, 0);
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "crypto.keyParamsAes80=%s\n",
            (int)pCrypto->keyParamsAes80, 0, 0);
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "crypto.keyParamsAes32=%s\n",
            (int)pCrypto->keyParamsAes32, 0, 0);

    return SIP_OK;
}

static vint _SDPDEC_EncryptionDecoder(
        tFSM      *pFSM,
        tL4Packet *pBuff,
        tSipHandle hIntMsg)
{
    tAttribute          *pAttr       = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue           *pValue      = &pAttr->value;
    tSdpAttrEncryption  *pEncryption = &(pValue->x.encryption);

    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_EncryptionDecoder:\n",0,0,0);  

    UNUSED(hIntMsg);

    pAttr->value.valType = SIP_VAL_DATA;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK ) {
        RETURN_ERROR(pFSM, 0);
    }
    
    TOKEN_copyToBuffer(pEncryption->status, sizeof(pEncryption->status),
                    &pFSM->CurrToken);

    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "encryption.status=%d\n",
            (int)pEncryption->status, 0, 0);

    return SIP_OK;
}
/*================================= _SDPDEC_RtcpDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_RtcpDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16       port;
   char        *pEnd;

   UNUSED( hIntMsg );
   SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
           "_SDPDEC_RtcpDecoder: hSdpMsg:%X hAttr:%X",
           (int)hIntMsg, (int)pAttr, 0);

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   port = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
  
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)port;   

   return SIP_OK;
}

/*================================= _SDPDEC_PTimeDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_PTimeDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
   uint16           ptime;
   char *pEnd;

   UNUSED( hIntMsg );
   
   SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
           "_SDPDEC_PTimeDecoder: hSdpMsg:%X hAttr:%X",
           (int)hIntMsg, (int)pAttr, 0);

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, ",\r\n") != SIP_OK )
      RETURN_ERROR(pFSM, 0);
   
   ptime = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
   
   pAttr->value.valType = SIP_VAL_DATA;
   pAttr->value.x.uparm = (uint32)ptime;   

   return SIP_OK;
}

/*================================= _SDPDEC_FmtpDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_FmtpDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
    char        *pEnd;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_FmtpDecoder: hSdpMsg:%X hAttr:%X", (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, " \r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            pAttr->value.x.fmtp.payloadType = (uint8)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            /* looks like no argument string ! */
            pAttr->value.x.fmtp.szArg[0] = 0;
            return (SIP_OK);
        }
        else if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.fmtp.szArg,
                    sizeof(pAttr->value.x.fmtp.szArg), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.fmtp.szArg);
        }
    }
    return SIP_OK;
}

/*================================= _SDPDEC_ExtMapDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_ExtMapDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
    char        *pEnd;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_ExtMapDecoder: hSdpMsg:%X hAttr:%X", (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, " \r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            pAttr->value.x.extMap.extmapId = (uint8)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            /* looks like no argument string ! */
            pAttr->value.x.extMap.extmapUri[0] = 0;
            return (SIP_OK);
        }
        else if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.extMap.extmapUri,
                    sizeof(pAttr->value.x.extMap.extmapUri), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.extMap.extmapUri);
        }
    }
    return SIP_OK;
}

/*================================= _SDPDEC_PathDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_PathDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_PathDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.szStr,
                    sizeof(pAttr->value.x.szStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
        }
    }
    return SIP_OK;
}

/*================================= _SDPDEC_AcceptTypesDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_AcceptTypesDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_AcceptTypesDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.szStr,
                    sizeof(pAttr->value.x.szStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
        }
    }
    return SIP_OK;
}

/*================================= _SDPDEC_AcceptWrappedTypesDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_AcceptWrappedTypesDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_AcceptWrappedTypesDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.szStr,
                    sizeof(pAttr->value.x.szStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
        }
    }
    return SIP_OK;
}



/*================================= _SDPDEC_FileSelectorDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_FileSelectorDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_FileSelectorDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_FileSelectorDecoder: not yet implemented",
            0, 0, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.szStr,
                    sizeof(pAttr->value.x.szStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
        }
    }
    return SIP_OK;
}



/*================================= _SDPDEC_FileDispositionDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_FileDispositionDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_FileDispositionDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.szStr,
                    sizeof(pAttr->value.x.szStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
        }
    }
    return SIP_OK;
}



/*================================= _SDPDEC_FileTransferIdDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_FileTransferIdDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_FileTransferIdDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.szStr,
                    sizeof(pAttr->value.x.szStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.szStr);
        }
    }
    return SIP_OK;
}



/*================================= _SDPDEC_SetupDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_SetupDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute          *pAttr = (tAttribute *)pFSM->hCurrBlock;
    tSdpAttrSetup       *pSetup;
    uint32               idx;
    uint32               TableSize;

    UNUSED(hIntMsg);

    /* allocate new Precondition value */
    pAttr->value.valType = SIP_VAL_DATA;
    pSetup = &pAttr->value.x.setup;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_SetupDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }

    /* Hash table lookup */
    TableSize = sizeof(_SdpDecSetupRoleTable)/sizeof(_SdpDecSetupRoleTable[0]);

    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecSetupRoleTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_SetupDecoder: Failed dont understand hash hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pSetup->role = (tSdpSetupRole)_SdpDecSetupRoleTable[idx].Int;

    return (SIP_OK);
}

/*================================= _SDPDEC_FingerprintDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_FingerprintDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute          *pAttr = (tAttribute *)pFSM->hCurrBlock;
    tSdpAttrFingerprint *pFingerprint;
    uint32               idx;
    uint32               TableSize;

    UNUSED(hIntMsg);

    /* allocate new Precondition value */
    pAttr->value.valType = SIP_VAL_DATA;
    pFingerprint = &pAttr->value.x.fingerprint;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_FingerprintDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }

    /* Hash table lookup */
    TableSize = sizeof(_SdpDecHashTable)/sizeof(_SdpDecHashTable[0]);

    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecHashTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_FingerprintDecoder:: Failed dont understand hash hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pFingerprint->hash = (tSdpHash)_SdpDecHashTable[idx].Int;
    
    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_FingerprintDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }

    /* Fingerprint */
    TOKEN_copyToBuffer(pFingerprint->fingerprint,
            sizeof(pFingerprint->fingerprint), &pFSM->CurrToken);
    return SIP_OK;
}

/*================================= _SDPDEC_TCapDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_TcapDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
    char        *pEnd;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_TCapDecoder: hSdpMsg:%X hAttr:%X", (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, " \r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            pAttr->value.x.tcap.tcapId = (uint8)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            /* looks like no argument string ! */
            pAttr->value.x.tcap.tcapStr[0] = 0;
            return (SIP_OK);
        }
        else if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.tcap.tcapStr,
                    sizeof(pAttr->value.x.tcap.tcapStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.tcap.tcapStr);
        }
    }
    return SIP_OK;
}

/*================================= _SDPDEC_PCfgDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_PcfgDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
    char        *pEnd;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_PCfgDecoder: hSdpMsg:%X hAttr:%X", (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, " \r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            pAttr->value.x.pcfg.pcfgId = (uint8)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            /* looks like no argument string ! */
            pAttr->value.x.pcfg.pcfgStr[0] = 0;
            return (SIP_OK);
        }
        else if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.pcfg.pcfgStr,
                    sizeof(pAttr->value.x.pcfg.pcfgStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.pcfg.pcfgStr);
        }
    }
    return SIP_OK;
}

/*================================= _SDPDEC_RtcpFbDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_RtcpfbDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
    char        *pEnd;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_RtcpFbDecoder: hSdpMsg:%X hAttr:%X", (int)hIntMsg, (int)pAttr, 0);

    /* get next token */
    if (pFSM->pfGetToken(pFSM, pBuff, " \r\n") == SIP_OK) {
        pAttr->value.valType = SIP_VAL_DATA;
        if (pFSM->CurrToken.length != 0) {
            pAttr->value.x.rtcpfb.rtcpfbId = (uint8)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            /* looks like no argument string ! */
            pAttr->value.x.rtcpfb.rtcpfbStr[0] = 0;
            return (SIP_OK);
        }
        else if (pFSM->pfGetToken(pFSM, pBuff, "\r\n") == SIP_OK) {
            /* then there's an argument */
            TOKEN_copyToBuffer(pAttr->value.x.rtcpfb.rtcpfbStr,
                    sizeof(pAttr->value.x.rtcpfb.rtcpfbStr), &pFSM->CurrToken);
            pAttr->value.valLength = OSAL_strlen(pAttr->value.x.rtcpfb.rtcpfbStr);
        }
    }
    return SIP_OK;
}
/*================================= _SDPDEC_CurrDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_CurrDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute           *pAttr = (tAttribute *)pFSM->hCurrBlock;
    tSdpAttrPrecondition *pPrecondition;
    uint32     idx;
    uint32     TableSize;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_CurrDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);

    /* allocate new Precondition value */
    pAttr->value.valType = SIP_VAL_DATA;
    pPrecondition = &pAttr->value.x.precondition;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }

    /* Precondition type table lookup */
    TableSize = sizeof(_SdpDecPrecTypeTable)/sizeof(_SdpDecPrecTypeTable[0]);

    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecPrecTypeTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder:: Failed dont understand media type hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pPrecondition->type = (tPrecType)_SdpDecPrecTypeTable[idx].Int;
    
    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }

    /* Precondition status type table lookup */
    TableSize = sizeof(_SdpDecPrecStatusTypeTable)/sizeof(_SdpDecPrecStatusTypeTable[0]);

    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecPrecStatusTypeTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder:: Failed dont understand media type hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pPrecondition->statusType = (tPrecStatusType)_SdpDecPrecStatusTypeTable[idx].Int;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }       

    /* Precondition dir table lookup */
    TableSize = sizeof(_SdpDecPrecDirTable)/sizeof(_SdpDecPrecDirTable[0]);
        
    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecPrecDirTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder:: Failed dont understand media type hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pPrecondition->dir = (tPrecDir)_SdpDecPrecDirTable[idx].Int;

    return SIP_OK;
}

/*================================= _SDPDEC_DesDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_DesDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute           *pAttr = (tAttribute *)pFSM->hCurrBlock;
    tSdpAttrPrecondition *pPrecondition;
    uint32                idx;
    uint32                TableSize;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPDEC_CurrDecoder: hSdpMsg:%X hAttr:%X",
            (int)hIntMsg, (int)pAttr, 0);

    /* allocate new Precondition value */
    pAttr->value.valType = SIP_VAL_DATA;
    pPrecondition = &pAttr->value.x.precondition;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }

    /* Precondition type table lookup */
    TableSize = sizeof(_SdpDecPrecTypeTable)/sizeof(_SdpDecPrecTypeTable[0]);

    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecPrecTypeTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder:: Failed dont understand media type hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pPrecondition->type = (tPrecType)_SdpDecPrecTypeTable[idx].Int;
    
    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }

    /* Precondition type table lookup */
    TableSize = sizeof(_SdpDecPrecStrengthTable)/sizeof(_SdpDecPrecStrengthTable[0]);

    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecPrecStrengthTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder:: Failed dont understand media type hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pPrecondition->strength = (tPrecStrength)_SdpDecPrecStrengthTable[idx].Int;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " ") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }

    /* Precondition status type table lookup */
    TableSize = sizeof(_SdpDecPrecStatusTypeTable)/sizeof(_SdpDecPrecStatusTypeTable[0]);

    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecPrecStatusTypeTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder:: Failed dont understand media type hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pPrecondition->statusType = (tPrecStatusType)_SdpDecPrecStatusTypeTable[idx].Int;

    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder: Failed getting token for hSdpMsg:%X currToken:%c",
            (int)hIntMsg, (int)(*pBuff->pCurr), 0);
        RETURN_ERROR(pFSM, 0);
    }       

    /* Precondition dir table lookup */
    TableSize = sizeof(_SdpDecPrecDirTable)/sizeof(_SdpDecPrecDirTable[0]);
        
    if ( TOKEN_ExtLookup(&pFSM->CurrToken, _SdpDecPrecDirTable, TableSize, &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_DEC_LVL_1, "_SDPDEC_CurrDecoder:: Failed dont understand media type hSdpMsg:%X",
                (int)hIntMsg, 0, 0);
        RETURN_ERROR(pFSM, 0);
    }

    pPrecondition->dir = (tPrecDir)_SdpDecPrecDirTable[idx].Int;

    return SIP_OK;
}

/*================================= _SDPDEC_ConfDecoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_ConfDecoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    return _SDPDEC_CurrDecoder(pFSM, pBuff, hIntMsg);
}
/*==================================== _SDPDEC_SilenceSupp ======================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_SilenceSupp(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr = (tAttribute *)pFSM->hCurrBlock;
    vint         isOn;

    UNUSED( hIntMsg );
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_SilenceSupp: hSdpMsg:%X hAttr:%X", (int)hIntMsg, (int)pAttr, 0);

    isOn = FALSE;
    /* get next token */
    if ( pFSM->pfGetToken(pFSM, pBuff, " \r\n") == SIP_OK ) {
        if (TOKEN_iCmpToken(&pFSM->CurrToken, SDP_ON_STR)) {
            isOn = TRUE;
        }
        /* make sure you have the end of the line */
        if (!pFSM->isEndOfPacket && !ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            pFSM->pfGetToken(pFSM, pBuff, "\r\n");
        }
    }
    pAttr->value.valType = SIP_VAL_DATA;
    pAttr->value.x.uparm = (uint32)isOn;
    return (SIP_OK);
}

/*==================================== _SDPDEC_Skip ======================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPDEC_Skip(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   UNUSED( hIntMsg );
   SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_Skip: hSdpMsg:%X hAttr:%X", (int)hIntMsg, 0, 0);

   /* get next token */
   if ( pFSM->pfGetToken(pFSM, pBuff, "\r\n") != SIP_OK && !pFSM->isEndOfPacket) {
      SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3, "_SDPDEC_Skip: failed...hSdpMsg:%X hAttr:%X", (int)hIntMsg, 0, 0);
      RETURN_ERROR(pFSM, 0);
   } 
   return SIP_OK;
}

/* END sdpcvdc.c */
