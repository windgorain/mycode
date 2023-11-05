/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/md5_utl.h"
#include "utl/pki_utl.h"
#include "utl/rsa_utl.h"
#include "openssl/ssl.h"
#include "openssl/pkcs12.h"

#include "pki_algo.h"
#include "pkey_local.h"
#include "pki_pubkey.h"


EVP_PKEY *PKEY_GenerateECKey(IN INT iNid)
{
    return EVP_BuildKey(iNid, 2048);
}

