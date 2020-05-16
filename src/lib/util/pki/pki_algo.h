/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-15
* Description: 
* History:     
******************************************************************************/

#ifndef __PKI_ALGO_H_
#define __PKI_ALGO_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

const EVP_MD *PKI_ALGO_GetDigestMethodByHashAlgoId(IN PKI_HASH_ALGMETHOD_E enHash);
VOID PKI_ALGO_add_all_algorithms(VOID);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__PKI_ALGO_H_*/


