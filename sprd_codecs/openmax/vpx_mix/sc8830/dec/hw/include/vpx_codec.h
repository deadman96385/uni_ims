#ifndef VPX_CODEC_H
#define VPX_CODEC_H


   /*!\brief Algorithm return codes */
    typedef enum {
        /*!\brief Operation completed without error */
        VPX_CODEC_OK,

        /*!\brief Unspecified error */
        VPX_CODEC_ERROR,

        /*!\brief Memory operation failed */
        VPX_CODEC_MEM_ERROR,

        /*!\brief ABI version mismatch */
        VPX_CODEC_ABI_MISMATCH,

        /*!\brief Algorithm does not have required capability */
        VPX_CODEC_INCAPABLE,

        /*!\brief The given bitstream is not supported.
         *
         * The bitstream was unable to be parsed at the highest level. The decoder
         * is unable to proceed. This error \ref SHOULD be treated as fatal to the
         * stream. */
        VPX_CODEC_UNSUP_BITSTREAM,

        /*!\brief Encoded bitstream uses an unsupported feature
         *
         * The decoder does not implement a feature required by the encoder. This
         * return code should only be used for features that prevent future
         * pictures from being properly decoded. This error \ref MAY be treated as
         * fatal to the stream or \ref MAY be treated as fatal to the current GOP.
         */
        VPX_CODEC_UNSUP_FEATURE,

        /*!\brief The coded data for this stream is corrupt or incomplete
         *
         * There was a problem decoding the current frame.  This return code
         * should only be used for failures that prevent future pictures from
         * being properly decoded. This error \ref MAY be treated as fatal to the
         * stream or \ref MAY be treated as fatal to the current GOP. If decoding
         * is continued for the current GOP, artifacts may be present.
         */
        VPX_CODEC_CORRUPT_FRAME,

        /*!\brief An application-supplied parameter is not valid.
         *
         */
        VPX_CODEC_INVALID_PARAM,

        /*!\brief An iterator reached the end of list.
         *
         */
        VPX_CODEC_LIST_END,

    }
    vpx_codec_err_t;

    /*! \brief Codec capabilities bitfield
     *
     *  Each codec advertises the capabilities it supports as part of its
     *  ::vpx_codec_iface_t interface structure. Capabilities are extra interfaces
     *  or functionality, and are not required to be supported.
     *
     *  The available flags are specified by VPX_CODEC_CAP_* defines.
     */
    typedef long vpx_codec_caps_t;
#define VPX_CODEC_CAP_DECODER 0x1 /**< Is a decoder */
#define VPX_CODEC_CAP_ENCODER 0x2 /**< Is an encoder */
#define VPX_CODEC_CAP_XMA     0x4 /**< Supports e_xternal Memory Allocation */


    /*! \brief Initialization-time Feature Enabling
     *
     *  Certain codec features must be known at initialization time, to allow for
     *  proper memory allocation.
     *
     *  The available flags are specified by VPX_CODEC_USE_* defines.
     */
    typedef long vpx_codec_flags_t;
#define VPX_CODEC_USE_XMA 0x00000001    /**< Use e_xternal Memory Allocation mode */


    /*!\brief Codec interface structure.
     *
     * Contains function pointers and other data private to the codec
     * implementation. This structure is opaque to the application.
     */
//      typedef const struct vpx_codec_iface vpx_codec_iface_t;


    /*!\brief Codec private data structure.
     *
     * Contains data private to the codec implementation. This structure is opaque
     * to the application.
     */
//      typedef       struct vpx_codec_priv  vpx_codec_priv_t;


    /*!\brief Iterator
     *
     * Opaque storage used for iterating over lists.
     */
    typedef const void *vpx_codec_iter_t;


	    /*!\brief Codec context structure
     *
     * All codecs \ref MUST support this context structure fully. In general,
     * this data should be considered private to the codec algorithm, and
     * not be manipulated or examined by the calling application. Applications
     * may reference the 'name' member to get a printable description of the
     * algorithm.
     */
    typedef struct
    {
        const char              *name;        /**< Printable interface name */
 //        vpx_codec_iface_t       *iface;       /**< Interface pointers */
        vpx_codec_err_t          err;         /**< Last returned error */
        const char              *err_detail;  /**< Detailed info, if available */
        vpx_codec_flags_t        init_flags;  /**< Flags passed at init time */
//        union
//        {
//            struct vpx_codec_dec_cfg  *dec;   /**< Decoder Configuration Pointer */
//            struct vpx_codec_enc_cfg  *enc;   /**< Encoder Configuration Pointer */
//            void                      *raw;
//        }                        config;      /**< Configuration pointer aliasing union */
 //        vpx_codec_priv_t        *priv;        /**< Algorithm private storage */
    } vpx_codec_ctx_t;

#endif //VPX_CODEC_H