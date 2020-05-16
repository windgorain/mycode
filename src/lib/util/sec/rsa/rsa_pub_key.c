/*================================================================
*   Created by LiXingang: 2018.12.11
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rsa_utl.h"

static char * g_public_key = "-----BEGIN PUBLIC KEY-----\n"
"MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAsUdSg2wF9HFlCDA5K/Jj\n"
"z3k9l6it5htLieDYI1A21xfT1xnp5LWvm2hR0qTM7Z+GP/xIvRsKxtczweZh546a\n"
"R9D4leKS0gUAq40kazqWannY/yasgAiMODGu9F0qF0zLsT0yOyebsygO+i5qsOwr\n"
"gO3DHO5eBE+0Chmeei0X0RSw59Ueg0RT5i1jVsNnbI28cXSsGAG9X1wiZl5L2w1u\n"
"GbgM85vVTkLtYtt81eT+nZTNYVr6nqae1CEvY4kFRL1EwJl3V8juPtCMfoeERTLl\n"
"Ew7vV13juA2PcGitGwb/j0fNLgqdNS9GHge3GdCyJ6tqsv/hz3g+EPFCDxOfCHJ3\n"
"+vrAT0py2g6O+r+eQMiEoYefvfDv4NUCPOhZ7pMGKB2mrn9Nr0V4fHpJClVlckQC\n"
"AVn6clV68ZBHCFt3dLQvJsv4e21KI0EYWQJkkv+d2B4vhySFRLDYe/8/eXttePhh\n"
"JpsAbTwwVZKZ3j1T7ilixHCaheKuBzEOeMBB2UucsrUG84ZXS0HJeOGmOxIUNPNc\n"
"g4NY7vIprEC0zL4bahxfM1Img8VhTc9n2BaNY/50E4LZRJHlUjE8OtUfXx3IQK09\n"
"suxej+M88CgqAnKknap1JRCLjDpvLFCHBx6EYWH8IzFfwymOnMW/ywMoIflU/BP6\n"
"ggKOh0zEgd+3vEHfFMSCansCAwEAAQ==\n"
"-----END PUBLIC KEY-----";

RSA * RSA_DftPublicKey()
{
    BIO* bp = NULL;
    RSA * pub_key;

    if ((bp = BIO_new_mem_buf(g_public_key, -1)) == NULL) {     
        return NULL;
    }

    pub_key = PEM_read_bio_RSA_PUBKEY(bp, NULL, NULL, NULL);

    BIO_free(bp);

    return pub_key;
}

