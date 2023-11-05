

#ifndef __VNET_USER_STATUS_H_
#define __VNET_USER_STATUS_H_

#ifdef __cplusplus
    extern "C" {
#endif 


typedef enum
{
    VNET_USER_STATUS_INIT = 0,                      
    VNET_USER_STATUS_OFFLINE,                       
    VNET_USER_STATUS_CONNECTING,                    
    VNET_USER_STATUS_GET_VER_ING,                   
    VNET_USER_STATUS_GET_VER_OK,                    
    VNET_USER_STATUS_CONSULT_SEC_ING,               
    VNET_USER_STATUS_CONSULT_SEC_OK,                
    VNET_USER_STATUS_AUTH_ING,                      
    VNET_USER_STATUS_ONLINE,                            

    VNET_USER_STATUS_MAX
}VNET_USER_STATUS_E;


typedef enum
{
    VNET_USER_REASON_NONE = 0,                      

    VNET_USER_REASON_VER_NOT_MATCH,                 
    VNET_USER_REASON_DYNAMIC_USER_LIMIT_REACHED,    
    VNET_USER_REASON_BUSY,                          
    VNET_USER_REASON_NEED_MONEY,                    
    VNET_USER_REASON_TMP_STOP_SERVICE,              
    VNET_USER_REASON_ALREADY_LOGIN,                 
    VNET_USER_REASON_SERVER_NO_ACK,                 
    VNET_USER_REASON_AUTH_FAILED,                   
    VNET_USER_REASON_NO_RESOURCE,                   
    VNET_USER_REASON_CONNECT_FAILED,                

    VNET_USER_REASON_MAX
}VNET_USER_REASON_E;

#ifdef __cplusplus
    }
#endif 

#endif 

