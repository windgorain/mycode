/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/leb_utl.h"


static U64 _leb_read(UCHAR *bytes, UINT *pos, BOOL_T sign)
{
    UINT64 result = 0;
    UINT64 byte;
    UINT shift = 0;

    while (1) {
        byte = bytes[*pos];
        *pos += 1;
        
        result |= ((byte & 0x7f) << shift);
        shift += 7;
        
        if ((byte & 0x80) == 0) {
            break;
        }
    }

    
    if (sign && (byte & 0x40)) {
        result |= -(1 << shift);
    }

    return result;
}


UINT64 LEB_Read(IN UCHAR *bytes, INOUT UINT *pos)
{
    return _leb_read(bytes, pos, 0);
}


UINT64 LEB_ReadSigned(IN UCHAR *bytes, INOUT UINT *pos)
{
    return _leb_read(bytes, pos, 1);
}

