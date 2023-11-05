/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-23
* Description: 
* History:     
******************************************************************************/

#ifndef __IN_MCAST_H_
#define __IN_MCAST_H_

#ifdef __cplusplus
    extern "C" {
#endif 


#define IP_MIN_MEMBERSHIPS  31
#define IP_MAX_MEMBERSHIPS  (32 * 1024 - 1)
#define IP_MAX_SOURCE_FILTER    1024    



union sockunion {
    struct sockaddr_storage    ss;
    struct sockaddr        sa;
    struct sockaddr_in    sin;
    struct sockaddr_in6    sin6;
};
typedef union sockunion sockunion_t;


typedef struct in_msource {
    TAILQ_ENTRY(in_msource) ims_next;   
    struct sockaddr_storage ims_addr;   
}IN_MSOURCE_S;


typedef struct in_multi
{
    union
    {
        
        struct    in_addr_4in6 ie46_multi;
        IN6_ADDR_S ie6_multi;
    } inm_dependfaddr;
    UINT inm_ifindex;       
#define    inm_addr    inm_dependfaddr.ie46_multi.ia46_addr4
#define    in6m_addr    inm_dependfaddr.ie6_multi
}IN_MULTI_S;



#define MCAST_UNDEFINED 0   
#define MCAST_INCLUDE   1   
#define MCAST_EXCLUDE   2   



struct ip_mreq {
    struct in_addr imr_multiaddr;  
    IF_INDEX imr_interface;  
};


struct ip_mreqn {
    UINT imr_multiaddr;  
    UINT imr_address;    
    IF_INDEX imr_ifindex;    
};


struct ip_mreq_source {
    UINT imr_multiaddr;  
    UINT imr_sourceaddr; 
    IF_INDEX imr_interface;  
};


struct group_req {
    IF_INDEX        gr_interface;   
    struct sockaddr_storage gr_group;   
};


typedef struct in_mfilter
{
    USHORT    imf_fmode;      
    USHORT    imf_nsources;   
    TAILQ_HEAD(, in_msource) imf_sources;   
}IN_MFILTER_S;


typedef struct ip_moptions {
    UINT   imo_multicast_if;            
    UCHAR  imo_multicast_ttl;           
    UCHAR  imo_multicast_loop;          
    USHORT imo_num_memberships;         
    USHORT imo_max_memberships;         
    USHORT imo_msmthcursor;             
    USHORT imo_mpullcursor;             
    IN_MULTI_S **imo_membership;        
    IN_MFILTER_S *imo_mfilters;         
}IP_MOPTIONS_S;

struct group_source_req {
    IF_INDEX        gsr_interface;  
    struct sockaddr_storage gsr_group;  
    struct sockaddr_storage gsr_source; 
};

UINT imo_match_group(IN IP_MOPTIONS_S *imo, IN UINT ifindex, IN SOCKADDR_S *group);
IN_MSOURCE_S * imo_match_source(IN IP_MOPTIONS_S *imo, IN UINT gidx, IN SOCKADDR_S *src);
void inp_freemoptions(IN IP_MOPTIONS_S *imo);
int inp_setmoptions(IN INPCB_S *inp, IN SOCKOPT_S *sopt);
int inp_getmoptions(struct inpcb *inp, struct sockopt *sopt);

#ifdef __cplusplus
    }
#endif 

#endif 


