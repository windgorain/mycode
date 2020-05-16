
#ifndef __VNETS_ENTER_DOMAIN_H_
#define __VNETS_ENTER_DOMAIN_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETS_EnterDomain_KickAll(IN UINT uiDomainID);
BS_STATUS VNETS_EnterDomain_RebootDomain(IN UINT uiDomainID);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_ENTER_DOMAIN_H_*/


