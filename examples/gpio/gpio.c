/* gpio.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
 *
 * This file is part of wolfTPM.
 *
 * wolfTPM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfTPM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* This examples demonstrates the use of GPIO available on some TPM modules */

#include <wolftpm/tpm2_wrap.h>

#ifndef WOLFTPM2_NO_WRAPPER

#include <examples/gpio/gpio.h>
#include <examples/tpm_io.h>

#include <stdio.h>
#include <stdlib.h> /* atoi */

/******************************************************************************/
/* --- BEGIN TPM2.0 GPIO example tool  -- */
/******************************************************************************/

static void usage(void)
{
    printf("Expected usage:\n");
    printf("./examples/gpio/gpio [num]\n");
    printf("* num is a GPIO number between 1-4 (default %d)\n", TPM_GPIO_A+1);
    printf("Demo usage without parameters, %d.\n", TPM_GPIO_A+1);
}

int TPM2_GPIO_Test(void* userCtx, int argc, char *argv[])
{
    int gpioNum, rc = -1;
    WOLFTPM2_DEV dev;
    GpioConfig_In gpio;

    if (argc == 2) {
        gpioNum = atoi(argv[1]);
        if (gpioNum < 1 || gpioNum > 3 || *argv[1] < '1' || *argv[1] > '3') {
            printf("GPIO is out of range (1-4)\n");
            usage();
            goto exit_badargs;
        }
    }
    else if (argc == 1) {
        gpioNum = TPM_GPIO_A;
    }
    else {
        printf("Incorrect arguments\n");
        usage();
        goto exit_badargs;
    }

    printf("Demo how to use extra GPIO on a TPM 2.0 modules\n");
    rc = wolfTPM2_Init(&dev, TPM2_IoCb, userCtx);
    if (rc != TPM_RC_SUCCESS) {
        printf("wolfTPM2_Init failed 0x%x: %s\n", rc, TPM2_GetRCString(rc));
        goto exit;
    }
    printf("wolfTPM2_Init: success\n");

    /* Trying new gpio command */
    XMEMSET(&gpio, 0, sizeof(gpio));
    gpio.authHandle = TPM_RH_PLATFORM;
    gpio.config.gpio[0].name = gpioNum;
    gpio.config.gpio[0].mode = TPM_GPIO_PUSHPULL;
    gpio.config.gpio[0].index = TPM_NV_GPIO_SPACE;
    printf("Trying to configure GPIO%d...\n", gpio.config.gpio[0].name);
    rc = TPM2_GPIO_Config(&gpio);
    if (rc != TPM_RC_SUCCESS) {
        printf("TPM2_GPIO_Config failed 0x%x: %s\n", rc,
            TPM2_GetRCString(rc));
        goto exit;
    }
    printf("TPM2_GPIO_Config success\n");

    /* TODO - Once we confirm GPIO config is successful on the real HW:
     *
     * * Add user option to choose GPIO mode (input, output, open drain)
     * * Use NV_Write to set GPIO level (high/low)
     * * Use NV_Read to read GPIO level
     * * Distinct GPIO_LP(GPIO_B) that can be used only as an input
     */

exit:

    wolfTPM2_Cleanup(&dev);

exit_badargs:

    return rc;
}

/******************************************************************************/
/* --- END TPM2.0 GPIO example tool -- */
/******************************************************************************/
#endif /* !WOLFTPM2_NO_WRAPPER */

#ifndef NO_MAIN_DRIVER
int main(int argc, char *argv[])
{
    int rc = -1;

#ifndef WOLFTPM2_NO_WRAPPER
    rc = TPM2_GPIO_Test(NULL, argc, argv);
#else
    printf("Wrapper code not compiled in\n");
    (void)argc;
    (void)argv;
#endif /* !WOLFTPM2_NO_WRAPPER */

    return rc;
}
#endif
