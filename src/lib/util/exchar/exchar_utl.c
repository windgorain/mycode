#include "bs.h"

#include "utl/exchar_utl.h"
#include "utl/ascii_utl.h"

typedef enum
{
    _CMD_EXP_CTRL_NORMAL = 0, 
    _CMD_EXP_CTRL_27,     
    _CMD_EXP_CTRL_27_91,     
    _CMD_EXP_CTRL_27_91_49,
    _CMD_EXP_CTRL_27_91_52,

    _CMD_EXP_CTRL_224, 
}_CMD_EXP_CTRL_STATUS_E;

typedef struct
{
    _CMD_EXP_CTRL_STATUS_E eStatus;
}_EXCHAR_CTRL_S;

typedef struct
{
    _CMD_EXP_CTRL_STATUS_E enCurrentStatus; 
    UCHAR ucCmdChar;  
    _CMD_EXP_CTRL_STATUS_E enNextStatus; 
    EXCHAR enResult;   
}_CMD_EXP_CTRL_STATE_MACHINE_S;



static _CMD_EXP_CTRL_STATE_MACHINE_S g_astCmdExpCtrlStateMachine[] = 
{
     
    {_CMD_EXP_CTRL_NORMAL, ASCII_CODE_ESC, _CMD_EXP_CTRL_27, EXCHAR_EXTEND_TEMP},
    {_CMD_EXP_CTRL_27, '[', _CMD_EXP_CTRL_27_91, EXCHAR_EXTEND_TEMP},
    {_CMD_EXP_CTRL_27_91, 'A', _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_UP},
    {_CMD_EXP_CTRL_27_91, 'B', _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_DOWN},
    {_CMD_EXP_CTRL_27_91, 'C', _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_RIGHT},
    {_CMD_EXP_CTRL_27_91, 'D', _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_LEFT},

      
    {_CMD_EXP_CTRL_27_91, 49, _CMD_EXP_CTRL_27_91_49, EXCHAR_EXTEND_TEMP},
    {_CMD_EXP_CTRL_27_91_49, 126, _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_HOME},

      
    {_CMD_EXP_CTRL_27_91, 52, _CMD_EXP_CTRL_27_91_52, EXCHAR_EXTEND_TEMP},
    {_CMD_EXP_CTRL_27_91_52, 126, _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_END},


      
    {_CMD_EXP_CTRL_NORMAL, 224, _CMD_EXP_CTRL_224, EXCHAR_EXTEND_TEMP},
    {_CMD_EXP_CTRL_224, 'H', _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_UP},
    {_CMD_EXP_CTRL_224, 'P', _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_DOWN},
    {_CMD_EXP_CTRL_224, 'K', _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_LEFT},
    {_CMD_EXP_CTRL_224, 'M', _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_RIGHT},

        
    {_CMD_EXP_CTRL_224, 71, _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_HOME},

        
    {_CMD_EXP_CTRL_224, 79, _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_END},

      
    {_CMD_EXP_CTRL_NORMAL, 1, _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_HOME},

    
    {_CMD_EXP_CTRL_NORMAL, 154, _CMD_EXP_CTRL_NORMAL, EXCHAR_EXTEND_TEMP},
};



HANDLE EXCHAR_Create()
{
    _EXCHAR_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_EXCHAR_CTRL_S));

    return pstCtrl;
}

void EXCHAR_Reset(IN HANDLE hExcharHandle)
{
    _EXCHAR_CTRL_S *pstCtrl = hExcharHandle;

    memset(pstCtrl, 0, sizeof(_EXCHAR_CTRL_S));
}

VOID EXCHAR_Delete(IN HANDLE hExcharHandle)
{
    _EXCHAR_CTRL_S *pstCtrl = hExcharHandle;

    MEM_Free(pstCtrl);
}


EXCHAR EXCHAR_Parse(IN HANDLE hExcharHandle, IN UCHAR ucChar)
{
    INT i;
    _EXCHAR_CTRL_S *pstCtrl = hExcharHandle;

    for (i=0; i<sizeof(g_astCmdExpCtrlStateMachine)/sizeof(_CMD_EXP_CTRL_STATE_MACHINE_S); i++)
    {
        if ((pstCtrl->eStatus == g_astCmdExpCtrlStateMachine[i].enCurrentStatus)
            && (ucChar == g_astCmdExpCtrlStateMachine[i].ucCmdChar))
        {
            pstCtrl->eStatus = g_astCmdExpCtrlStateMachine[i].enNextStatus;
            return g_astCmdExpCtrlStateMachine[i].enResult;
        }
    }

    if (pstCtrl->eStatus != _CMD_EXP_CTRL_NORMAL)
    {
        
        pstCtrl->eStatus = _CMD_EXP_CTRL_NORMAL;
        return EXCHAR_EXTEND_TEMP;
    }

    return (EXCHAR)ucChar;
}



