#ifndef _SRIL_PERSONALIZATION_H
#define _SRIL_PERSONALIZATION_H

//-------------------------------------------------
// Constant Definitions
//-------------------------------------------------

#define PERSO_CODE_ITEM_LENGTH        1024
#define PERSO_CKS_ITEM_LENGTH         32

//-------------------------------------------------
// Data Structure
//-------------------------------------------------
// Structure for FILE Personalization
typedef struct {
    unsigned char state_inds;
    unsigned char nw_tag;
    int nw_len;
    unsigned char nw_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char ns_tag;
    int ns_len;
    unsigned char ns_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char sp_tag;
    int sp_len;
    unsigned char sp_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char cp_tag;
    int cp_len;
    unsigned char cp_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char sim_tag;
    unsigned char sim_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char ms_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char nw_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char ns_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char sp_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char cp_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char sim_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char cks_attempts;
    unsigned char cks_max_attempts;
} perso_me_data_type_t;

#endif // _SRIL_PERSONALIZATION_H
