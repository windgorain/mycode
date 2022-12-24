/*================================================================
*   Created by LiXingang
*   Description:  对lib config的一些封装
*
================================================================*/
#ifndef _LIBCONFIG_UTL_H
#define _LIBCONFIG_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

static inline int LibConfig_SetInt(void *cfg, char *name, int val, int dft)
{
    if (val == dft) {
        return 0;
    }

    config_setting_t *setting = config_lookup(cfg, name);
    if (setting == NULL) {
        fprintf(stderr, "No '%s' setting in configuration file.\n", name);
        return -1;
    }

    if (!config_setting_set_int(setting, val)) {
        fprintf(stderr, "Failed to set %s.\n", name);
        return -1;
    }

    return 0;
}

#define LIBCONFIG_SET_INT(_cfg, _name, _val, _dft) do { \
    if (LibConfig_SetInt((_cfg), (_name), (_val), (_dft)) < 0) { \
        return -1; \
    } \
}while (0)

static inline int LibConfig_SetString(void *cfg, char *name, char *val)
{
    if (! val) {
        return 0;
    }

    config_setting_t *setting = config_lookup(cfg, name);
    if (setting == NULL) {
        fprintf(stderr, "No '%s' setting in configuration file.\n", name);
        return -1;
    }

    if (!config_setting_set_string(setting, val)) {
        fprintf(stderr, "Failed to set encrypt_key.\n");
        return -1;
    }

    return 0;
}

#define LIBCONFIG_SET_STRING(_cfg, _name, _val) do {\
    if (LibConfig_SetString((_cfg), (_name), (_val)) < 0) { \
        return -1; \
    } \
}while(0)

static inline int LibConfig_SetStrNumArray(void *cfg, char *name, char *eles) 
{
    int setting_num = 0;
    int itmp = 0;
    char *str1;
    config_setting_t *setting; 

    if (! eles) {
        return 0;
    }

    setting = config_lookup(cfg, name);
    if (! setting) {
        fprintf(stderr, "No '%s' setting in configuration file.\n", name); 
        return -1; 
    } 

    setting_num = config_setting_length(setting);
    str1 = strtok(eles, ";");

    while (str1 != NULL) {
        if (itmp >= setting_num) {
            if (!config_setting_add(setting, NULL, CONFIG_TYPE_INT)) {
                fprintf(stderr, "Failed to set local_port.\n");
                return -1;
            }
        }
        if (!config_setting_set_int_elem(setting, itmp, atoi(str1))) {
            fprintf(stderr, "Failed to set local_port.\n");
            return -1;
        }

        itmp++;
        str1 = strtok(NULL, ";");
    }

    while (setting_num > itmp) {
        if (!config_setting_remove_elem(setting, setting_num - 1)) {
            fprintf(stderr, "Failed to set local_port.\n");
            return -1;
        }

        setting_num--;
    }

    return 0;
}

#define LIBCONFIG_SET_STRNUM_ARRAY(_cfg, _name, _eles) do { \
    int _ret = LibConfig_SetStrNumArray((_cfg), (_name), (_eles)); \
    if (_ret < 0) { \
        return _ret; \
    } \
}while (0)

#ifdef __cplusplus
}
#endif
#endif //LIBCONFIG_UTL_H_
