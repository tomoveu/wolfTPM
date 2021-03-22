/* read.c
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

/* Example for reading the voltage level of TPM's GPIO
 *
 * NB: GPIO must be first configured using gpio/config
 *
 */

#include <wolftpm/tpm2_wrap.h>

#include <examples/gpio/gpio.h>
#include <examples/tpm_io.h>
#include <examples/tpm_test.h>


#include <stdio.h>
#include <stdlib.h>

#ifndef WOLFTPM2_NO_WRAPPER

/******************************************************************************/
/* --- BEGIN TPM GPIO Read Example -- */
/******************************************************************************/
static void usage(void)
{
    printf("Expected usage:\n");
    printf("./examples/gpio/read [num]\n");
    printf("num is a GPIO (between 0-3)\n");
    printf("Default usage, without parameters, read GPIO%d\n", TPM_GPIO_A);
}

int TPM2_GPIO_Read_Example(void* userCtx, int argc, char *argv[])
{
    int rc, pin = 0;
    word32 readSize, nvAttributes;
    WOLFTPM2_DEV dev;
    WOLFTPM2_HANDLE parent;
    WOLFTPM2_NV nv;
    TPM_HANDLE nvIndex = TPM_NV_GPIO_SPACE;
    BYTE pinState;

    if (argc >= 2) {
        if (XSTRNCMP(argv[1], "-?", 2) == 0 ||
            XSTRNCMP(argv[1], "-h", 2) == 0 ||
            XSTRNCMP(argv[1], "--help", 6) == 0) {
            usage();
            return 0;
        }
        pin = atoi(argv[1]);
        if(pin < 0 || pin > 3) {
            usage();
            return 0;
        }
        nvIndex += pin;
    }

    rc = wolfTPM2_Init(&dev, TPM2_IoCb, userCtx);
    if (rc != TPM_RC_SUCCESS) {
        printf("\nwolfTPM2_Init failed\n");
        goto exit;
    }

    XMEMSET(&nv, 0, sizeof(nv));
    XMEMSET(&parent, 0, sizeof(parent));
    /* Prep NV attributes */
    parent.hndl = TPM_RH_OWNER;
    rc = wolfTPM2_GetNvAttributesTemplate(parent.hndl, &nvAttributes);
    if (rc != 0) {
        printf("Setting NV attributes failed\n");
        goto exit;
    }
    /* Define NV Index for GPIO */
    readSize = sizeof(pinState);
    rc = wolfTPM2_NVCreateAuth(&dev, &parent, &nv, nvIndex, nvAttributes,
                               readSize, (byte*)gNvAuth, sizeof(gNvAuth)-1);
    if (rc != 0 && rc != TPM_RC_NV_DEFINED) {
        printf("Creating NV Index for GPIO acccess failed\n");
        goto exit;
    }
    /* Read GPIO state */
    rc = wolfTPM2_NVReadAuth(&dev, &nv, TPM_NV_GPIO_SPACE, &pinState, &readSize, 0);
    if (rc != 0) {
            printf("Error while reading GPIO state\n");
            goto exit;
    }

    if (pinState == 0x01) {
        printf("GPIO%d is High.\n", pin);
    }
    else if (pinState == 0x00) {
        printf("GPIO%d is Low.\n", pin);
    }
    else {
        printf("GPIO%d level read, invalid value = 0x%X\n", pin, pinState);
    }

exit:

    if (rc != 0) {
        printf("\nFailure 0x%x: %s\n\n", rc, wolfTPM2_GetRCString(rc));
    }

    wolfTPM2_Cleanup(&dev);
    return rc;
}

/******************************************************************************/
/* --- END TPM GPIO Store Example -- */
/******************************************************************************/
#endif /* !WOLFTPM2_NO_WRAPPER */

#ifndef NO_MAIN_DRIVER
int main(int argc, char *argv[])
{
    int rc = NOT_COMPILED_IN;

#ifndef WOLFTPM2_NO_WRAPPER
    rc = TPM2_GPIO_Read_Example(NULL, argc, argv);
#else
    printf("GPIO code not compiled in\n");
    (void)argc;
    (void)argv;
#endif

    return rc;
}
#endif
