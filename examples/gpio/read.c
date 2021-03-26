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
    printf("num is a GPIO number (between 1-4)\n");
    printf("Default usage, without parameters, read GPIO%d\n", TPM_GPIO_A);
}

int TPM2_GPIO_Read_Example(void* userCtx, int argc, char *argv[])
{
    int pin, rc;
    WOLFTPM2_DEV dev;
    TPM_HANDLE auth = 0;
    BYTE pinState;
    word32 readSize;

    if (argc >= 2) {
        if (XSTRNCMP(argv[1], "-?", 2) == 0 ||
            XSTRNCMP(argv[1], "-h", 2) == 0 ||
            XSTRNCMP(argv[1], "--help", 6) == 0) {
            usage();
            return 0;
        }
        pin = atoi(argv[1]);
        if(pin < 1 || pin > 4) {
            usage();
            return 0;
        }
        /* Use pin as offset and consider that pin{1,4}->GPIO{0,3} */
        pin = TPM_GPIO_A+pin-1;
    }

    rc = wolfTPM2_Init(&dev, TPM2_IoCb, userCtx);
    if (rc != TPM_RC_SUCCESS) {
        printf("\nwolfTPM2_Init failed\n");
        goto exit;
    }

    readSize = (word32)sizeof(pinState);
    rc = wolfTPM2_NVRead(&dev, auth, TPM_NV_GPIO_SPACE, &pinState, &readSize, 0);
    if (rc != 0) {
        printf("Reading the state of GPIO failed\n");
        goto exit;
    }
    printf("Voltage level of GPIO\n\n");

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
