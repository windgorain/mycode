
#ifndef __VNETS_ENTER_DOMAIN_H_
#define __VNETS_ENTER_DOMAIN_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS VNETS_EnterDomain_KickAll(IN UINT uiDomainID);
BS_STATUS VNETS_EnterDomain_RebootDomain(IN UINT uiDomainID);

#ifdef __cplusplus
    }
#endif 

#endif 


