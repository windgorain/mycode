#include "wmi.h"

int main()
{
	WMILib cWmi;

	cWmi.Init("ROOT\\CIMV2");

	cWmi.ModifyDns("8.8.8.8", "9.9.9.9");
	
	return 0;
}