/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/object_utl.h"
#include "utl/mutex_utl.h"
#include "utl/exec_utl.h"
#include "utl/passwd_utl.h"
#include "utl/json_utl.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_localuser.h"

#include "../h/localuser_core.h"

BS_STATUS LocalUser_Init()
{
    LocalUserCore_Init();

    return 0;
}
