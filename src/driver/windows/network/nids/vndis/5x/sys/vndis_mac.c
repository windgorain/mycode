/******************************************************************************
* Copyright (C), LiXingang
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/
#include "ndis.h"
    
#include "vndis_def.h"    
#include "vndis_pub.h"
#include "vndis_que.h"
#include "vndis_dev.h"
#include "vndis_adapter.h"
#include "vndis_instance.h"
#include "vndis_mem.h"
#include "vndis_mac.h"

#define IsMacDelimiter(a) (a == ':' || a == '-' || a == '.')
#define IsHexDigit(c) ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))


int HexStringToDecimalInt (char p_Character)
{
  int l_Value = 0;

  if (p_Character >= 'A' && p_Character <= 'F')
    l_Value = (p_Character - 'A') + 10;
  else if (p_Character >= 'a' && p_Character <= 'f')
    l_Value = (p_Character - 'a') + 10;
  else if (p_Character >= '0' && p_Character <= '9')
    l_Value = p_Character - '0';

  return l_Value;
}

BOOLEAN VNDIS_MAC_ParseMAC (OUT MACADDR dest, IN char *src)
{
  char c;
  int mac_index = 0;
  BOOLEAN high_digit = FALSE;
  int delim_action = 1;

  NdisZeroMemory(dest, sizeof (MACADDR));

  while ((c = *src++) != '\0')
    {
      if (IsMacDelimiter (c))
	{
	  mac_index += delim_action;
	  high_digit = FALSE;
	  delim_action = 1;
	}
      else if (IsHexDigit (c))
	{
	  const int digit = HexStringToDecimalInt (c);
	  if (mac_index < sizeof (MACADDR))
	    {
	      if (!high_digit)
		{
		  dest[mac_index] = (char)(digit);
		  high_digit = TRUE;
		  delim_action = 1;
		}
	      else
		{
		  dest[mac_index] = (char)(dest[mac_index] * 16 + digit);
		  ++mac_index;
		  high_digit = FALSE;
		  delim_action = 0;
		}
	    }
	  else
	    return FALSE;
	}
      else
	return FALSE;
    }

  return (mac_index + delim_action) >= sizeof (MACADDR);
}

VOID VNDIS_MAC_GenerateRandomMac (OUT MACADDR mac, IN UCHAR *adapter_name)
{
  unsigned const char *cp = adapter_name;
  unsigned char c;
  unsigned int i = 2;
  unsigned int byte = 0;
  int brace = 0;
  int state = 0;

  NdisZeroMemory (mac, sizeof (MACADDR));

  mac[0] = 0x00;
  mac[1] = 0xFF;

  while ((c = *cp++) != '\0')
    {
      if (i >= sizeof (MACADDR))
	break;
      if (c == '{')
	brace = 1;
      if (IsHexDigit (c) && brace)
	{
	  const unsigned int digit = HexStringToDecimalInt (c);
	  if (state)
	    {
	      byte <<= 4;
	      byte |= digit;
	      mac[i++] = (unsigned char) byte;
	      state = 0;
	    }
	  else
	    {
	      byte = digit;
	      state = 1;
	    }
	}
    }
}


