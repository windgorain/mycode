/*
* Copyright (C), Xingang.Li
* File name     : azHead.h
* Date          : 2005/2/25
* Description   : 
*/

#ifndef __INCazHead_h
#define __INCazHead_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef NULL
#define NULL 0
#endif

#define	IN      /*IN*/
#define	OUT     /*OUT*/
#define	INOUT   /*INOUT*/

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef MAX
#define MAX(a,b)  ((a)>(b) ? (a) : (b))
#define MIN(a,b)  ((a)<(b) ? (a) : (b))
#endif

/* 出错返回-1 */
#define SNPRINTF(buf,size, ...) ({ \
        int _nlen = snprintf((buf), (size), __VA_ARGS__); \
        if (_nlen >= (size)) _nlen = -1; \
        _nlen; })

/* 计算宏定义中可变参数个数 */
#define _BS_ARG_N(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,N,...) N
#define BS_ARG_COUNT(...) _BS_ARG_N(__VA_ARGS__,10,9,8,7,6,5,4,3,2,1,0)

/* 获取可变参数中的第几个参数 */
#define _BS_ARG_GET1(a1,...) (a1)
#define _BS_ARG_GET2(a1,a2,...) (a2)
#define _BS_ARG_GET3(a1,a2,a3,...) (a3)
#define _BS_ARG_GET4(a1,a2,a3,a4,...) (a4)
#define _BS_ARG_GET5(a1,a2,a3,a4,a5,...) (a5)
#define _BS_ARG_GET6(a1,a2,a3,a4,a5,a6,...) (a6)
#define _BS_ARG_GET7(a1,a2,a3,a4,a5,a6,a7,...) (a7)
#define _BS_ARG_GET8(a1,a2,a3,a4,a5,a6,a7,a8,...) (a8)
#define _BS_ARG_GET9(a1,a2,a3,a4,a5,a6,a7,a8,a9,...) (a9)
#define _BS_ARG_GET10(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,...) (a10)
#define BS_ARG_GET(N,...) _BS_ARG_GET##N(__VA_ARGS__,0,0,0,0,0,0,0,0,0,0)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef BOOL_TRUE
#define BOOL_TRUE TRUE
#endif

#ifndef BOOL_FALSE
#define BOOL_FALSE FALSE
#endif

#ifndef false
#define true 1
#define false 0
#endif

#ifndef bool
typedef char bool;
#endif

#ifndef STATIC
#define STATIC static
#endif

#define VOID    void
#define VRF_ANY (VRF_INDEX)0

#ifdef IN_WINDOWS
    #define CONSTRUCTOR(f)  \
        static void f(void);  \
        static int __f1(void){f();return 0;}  \
        __pragma(data_seg(".CRT$XIU"))  \
        static int(*__f2) (void) = __f1;  \
        __pragma(data_seg())  \
        static void f(void)

    #define DESTRUCTOR(fin) \
        static void fin(void);  \
        static int __fin1(void){fin();return 0;}  \
        __pragma(data_seg(".CRT$XPU"))  \
        static int(*__fin2) (void) = __fin1;  \
        __pragma(data_seg())  \
        static void fin(void)

    #define PLUG_HIDE
    #define PLUG_API  __declspec(dllexport)
    #define PLUG_ID    HINSTANCE 
    #define PLUG_LOAD(pcPlugFilePath)                  (PLUG_ID)LoadLibraryA(pcPlugFilePath)
    #define PLUG_FREE(ulPlugId)                        FreeLibrary(ulPlugId)
    #define PLUG_GET_FUNC_BY_NAME(ulPlugId,pcFuncName) GetProcAddress(ulPlugId, pcFuncName)
    #define THREAD_LOCAL __declspec(thread) 
#endif

#ifdef IN_UNIXLIKE
#define CONSTRUCTOR(_init)  __attribute__((constructor)) static void _init(void)
#define DESTRUCTOR(_final) __attribute__((destructor)) static void _final(void)
#define PLUG_API  __attribute__ ((visibility("default")))
#define PLUG_HIDE __attribute__ ((visibility("hidden"))) 
#define PLUG_ID    void*
#define PLUG_LOAD(pcPlugFilePath)   PLUG_LoadLib(pcPlugFilePath)
#define PLUG_FREE(ulPlugId)         PLUG_UnloadLib(ulPlugId)
#define PLUG_GET_FUNC_BY_NAME(ulPlugId,pcFuncName)    dlsym((PLUG_ID)ulPlugId,pcFuncName)
#define THREAD_LOCAL __thread
#endif

#define UINT_HANDLE(uiValue)     ((HANDLE)((ULONG)((UINT)(uiValue))))
#define HANDLE_UINT(hValue)     ((UINT)((ULONG)(hValue)))
#define ULONG_HANDLE(uiValue)     ((HANDLE)((ULONG)(uiValue)))
#define HANDLE_ULONG(hValue)     (((ULONG)(hValue)))

#define STR(x)  #x

typedef short		        SHORT;
typedef unsigned short		USHORT;
typedef unsigned int 		UINT; /* 32 bits */
typedef unsigned long long  UINT64;
typedef long long           INT64;
typedef int 				INT; /* 32 bits */
typedef unsigned char 		UCHAR;
typedef char			    CHAR;
typedef char                BOOL_T;
typedef void*               HANDLE;
typedef unsigned long       ULONG;  /* 变长的类型,32位系统上4个字节,64位系统8个字节 */
typedef long                LONG;   /* 变长的类型,32位系统上4个字节,64位系统8个字节 */
typedef UINT                VRF_INDEX;

typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef unsigned long long  U64;

typedef char S8;
typedef short S16;
typedef int S32;
typedef long long S64;

typedef struct {
    char *pcData;
    UINT uiLen;
}LSTR_S;

typedef struct {
    UCHAR *pucData;
    UINT uiLen;
}LDATA_S;
#define BS_DATA_ZERO(_pstData) do {(_pstData)->pucData = NULL; (_pstData)->uiLen = 0;} while(0)

typedef VOID 		(*VOID_FUNC)(void);
typedef UINT 		(*UINT_FUNC)(void);
typedef UINT 		(*UINT_FUNC_1)(VOID *pArg1);
typedef UINT 		(*UINT_FUNC_2)(VOID *pArg1, VOID *pArg2);
typedef UINT 		(*UINT_FUNC_3)(VOID *pArg1, VOID *pArg2, VOID *pArg3);
typedef UINT 		(*UINT_FUNC_4)(VOID *pArg1, VOID *pArg2, VOID *pArg3, VOID *pArg4);
typedef UINT 		(*UINT_FUNC_5)(VOID *pArg1, VOID *pArg2, VOID *pArg3, VOID *pArg4, VOID *pArg5);
typedef UINT 		(*UINT_FUNC_6)(VOID *pArg1, VOID *pArg2, VOID *pArg3, VOID *pArg4, VOID *pArg5, VOID *pArg6);
typedef HANDLE 		(*HANDLE_FUNC)(void);
typedef BOOL_T		(*BOOL_FUNC)(void);
typedef int         (*PF_CMP_FUNC)(void *n1, void *n2, void *ud);
typedef int         (*PF_PRINT_FUNC)(const char *fmt, ...);

typedef enum{
    /* 通用定义 */
	BS_OK = 0,
	BS_ERR = -1,
	BS_NO_SUCH = -2,
	BS_ALREADY_EXIST = -3,
	BS_BAD_PTR = -4,
	BS_CAN_NOT_OPEN = -5,
	BS_WRONG_FILE = -6,
	BS_NOT_SUPPORT = -7,
	BS_OUT_OF_RANGE = -8,
	BS_TIME_OUT = -9,
	BS_NO_MEMORY = -10,
	BS_NULL_PARA = -11,
	BS_NO_RESOURCE = -12,
	BS_BAD_PARA = -13,
	BS_NO_PERMIT = -14,
	BS_FULL = -15,
	BS_EMPTY = -16,
	BS_PAUSE = -17,
	BS_STOP  = -18,
	BS_CONTINUE = -19,
	BS_NOT_FOUND = -20,
	BS_NOT_COMPLETE = -21,
	BS_CAN_NOT_CONNECT = -22,
	BS_CONFLICT = -23,
	BS_TOO_LONG = -24,
	BS_TOO_SMALL = -25,
	BS_BAD_REQUEST = -26,
	BS_AGAIN = -27,
	BS_CAN_NOT_WRITE = -28,
	BS_NOT_READY = -29,
	BS_PROCESSED= -30,
	BS_PEER_CLOSED = -31,
	BS_NOT_MATCHED = -32,
	BS_VERIFY_FAILED = -33,
	BS_NOT_INIT = -34,
	BS_REF_NOT_ZERO = -35, /* 引用计数不为0 */
    BS_BUSY = -36,
    BS_PARSE_FAILED = -37,
	BS_REACH_MAX = -38,
    BS_STOLEN = -39,

    /* 模块私有定义,在BS_PRIVATE_BASE 基础上增加 */
    BS_PRIVATE_BASE = -100
}BS_STATUS;

typedef BS_STATUS   (*BS_STATUS_FUNC)(void);
extern CHAR * ErrInfo_Get(IN BS_STATUS eRet);

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS BS_OK
#endif

#ifndef ERROR_FAILED
#define ERROR_FAILED BS_ERR
#endif

typedef enum{
    BS_WALK_CONTINUE = 0,
    BS_WALK_STOP
}BS_WALK_RET_E;

typedef enum {
	BS_ACTION_UNDEF = 0,
    BS_ACTION_DENY,
    BS_ACTION_PERMIT,
    
    
    BS_ACTION_MAX
}BS_ACTION_E;

typedef enum {
    BS_MATCH = 0,   /* 完全匹配 */
    BS_PART_MATCH,  /* 部分匹配 */
    BS_NOT_MATCH    /* 不匹配 */
}BS_MATCH_RET_E;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*azHead.h*/



