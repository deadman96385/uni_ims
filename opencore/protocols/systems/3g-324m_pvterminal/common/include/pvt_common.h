/* ------------------------------------------------------------------
 * Copyright (C) 1998-2010 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#ifndef PVT_COMMON_H_INCLUDED
#define PVT_COMMON_H_INCLUDED

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OSCL_DEFALLOC_H_INCLUDED
#include "oscl_defalloc.h"
#endif

#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif

#ifndef OSCL_SHARED_PTR_H_INCLUDED
#include "oscl_shared_ptr.h"
#endif

#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif

#ifndef PVT_2WAY_BASIC_TYPES_H_INCLUDED
#include "pv_2way_basic_types.h"
#endif

#ifndef PVMF_FORMAT_TYPE_H_INCLUDED
#include "pvmf_format_type.h"
#endif

#ifndef PVMF_RETURN_CODES_H
#include "pvmf_return_codes.h"
#endif

#ifndef PVMF_VIDEO_H_INCLUDED
#include "pvmf_video.h"
#endif

#ifndef PVMF_TIMESTAMP_H_INCLUDED
#include "pvmf_timestamp.h"
#endif

#ifndef PV_INTERFACE_H
#include "pv_interface.h"
#endif

#ifndef PVMI_KVP_H_INCLUDED
#include "pvmi_kvp.h"
#endif

#include "h245def.h"

#define REVERSE_DIR(dir) (TPVDirection)(PV_DIRECTION_BOTH-dir)

typedef int TPVStatusCode;

/**
 * TPV2wayDirectionality Enum
 *
 * TPV2wayDirectionality enum is used to convey the directionality of a pv2way track
 **/
enum TPV2wayDirectionality
{
    EPvtUnidirectional = 1,
    EPvtBiDirectional
};


typedef enum
{
    PV_MUX_COMPONENT_MUX,
    PV_MUX_COMPONENT_LOGICAL_CHANNEL,
} TPVMuxComponent;

/**
Enumeration of format types
**/
typedef enum
{
    PV_INVALID_CODEC_TYPE,
    PV_AUD_TYPE_G723 = 1,
    PV_AUD_TYPE_GSM,
    PV_VID_TYPE_H263 = 20,
    PV_VID_TYPE_MPEG4,
    PV_VID_TYPE_H264,
    PV_UI_BASIC_STRING = 40,
    PV_UI_IA5_STRING,
    PV_UI_GENERAL_STRING,
    PV_UI_DTMF,
    PV_CODEC_TYPE_NONE = 99
} PVCodecType_t;

typedef uint32 TPVChannelId;
#define CHANNEL_ID_UNKNOWN 0xFFFFFFFF
#define CHANNEL_ID_DEFAULT 0xFFFFFFFF

/*********************************/
/* H324 System Definition Value  */
/*********************************/
//#define   OK      1
#define NG      0
#define ON      1
#define OFF     0
#define PVT_NOT_SET (-1)


// For requesting ports
#define PV2WAY_AUDIO_OUTGOING_MIMETYPE    "x-pvmf/audio;dir=outgoing"
#define PV2WAY_VIDEO_OUTGOING_MIMETYPE    "x-pvmf/video;dir=outgoing"
#define PV2WAY_INCOMING_MIMETYPE          "x-pvmf/dir=incoming"
#define PV2WAY_OUTGOING_MIMETYPE          "x-pvmf/dir=outgoing"

const int KPVDefaultVideoBitRate = 42000; // bits/s
const int KPVDefaultFrameRate = 5;   // frames/s
const int KPVDefaultIFrameInterval = 10000;  // s
const int KPVDefaultIFrameRequestInterval = 10000;  // s

#define PV2WAY_MAX_FSI_SIZE 64

typedef bool TPVRemoteTerminalType; /* same = true, other = false */

enum TPVTerminalIdentifier
{
    EPVT_NONE = 0,
    EPVT_LOCAL = 1,
    EPVT_REMOTE = 2,
    EPVT_BOTH = 3,
    EPVT_TERMINAL_IDENTIFIER_MAX = EPVT_BOTH + 1
};

enum TPVDirectionality
{
    EPVT_UNI_DIRECTIONAL = 1,
    EPVT_BI_DIRECTIONAL
};

enum TPVChannelSegmentableType
{
    NONSEGMENTABLE = 0,
    SEGMENTABLE
};


typedef enum
{
    EPVT_Failed = 0,
    EPVT_Success,
    EPVT_Pending,

    EPVT_ErrorBusy = 0x03000000,
    //Terminal busy
    EPVT_ErrorNotImplemented,
    //feature or command is not implemented.
    EPVT_ErrorInvalidParameter,
    //invalid parameter in the command
    EPVT_ErrorInvalidCodecType,
    // invalid codec type specified in the command
    EPVT_ErrorInvalidState,
    //command is not valid for the current authoring state.
    EPVT_ErrorNoMemory,
    //system dynamic memory error
    EPVT_ErrorLocalCapability,
    // We cant handle it
    EPVT_ErrorRemoteCapability,
    // the other terminal cant handle it
    EPVT_ErrorRemoteRejected,
    // The remote terminal rejected it
    EPVT_Timeout,
    // No response received from the remote terminal
    EPVT_FailedToInitialize,
    // The specified component failed to initialize properly
    EPVT_FailedToSetup,
    // Could not setup the specified components with the specified parameters
    EPVT_FailedToNegotiate,
    // Failed to negotiate a set of parameters with the remote terminal
    EPVT_ErrorRemoteDisconnected
    // remote terminal disconnected while establishing connection
} TPVTReturnStatus;


enum TPVSeverity
{
    PVT_INFORMATIONAL = 0,
    PVT_WARNING,
    PVT_ERROR
};

typedef enum
{
    E_EP_LOW = 1,
    E_EP_MEDIUM,
    E_EP_HIGH
} ErrorProtectionLevel_t;


// Used to convey audio type down (App->324)
typedef enum
{
    PV_AUD_TYPE_GSM_475 = 1,
    PV_AUD_TYPE_GSM_515,
    PV_AUD_TYPE_GSM_590,
    PV_AUD_TYPE_GSM_670,
    PV_AUD_TYPE_GSM_740,
    PV_AUD_TYPE_GSM_795,
    PV_AUD_TYPE_GSM_102,
    PV_AUD_TYPE_GSM_122,
    PV_AUD_TYPE_G723_53,
    PV_AUD_TYPE_G723_63,
    PV_AUD_TYPE_NONE = 99
} PVAudType_t;


const PVCodecType_t PV_VID_TYPE_NONE = PV_CODEC_TYPE_NONE ;
const PVCodecType_t PV_SIMPLE_AUD_TYPE_NONE = PV_CODEC_TYPE_NONE ;

typedef PVCodecType_t PVAudTypeSimple_t;

// Used to convey video type (up or down)
typedef PVCodecType_t PVVidType_t;

/**
CPVParam class
Base class for audio/video/mux parameters
**/
class CPVParam
{
    public:
        CPVParam() {}
        virtual ~CPVParam() {}
        virtual CPVParam* Copy() = 0;
};

class CPVMediaParam : public CPVParam
{
    public:
        OSCL_IMPORT_REF CPVMediaParam(PVCodecType_t aCodecType = PV_CODEC_TYPE_NONE);
        OSCL_IMPORT_REF virtual ~CPVMediaParam();
        virtual PV2WayMediaType GetMediaType() = 0;
        OSCL_IMPORT_REF PVCodecType_t GetCodecType();
    private:
        PVCodecType_t iCodecType;
};



class CPVAudioParam : public CPVMediaParam
{
    public:
        OSCL_IMPORT_REF CPVParam* Copy();
        OSCL_IMPORT_REF PV2WayMediaType GetMediaType();
    protected:
        CPVAudioParam(PVCodecType_t aCodecType = PV_CODEC_TYPE_NONE);
    private:
};

class CPVVideoParam : public CPVMediaParam
{
    public:
        OSCL_IMPORT_REF ~CPVVideoParam();

        OSCL_IMPORT_REF PV2WayMediaType GetMediaType();

        OSCL_IMPORT_REF CPVParam* Copy();

        OSCL_IMPORT_REF uint16 GetWidth();

        OSCL_IMPORT_REF uint16 GetHeight();
    protected:
        CPVVideoParam(uint16 aWidth, uint16 aHeight, PVCodecType_t aCodecType = PV_CODEC_TYPE_NONE);
    private:
        uint16 iWidth;
        uint16 iHeight;
};


class CPVAMRAudioParam : public CPVAudioParam
{
    public:
        OSCL_IMPORT_REF CPVAMRAudioParam();
        OSCL_IMPORT_REF ~CPVAMRAudioParam();
};

class CPVG723AudioParam : public CPVAudioParam
{
    public:
        OSCL_IMPORT_REF CPVG723AudioParam();
        OSCL_IMPORT_REF ~CPVG723AudioParam();
};

class CPVH263VideoParam : public CPVVideoParam
{
    public:
        OSCL_IMPORT_REF CPVH263VideoParam(uint16 w, uint16 h);
        OSCL_IMPORT_REF ~CPVH263VideoParam();
};


class CPVM4vVideoParam : public CPVVideoParam
{
    public:
        OSCL_IMPORT_REF CPVM4vVideoParam(uint16 w, uint16 h, uint16 sz, uint8 *cfg);

        OSCL_IMPORT_REF ~CPVM4vVideoParam();

        OSCL_IMPORT_REF OsclAny Set(uint16 config_sz, uint8* cfg);

        OSCL_IMPORT_REF uint16 GetDecoderConfigSize();

        OSCL_IMPORT_REF uint8 *GetDecoderConfig();

        OSCL_IMPORT_REF CPVParam* Copy();

    private:
        uint16 iSz;
        uint8 *iCfg;
};

class CPVTrackInfo
{
    public:
        virtual ~CPVTrackInfo() {}
        virtual TPVDirection GetDirection() = 0;
        virtual TPVChannelId GetChannelId() = 0;
        virtual TPVDirectionality GetDirectionality() = 0;
        virtual int GetNumSduSizes() = 0;
        virtual int GetSduSize(int index = 0) = 0;
        virtual int* GetSduSizes() = 0;
        virtual CPVMediaParam* GetMediaParam() = 0;
        virtual CPVTrackInfo* Copy() = 0;
//  friend bool operator== (CPVTrackInfo &a, CPVTrackInfo &b);
};

OSCL_IMPORT_REF bool operator==(CPVTrackInfo &a, CPVTrackInfo &b);

class CPvtMediaCapability: public HeapBase
{
    public:
        CPvtMediaCapability(PVMFFormatType format_type, uint32 bitrate = 0, bool aMandatory = false)
                : iFormatType(format_type), iBitrate(bitrate), iMandatory(aMandatory) {}
        virtual ~CPvtMediaCapability() {}
        virtual PVMFFormatType GetFormatType()const
        {
            return iFormatType;
        }
        virtual uint32 GetBitrate()const
        {
            return iBitrate;
        }
        virtual bool IsMandatory() const
        {
            return iMandatory;
        }

        PVMFFormatType iFormatType;
        uint32 iBitrate;
        bool iMandatory;
};

class CPvtUserInputCapability : public CPvtMediaCapability
{
    public:
        CPvtUserInputCapability(PVMFFormatType format_type, uint32 bitrate = 0, bool aMandatory = false)
                : CPvtMediaCapability(format_type, bitrate, aMandatory) {}
        ~CPvtUserInputCapability() {}
};

class CPvtAudioCapability : public CPvtMediaCapability
{
    public:
        CPvtAudioCapability(PVMFFormatType format_type, uint32 bitrate = 0, bool aMandatory = false)
                : CPvtMediaCapability(format_type, bitrate, aMandatory) {}
        ~CPvtAudioCapability() {}
};

#define PV2WAY_MAX_AMR_NB_BITRATE 12200
class CPvtAmrNbCapability : public CPvtAudioCapability
{
    public:
        CPvtAmrNbCapability(uint32 bitrate = 0)
                : CPvtAudioCapability(PVMF_MIME_AMR_IF2, PV2WAY_MAX_AMR_NB_BITRATE, true)
        {
            OSCL_UNUSED_ARG(bitrate);
            OSCL_UNUSED_ARG(bitrate);
        }
};


class CPvtVideoCapability : public CPvtMediaCapability
{
    public:
        CPvtVideoCapability(PVMFFormatType format_type, uint32 bitrate = 0, bool aMandatory = false)
                : CPvtMediaCapability(format_type, bitrate, aMandatory)
        {
            iVideoResolution = OSCL_NEW(PVMFVideoResolution, (0, 0));
            iFrameRate = 0;
        }
        ~CPvtVideoCapability()
        {
            if (iVideoResolution)
                OSCL_DELETE(iVideoResolution);
        }
        PVMFFormatType GetFormatType()const
        {
            return iFormatType;
        }
        uint32 GetBitrate()const
        {
            return iBitrate;
        }
        PVMFVideoResolution* GetMaxResolution(uint32& aFrameRate)
        {
            aFrameRate = iFrameRate;
            return iVideoResolution;
        }
        void SetMaxResolution(PVMFVideoResolution aVideoResolution, uint32 aFrameRate)
        {
            *iVideoResolution = aVideoResolution;
            iFrameRate = aFrameRate;
        }
        virtual uint16 Getfsi(uint8*& aDecoderConfig) = 0;

        PVMFVideoResolution *iVideoResolution;
        uint32 iFrameRate;

};

class CPvtMpeg4Capability : public CPvtVideoCapability
{
    public:
        CPvtMpeg4Capability(uint32 bitrate = 0) : CPvtVideoCapability(PVMF_MIME_M4V, bitrate, false), iDecoderConfig(NULL)
        {
            iProfile = -1;
            iLevel = -1;
            iDecoderConfigLen = 0;
            iGenericCapability = NULL;
        }
        ~CPvtMpeg4Capability()
        {
            if (iDecoderConfig)
            {
                OSCL_DEFAULT_FREE(iDecoderConfig);
            }
        }

        uint16 Getfsi(uint8*& aDecoderConfig)
        {
            aDecoderConfig = iDecoderConfig;
            return iDecoderConfigLen;
        }

        int32 iProfile;
        int32 iLevel;
        uint8* iDecoderConfig;
        uint16 iDecoderConfigLen;
        PS_GenericCapability iGenericCapability;


};

class CPvtH263Capability : public CPvtVideoCapability
{
    public:
        CPvtH263Capability(uint32 bitrate = 0) : CPvtVideoCapability(PVMF_MIME_H2632000, bitrate, true)
        {
            iH263VideoCapability = NULL;
        }
        ~CPvtH263Capability()
        {

        }

        uint16 Getfsi(uint8*& aDecoderConfig)
        {
            OSCL_UNUSED_ARG(aDecoderConfig);
            return 0;
        }

        PS_H263VideoCapability iH263VideoCapability;
};

class CPvtAvcCapability : public CPvtVideoCapability
{
    public:
        CPvtAvcCapability(uint32 bitrate = 0) : CPvtVideoCapability(PVMF_MIME_H264_VIDEO_RAW, bitrate, false), iDecoderConfig(NULL)
        {
            iProfile = -1;
            iLevel = -1;
            iCustomMaxMBPS = -1;
            iCustomMaxFS = -1;
            iCustomMaxDPB = -1;
            iCustomMaxBRandCPB = -1;
            iMaxStaticMBPS = -1;
            iMaxRcmdNalUnitSize = 0;
            iMaxNalUnitSize = 0;
            iDecoderConfigLen = 0;
        }
        ~CPvtAvcCapability()
        {
            if (iDecoderConfig)
            {
                OSCL_DEFAULT_FREE(iDecoderConfig);
            }
        }

        uint16 Getfsi(uint8*& aDecoderConfig)
        {
            aDecoderConfig = iDecoderConfig;
            return iDecoderConfigLen;
        }

        int32 iProfile;
        int32 iLevel;
        int32 iCustomMaxMBPS;
        int32 iCustomMaxFS;
        int32 iCustomMaxDPB;
        int32 iCustomMaxBRandCPB;
        int32 iMaxStaticMBPS;
        uint32 iMaxRcmdNalUnitSize;
        uint32 iMaxNalUnitSize;
        uint8* iDecoderConfig;
        uint16 iDecoderConfigLen;
};

class CPvtTerminalCapability
{
    public:
        CPvtTerminalCapability(Oscl_Vector<CPvtMediaCapability*, OsclMemAllocator>& capability_items):
                iCapabilityItems(capability_items)
        {
        }
        ~CPvtTerminalCapability()
        {
            for (uint16 i = 0; i < iCapabilityItems.size(); i++)
                OSCL_DELETE(iCapabilityItems[i]);
        }
        uint16 GetNumCapabilityItems()const
        {
            return (uint16)iCapabilityItems.size();
        }
        CPvtMediaCapability* GetCapabilityItem(uint16 index)
        {
            return iCapabilityItems[index];
        }
    private:
        Oscl_Vector<CPvtMediaCapability*, OsclMemAllocator> iCapabilityItems;
};

typedef enum
{
    EObjectIdentifier,
    EH221NonStandard
} TPVH245VendorType;

class TPVH245Vendor
{
    public:
        virtual ~TPVH245Vendor() {}
        virtual TPVH245VendorType GetVendorType() = 0;
        virtual TPVH245Vendor* Copy() = 0;
};

class TPVH245VendorObjectIdentifier : public TPVH245Vendor
{
    public:
        OSCL_IMPORT_REF TPVH245VendorObjectIdentifier(uint8* vendor, uint16 vendorLength);
        OSCL_IMPORT_REF ~TPVH245VendorObjectIdentifier();
        OSCL_IMPORT_REF TPVH245VendorType GetVendorType();
        OSCL_IMPORT_REF uint8* GetVendor(uint16* length);
        OSCL_IMPORT_REF TPVH245Vendor* Copy();

    protected:
        uint8* iVendor;
        uint16 iVendorLength;
};

class TPVVendorH221NonStandard : public TPVH245Vendor
{
    public:
        OSCL_IMPORT_REF TPVVendorH221NonStandard(uint8 t35countryCode, uint8 t35extension, uint32 manufacturerCode);
        OSCL_IMPORT_REF ~TPVVendorH221NonStandard();
        OSCL_IMPORT_REF TPVH245VendorType GetVendorType();
        OSCL_IMPORT_REF TPVH245Vendor* Copy();
        OSCL_IMPORT_REF uint8 GetT35CountryCode();
        OSCL_IMPORT_REF uint8 GetT35Extension();
        OSCL_IMPORT_REF uint32 GetManufacturerCode();
    private:
        uint8 iT35CountryCode; /* INTEGER (0..255) */
        uint8 iT35Extension; /* INTEGER (0..255) */
        uint32 iManufacturerCode; /* INTEGER (0..65535) */
};

// Vendor identification classes
class TPVVendorIdentification
{
    public:
        OSCL_IMPORT_REF TPVVendorIdentification();
        OSCL_IMPORT_REF TPVVendorIdentification(TPVH245Vendor* vendor,
                                                uint8* pn, uint16 pn_len,
                                                uint8* vn, uint16 vn_len);
        OSCL_IMPORT_REF ~TPVVendorIdentification();
        TPVH245Vendor* iVendor;
        uint8* iProductNumber;
        uint16 iProductNumberLen;
        uint8* iVersionNumber;
        uint16 iVersionNumberLen;
};



#define PARAM_DEFAULT -1
typedef enum
{
    MUX_GENERIC = 0,
    MUX_H223,
    MUX_RTP
} TPVMuxType;


#define MAX_H223_LEVELS 5
typedef enum
{
    H223_LEVEL0 = 0,
    H223_LEVEL1,
    H223_LEVEL1_DF, /* Double flag */
    H223_LEVEL2,
    H223_LEVEL2_OH, /* Optional header */
    H223_LEVEL3,
    H223_LEVEL_UNKNOWN
} TPVH223Level;

typedef enum
{
    H223_PDU_SIMPLE, /* Audio only, Video only, Control only  */
    H223_PDU_COMBINED /* A+V */
} TPVH223MuxPduType;

typedef enum
{
    H223_IDLE_SYNC_NONE = 0,
    H223_IDLE_SYNC_OCTET, /* Idle sync is a repeatition of a particular octet - 0, 1, F etc */
    H223_IDLE_SYNC_FLAGS /* Idle sync is a repeatition of flags of the current level */
} TPVH223MuxIdleSyncType;

class CPVTerminalParam : public CPVParam
{
    public:
        OSCL_IMPORT_REF CPVTerminalParam();
        OSCL_IMPORT_REF CPVTerminalParam(CPVTerminalParam& that);
        virtual ~CPVTerminalParam();
        virtual TPVTerminalType GetTerminalType() = 0;
        OSCL_IMPORT_REF CPVParam* Copy(CPVTerminalParam* param);
    protected:
};

/* MasterSlaveDetermination Decision */
typedef enum
{
    PVT_SLAVE = 0,
    PVT_MSD_INDETERMINATE = 128,
    PVT_MASTER = 255
} TPVMasterSlave;

typedef enum
{
    PVT_AL1,
    PVT_AL2,
    PVT_AL3,
    PVT_AL_UNKNOWN
} TPVAdaptationLayer;

OSCL_IMPORT_REF int IndexForAdaptationLayer(TPVAdaptationLayer al);
OSCL_IMPORT_REF TPVAdaptationLayer AdaptationLayerForIndex(int al_index);
OSCL_IMPORT_REF ErrorProtectionLevel_t EplForAdaptationLayer(TPVAdaptationLayer al);

class CPVH324MParam : public CPVTerminalParam
{
    public:
        OSCL_IMPORT_REF CPVH324MParam();
        OSCL_IMPORT_REF CPVH324MParam(const CPVH324MParam& that);
        OSCL_IMPORT_REF ~CPVH324MParam();
        OSCL_IMPORT_REF TPVTerminalType GetTerminalType();
        OSCL_IMPORT_REF CPVParam* Copy();

        bool iAllowAl1Video;        /* Local terminal */
        bool iAllowAl2Video;        /* Local terminal */
        bool iAllowAl3Video;        /*   (These are sent in outgoing CE) */
        /* The above flags determine if support for the layers will be indicate in the
           TSC.  The following  determine if they will be used or not */
        bool iUseAl1Video;
        bool iUseAl2Video;
        bool iUseAl3Video;
        TPVAdaptationLayer iVideoLayer;         /* Layer to use, decided by local terminal */
        TPVAdaptationLayer iForceVideoLayerIfMaster;            /* Force the terminal to use this layer - for testing purposes */
        TPVAdaptationLayer iForceVideoLayerIfSlave;         /* Force the terminal to use this layer - for testing purposes */
        TPVMasterSlave iMasterSlave;
        bool iSpecifyReceiveAndTransmitCapability;
        bool iSendRme; /* Send Request Multiplex Entry to the remote terminal */
        bool iSkipMsd; /* 1 = Skip MSD */
        uint16 iRequestMaxMuxPduSize; /* Requests maxMuxPduSize to the remote terminal if > 0.  This is done after TCS
                                                 if the remote terminal supports the maxMuxPduCapability */
};


class CPVTerminalStatistics
{

};

class CPVH324TerminalStatistics : public CPVTerminalStatistics
{

};

OSCL_IMPORT_REF PVMFFormatType PVCodecTypeToPVMFFormatType(PVCodecType_t aCodecType);
OSCL_IMPORT_REF PVCodecType_t PVMFFormatTypeToPVCodecType(PVMFFormatType aFormatType);
OSCL_IMPORT_REF PV2WayMediaType PVMFFormatTypeToPVMediaType(PVMFFormatType aFormatType);

#define min2(a, b) ((a > b) ? b : a)

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "

class BasicAlloc : public Oscl_DefAlloc
{
    public:
        void* allocate(const uint32 size)
        {
            void* tmp = (void*)OSCL_DEFAULT_MALLOC(size);
            OSCL_ASSERT(tmp != 0);
            return tmp;
        }
        void deallocate(void* p)
        {
            OSCL_DEFAULT_FREE(p);
        }
};

typedef enum
{
    PV_H324COMPONENT_H245_USER,
    PV_H324COMPONENT_H245,
    PV_H324COMPONENT_SRP,
    PV_H324COMPONENT_H223
} TPVH324Component;

const PVMFStatus PV2WayH324ErrorStatusStart = (-10600);
const PVMFStatus PV2WayH324ErrorSymmetryViolation = PV2WayH324ErrorStatusStart;

class CodecCapabilityInfo
{
    public:
        OSCL_IMPORT_REF CodecCapabilityInfo();
        virtual ~CodecCapabilityInfo() {}
        OSCL_IMPORT_REF virtual CodecCapabilityInfo* Copy();
        PVCodecType_t codec;
        TPVDirection dir;
        uint32 max_bitrate;
        uint32 min_sample_size;
        uint32 max_sample_size;
};

class VideoCodecCapabilityInfo : public CodecCapabilityInfo
{
    public:
        OSCL_IMPORT_REF VideoCodecCapabilityInfo();
        ~VideoCodecCapabilityInfo()
        {
            if (codec_specific_info)
            {
                OSCL_DEFAULT_FREE(codec_specific_info);
            }
        }
        OSCL_IMPORT_REF CodecCapabilityInfo* Copy();
        Oscl_Vector<PVMFVideoResolutionRange, OsclMemAllocator> resolutions;
        uint8* codec_specific_info;
        uint16 codec_specific_info_len;
};


typedef enum
{
    APP,
    ENG
} TPVPriority;

class FormatCapabilityInfo
{
    public:
        FormatCapabilityInfo() : id(CHANNEL_ID_UNKNOWN),
                format(PVMF_MIME_FORMAT_UNKNOWN),
                dir(PV_DIRECTION_NONE),
                bitrate(0),
                min_sample_size(0),
                max_sample_size(0),
                capabilities(1),
                iFsiLen(0) {}
        FormatCapabilityInfo(PVMFFormatType aFormat,
                             TPVDirection aDir,
                             uint32 aCapabilities)
                : format(aFormat),
                dir(aDir),
                bitrate(0),
                min_sample_size(0),
                max_sample_size(0),
                capabilities(aCapabilities),
                iFsiLen(0) {}

        PVMFStatus SetFormatSpecificInfo(const uint8* apFormatSpecificInfo, uint32 aFormatSpecificInfoLen)
        {
            if (aFormatSpecificInfoLen > PV2WAY_MAX_FSI_SIZE)
            {
                return PVMFFailure;
            }
            oscl_memcpy(iFsi, apFormatSpecificInfo, aFormatSpecificInfoLen);
            iFsiLen = aFormatSpecificInfoLen;
            return PVMFSuccess;
        }

        void GetFormatSpecificInfo(uint8** appFormatSpecificInfo, uint32 &aFormatSpecificInfoLen)
        {
            *appFormatSpecificInfo = NULL;
            aFormatSpecificInfoLen = 0;

            if (iFsiLen)
            {
                *appFormatSpecificInfo = iFsi;
                aFormatSpecificInfoLen = iFsiLen;
            }
        }

    public:
        TPVChannelId id;
        PVMFFormatType format;
        TPVDirection dir;
        uint32 bitrate;
        uint32 min_sample_size;
        uint32 max_sample_size;
        uint32 capabilities; // additional capabilities

        /* Enum for Priority */
        TPVPriority iPriority;

    protected:
        uint8 iFsi[PV2WAY_MAX_FSI_SIZE];
        uint32 iFsiLen;
};

typedef Oscl_Vector<FormatCapabilityInfo, OsclMemAllocator> FormatCapabilityInfoVector;

class H324ChannelParameters
{
    public:
        OSCL_IMPORT_REF H324ChannelParameters(unsigned bandwidth);
        OSCL_IMPORT_REF H324ChannelParameters(const H324ChannelParameters& that);
        OSCL_IMPORT_REF ~H324ChannelParameters();
        OSCL_IMPORT_REF void SetCodecs(Oscl_Vector<FormatCapabilityInfo, OsclMemAllocator>& codecs);
        OSCL_IMPORT_REF Oscl_Vector<FormatCapabilityInfo, OsclMemAllocator>* GetCodecs();

        OSCL_IMPORT_REF PV2WayMediaType GetMediaType();
        OSCL_IMPORT_REF unsigned GetBandwidth();
    private:
        Oscl_Vector<FormatCapabilityInfo, OsclMemAllocator>* iCodecs;
        unsigned iBandwidth;
};

typedef enum
{
    EH324Timer,
    EH324Counter
} TPVH324TimerCounter;


#define PVH324MLogicalChannelInfoUuid PVUuid(0x200306a0,0xffab,0x11d9,0xba,0x43,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
/* Base class for media logical channels */
class LogicalChannelInfo
{
    public:
        virtual ~LogicalChannelInfo() {}

        virtual TPVDirection GetDirection() = 0;

        virtual TPVChannelId GetLogicalChannelNumber() = 0;

        virtual uint32 GetSduSize() = 0;

        virtual bool IsSegmentable() = 0;

        virtual uint32 GetBitrate() = 0;

        virtual uint32 GetSampleInterval() = 0;

        virtual const uint8* GetFormatSpecificInfo(uint32* format_specific_info_len) = 0;

        virtual PVMFTimestamp GetLastSduTimestamp() = 0;

        virtual PVMFFormatType GetFormatType() = 0;

};

#define PV_H324_MUX_INPUT_FORMATS "x-pvmf/multiplexed/input_formats"
#define PV_H324_MUX_INPUT_FORMATS_VALTYPE "x-pvmf/multiplexed/formattype;valtype=int32"
#define PV_H324_MUX_OUTPUT_FORMATS "x-pvmf/multiplexed/output_formats"
#define PV_H324_MUX_OUTPUT_FORMATS_VALTYPE "x-pvmf/multiplexed/formattype;valtype=int32"

#define PV_H324_VIDEO_INPUT_FORMATS "x-pvmf/video/decode/input_formats"
#define PV_H324_VIDEO_INPUT_FORMATS_VALTYPE "x-pvmf/video/decode/formattype;valtype=int32"
#define PV_H324_AUDIO_INPUT_FORMATS "x-pvmf/audio/decode/input_formats"
#define PV_H324_AUDIO_INPUT_FORMATS_VALTYPE "x-pvmf/audio/decode/formattype;valtype=int32"

#define PV_H324_VIDEO_OUTPUT_FORMATS "x-pvmf/video/encode/output_formats"
#define PV_H324_VIDEO_OUTPUT_FORMATS_VALTYPE "x-pvmf/video/encode/formattype;valtype=int32"
#define PV_H324_AUDIO_OUTPUT_FORMATS "x-pvmf/audio/encode/output_formats"
#define PV_H324_AUDIO_OUTPUT_FORMATS_VALTYPE "x-pvmf/audio/encode/formattype;valtype=int32"

OSCL_IMPORT_REF const char* GetFormatsString(TPVDirection aDir, PV2WayMediaType aMediaType);
OSCL_IMPORT_REF const char* GetFormatsValtypeString(TPVDirection aDir, PV2WayMediaType aMediaType);

template<class elementalClass>
class Basic2WayDestructDealloc : public OsclDestructDealloc
{
    public:
        virtual void destruct_and_dealloc(OsclAny *ptr)
        {
            OSCL_DEFAULT_FREE(ptr);
        }
};

OSCL_IMPORT_REF void GetSampleSize(PVMFFormatType aFormatType, uint32* aMin, uint32* aMax);

/**
 * CPV2WayM4VConfigInfo Class
 *
 * The CPV2WayM4VConfigInfo class contains configuration information for an M4V encoder.  This information can be used to configure a encoded source or sink.
 **/

class CPV2WayM4VConfigInfo
{
    public:
        CPV2WayM4VConfigInfo() : m4v_VOLHeaderSize(0), frameWidth(176), frameHeight(144), frameRate(5), profileLevelID(0) {};

        CPV2WayM4VConfigInfo(OsclSharedPtr<uint8> am4v_VOLHeader,
                             uint32 am4v_VOLHeaderSize,
                             int aframeWidth,
                             int aframeHeight,
                             int aprofileLevelID,
                             int aframeRate)
        {
            m4v_VOLHeader.Bind(am4v_VOLHeader);
            m4v_VOLHeaderSize = am4v_VOLHeaderSize;
            frameWidth = aframeWidth;
            frameHeight = aframeHeight;
            profileLevelID = aprofileLevelID;
            frameRate = aframeRate;
        }


        CPV2WayM4VConfigInfo & operator=(const CPV2WayM4VConfigInfo& aInfo)
        {
            m4v_VOLHeader.Bind(aInfo.m4v_VOLHeader);
            m4v_VOLHeaderSize = aInfo.m4v_VOLHeaderSize;
            frameWidth = aInfo.frameWidth;
            frameHeight = aInfo.frameHeight;
            frameRate = aInfo.frameRate;
            profileLevelID = aInfo.profileLevelID;
            return *this;
        }


        OsclSharedPtr<uint8> m4v_VOLHeader;
        uint32 m4v_VOLHeaderSize;
        int frameWidth;
        int frameHeight;
        int frameRate;
        int profileLevelID;

};

/**
 * CPV2WayH263ConfigInfo Class
 *
 * The CPV2WayH263ConfigInfo class contains configuration information for an H263 encoder.  This information can be used to configure a encoded source or sink.
 **/

class CPV2WayH263ConfigInfo
{
    public:
        CPV2WayH263ConfigInfo() : profile(0), level(10), frameWidth(176), frameHeight(144), frameRate(5) {};

        CPV2WayH263ConfigInfo & operator=(const CPV2WayH263ConfigInfo& aInfo)
        {
            profile = aInfo.profile;
            level = aInfo.level;
            frameWidth = aInfo.frameWidth;
            frameHeight = aInfo.frameHeight;
            frameRate = aInfo.frameRate;
            return *this;
        }


        int profile;
        int level;
        int frameWidth;
        int frameHeight;
        int frameRate;
};


class OlcFormatInfo
{
    public:
        OlcFormatInfo() : iId(CHANNEL_ID_UNKNOWN),
                iCodec(PV_CODEC_TYPE_NONE),
                isSymmetric(true) {}
        TPVChannelId iId;
        PVCodecType_t iCodec;
        bool isSymmetric;
};

template < uint32 numchunk, uint32 chunksize = 0 >
class PoolMemAlloc : public OsclMemPoolFixedChunkAllocator
{
    public:
        PoolMemAlloc(): OsclMemPoolFixedChunkAllocator(numchunk, chunksize) {}
};
typedef PoolMemAlloc<10> PoolMemAlloc_OsclMemAllocator_10;

#define PORT_TYPE_FOR_DIRECTION(dir) ((dir == OUTGOING) ? EPVInputPort : EPVOutputPort)
OSCL_IMPORT_REF PV2WayMediaType GetMediaType(PVCodecType_t codec);
OSCL_IMPORT_REF void GetSampleSize(PVMFFormatType aFormatType, uint32* aMin, uint32* aMax);
OSCL_IMPORT_REF bool CodecRequiresFsi(PVCodecType_t codec);


// This function returns a priority index for each format type.
OSCL_IMPORT_REF uint32 GetPriorityIndexForPVMFFormatType(PVMFFormatType aFormatType);

//Priority to MP4 over H.263
template<class Alloc> struct PV2WayRegFormatTypeCompare
{
    bool operator()(const PVMFFormatType& x, const PVMFFormatType& y) const
    {
        uint32 x_val = GetPriorityIndexForPVMFFormatType(x);
        uint32 y_val = GetPriorityIndexForPVMFFormatType(y);

        return (x_val < y_val) ? true : false;
    }
};

typedef PV2WayRegFormatTypeCompare<OsclMemAllocator> pvmf_format_type_key_compare_class;

OSCL_IMPORT_REF bool IsFormatType(const PvmiKvp& aKvp);
OSCL_IMPORT_REF bool IsMatch(const PVMFFormatType& aFormat, const PvmiKvp& aKvp);

#endif
