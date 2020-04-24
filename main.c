#include <osapi.h>

void app_init(void)
{
    // TODO...
}

void user_init(void)
{
    system_init_done_cb(app_init);
}
