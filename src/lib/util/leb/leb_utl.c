/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/leb_utl.h"

/*
 * LEB128（Little Endian Base 128） 变长编码格式
 * 对于 32 位整数，编码后可能是 1 到 5 个字节
 * 对于 64 位整数，编码后可能是 1 到 10 个字节
 * 1. 采用小端编码方式，即低位字节在前，高位字节在后
 * 2. 采用 128 进制，每 7 个比特为一组，由一个字节的后 7 位承载，空出来的最高位是标记位，1 表示后面还有后续字节
*/
static U64 _leb_read(UCHAR *bytes, UINT *pos, BOOL_T sign)
{
    UINT64 result = 0;
    UINT64 byte;
    UINT shift = 0;

    while (1) {
        byte = bytes[*pos];
        *pos += 1;
        // 取字节中后 7 位作为值插入到 result 中
        result |= ((byte & 0x7f) << shift);
        shift += 7;
        // 如果某个字节的最高位为 0，表示该字节为最后一个字节，没有后续字节了
        if ((byte & 0x80) == 0) {
            break;
        }
    }

    // 有符号整数
    if (sign && (byte & 0x40)) {
        result |= -(1 << shift);
    }

    return result;
}

/* 无符号数 */
UINT64 LEB_Read(IN UCHAR *bytes, INOUT UINT *pos)
{
    return _leb_read(bytes, pos, 0);
}

/* 有符号数 */
UINT64 LEB_ReadSigned(IN UCHAR *bytes, INOUT UINT *pos)
{
    return _leb_read(bytes, pos, 1);
}

