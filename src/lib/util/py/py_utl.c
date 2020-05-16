/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-12-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/string_utl.h"
#include "utl/py_utl.h"
#include "python/Python.h"

/* 初始化Py环境 */
BS_STATUS PY_Init()
{
    if (Py_IsInitialized())
    {
        return BS_OK;
    }

    Py_Initialize();

    if (! Py_IsInitialized())
    {
        return BS_ERR;
    }

    return BS_OK;
}

VOID PY_Finit()
{
    Py_Finalize();
}

VOID PY_RunString(IN CHAR *pcString)
{
    PyRun_SimpleString(pcString);
}

VOID PY_RunFile(IN CHAR *pcFile)
{
    CHAR szTmp[FILE_MAX_PATH_LEN + 1];

    snprintf(szTmp, sizeof(szTmp), "exec(open(\'%s\').read())", pcFile);

    PY_RunString(szTmp);
}

VOID PY_DecRef(IN VOID *pPyObject)
{
    if (pPyObject)
    {
        Py_DECREF(pPyObject);
    }
}

/* 获取Python模块 */
VOID * PY_ImportModule(IN CHAR *pcModule)
{
    return PyImport_ImportModule(pcModule);
}

VOID * PY_GetAttrString(IN VOID *pPyObj, IN CHAR *pcAttrName)
{
    return PyObject_GetAttrString(pPyObj, pcAttrName);
}

VOID * PY_NewClassInstance(IN VOID *pMod, IN CHAR *pcClassName, IN CHAR *pcFormat, ...)
{
    PyObject *pParm = NULL, *pClass, *pInstance;
    va_list    vargs;

    /* 获取类 */
    pClass = PY_GetAttrString(pMod, pcClassName);
    if (!pClass)
    {
        return NULL;
    }

    /* 创建参数 */
    if (pcFormat)
    {
        va_start(vargs, pcFormat);
        pParm = Py_VaBuildValue(pcFormat, vargs);
        va_end(vargs);

        if (!pParm)
        {
            PY_DecRef(pClass);
        }
    }

    /* 生成一个对象 */
    pInstance = PyEval_CallObject(pClass, pParm);

    PY_DecRef(pParm);
    PY_DecRef(pClass);

    return pInstance;
}

/* 调用Python方法 */
VOID * PY_CallObject(IN VOID *pMethod, IN CHAR *pcFormat, ...)
{
    PyObject * pParm = NULL;
    PyObject * pRetVal;
    va_list    vargs;

    /* 创建参数 */
    if (pcFormat)
    {
        va_start(vargs, pcFormat);
        pParm = Py_VaBuildValue(pcFormat, vargs);
        va_end(vargs);

        if (! pParm)
        {
            return NULL;
        }
    }

    /* 函数调用 */
    pRetVal = PyEval_CallObject(pMethod, pParm);

    PY_DecRef(pParm);

    return pRetVal;
}

VOID * PY_SimpleCallFunction(IN CHAR *pcModuleName, IN CHAR *pcFunctionName, IN CHAR *pcFormat, ...)
{
    PyObject * pMod;
    PyObject * pFunc;
    PyObject * pParm = NULL;
    PyObject * pRetVal;
    va_list    vargs;

    pMod = PY_ImportModule(pcModuleName);
    if (!pMod)
    {
        return NULL;
    }

    pFunc = PY_GetAttrString(pMod, pcFunctionName);
    if (!pFunc)
    {
        PY_DecRef(pMod);
        return NULL;
    }

    /* 创建参数 */
    if (pcFormat)
    {
        va_start(vargs, pcFormat);
        pParm = Py_VaBuildValue(pcFormat, vargs);
        va_end(vargs);

        if (! pParm)
        {
            PY_DecRef(pFunc);
            PY_DecRef(pMod);
            return NULL;
        }
    }

    /* 函数调用 */
    pRetVal = PyEval_CallObject(pFunc, pParm);

    PY_DecRef(pParm);
    PY_DecRef(pFunc);
    PY_DecRef(pMod);

    return pRetVal;
}

VOID * PY_SimpleCallMethod(IN CHAR *pcModuleName, IN CHAR *pcClass, IN CHAR *pcMethod, IN CHAR *pcFormat, ...)
{
    PyObject * pMod;
    PyObject * pInstance;
    PyObject * pMethod;
    PyObject * pParm = NULL;
    PyObject * pRetVal;
    va_list    vargs;

    pMod = PY_ImportModule(pcModuleName);
    if (!pMod)
    {
        return NULL;
    }

    pInstance = PY_NewClassInstance(pMod, pcClass, NULL);
    if (!pInstance)
    {
        PY_DecRef(pMod);
        return NULL;
    }

    pMethod = PY_GetAttrString(pInstance, pcMethod);
    if (!pMethod)
    {
        PY_DecRef(pInstance);
        PY_DecRef(pMod);
        return NULL;
    }

    /* 创建参数 */
    if (pcFormat)
    {
        va_start(vargs, pcFormat);
        pParm = Py_VaBuildValue(pcFormat, vargs);
        va_end(vargs);

        if (! pParm)
        {
            PY_DecRef(pMethod);
            PY_DecRef(pInstance);
            PY_DecRef(pMod);
            return NULL;
        }
    }

    /* 函数调用 */
    pRetVal = PyEval_CallObject(pMethod, pParm);

    PY_DecRef(pParm);
    PY_DecRef(pMethod);
    PY_DecRef(pInstance);
    PY_DecRef(pMod);

    return pRetVal;
}

INT PY_ParseArgAsInt(IN VOID *pArg)
{
    INT iRet;
    
    PyArg_Parse(pArg, "i", &iRet);

    return iRet;
}

CHAR * PY_ParseArgAsString(IN VOID *pArg)
{
    CHAR *pcRet;
    
    PyArg_Parse(pArg, "s", &pcRet);

    return pcRet;
}

