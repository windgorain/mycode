#include "bs.h"

#include "utl/pcap_file.h"

FILE * PCAPFILE_Open(char *pcFileName, char *flag)
{
    FILE *fp;

    fp = fopen(pcFileName, flag);
    if (fp == NULL) {
        return NULL;
    }

    return fp;
}

void PCAPFILE_WriteHeader(FILE *fp, int linktype)
{
    PCAPFILE_HEADER_S header = {0};

    header.magic = 0xa1b2c3d4;
    header.major = 2;
    header.minor = 4;
    header.snaplen = 65535;
    header.link_type = linktype;

    fwrite(&header, 1, sizeof(header), fp);
}

int PCAPFILE_WritePkt(FILE *fp, void *pkt, struct timeval *ts,
        UINT cap_len, UINT pkt_len)
{
    fwrite(ts, 1, sizeof(UINT64), fp);
    fwrite(&cap_len, 1, 4, fp);
    fwrite(&pkt_len, 1, 4, fp);

    fwrite(pkt, 1, pkt_len, fp);

    fflush(fp);

    return 0;
}

int PCAPFILE_ReadHeader(FILE *fp, OUT PCAPFILE_HEADER_S *header)
{
    int ret;

    fseek(fp, 0, SEEK_SET);
    ret = fread(header, 1, sizeof(PCAPFILE_HEADER_S), fp);
    if (ret != sizeof(PCAPFILE_HEADER_S)) {
        return -1;
    }

    return 0;
}

int PCAPFILE_ReadPkt(FILE *fp, OUT void *data, IN UINT data_size, OUT PCAP_PKT_HEADER_S *header)
{
    UINT read_size;
    int ret;

    ret = fread(header, 1, sizeof(PCAP_PKT_HEADER_S), fp);
    if (ret != sizeof(PCAP_PKT_HEADER_S)) {
        return -1;
    }

    read_size = MIN(header->cap_len, data_size);
    ret = fread(data, 1, read_size, fp);
    if (ret != read_size) {
        return -1;
    }

    if (read_size < header->cap_len) {
        fseek(fp, header->cap_len - read_size, SEEK_CUR);
    }

    return 0;
}

void PCAPFILE_Close(FILE *fp)
{
    fclose(fp);
}

