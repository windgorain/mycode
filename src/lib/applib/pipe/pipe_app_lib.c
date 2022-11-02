/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/socket_utl.h"
#include "utl/npipe_utl.h"
#include "utl/stream_server.h"
#include "comp/comp_poller.h"
#include "app/pipe_app_lib.h"

static void _pipe_app_clean(PIPE_APP_S *cfg)
{
    StreamServer_Stop(&cfg->server);
    cfg->stopping = 0;
}

static void _pipe_app_ob(void *ob)
{
    PIPE_APP_S *cfg = container_of(ob, PIPE_APP_S, trigger_ob);

    if (! cfg->stopping) {
        return;
    }

    _pipe_app_clean(cfg);
    PollerComp_UnRegOb(ob);
}

static int _pipe_app_init_mypoll(PIPE_APP_S *cfg)
{
    if (! cfg->poller_ins) {
        cfg->poller_ins = PollerComp_Get(NULL);
        if (NULL == cfg->poller_ins) {
            RETURN(BS_NOT_FOUND);
        }
        cfg->trigger_ob.func = _pipe_app_ob;
        PollerComp_RegOb(cfg->poller_ins, &cfg->trigger_ob);
    }

    if (! cfg->server.mypoll) {
        cfg->server.mypoll = PollerComp_GetMyPoll(cfg->poller_ins);
        if (! cfg->server.mypoll) {
            RETURN(BS_ERR);
        }
    }

    return 0;
}

void PipeApp_Init(PIPE_APP_S *cfg, char *dft_name)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->name_type = PIPE_APP_NAME_TYPE_NONE;
    strlcpy(cfg->cfg_name, dft_name, sizeof(cfg->cfg_name));
    cfg->server.listen_fd = -1;
    cfg->server.pipe_name = cfg->cfg_name;
    cfg->server.type = STREAM_SERVER_TYPE_PIPE;
}

int PipeApp_Start(PIPE_APP_S *cfg)
{
    int ret;

    if (cfg->server.enable) {
        return 0;
    }

    if (_pipe_app_init_mypoll(cfg) < 0) {
        RETURN(BS_ERR);
    }

    ret = StreamServer_Start(&cfg->server);
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void PipeApp_Stop(PIPE_APP_S *cfg)
{
    cfg->stopping = 1;
    PollerComp_Trigger(cfg->poller_ins);
}

void PipeApp_Clean(PIPE_APP_S *cfg)
{
    _pipe_app_clean(cfg);
}

