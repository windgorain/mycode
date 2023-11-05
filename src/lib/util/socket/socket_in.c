/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/socket_utl.h"

#define IN6ADDRSZ       16
#define INADDRSZ         4
#define INT16SZ          2

static int inet_pton4(const char *src, unsigned char *dst)
{
  static const char digits[] = "0123456789";
  int saw_digit, octets, ch;
  unsigned char tmp[INADDRSZ], *tp;

  saw_digit = 0;
  octets = 0;
  tp = tmp;
  *tp = 0;
  while((ch = *src++) != '\0') {
    const char *pch;

    if((pch = strchr(digits, ch)) != NULL) {
      unsigned int val = *tp * 10 + (unsigned int)(pch - digits);

      if(saw_digit && *tp == 0)
        return (0);
      if(val > 255)
        return (0);
      *tp = (unsigned char)val;
      if(! saw_digit) {
        if(++octets > 4)
          return (0);
        saw_digit = 1;
      }
    }
    else if(ch == '.' && saw_digit) {
      if(octets == 4)
        return (0);
      *++tp = 0;
      saw_digit = 0;
    }
    else
      return (0);
  }
  if(octets < 4)
    return (0);
  memcpy(dst, tmp, INADDRSZ);
  return (1);
}

static int inet_pton6(const char *src, unsigned char *dst)
{
  static const char xdigits_l[] = "0123456789abcdef",
    xdigits_u[] = "0123456789ABCDEF";
  unsigned char tmp[IN6ADDRSZ], *tp, *endp, *colonp;
  const char *xdigits, *curtok;
  int ch, saw_xdigit;
  size_t val;

  memset((tp = tmp), 0, IN6ADDRSZ);
  endp = tp + IN6ADDRSZ;
  colonp = NULL;
  
  if(*src == ':')
    if(*++src != ':')
      return (0);
  curtok = src;
  saw_xdigit = 0;
  val = 0;
  while((ch = *src++) != '\0') {
    const char *pch;

    if((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
      pch = strchr((xdigits = xdigits_u), ch);
    if(pch != NULL) {
      val <<= 4;
      val |= (pch - xdigits);
      if(++saw_xdigit > 4)
        return (0);
      continue;
    }
    if(ch == ':') {
      curtok = src;
      if(!saw_xdigit) {
        if(colonp)
          return (0);
        colonp = tp;
        continue;
      }
      if(tp + INT16SZ > endp)
        return (0);
      *tp++ = (unsigned char) (val >> 8) & 0xff;
      *tp++ = (unsigned char) val & 0xff;
      saw_xdigit = 0;
      val = 0;
      continue;
    }
    if(ch == '.' && ((tp + INADDRSZ) <= endp) &&
        inet_pton4(curtok, tp) > 0) {
      tp += INADDRSZ;
      saw_xdigit = 0;
      break;    
    }
    return (0);
  }
  if(saw_xdigit) {
    if(tp + INT16SZ > endp)
      return (0);
    *tp++ = (unsigned char) (val >> 8) & 0xff;
    *tp++ = (unsigned char) val & 0xff;
  }
  if(colonp != NULL) {
    
    const size_t n = tp - colonp;
    size_t i;

    if(tp == endp)
      return (0);
    for (i = 1; i <= n; i++) {
      endp[- i] = colonp[n - i];
      colonp[n - i] = 0;
    }
    tp = endp;
  }
  if(tp != endp)
    return (0);
  memcpy(dst, tmp, IN6ADDRSZ);
  return (1);
}


int inet_pton(int af,const char *src,void *dst)
{
    switch (af) {
    case AF_INET:
        return (inet_pton4(src, (u_char *)dst));
    case AF_INET6:
        return (inet_pton6(src, (u_char *)dst));
    default:
        return (-1);
    }
}



BS_STATUS INET_ADDR_Str2IP(IN USHORT usFamily, IN const CHAR *pcStr, OUT INET_ADDR_S *pstAddr)
{
    if( inet_pton(usFamily, pcStr, (VOID *)&(pstAddr->un_addr)) > 0 )
    {
        INET_ADDR_FAMILY(pstAddr) = usFamily;
        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_FAILED;
    }
}


BS_STATUS INET_ADDR_N_Str2IP(IN USHORT usFamily, IN const CHAR *pcStr, IN UINT uiStrLen, OUT INET_ADDR_S *pstAddr)
{
    CHAR szTmp[INET_ADDR_STR_LEN + 1];

    if (uiStrLen >= sizeof(szTmp))
    {
        return BS_ERR;
    }

    memcpy(szTmp, pcStr, uiStrLen);
    szTmp[uiStrLen] = '\0';
    
    return INET_ADDR_Str2IP(usFamily, szTmp, pstAddr);
}


