

#ifndef __IN_CHECKSUM_H_
#define __IN_CHECKSUM_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* 返回原始的checksum */
USHORT IN_CHKSUM_AddRaw(IN USHORT usCurrentSum, IN UCHAR *pucData, IN UINT uiDataLen);
USHORT IN_CHKSUM_DelRaw(IN USHORT usCurrentSum, IN UCHAR *pucData, IN UINT uiDataLen);

static inline USHORT IN_CHKSUM_AddRawWord(IN USHORT usCurrentSum, IN USHORT usWord/* 必须是主机序的Word */)
{
    UINT uiCheckSum = usCurrentSum;

    uiCheckSum += usWord;

    /* 将高16叠加到低16位 */
    if (uiCheckSum & 0xffff0000)
    {
        uiCheckSum = (uiCheckSum & 0xFFFF) + ((uiCheckSum >> 16) & 0xFFFF);
    }

    return (USHORT) uiCheckSum;
}

/* 返回原始checksum */
static inline USHORT IN_CHKSUM_Raw(IN UCHAR *pucData, IN UINT uiDataLen)
{
    return IN_CHKSUM_AddRaw(0, pucData, uiDataLen);
}

/* 将原始checksum转变为报文需要的checksum */
static inline USHORT IN_CHKSUM_Wrap(IN USHORT usRawCheckSum)
{
    USHORT usSum = ~usRawCheckSum;
	return htons(usSum);
}

/* 将报文checksum转变为原始checksum */
static inline USHORT IN_CHKSUM_UnWrap(IN USHORT usCheckSum)
{
    USHORT usSum = ~usCheckSum;
	return htons(usSum);
}

/* 返回报文需要的checksum */
static inline USHORT IN_CHKSUM_CheckSum(IN UCHAR *pucData, IN UINT uiDataLen)
{
    return IN_CHKSUM_Wrap(IN_CHKSUM_Raw(pucData, uiDataLen));
}

/* 增量计算checksum */
static inline USHORT IN_CHKSUM_Change(USHORT *olddata, int oldcount,
        USHORT *newdata, int newcount, USHORT old_checksum)
{
    UINT sum;
    int i;

    sum = (~old_checksum) & 0xffff;

    for (i=0; i<oldcount; i++) {
        sum += (~olddata[i] & 0xffff);
    }
    for (i=0; i<newcount; i++) {
        sum += newdata[i];
    }

    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);

    if (sum == 0xffff) {
        return sum;
    }

    return (~sum) & 0xffff;
}

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IN_CHECKSUM_H_*/

