#include <stdio.h>
#include <osapi.h>
#include <user_interface.h>

void user_rf_pre_init(void)
{
    system_phy_set_rfoption(1);
    system_phy_set_max_tpw(82);
}

uint32_t user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();

    switch(size_map)
    {
        case FLASH_SIZE_4M_MAP_256_256:
            return 128 - 5;
        case FLASH_SIZE_8M_MAP_512_512:
            return 256 - 5;
        case FLASH_SIZE_16M_MAP_512_512:
            return 512 - 5;
        case FLASH_SIZE_16M_MAP_1024_1024:
            return 512 - 5;
        case FLASH_SIZE_32M_MAP_512_512:
            return 1024 - 5;
        case FLASH_SIZE_32M_MAP_1024_1024:
            return 1024 - 5;
    }

    return 0;
}
