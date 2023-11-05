
#include "bs.h"

#ifndef IN_WINDOWS

#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utl/x509_dereplication.h"

typedef struct pub_sha_info {
    unsigned char hash[EVP_MAX_MD_SIZE * 2];
    int id;
} pub_sha_info_t;

int load_pem_files_in_one(const char *filename, STACK_OF(X509) **pcerts)
{
    int i;
    BIO *bio;
    STACK_OF(X509_INFO) *xis = NULL;
    X509_INFO *xi;

    bio = BIO_new_file(filename, "r");
    if (bio == NULL)
        return 0;

    xis = PEM_X509_INFO_read_bio(bio, NULL, NULL, NULL);
    BIO_free(bio);

    if (pcerts != NULL && *pcerts == NULL) {
        *pcerts = sk_X509_new_null();
        if (*pcerts == NULL)
            return 0;
    }

    for (i = 0; i < sk_X509_INFO_num(xis); i++) {
        xi = sk_X509_INFO_value(xis, i);
        if (xi->x509 != NULL && pcerts != NULL) {
            if (!sk_X509_push(*pcerts, xi->x509))
                return 0;
            xi->x509 = NULL;
        }
    }

    return 1;
}

int get_cert_fingerprint(X509 *cert, unsigned char *finger_value, int finger_size) 
{
    const EVP_MD *fprint_type;
    fprint_type = EVP_sha1();
    int j, offset = 0;
    unsigned int md_value_size;
    unsigned char md_value[EVP_MAX_MD_SIZE];

    if (!X509_digest(cert, fprint_type, md_value, &md_value_size)) {
        printf("Error creating the certificate fingerprint.\n");
        return 0;
    }

    for (j = 0; j < md_value_size; ++j) {
        offset += scnprintf((char*)finger_value + offset, finger_size - offset, "%02x", md_value[j]);

    }
    finger_value[offset] = 0;

    return 1;
}

int compare(const void * a, const void * b) 
{ 
    pub_sha_info_t *cmp1, *cmp2;
    cmp1 = (pub_sha_info_t*) a;
    cmp2 = (pub_sha_info_t*) b;
    return strcmp((char*)cmp1->hash, (char*)cmp2->hash); 
} 

int print_help(char *prog) {
    fprintf(stderr, "Usage: %s [-f] pem file/directory name, [-s] file/directory for merged pems\n",
            prog);
    return 0;
}

int certs_dereplication(char *src_file, char *dst_file, int *total, int *unique)
{
    STACK_OF(X509) *certs = NULL;
    char buf[1024];
    FILE *psavefile = NULL;
    struct stat st;
    int sz;

    if (stat(src_file, &st) != 0) {
        return 0;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(src_file); 
        if (!dir)  return 0;

        struct dirent *ent;
        while ((ent = readdir(dir))) {
            sz = scnprintf(buf, sizeof(buf), "%s/%s", src_file, ent->d_name);
            buf[sz] = 0;
            if ((stat(buf, &st) == 0) && S_ISREG(st.st_mode)) {
                load_pem_files_in_one(buf, &certs);
            }
        }
        closedir(dir);
    } else if (S_ISREG(st.st_mode)) {
        load_pem_files_in_one(src_file, &certs);
    }
 
    sz = sk_X509_num(certs);
    *total = sz;
    pub_sha_info_t *pub_sha_info_arr = malloc(sizeof(pub_sha_info_t) * sz);
    if (sz && pub_sha_info_arr) {
        X509 *cert = NULL;
        int j, duplicate = 0;
        for (j = 0; j < sz; j++) {
            cert = sk_X509_value(certs, j);
            get_cert_fingerprint(cert, pub_sha_info_arr[j].hash, sizeof(pub_sha_info_arr[j].hash));
            pub_sha_info_arr[j].id = j;
        }
        qsort(pub_sha_info_arr, sz, sizeof(pub_sha_info_t), compare);
        psavefile = fopen(dst_file, "wa+");
        if (!psavefile) {
            printf("dest file can't open\n");
            free(pub_sha_info_arr);
            return 0;
        }
#if 0
        BIO *b = BIO_new_fp(stdout, BIO_NOCLOSE | BIO_FP_TEXT );
#endif
        for (j = 0; j < sz; j++) {
            if (j >= 1 && strcmp((char*)pub_sha_info_arr[j].hash, (char*)pub_sha_info_arr[j - 1].hash) == 0) {
#if 0
                cert = sk_X509_value(certs, pub_sha_info_arr[j].id);
                X509_print(b, cert);
                printf("-----------");
                cert = sk_X509_value(certs, pub_sha_info_arr[j-1].id);
                X509_print(b, cert);
                printf("\n");
#endif
                duplicate++;
            } else {
                PEM_write_X509(psavefile, sk_X509_value(certs, pub_sha_info_arr[j].id));
            }
        }
#if 0
        BIO_free(b);
#endif
        fclose(psavefile);
        *unique = *total - duplicate;
    }

    free(pub_sha_info_arr);
    sk_X509_pop_free(certs, X509_free);

    return 1;
}

#if 0
int main(int argc, char *argv[]) 
{
    char *file_path, *save_path = "all.pem";
    int opt;

    while ((opt = getopt(argc, argv, "f:s:h")) != -1) {
        switch (opt) {
            case 'f':
                file_path = optarg;
                break;
            case 's':
                save_path = optarg;
                break;
            case 'h':
            default: 
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    struct stat st;
    if (stat(file_path, &st) != 0) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
    int total = 0, unique = 0;
    int ret = certs_dereplication(file_path, save_path, &total, &unique); 
    if (ret) {
        printf("Total certficates num # %d, duplicate # %d, unique # %d\n", total, total - unique, unique);
    } else {
        printf("dereplication certficate fail\n");
    }
}
#endif

#endif
