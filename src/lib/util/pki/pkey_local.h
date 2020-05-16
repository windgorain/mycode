/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-15
* Description: 
* History:     
******************************************************************************/

#ifndef __PKEY_LOCAL_H_
#define __PKEY_LOCAL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define PKEY_EVP_SET_SUCCESS          1
#define PKEY_DSA_SEED_LEN    20          /* 产生DSA随机数的种子长度 */


EVP_PKEY *PKEY_GenerateRSAKey(IN UINT uiKeySize);
EVP_PKEY *PKEY_GenerateDSAKey(IN UINT uiKeySize);
EVP_PKEY *PKEY_GenerateECKey(IN INT iNid);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__PKEY_LOCAL_H_*/


