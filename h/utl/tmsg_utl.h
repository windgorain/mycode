#ifndef __TMSG_UTL_H_
#define __TMSG_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define TMSG_FLAG_IS_ACK 0x1  /* 是ACK报文 */
#define TMSG_FLAG_NEED_ACK 0x10  /* 需要接收方回应ACK */


#define TMSG_EVENT_SEND     1   /* 发送消息 */
#define TMSG_EVENT_SUCCESS  2   /* 成功. 对于需要回ACK的报文，收到ACK后才认为成功.
                                   对于不需要ACK的报文, 发送指定次数之后认为成功 */
#define TMSG_EVENT_FAILED   3   /* 失败 */

#define TMSG_IS_ACK_MSG(pstMsg) ((pstMsg)->uiFlag & TMSG_FLAG_IS_ACK)
#define TMSG_NEED_MSG(pstMsg) ((pstMsg)->uiFlag & TMSG_FLAG_NEED_ACK)

typedef HANDLE TMSG_HANDLE;

typedef struct
{
    UINT uiFlag;
    UINT uiSequence;     /* 报文序号 */
}TMSG_S;


typedef void (*PF_TMSG_CALL_BACK_FUNC)(IN UINT uiEvent, IN void *pstOpt, IN void *ud);

typedef struct TMSG_OPT_S {
    TMSG_S stTmsg;
    UINT uiTicks;    /* 多长时间发送一次 */
    UINT uiRetryTimes; /* 重试次数. 0表示无限次数 */

    PF_TMSG_CALL_BACK_FUNC pfEventFunc;
}TMSG_OPT_S;

TMSG_HANDLE TMSG_Create();
VOID TMSG_Destory(IN TMSG_HANDLE hTmsgHandle);
BS_STATUS TMSG_Send
(
    IN TMSG_HANDLE hTmsgHandle,
    IN TMSG_OPT_S *pstOpt,
    IN USER_HANDLE_S *pstUserHandle
);
UINT TMSG_GetMsgNum(IN TMSG_HANDLE hTmsgHandle);
VOID TMSG_StopAll(IN TMSG_HANDLE hTmsgHandle);
BOOL_T TMSG_RecvedAck(IN TMSG_HANDLE hTmsgHandle, IN TMSG_S *pstTmsg);
VOID TMSG_TickStep(IN TMSG_HANDLE hTmsgHandle);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /* __TMSG_UTL_H_ */

