/* unconfig.c
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

/* This is a helper tool for destroying an NV Index used for GPIO */

#include <wolftpm/tpm2_wrap.h>

#ifndef WOLFTPM2_NO_WRAPPER

#include <examples/gpio/gpio.h>
#include <examples/tpm_io.h>

#include <stdio.h>
#include <stdlib.h> /* atoi */


/******************************************************************************/
/* --- BEGIN TPM2.0 GPIO Unconfig example  -- */
/******************************************************************************/

static void usage(void)
{
    printf("Expected usage:\n");
    printf("./examples/gpio/unconfig [num]\n");
    printf("* num is a GPIO number between 0-3 (default %d)\n", TPM_GPIO_A);
    printf("\tThis example undefines NV Index used for GPIO access\n");
    printf("Demo usage, without parameters, undefines NV Index 0x%8.8X.\n", TPM_NV_GPIO_SPACE);
}

int TPM2_GPIO_Unconfig_Example(void* userCtx, int argc, char *argv[])
{
    int rc = -1;
    int gpioNum, nvIndex = TPM_NV_GPIO_SPACE;
    WOLFTPM2_DEV dev;

    if (argc == 2) {
        gpioNum = atoi(argv[1]);
        if (gpioNum < 0 || gpioNum > 3) {
            printf("GPIO is out of range (0-3)\n");
            usage();
            goto exit;
        }
        /* Increment NV index used for GPIO access */
        nvIndex += gpioNum;
    }
    else if (argc == 1) {
        printf("Using default NV Index 0x%8.8X for GPIO.\n", TPM_NV_GPIO_SPACE);
    }
    else {
        printf("Incorrect arguments\n");
        usage();
        goto exit;
    }

    rc = wolfTPM2_Init(&dev, TPM2_IoCb, userCtx);
    if (rc != TPM_RC_SUCCESS) {
        printf("wolfTPM2_Init failed 0x%x: %s\n", rc, TPM2_GetRCString(rc));
        goto exit;
    }
    printf("wolfTPM2_Init: success\n");

    printf("Trying to remove NV index 0x%8.8X used for GPIO\n", nvIndex);
    rc = wolfTPM2_NVDelete(&dev, TPM_RH_OWNER, nvIndex);
    if (rc != TPM_RC_SUCCESS) {
        printf("wolfTPM2_NVDelete failed 0x%x: %s\n", rc, TPM2_GetRCString(rc));
        goto exit;
    }
    printf("NV indexundefined\n");

exit:

    wolfTPM2_Cleanup(&dev);
    return rc;
}

/******************************************************************************/
/* --- END TPM2.0 GPIO Unconfig example -- */
/******************************************************************************/
#endif /* !WOLFTPM2_NO_WRAPPER */

#ifndef NO_MAIN_DRIVER
int main(int argc, char *argv[])
{
    int rc = -1;

#ifndef WOLFTPM2_NO_WRAPPER
    rc = TPM2_GPIO_Unconfig_Example(NULL, argc, argv);
#else
    printf("Wrapper code not compiled in\n");
    (void)argc;
    (void)argv;
#endif /* !WOLFTPM2_NO_WRAPPER */

    return rc;
}
#endif
