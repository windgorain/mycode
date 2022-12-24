
#ifndef __UTL_EXTURL_H_
#define __UTL_EXTURL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define EXTURL_MAX_PROTOCOL_LEN 15
#define EXTURL_MAX_ADDRESS_LEN  255
#define EXTURL_MAX_PATH_LEN     255

typedef struct
{
	CHAR szProtocol[EXTURL_MAX_PROTOCOL_LEN + 1];
	CHAR szAddress[EXTURL_MAX_ADDRESS_LEN + 1];
	CHAR szPath[EXTURL_MAX_PATH_LEN + 1];
	USHORT usPort;
}EXTURL_S;

/* 
 扩展的URL地址解析.
   通常的地址是 http://xxx.com:port;  https://xxx.com:port
   扩展一下可以为: yyy://xxx.com:port
*/

/* 解析地址,将地址解析为协议/地址/端口 */
BS_STATUS EXTURL_Parse
(
	IN CHAR *pcExtUrl,
	OUT EXTURL_S *pstExtUrl
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /* __UTL_EXTURL_H_ */

