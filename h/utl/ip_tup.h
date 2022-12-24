/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _IP_TUP_H
#define _IP_TUP_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    USHORT sport;
    USHORT dport;
}L4_PORTS_S;

typedef struct {
    unsigned int sip;
    unsigned int dip;
}IP_2TUPS_S;

typedef struct {
    unsigned char ip_proto;
    unsigned int sip;
    unsigned int dip;
}IP_3TUPS_S;

typedef struct {
    unsigned int sip;
    unsigned int dip;
    unsigned short sport;
    unsigned short dport;
}IP_4TUPS_S;

typedef struct {
    unsigned char ip_proto;
    unsigned int sip;
    unsigned int dip;
    unsigned short sport;
    unsigned short dport;
}IP_5TUPS_S;


#ifdef __cplusplus
}
#endif
#endif //IP_TUP_H_
