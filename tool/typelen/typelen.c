
typedef struct
{
    char *pszType;
    unsigned long ulLen;
}_TYPES_LEN_S;

#define _TYPE_LEN(type)  {#type, sizeof(type)}

static _TYPES_LEN_S g_stTypes[] = 
{
    _TYPE_LEN(char),
    _TYPE_LEN(unsigned char),
    _TYPE_LEN(short),
    _TYPE_LEN(unsigned short),
    _TYPE_LEN(int),
    _TYPE_LEN(unsigned int),
    _TYPE_LEN(long),
    _TYPE_LEN(unsigned long),
    _TYPE_LEN(void*),
    _TYPE_LEN(long long),
};

void main()
{
    int i;

    for (i=0; i<sizeof(g_stTypes) / sizeof(_TYPES_LEN_S); i++)
    {
        printf("%-15s : %d\r\n", g_stTypes[i].pszType, g_stTypes[i].ulLen);
    }
}

