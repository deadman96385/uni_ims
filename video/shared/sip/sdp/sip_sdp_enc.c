/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 */
#include "sip_cfg.h"
#include "sip_types.h"
#include "sip_sip_const.h"
#include "sip_voipnet.h"
#include "sip_sip.h"
#include "sip_abnfcore.h"
#include "sip_token.h"
#include "sip_clib.h"
#include "sip_sdp_msg.h"
#include "sip_sdp_enc.h"

#include "_sip_sdp_syntax.h"

#define TABLE_SIZE(x) sizeof((x))/sizeof((x)[0])
#define SDP_MAX_BASETEN_NUM_STRING  (16)

/* internal targets for SDP Message encoding */
static vint _SDPENC_ProtVersEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_OriginEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_SessNameEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

static vint _SDPENC_CxEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_AddrInEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_AddrAtmEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_AddrLocalEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_BwEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_TimeEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

static vint _SDPENC_AttrEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_RtpMapEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_PTimeEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_FmtpEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_ExtMapEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_RtcpEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38FaxVersionEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38MaxBitRateEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38FaxFillBitRemovalEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38FaxTranscodingMMREncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38FaxTranscodingJBIGEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38FaxRateManagementEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38FaxMaxBufferEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38FaxMaxDatagramEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_T38FaxUdpECEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_CryptoEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_EncryptionEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_PathEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_AcceptTypesEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_AcceptWrappedTypesEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_SilenceSuppEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_MediaEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_MediaSetEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_CurrEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_DesEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_ConfEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_FileSelectorEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
//static vint _SDPENC_FileSubjectEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_FileDispositionEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_FileTransferIdEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_SetupEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_FingerprintEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_FrameSizeEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_TcapEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_PcfgEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _SDPENC_RtcpfbEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

static vint _SDPENC_SkipENC(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

static const tTokenizer  _SdpEncFieldsTable[] = 
{
   { SDP_FIELD_PVERSION    , eSdpProtVersion , _SDPENC_ProtVersEncoder }, 
   { SDP_FIELD_ORIGIN      , eSdpOrigin      , _SDPENC_OriginEncoder   }, 
   { SDP_FIELD_SESSNAME    , eSdpName        , _SDPENC_SessNameEncoder }, 
   { SDP_FIELD_INFO        , eSdpInfo        , _SDPENC_SkipENC         }, 
   { SDP_FIELD_URI         , eSdpURI         , _SDPENC_SkipENC         }, 
   { SDP_FIELD_EMAIL       , eSdpEMail       , _SDPENC_SkipENC         }, 
   { SDP_FIELD_PHONE       , eSdpPhone       , _SDPENC_SkipENC         },
   { SDP_FIELD_CONN        , eSdpConnInfo    , _SDPENC_CxEncoder       },
   { SDP_FIELD_BANDWIDTH   , eSdpBWInfo      , _SDPENC_BwEncoder       },
   { SDP_FIELD_TIME        , eSdpTime        , _SDPENC_TimeEncoder     },
   { SDP_FIELD_KEY         , eSdpEncriptKey  , _SDPENC_SkipENC         },
   { SDP_FIELD_ATTR        , eSdpAttr        , _SDPENC_AttrEncoder     },
   { SDP_FIELD_MEDIA       , eSdpMedia       , _SDPENC_MediaEncoder    }
};

static const tTokenizer  _SdpEncNetTypesTable[] =
{
   { SDP_NETTYPE_IN        , eNetworkIN    , _SDPENC_AddrInEncoder     },
   { SDP_NETTYPE_ATM       , eNetworkATM   , _SDPENC_AddrAtmEncoder    },
   { SDP_NETTYPE_LOCAL     , eNetworkLocal , _SDPENC_AddrLocalEncoder  }
};

static const tTokenizer  _SdpEncMediaTypesTable[] = 
{
   { SDP_MEDIA_AUDIO       , eSdpMediaAudio       , 0   },
   { SDP_MEDIA_IMAGE       , eSdpMediaImage       , 0   }, 
   { SDP_MEDIA_VIDEO       , eSdpMediaVideo       , 0   }, 
   { SDP_MEDIA_APPL        , eSdpMediaApplication , 0   },
   { SDP_MEDIA_MSG         , eSdpMediaMsMessage   , 0   }
};

static tTokenizer _SdpEncPrecTypeTable[] =
{
   { SDP_PREC_QOS         , ePrecTypeQos         , 0    }
};

static tTokenizer _SdpEncPrecStatusTypeTable[] =
{
   { SDP_PREC_E2E         , ePrecStatusTypeE2e   , 0    },
   { SDP_PREC_LOCAL       , ePrecStatusTypeLocal , 0    },
   { SDP_PREC_REMOTE      , ePrecStatusTypeRemote, 0    }
};

static tTokenizer _SdpEncPrecDirTable[] =
{
   { SDP_PREC_NONE        , ePrecDirNone         , 0    },
   { SDP_PREC_SEND        , ePrecDirSend         , 0    },
   { SDP_PREC_RECV        , ePrecDirRecv         , 0    },
   { SDP_PREC_SENDRECV    , ePrecDirSendRecv     , 0    }
};

static tTokenizer _SdpEncPrecStrengthTable[] =
{
   { SDP_PREC_MANDATORY   , ePrecStrengthMandatory, 0    },
   { SDP_PREC_OPTIONAL    , ePrecStrengthOptional , 0    },
   { SDP_PREC_NONE        , ePrecStrengthNone     , 0    },
   { SDP_PREC_FAILURE     , ePrecStrengthFailure  , 0    },
   { SDP_PREC_UNKNOWN     , ePrecStrengthUnknown  , 0    }
};

static tTokenizer _SdpEncHashTable[] =
{
   { SDP_HASH_SHA1        , eSdpHashSha1   , 0    },
   { SDP_HASH_SHA224      , eSdpHashSha224 , 0    },
   { SDP_HASH_SHA256      , eSdpHashSha256 , 0    },
   { SDP_HASH_SHA384      , eSdpHashSha384 , 0    },
   { SDP_HASH_SHA512      , eSdpHashSha512 , 0    },
   { SDP_HASH_MD5         , eSdpHashMd5    , 0    },
   { SDP_HASH_MD2         , eSdpHashMd2    , 0    }
};

static const tTokenizer  _SdpEncTpTypesTable[] = 
{
   { SDP_TP_LOCAL          , eTransportLocal   , 0   }, 
   { SDP_TP_RTPAVP         , eTransportRtpAvp  , 0   },
   { SDP_TP_RTPSAVP        , eTransportRtpSavp , 0   },
   { SDP_TP_RTPAVPF        , eTransportRtpAvpf , 0   },
   { SDP_TP_RTPSAVPF       , eTransportRtpSavpf, 0   },
   { SDP_TP_ATMAVP         , eTransportAtmAvp  , 0   },
   { SDP_TP_UDP            , eTransportUdp     , 0   },
   { SDP_TP_UDPTL          , eTransportUdptl   , 0   },
   { SDP_TP_SIP            , eTransportSip     , 0   },
   { SDP_TP_MSRPTCP        , eTransportMsrpTcp , 0   },
   { SDP_TP_MSRPTLS        , eTransportMsrpTls , 0   }
};

static const tTokenizer  _SdpEncMediaSetTable[] = 
{
   { SDP_FIELD_INFO        , eSdpInfo       , _SDPENC_SkipENC     },
   { SDP_FIELD_CONN        , eSdpConnInfo   , _SDPENC_CxEncoder   },
   { SDP_FIELD_BANDWIDTH   , eSdpBWInfo     , _SDPENC_BwEncoder   },
   { SDP_FIELD_KEY         , eSdpEncriptKey , _SDPENC_SkipENC     },
   { SDP_FIELD_ATTR        , eSdpAttr       , _SDPENC_AttrEncoder }
};

static const tTokenizer  _SdpEncSetupRoleTable[] =
{
   { SDP_SETUP_ACTIVE      , eSdpSetupActive , 0     },
   { SDP_SETUP_ACTPASS     , eSdpSetupActpass, 0     },
   { SDP_SETUP_PASSIVE     , eSdpSetupPassive, 0     }
};

static const tTokenizer  _SdpEncAttrSetTable[] = 
{
   { SDP_ATTR_RTPMAP       , eSdpAttrRtpMap     , _SDPENC_RtpMapEncoder },
   { SDP_ATTR_CAT          , eSdpAttrCat        , 0   },
   { SDP_ATTR_KEYWDS       , eSdpAttrKeywds     , 0   },
   { SDP_ATTR_TOOL         , eSdpAttrTool       , 0   },
   { SDP_ATTR_PTIME        , eSdpAttrPTime      , _SDPENC_PTimeEncoder  },
   { SDP_ATTR_MAXPTIME     , eSdpAttrMaxPTime   , _SDPENC_PTimeEncoder  },
   { SDP_ATTR_RECVONLY     , eSdpAttrRecvOnly   , 0   },
   { SDP_ATTR_SENDRECV     , eSdpAttrSendRecv   , 0   },
   { SDP_ATTR_SENDONLY     , eSdpAttrSendOnly   , 0   },
   { SDP_ATTR_INACTIVE     , eSdpAttrInactive   , 0   },
   { SDP_ATTR_ORIENT       , eSdpAttrOrient     , 0   },
   { SDP_ATTR_TYPE         , eSdpAttrType       , 0   },
   { SDP_ATTR_CHARSET      , eSdpAttrCharset    , 0   },
   { SDP_ATTR_SDPLANG      , eSdpAttrSdpLang    , 0   },
   { SDP_ATTR_LANG         , eSdpAttrLang       , 0   },
   { SDP_ATTR_FRAMERATE    , eSdpAttrFrameRate  , _SDPENC_PTimeEncoder  },
   { SDP_ATTR_QUALITY      , eSdpAttrQuality    , 0   },
   { SDP_ATTR_FMTP         , eSdpAttrFmtp       , _SDPENC_FmtpEncoder   },
   { SDP_ATTR_EXTMAP       , eSdpAttrExtMap     , _SDPENC_ExtMapEncoder },
   { SDP_ATTR_SILENCESUPP  , eSdpAttrSilenceSupp, _SDPENC_SilenceSuppEncoder },
   { SDP_ATTR_RTCP                  , eSdpAttrRtcp                  , _SDPENC_RtcpEncoder                  },
   { SDP_ATTR_T38FAXVERSION         , eSdpAttrT38FaxVersion         , _SDPENC_T38FaxVersionEncoder         },
   { SDP_ATTR_T38MAXBITRATE         , eSdpAttrT38MaxBitRate         , _SDPENC_T38MaxBitRateEncoder         },
   { SDP_ATTR_T38FAXFILLBITREMOVAL  , eSdpAttrT38FaxFillBitRemoval  , _SDPENC_T38FaxFillBitRemovalEncoder  },
   { SDP_ATTR_T38FAXTRANSCODINGMMR  , eSdpAttrT38FaxTranscodingMMR  , _SDPENC_T38FaxTranscodingMMREncoder  },
   { SDP_ATTR_T38FAXTRANSCODINGJBIG , eSdpAttrT38FaxTranscodingJBIG , _SDPENC_T38FaxTranscodingJBIGEncoder },
   { SDP_ATTR_T38FAXRATEMANAGEMENT  , eSdpAttrT38FaxRateManagement  , _SDPENC_T38FaxRateManagementEncoder  },
   { SDP_ATTR_T38FAXMAXBUFFER       , eSdpAttrT38FaxMaxBuffer       , _SDPENC_T38FaxMaxBufferEncoder       },
   { SDP_ATTR_T38FAXMAXDATAGRAM     , eSdpAttrT38FaxMaxDatagram     , _SDPENC_T38FaxMaxDatagramEncoder     },
   { SDP_ATTR_T38FAXUDPEC           , eSdpAttrT38FaxUdpEC           , _SDPENC_T38FaxUdpECEncoder           },
   { SDP_ATTR_CRYPTO                , eSdpAttrCrypto                , _SDPENC_CryptoEncoder                },
   { SDP_ATTR_ENCRYPTION            , eSdpAttrEncryption            , _SDPENC_EncryptionEncoder            },
   { SDP_ATTR_PATH                  , eSdpAttrPath                  , _SDPENC_PathEncoder                  },
   { SDP_ATTR_ACCEPT_TYPES          , eSdpAttrAcceptTypes           , _SDPENC_AcceptTypesEncoder           },
   { SDP_ATTR_ACCEPT_WRAPPED_TYPES  , eSdpAttrAcceptWrappedTypes    , _SDPENC_AcceptWrappedTypesEncoder    },
   { SDP_ATTR_FILE_SELECTOR         , eSdpAttrFileSelector          , _SDPENC_FileSelectorEncoder          },
   { SDP_ATTR_FILE_DISPOSITION      , eSdpAttrFileDisposition       , _SDPENC_FileDispositionEncoder       },
   { SDP_ATTR_FILE_TRANSFER_ID      , eSdpAttrFileTransferId        , _SDPENC_FileTransferIdEncoder        },
   { SDP_ATTR_CURR                  , eSdpAttrCurr                  , _SDPENC_CurrEncoder                  },
   { SDP_ATTR_DES                   , eSdpAttrDes                   , _SDPENC_DesEncoder                   },
   { SDP_ATTR_CONF                  , eSdpAttrConf                  , _SDPENC_ConfEncoder                  },
   { SDP_ATTR_SETUP                 , eSdpAttrSetup                 , _SDPENC_SetupEncoder                 },
   { SDP_ATTR_FINGERPRINT           , eSdpAttrFingerprint           , _SDPENC_FingerprintEncoder           },
   { SDP_ATTR_FRAMESIZE             , eSdpAttrFramesize             , _SDPENC_FrameSizeEncoder             },
   { SDP_ATTR_FINGERPRINT           , eSdpAttrFingerprint           , _SDPENC_FingerprintEncoder           },
   { SDP_ATTR_TCAP                  , eSdpAttrTcap                  , _SDPENC_TcapEncoder                  },
   { SDP_ATTR_PCFG                  , eSdpAttrPcfg                  , _SDPENC_PcfgEncoder                  },
   { SDP_ATTR_RTCPFB                , eSdpAttrRtcpfb                , _SDPENC_RtcpfbEncoder                }
};


/*================================ CVEC_SdpExec ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
vint SDPENC_Exec(tFSM *pFSM, tL4Packet *pBuff, tSdpMsg *pIntMsg)
{
    uint32    PrmPresenceMask = pIntMsg->parmPresenceMask;
    uint32    CurrMask = 1;
    uint16    Int    = 0;
    uint32    idx;
   
    while (PrmPresenceMask) {
        if (CurrMask & PrmPresenceMask) {
            /* parameters table lookup */
            if (TOKEN_IntLookup(Int, _SdpEncFieldsTable, TABLE_SIZE(_SdpEncFieldsTable), &idx) != SIP_OK) {
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "SDPENC_Exec: Failed, Unknown Field:%d for hSdpMsg:%X", 
                        (int)Int, (int)pIntMsg, 0);
                return SIP_FAILED;
            }

            /* add the parameter Name and it's delimiter into the Sdp external message */
            TOKEN_Put(_SdpEncFieldsTable[idx].pExt, "=", pBuff);
                  
            pFSM->hCurrBlock = NULL;

            /* set the corresponding to this parameter encoding target */         
            if (NULL == (pFSM->pfHandler = (tpfTokenHndlr)_SdpEncFieldsTable[idx].pfHandler)) {
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "SDPENC_Exec: Failed, no handler defined for %s for hSdpMsg:%X", 
                        (int)_SdpEncFieldsTable[idx].pExt, (int)pIntMsg, 0);
                return SIP_FAILED;
            }
         
            /* and launch the target */         
            if (pFSM->pfHandler(pFSM, pBuff, pIntMsg) != SIP_OK ) {
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "SDPENC_Exec: Failed, handler failed for %s for hSdpMsg:%X", 
                        (int)_SdpEncFieldsTable[idx].pExt, (int)pIntMsg, 0);
                return SIP_FAILED;            
            }

            /* clear the bit corresponding to this parameter in presence mask */
            PrmPresenceMask &= ~CurrMask;         
        }
        CurrMask <<= 1;
        Int++;
    }
    return SIP_OK;
}  

/*============================== _SDPENC_ProtVersEncoder =================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_ProtVersEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tSdpMsg   *pIntMsg = (tSdpMsg *)hIntMsg;
   char        tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);

   if (0 >= OSAL_itoa(pIntMsg->version, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the Protocol Version and EOL delimiter into the Sdp external message */
   TOKEN_Put(tmpStr, ABNF_EOL, pBuff);
      
   return SIP_OK;
}

/*================================ _SDPENC_OriginEncoder =================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_OriginEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg   *pIntMsg = (tSdpMsg *)hIntMsg;
    tAttribute *pAttr = pIntMsg->pParmAttr;
    vint flag = FALSE;
    char numStr[SIP_MAX_BASETEN_NUM_STRING];

    while(pAttr) {
        if (pAttr->id == eSdpPAttrOrigin) {
            /* add the ... into the Sdp external message */
            TOKEN_Put(pAttr->value.x.origin.userName, " ", pBuff); 
            OSAL_itoa(pAttr->value.x.origin.sessId, numStr, 
                    SIP_MAX_BASETEN_NUM_STRING);
            TOKEN_Put(numStr, " ", pBuff); 
            OSAL_itoa(pAttr->value.x.origin.sessVersion, numStr, 
                    SIP_MAX_BASETEN_NUM_STRING);
            TOKEN_Put(numStr, " ", pBuff); 
            flag = TRUE;
        }
        pAttr = pAttr->next;
    }
    /* search for the name of the originator */
    if (!flag)
        TOKEN_Put("-", " ", pBuff);
    
    if ( _SDPENC_CxEncoder(pFSM, pBuff, hIntMsg) != SIP_OK )
        return SIP_FAILED;

    return SIP_OK;
}

/*============================== _SDPENC_SessNameEncoder =================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_SessNameEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   UNUSED( pFSM );
   UNUSED( hIntMsg );

   /* add the ... into the Sdp external message */
   TOKEN_Put("-", ABNF_EOL, pBuff);

   return SIP_OK;
}

/*================================= _SDPENC_TimeEncoder ==================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_TimeEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   UNUSED( pFSM );
   UNUSED( hIntMsg );

   /* add the ... into the Sdp external message */
   TOKEN_Put("0 0", ABNF_EOL, pBuff);

   return SIP_OK;
}

/*================================= _SDPENC_CxEncoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_CxEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg          *pIntMsg = (tSdpMsg *)hIntMsg;
    tSdpConnInfo     *pConnInfo;
    tNetworkAddress  *pNetAddr;
    uint32            idx;
    char              tmpStr[SDP_MAX_BASETEN_NUM_STRING];
    tIPAddr           ip;

    if ( !pFSM->hCurrBlock )
        pConnInfo = &pIntMsg->connInfo;
    else
        pConnInfo = &((tSdpMedia *)pFSM->hCurrBlock)->connInfo;

    pNetAddr = &pConnInfo->nwAddress;

    if (eNwAddrIPv6 == pNetAddr->addressType) {
        OSAL_memCpy(ip.v6, pNetAddr->x.ip.v6, sizeof(ip.v6));
    }
    else if (eNwAddrIPv4 == pNetAddr->addressType)
    {
        ip.v4.ul = pNetAddr->x.ip.v4.ul;
    }

    /* Network Types table lookup */
    if (TOKEN_IntLookup(pConnInfo->nwType, _SdpEncNetTypesTable, TABLE_SIZE(_SdpEncNetTypesTable), &idx) != SIP_OK) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CxEncoder: Failed, Unknown Field", 0, 0, 0); 
        return SIP_FAILED;
    }
    /* add the NetType into the Sdp external message */
    TOKEN_Put(_SdpEncNetTypesTable[idx].pExt, " ", pBuff);

    pFSM->hCurrBlock = (tSipHandle)pNetAddr;

    /* set the corresponding to this parameter encoding target */         
    if (NULL == (pFSM->pfHandler = (tpfTokenHndlr)_SdpEncNetTypesTable[idx].pfHandler) ) {
       SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CxEncoder: Failed, no handler", 0, 0, 0); 
       return SIP_FAILED;
    } 

    /* and launch the target */         
    if ( pFSM->pfHandler(pFSM, pBuff, pIntMsg) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CxEncoder: Failed, handler failed for %s (hSdpMsg: %X)", 
            (int)_SdpEncNetTypesTable[idx].pExt, (int)pIntMsg, 0);
        return SIP_FAILED;            
    }

    if (eNwAddrIPv6 == pNetAddr->addressType) {
        /* XXX Currently doesn't handle multicast for ipv6 */
    }
    else if ((ip.v4.b[3] & 0xE0) == 0xE0) {
        TOKEN_Put("", "/", pBuff);

        if (0 >= OSAL_itoa(pConnInfo->x.extMCast.ttl, tmpStr, 
                SDP_MAX_BASETEN_NUM_STRING))
            return SIP_FAILED;
        TOKEN_Put(tmpStr, "", pBuff);

        if ( pConnInfo->x.extMCast.num ) {
            TOKEN_Put("", "/", pBuff);
            if (0 >= OSAL_itoa(pConnInfo->x.extMCast.num, tmpStr, 
                     SDP_MAX_BASETEN_NUM_STRING))
                return SIP_FAILED;
            TOKEN_Put(tmpStr, "", pBuff);         
        }
    }
   
    /* add the EOL into the Sdp external message */
    TOKEN_Put("", ABNF_EOL, pBuff);
    return SIP_OK;
}

/*=============================== _SDPENC_AddrInEncoder ==================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_AddrInEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tNetworkAddress  *pNetAddr = (tNetworkAddress *)pFSM->hCurrBlock;
    OSAL_NetAddress   addrObj;
    char              str_ptr[MAX_IPV6_STR_LEN];

    UNUSED(hIntMsg);

    /* add the AddrType into the Sdp external message */
    if (eNwAddrIPv6 == pNetAddr->addressType) {
        TOKEN_Put(SDP_ADDRTYPE_IP6, " ", pBuff);
        addrObj.type = OSAL_NET_SOCK_UDP_V6;
        OSAL_netIpv6Hton(addrObj.ipv6, pNetAddr->x.ip.v6);
    }
    else if (eNwAddrIPv4 == pNetAddr->addressType) {
        TOKEN_Put(SDP_ADDRTYPE_IP4, " ", pBuff);
        addrObj.type = OSAL_NET_SOCK_UDP;
        addrObj.ipv4 = OSAL_netHtonl(pNetAddr->x.ip.v4.ul);
    }
    else {
        return SIP_FAILED;
    }

    if (OSAL_SUCCESS != OSAL_netAddressToString((int8 *)str_ptr, &addrObj)) { 
        return (SIP_FAILED);
    }
    TOKEN_Put(str_ptr, "", pBuff);

    return SIP_OK;
}

/*=============================== _SDPENC_AddrAtmEncoder =================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_AddrAtmEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   UNUSED(pFSM);
   UNUSED(hIntMsg);

   /* add the EOL delimiter into the Sdp external message */
   TOKEN_Put("<not supported>", "", pBuff);

   return SIP_OK;
}

/*============================== _SDPENC_AddrLocalEncoder ================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_AddrLocalEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tNetworkAddress  *pNetAddr = (tNetworkAddress *)pFSM->hCurrBlock;

   UNUSED(hIntMsg);

   /* add the AddrType into the Sdp external message */
   if ( pNetAddr->addressType == eNwAddrLocal )
      TOKEN_Put(SDP_ADDRTYPE_EPN, " ", pBuff);
   else
      return SIP_FAILED;

   /* add the endpointName into the Sdp external message */
   TOKEN_Put(pNetAddr->x.epName, "", pBuff);

   return SIP_OK;
}

/*================================= _SDPENC_BwEncoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_BwEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg    *pIntMsg = (tSdpMsg *)hIntMsg;
    tAttribute *pCurrAttr;
    tSdpBw     *pBw;
    char        tmpStr[SDP_MAX_BASETEN_NUM_STRING];

    if (!pFSM->hCurrBlock) {
        pCurrAttr = pIntMsg->pParmAttr;
    }
    else {
        pCurrAttr = ((tSdpMedia *)pFSM->hCurrBlock)->pParmAttr;
    }

    while (pCurrAttr) {
        if (eSdpPAttrBwInfo == pCurrAttr->id) {
            pBw = &pCurrAttr->value.x.bwInfo;
            /* compare this token with BandwidthType expression constant */
            if (pBw->bwModifier == eSdpBwCT) {
                TOKEN_Put(SDP_BW_CT, ":", pBuff);
            }
            else if (pBw->bwModifier == eSdpBwAS) {
                TOKEN_Put(SDP_BW_AS, ":", pBuff);
            }
            else if (pBw->bwModifier == eSdpBwRS) {
                TOKEN_Put(SDP_BW_RS, ":", pBuff);
            }
            else if (pBw->bwModifier == eSdpBwRR) {
                TOKEN_Put(SDP_BW_RR, ":", pBuff);
            }
            else if (pBw->bwModifier == eSdpBwTIAS) {
                TOKEN_Put(SDP_BW_TIAS, ":", pBuff);
            }
            else {
                /* Invalid BW modifier. */
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1,
                        "_SDPENC_BwEncoder: Failed Unknown BW modifier:%d\n",
                        (int)pBw->bwModifier, 0, 0);
                continue;
            }

            if (0 >= OSAL_itoa(pBw->bw, tmpStr, SDP_MAX_BASETEN_NUM_STRING)) {
                return SIP_FAILED;
            }
            /* add the Bandvidth and EOL delimiter into the Sdp external message */
            TOKEN_Put(tmpStr, ABNF_EOL, pBuff);
        }


        /* Get next */
        pCurrAttr = pCurrAttr->next;
        if (pCurrAttr) {
            /* Put another 'b=' */
            TOKEN_Put(SDP_FIELD_BANDWIDTH, "=", pBuff);
        }
    }

    return SIP_OK;
}

/*=============================== _SDPENC_MediaEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_MediaEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg    *pIntMsg   = (tSdpMsg *)hIntMsg;
    tSdpMedia  *pSdpMedia = &pIntMsg->media;
    uint32          idx;
    char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

    for (;;) {
        /* MediaTypes table lookup */
        if ( TOKEN_IntLookup(pSdpMedia->mediaType, _SdpEncMediaTypesTable, TABLE_SIZE(_SdpEncMediaTypesTable), &idx) != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_MediaEncoder: Failed Unknown media type:%d for hSdpMsg:%X", 
                    (int)pSdpMedia->mediaType, (int)pIntMsg, 0);
                return SIP_FAILED;
        }
    
        TOKEN_Put(_SdpEncMediaTypesTable[idx].pExt, " ", pBuff);

        if (0 >= OSAL_itoa(pSdpMedia->port, tmpStr, 
                SDP_MAX_BASETEN_NUM_STRING))
            return SIP_FAILED;
        TOKEN_Put(tmpStr, "", pBuff);
   
        if ( !pSdpMedia->nPorts )
            TOKEN_Put("", " ", pBuff);
        else {
            TOKEN_Put("", "/", pBuff);
            if (0 >= OSAL_itoa(pSdpMedia->nPorts, tmpStr, 
                    SDP_MAX_BASETEN_NUM_STRING))
                return SIP_FAILED;
            TOKEN_Put(tmpStr, " ", pBuff);
        }
        /* TransportTypes table lookup */
        if (TOKEN_IntLookup(pSdpMedia->transpType, _SdpEncTpTypesTable, TABLE_SIZE(_SdpEncTpTypesTable), &idx) != SIP_OK)
            return SIP_FAILED;
    
        TOKEN_Put(_SdpEncTpTypesTable[idx].pExt, "", pBuff);
        if (pSdpMedia->transpType == eTransportUdptl) {
            /*
             * Only T.38 supported with transport type udptl.
             * Several coders cannot be combined in UDPTL currently.
             */
            TOKEN_Put("", " t38", pBuff);
        }
        else if (pSdpMedia->transpType == eTransportMsrpTcp) {
            TOKEN_Put("", " *", pBuff);
        }
        else if (pSdpMedia->transpType == eTransportMsrpTls) {
            TOKEN_Put("", " *", pBuff);
        }
        else {
            for(idx = 0; idx < pSdpMedia->nFormats; idx++) {
                TOKEN_Put("", " ", pBuff);

                if (0 >= OSAL_itoa(pSdpMedia->formats[idx], tmpStr,
                        SDP_MAX_BASETEN_NUM_STRING))
                    return SIP_FAILED;
                TOKEN_Put(tmpStr, "", pBuff);      
            }
        }
        
        /* add the EOL into the Sdp external message */
        TOKEN_Put("", ABNF_EOL, pBuff);
      
        pFSM->hCurrBlock = (tSipHandle)pSdpMedia;

        if ( _SDPENC_MediaSetEncoder(pFSM, pBuff, hIntMsg) != SIP_OK ) {
            SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_MediaEncoder: _SDPENC_MediaSetEncoder Failed for hSdpMsg:%X", 
                        (int)pIntMsg, 0, 0);
            return SIP_FAILED;
        }

        pSdpMedia = pSdpMedia->next;
        if (pSdpMedia) {
            /* place another 'm=' */
            TOKEN_Put(SDP_FIELD_MEDIA, "=", pBuff);
        }
        else {
            break;
        }
    } /* end of while loop */

    return SIP_OK;
}


/*=============================== _SDPENC_MediaSetEncoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_MediaSetEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMedia  *pSdpMedia  = (tSdpMedia *)pFSM->hCurrBlock;   
    uint32          PrmPresenceMask = pSdpMedia->parmPresenceMask;
    uint32          CurrMask = 1;
    uint16          Int    = 0;
    uint32          idx;

    while (PrmPresenceMask) {
        if (CurrMask & PrmPresenceMask) {
            /* parameters table lookup */
            if ( TOKEN_IntLookup(Int, _SdpEncMediaSetTable, TABLE_SIZE(_SdpEncMediaSetTable), &idx) != SIP_OK ) {
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_MediaSetEncoder: Failed Unknown media set %d for hSdpMedia:%X", 
                        (int)Int, (int)pSdpMedia, 0);
                return SIP_FAILED;
            }
        
            /* add the parameter Name and it's delimiter into the Sdp external message */
            TOKEN_Put(_SdpEncMediaSetTable[idx].pExt, "=", pBuff);
      
            pFSM->hCurrBlock = (tSipHandle)pSdpMedia;

            /* set the corresponding to this parameter encoding target */         
            if (NULL == (pFSM->pfHandler = (tpfTokenHndlr)_SdpEncMediaSetTable[idx].pfHandler)) {
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_MediaSetEncoder: Failed, no handler defined for %s for hSdpMedia:%X", 
                            (int)_SdpEncMediaSetTable[idx].pExt, (int)pSdpMedia, 0);
                return SIP_FAILED;
            }
         
            /* and launch the target */         
            if (pFSM->pfHandler(pFSM, pBuff, hIntMsg) != SIP_OK) {
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_MediaSetEncoder: Failed, handler failed for %s for hSdpMedia:%X", 
                            (int)_SdpEncMediaSetTable[idx].pExt, (int)pSdpMedia, 0);
                return SIP_FAILED;            
            }

            /* clear the bit corresponding to this parameter in presence mask */
            PrmPresenceMask &= ~CurrMask;         
        }
        CurrMask <<= 1;
        Int++;
    }
   
    return SIP_OK;
}


/*=============================== _SDPENC_AttrEncoder ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_AttrEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tSdpMsg          *pIntMsg = (tSdpMsg *)hIntMsg;
    tSdpMedia        *pMedia  = (tSdpMedia *)pFSM->hCurrBlock;
    tAttribute       *pCurrAttr;
    uint32               idx;

    if (pMedia)
        pCurrAttr = pMedia->pAttr;
    else
        pCurrAttr = pIntMsg->pAttr;

    while (pCurrAttr) {
        /* SDP Attributes table lookup */
        if (TOKEN_IntLookup(pCurrAttr->id, _SdpEncAttrSetTable, TABLE_SIZE(_SdpEncAttrSetTable), &idx) != SIP_OK) {
            SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_AttrEncoder: Failed Unknown attribute %d for hSdpMsg:%X", 
                        (int)pCurrAttr->id, (int)pIntMsg, 0);
            return SIP_FAILED;
        }
    
        /* add the Attr Name into the Sdp external message */
        TOKEN_Put(_SdpEncAttrSetTable[idx].pExt, "", pBuff);

        if (pCurrAttr->value.valType != SIP_VAL_NONE ) {
            TOKEN_Put("", ":", pBuff);
         
            /* set current encoding block */
            pFSM->hCurrBlock = (tSipHandle)pCurrAttr;
         
            /* set the corresponding to this field target */         
            if (NULL == (pFSM->pfHandler = (tpfTokenHndlr)_SdpEncAttrSetTable[idx].pfHandler)) {
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_AttrEncoder: Failed, no handler defined for %s for hSdpMsg:%X", 
                            (int)_SdpEncAttrSetTable[idx].pExt, (int)pIntMsg, 0);
                return SIP_FAILED;
            }
         
            /* and launch the target */         
            if (pFSM->pfHandler(pFSM, pBuff, hIntMsg) != SIP_OK) {
                SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_AttrEncoder: Failed, handler failed for %s for hSdpMsg:%X", 
                            (int)_SdpEncAttrSetTable[idx].pExt, (int)pIntMsg, 0);
                return SIP_FAILED;
            }
        }
        TOKEN_Put("", ABNF_EOL, pBuff);
        
        pCurrAttr = pCurrAttr->next;

        if (pCurrAttr)
            TOKEN_Put(SDP_FIELD_ATTR, "=", pBuff);

    } /* end of while */
    return SIP_OK;
}

/*============================== _SDPENC_RtpMapEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_RtpMapEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute       *pAttr   = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue        *pValue  = &pAttr->value;
    tSdpAttrRtpMap   *pRtpMap = &(pValue->x.rtpMap);
    char              tmpStr[SDP_MAX_BASETEN_NUM_STRING];
    
    UNUSED(pFSM);
    UNUSED(hIntMsg);

    if (0 >= OSAL_itoa(pRtpMap->payloadType, tmpStr, 
            SDP_MAX_BASETEN_NUM_STRING))
        return SIP_FAILED;

    /* add the Payload Type into the Sdp external message */
    TOKEN_Put(tmpStr, " ", pBuff);

    /* add the Encoding Name and delimiter into the Sdp external message */
    if ( pRtpMap->szEncodingName[0] != 0 ) 
        TOKEN_Put(pRtpMap->szEncodingName, "/", pBuff);
    
    if (0 >= OSAL_itoa(pRtpMap->clockRate, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
        return SIP_FAILED;

    /* add the ClockRate into the Sdp external message */
    TOKEN_Put(tmpStr, "", pBuff);

    if (pRtpMap->numChan) {
        TOKEN_Put("", "/", pBuff);

        if (0 >= OSAL_itoa(pRtpMap->numChan, tmpStr, 
                SDP_MAX_BASETEN_NUM_STRING))
            return SIP_FAILED;

        TOKEN_Put(tmpStr, "", pBuff);
   }
   return SIP_OK;
}

/*============================== _SDPENC_FrameSizeEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_FrameSizeEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute       *pAttr   = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue        *pValue  = &pAttr->value;
    tSdpAttrFramesize *framesize = &(pValue->x.framesize);
    char              tmpStr[SDP_MAX_BASETEN_NUM_STRING];
    
    UNUSED(pFSM);
    UNUSED(hIntMsg);

    /* add width and height */
    OSAL_snprintf(tmpStr, SDP_MAX_BASETEN_NUM_STRING, "%d %d-%d", framesize->payloadType, framesize->width, framesize->height);
    TOKEN_Put(tmpStr, "", pBuff);

    return SIP_OK;
}

/*=============================== _SDPENC_PTimeEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_PTimeEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16           ptime  = (uint16)pAttr->value.x.uparm;
   char          tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(ptime, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the Packet Time into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}

/*=============================== _SDPENC_T38xxEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/

static vint _SDPENC_T38FaxVersionEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16       num    = (uint16)pAttr->value.x.uparm;
   char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(num, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}

static vint _SDPENC_T38MaxBitRateEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16       num    = (uint16)pAttr->value.x.uparm;
   char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(num, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}
    
static vint _SDPENC_T38FaxFillBitRemovalEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16       num    = (uint16)pAttr->value.x.uparm;
   char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(num, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}

static vint _SDPENC_T38FaxTranscodingMMREncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16       num    = (uint16)pAttr->value.x.uparm;
   char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(num, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}

static vint _SDPENC_T38FaxTranscodingJBIGEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16       num    = (uint16)pAttr->value.x.uparm;
   char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(num, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}

static vint _SDPENC_T38FaxRateManagementEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(pAttr->value.x.szStr, "", pBuff);

   return SIP_OK;
}

static vint _SDPENC_T38FaxMaxBufferEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16       num    = (uint16)pAttr->value.x.uparm;
   char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(num, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}

static vint _SDPENC_T38FaxMaxDatagramEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16       num    = (uint16)pAttr->value.x.uparm;
   char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(num, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}

static vint _SDPENC_T38FaxUdpECEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(pAttr->value.x.szStr, "", pBuff);

   return SIP_OK;
}

static vint _SDPENC_CryptoEncoder(
        tFSM      *pFSM,
        tL4Packet *pBuff,
        tSipHandle hIntMsg)
{
    tAttribute      *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue       *pValue = &pAttr->value;
    tSdpAttrCrypto  *pCrypto  = &(pValue->x.crypto);
    char             tmpStr[SDP_MAX_BASETEN_NUM_STRING];

    UNUSED(pFSM);
    UNUSED(hIntMsg);

    /* add the tag into the SDP external message */
    if (!(OSAL_itoa(pCrypto->tag, tmpStr, 2))) {
        return SIP_FAILED;
    }
    TOKEN_Put(tmpStr, " ", pBuff);
    
    /* add the crypto suite into the SDP external message */
    if (0 != pCrypto->cryptoSuite[0]) {
        TOKEN_Put(pCrypto->cryptoSuite, " ", pBuff);
    }
    
    TOKEN_Put("inline", ":", pBuff);
    
    /* add the key parameters into the SDP external message */
    if (0 == OSAL_strncmp(pCrypto->cryptoSuite,"AES_CM_128_HMAC_SHA1_80",23)) {
        TOKEN_Put(pCrypto->keyParamsAes80, "", pBuff);
    }
    else if (0 == OSAL_strncmp(pCrypto->cryptoSuite,"AES_CM_128_HMAC_SHA1_32",23)) {
        TOKEN_Put(pCrypto->keyParamsAes32, "", pBuff);
    }
        
    return SIP_OK;
}

static vint _SDPENC_EncryptionEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute         *pAttr       = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue          *pValue      = &pAttr->value;
    tSdpAttrEncryption *pEncryption = &(pValue->x.encryption);
    
    UNUSED(pFSM);
    UNUSED(hIntMsg);

    /* add the encryption status into the Sdp external message */
    if (0 != pEncryption->status[0]) {
        TOKEN_Put(pEncryption->status, "", pBuff);
    }

   return SIP_OK;
}


/*=============================== _SDPENC_RtcpEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_RtcpEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
   uint16       port   = (uint16)pAttr->value.x.uparm;
   char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];

   UNUSED(pFSM);
   UNUSED(hIntMsg);

   if (0 >= OSAL_itoa(port, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
      return SIP_FAILED;

   /* add the rtcp port into the Sdp external message */
   TOKEN_Put(tmpStr, "", pBuff);

   return SIP_OK;
}

/*=============================== _SDPENC_FmtpEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_FmtpEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    uint16          ptype  = (uint16)pAttr->value.x.fmtp.payloadType;
   
    UNUSED(hIntMsg);

    if (pAttr->value.valType != SIP_VAL_NONE) {
        if (0 >= OSAL_itoa(ptype, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
            return SIP_FAILED;
        /* then put the payloadType */ 
        if (pAttr->value.x.fmtp.szArg[0] == 0)
            TOKEN_Put(tmpStr, "", pBuff);
        else {
            TOKEN_Put(tmpStr, " ", pBuff);
            TOKEN_Put(pAttr->value.x.fmtp.szArg, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_ExtMapEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_ExtMapEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    uint16       extId  = (uint16)pAttr->value.x.extMap.extmapId;

    UNUSED(hIntMsg);

    if (pAttr->value.valType != SIP_VAL_NONE) {
        if (0 >= OSAL_itoa(extId, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
            return SIP_FAILED;
        /* then put the Extension Uri */
        if (pAttr->value.x.extMap.extmapUri[0] == 0)
            TOKEN_Put(tmpStr, "", pBuff);
        else {
            TOKEN_Put(tmpStr, " ", pBuff);
            TOKEN_Put(pAttr->value.x.extMap.extmapUri, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_PathEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_PathEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    
    UNUSED(hIntMsg);

    if (pAttr->value.valType != SIP_VAL_NONE) {
        if (pAttr->value.x.szStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.szStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_AcceptTypesEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_AcceptTypesEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    
    UNUSED(hIntMsg);

    if (pAttr->value.valType != SIP_VAL_NONE) {
        if (pAttr->value.x.szStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.szStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_AcceptWrappedTypesEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_AcceptWrappedTypesEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;

    UNUSED(hIntMsg);

    if (pAttr->value.valType != SIP_VAL_NONE) {
        if (pAttr->value.x.szStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.szStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_SetupEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_SetupEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute           *pAttr         = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue            *pValue        = &pAttr->value;
    tSdpAttrSetup        *pSetup        = &(pValue->x.setup);
    uint32                idx;

    UNUSED(hIntMsg);

    /* Setup role table lookup*/
    if ( TOKEN_IntLookup(pSetup->role, _SdpEncSetupRoleTable,
            TABLE_SIZE(_SdpEncSetupRoleTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_SetupEncoder: Failed Unknown role:%d for hSdpMsg:%X",
                (int)pSetup->role, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    /* Huawei RCSe cannot parsing setup field end with "space", remove it.*/
    TOKEN_Put(_SdpEncSetupRoleTable[idx].pExt, "", pBuff);

    return (SIP_OK);
}

/*=============================== _SDPENC_FingerprintEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_FingerprintEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute           *pAttr         = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue            *pValue        = &pAttr->value;
    tSdpAttrFingerprint  *pFingerprint  = &(pValue->x.fingerprint);
    uint32                idx;

    UNUSED(hIntMsg);

    /* Hash function table lookup*/
    if ( TOKEN_IntLookup(pFingerprint->hash, _SdpEncHashTable, TABLE_SIZE(_SdpEncHashTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_FingerprintEncoder: Failed Unknown hash function:%d for hSdpMsg:%X",
                (int)pFingerprint->hash, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    TOKEN_Put(_SdpEncHashTable[idx].pExt, " ", pBuff);

    if (0 != pFingerprint->fingerprint[0]) {
        TOKEN_Put(pFingerprint->fingerprint, "", pBuff);
        return (SIP_OK);
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_TCapEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_TcapEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    uint16       tcapId  = (uint16)pAttr->value.x.tcap.tcapId;

    UNUSED(hIntMsg);

    if (0 >= OSAL_itoa(tcapId, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
        return SIP_FAILED;
    if (pAttr->value.valType != SIP_VAL_NONE) {
        TOKEN_Put(tmpStr, " ", pBuff);
        if (pAttr->value.x.tcap.tcapStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.tcap.tcapStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_PCfgEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_PcfgEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    uint16       pcfgId  = (uint16)pAttr->value.x.pcfg.pcfgId;

    UNUSED(hIntMsg);

    if (0 >= OSAL_itoa(pcfgId, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
        return SIP_FAILED;
    if (pAttr->value.valType != SIP_VAL_NONE) {
        TOKEN_Put(tmpStr, " ", pBuff);
        if (pAttr->value.x.pcfg.pcfgStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.pcfg.pcfgStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_RtcpFbEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_RtcpfbEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    char         tmpStr[SDP_MAX_BASETEN_NUM_STRING];
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    uint16       rtcpfbId  = (uint16)pAttr->value.x.rtcpfb.rtcpfbId;

    UNUSED(hIntMsg);

    if (0 >= OSAL_itoa(rtcpfbId, tmpStr, SDP_MAX_BASETEN_NUM_STRING))
        return SIP_FAILED;
    if (pAttr->value.valType != SIP_VAL_NONE) {
        TOKEN_Put(tmpStr, " ", pBuff);
        if (pAttr->value.x.rtcpfb.rtcpfbStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.rtcpfb.rtcpfbStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}

/*=============================== _SDPENC_FileSelectorEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_FileSelectorEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;

    UNUSED(hIntMsg);
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPENC_FileSelectorEncoder: hSdpMsg:%X",
            (int)hIntMsg, 0, 0);
    SIP_DebugLog(SIP_DB_SDP_DEC_LVL_3,
            "_SDPENC_FileSelectorEncoder: not yet implemented",
            0, 0, 0);

//    /* add the EOL delimiter into the Sdp external message */
//    TOKEN_Put("<not supported>", "", pBuff);
//
//    return SIP_OK;

    if (pAttr->value.valType != SIP_VAL_NONE) {
        if (pAttr->value.x.szStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.szStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}


/*=============================== _SDPENC_FileDispositionEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_FileDispositionEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;

    UNUSED(hIntMsg);

    if (pAttr->value.valType != SIP_VAL_NONE) {
        if (pAttr->value.x.szStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.szStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}


/*=============================== _SDPENC_FileTransferIdEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_FileTransferIdEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;

    UNUSED(hIntMsg);

    if (pAttr->value.valType != SIP_VAL_NONE) {
        if (pAttr->value.x.szStr[0] != 0) {
            TOKEN_Put(pAttr->value.x.szStr, "", pBuff);
        }
        return SIP_OK;
    }
    else {
        /* there is no value ! */
        return (SIP_BADPARM);
    }
}


/*=============================== _SDPENC_CurrEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_CurrEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute           *pAttr         = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue            *pValue        = &pAttr->value;
    tSdpAttrPrecondition *pPrecondition = &(pValue->x.precondition);
    uint32                idx;

    UNUSED(pFSM);
    UNUSED(hIntMsg);

    /* precondition type table lookup*/
    if ( TOKEN_IntLookup(pPrecondition->type, _SdpEncPrecTypeTable, TABLE_SIZE(_SdpEncPrecTypeTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CurrEncoder: Failed Unknown precondition type:%d for hSdpMsg:%X",
                (int)pPrecondition->type, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    TOKEN_Put(_SdpEncPrecTypeTable[idx].pExt, " ", pBuff);

    /* status type */
    if ( TOKEN_IntLookup(pPrecondition->statusType, _SdpEncPrecStatusTypeTable, TABLE_SIZE(_SdpEncPrecStatusTypeTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CurrEncoder: Failed Unknown precondition status type:%d for hSdpMsg:%X",
                (int)pPrecondition->statusType, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    TOKEN_Put(_SdpEncPrecStatusTypeTable[idx].pExt, " ", pBuff);

    /* dir */
    if ( TOKEN_IntLookup(pPrecondition->dir, _SdpEncPrecDirTable, TABLE_SIZE(_SdpEncPrecDirTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CurrEncoder: Failed Unknown precondition dir:%d for hSdpMsg:%X",
                (int)pPrecondition->dir, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    TOKEN_Put(_SdpEncPrecDirTable[idx].pExt, " ", pBuff);
    return (SIP_OK);
}


/*=============================== _SDPENC_DesEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_DesEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute           *pAttr         = (tAttribute *)pFSM->hCurrBlock;
    tSdpValue            *pValue        = &pAttr->value;
    tSdpAttrPrecondition *pPrecondition = &(pValue->x.precondition);
    uint32                idx;

    UNUSED(pFSM);
    UNUSED(hIntMsg);

    /* precondition type table lookup */
    if ( TOKEN_IntLookup(pPrecondition->type, _SdpEncPrecTypeTable, TABLE_SIZE(_SdpEncPrecTypeTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CurrEncoder: Failed Unknown precondition type:%d for hSdpMsg:%X",
                (int)pPrecondition->type, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    TOKEN_Put(_SdpEncPrecTypeTable[idx].pExt, " ", pBuff);

    /* precondition strength table lookup*/
    if ( TOKEN_IntLookup(pPrecondition->strength, _SdpEncPrecStrengthTable, TABLE_SIZE(_SdpEncPrecStrengthTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CurrEncoder: Failed Unknown precondition type:%d for hSdpMsg:%X",
                (int)pPrecondition->strength, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    TOKEN_Put(_SdpEncPrecStrengthTable[idx].pExt, " ", pBuff);

    /* status type */
    if ( TOKEN_IntLookup(pPrecondition->statusType, _SdpEncPrecStatusTypeTable, TABLE_SIZE(_SdpEncPrecStatusTypeTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CurrEncoder: Failed Unknown precondition status type:%d for hSdpMsg:%X",
                (int)pPrecondition->statusType, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    TOKEN_Put(_SdpEncPrecStatusTypeTable[idx].pExt, " ", pBuff);

    /* dir */
    if ( TOKEN_IntLookup(pPrecondition->dir, _SdpEncPrecDirTable, TABLE_SIZE(_SdpEncPrecDirTable), &idx) != SIP_OK ) {
        SIP_DebugLog(SIP_DB_SDP_ENC_LVL_1, "_SDPENC_CurrEncoder: Failed Unknown precondition dir:%d for hSdpMsg:%X",
                (int)pPrecondition->dir, (int)hIntMsg, 0);
            return (SIP_BADPARM);
    }

    TOKEN_Put(_SdpEncPrecDirTable[idx].pExt, " ", pBuff);
    return (SIP_OK);
}

/*=============================== _SDPENC_ConfEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_ConfEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    return _SDPENC_CurrEncoder(pFSM, pBuff, hIntMsg);
}

/*=============================== _SDPENC_SilenceSuppEncoder ===================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_SilenceSuppEncoder(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAttribute  *pAttr  = (tAttribute *)pFSM->hCurrBlock;
    uint16          isOn   = (uint16)pAttr->value.x.uparm;
    
    UNUSED(hIntMsg);

    if (isOn) {
        /* add the 'on' string */
        return TOKEN_Put(SDP_ON_STR, "", pBuff);
    }
    else {
        /* add the 'off' string */
        return TOKEN_Put(SDP_OFF_STR, "", pBuff);
    }
}

/*===================================== _SDPENC_Skip ====================================*/
/*
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RESULTS:
 *----------------------------------------------------------------------------------*/
static vint _SDPENC_SkipENC(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
   UNUSED(pFSM);
   UNUSED(hIntMsg);
   
   /* add the ... into the Sdp external message */
   TOKEN_Put("-", ABNF_EOL, pBuff);

   return SIP_OK;
}
