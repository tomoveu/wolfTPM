/* size/keygen.c
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

/* Example app used for library size comparision,
 * performing the most common TPM operations:
 * - Create a primary key in the Owner Hierarchy
 * - Create a new signing key under the primary key
 * - Load the new signing key
 * All done using wolfTPM wrappers.
 */

#include <wolftpm/tpm2_wrap.h>

#include <examples/size/keygen.h>
#include <examples/tpm_io.h>
#include <examples/tpm_test.h>

#include <stdio.h>

#ifndef WOLFTPM2_NO_WRAPPER

/******************************************************************************/
/* --- BEGIN Example -- */
/******************************************************************************/
static void usage(void)
{
    printf("Expected usage:\n");
    printf("./examples/size/keygen -ecc/-rsa [-aes]\n");
    printf("* -ecc: Use ECC for keys\n");
    printf("* -rsa: Use RSA for keys\n");
    printf("* -aes: Use Parameter Encryption (AES CFB)\n");
}

int wolfTPM2_Size_Keygen_Example(void* userCtx, int argc, char *argv[])
{
    int rc;
    WOLFTPM2_DEV dev;
    WOLFTPM2_KEY storageKey;
    WOLFTPM2_KEY signingKey;
    WOLFTPM2_SESSION tpmSession;
    TPMT_PUBLIC publicTemplate;
    TPMI_ALG_PUBLIC alg = TPM_ALG_NULL;
    TPM_ALG_ID paramEncAlg = TPM_ALG_NULL;
    TPM2B_AUTH auth;

    if (argc >= 2) {
        if (XSTRNCMP(argv[1], "-?", 2) == 0 ||
            XSTRNCMP(argv[1], "-h", 2) == 0 ||
            XSTRNCMP(argv[1], "--help", 6) == 0) {
            usage();
            return 0;
        }
    }

    while (argc > 1) {
        if (XSTRNCMP(argv[argc-1], "-ecc", 4) == 0) {
            alg = TPM_ALG_ECC;
        }
        else if (XSTRNCMP(argv[argc-1], "-rsa", 4) == 0) {
            alg = TPM_ALG_RSA;
        }
        else if (XSTRNCMP(argv[argc-1], "-aes", 4) == 0) {
            paramEncAlg = TPM_ALG_CFB;
        }
        else {
            printf("Wrong argument %d\n", argc-1);
            usage();
            return 0;
        }
        argc--;
    }

    if (alg == TPM_ALG_NULL) {
        usage();
        return 0;
    }

    XMEMSET(&storageKey, 0, sizeof(storageKey));
    XMEMSET(&signingKey, 0, sizeof(signingKey));
    XMEMSET(&tpmSession, 0, sizeof(tpmSession));
    XMEMSET(&auth, 0, sizeof(auth));

    printf("TPM2.0 Key generation example for library size comparision\n");
    printf("\tAlgorithm: %s\n", TPM2_GetAlgName(alg));
    printf("\tUse Parameter Encryption: %s\n", TPM2_GetAlgName(paramEncAlg));

    /* Init the TPM2 device */
    rc = wolfTPM2_Init(&dev, TPM2_IoCb, userCtx);
    if (rc != 0) {
        return rc;
    }
    printf("wolfTPM2_Init: Success\n");

    /* Create primary storage key */
    rc = wolfTPM2_CreateSRK(&dev, &storageKey, TPM_ALG_RSA, (byte*)gStorageKeyAuth, sizeof(gStorageKeyAuth)-1);
    if (rc != 0) {
        goto exit;
    }
    printf("wolfTPM2_CreateSRK: Success\n");

    if (paramEncAlg != TPM_ALG_NULL) {
        /* Start a TPM session for Parameter Encryption */
        rc = wolfTPM2_StartSession(&dev, &tpmSession, &storageKey, NULL,
            TPM_SE_HMAC, paramEncAlg);
        if (rc != 0) {
            goto exit;
        }
        printf("wolfTPM2_StartSession: sessionHandle 0x%x\n",
            (word32)tpmSession.handle.hndl);

        /* Set session properties for Parameter Encryption */
        rc = wolfTPM2_SetAuthSession(&dev, 1, &tpmSession,
            (TPMA_SESSION_decrypt | TPMA_SESSION_encrypt | TPMA_SESSION_continueSession));
        if (rc != 0) {
             goto exit;
        }
    }

    /* Prepare publicTemplate for new signing key */
    rc = wolfTPM2_GetKeyTemplate_RSA(&publicTemplate,
        TPMA_OBJECT_sensitiveDataOrigin | TPMA_OBJECT_userWithAuth |
        TPMA_OBJECT_decrypt | TPMA_OBJECT_sign | TPMA_OBJECT_noDA);
    if (rc != 0) {
        goto exit;
    }

    /* Create and load the new signing key */
    rc = wolfTPM2_CreateAndLoadKey(&dev, &signingKey, &storageKey.handle,
        &publicTemplate, (byte*)gKeyAuth, sizeof(gKeyAuth)-1);
    if (rc != 0) {
        goto exit;
    }
    printf("wolfTPM2_CreateAndLoadKey: Success\n");

exit:

    if (rc != 0) {
        printf("Failure %d (0x%x): %s\n", rc, rc, wolfTPM2_GetRCString(rc));
    }

    wolfTPM2_UnloadHandle(&dev, &signingKey.handle);
    wolfTPM2_UnloadHandle(&dev, &storageKey.handle);
    wolfTPM2_UnloadHandle(&dev, &tpmSession.handle);
    printf("wolfTPM2_UnloadHandle: Success\n");

    wolfTPM2_Cleanup(&dev);
    printf("wolfTPM2_Cleanup: Success\n");

    return rc;
}

/******************************************************************************/
/* --- END TPM Keygen Example -- */
/******************************************************************************/
#endif /* !WOLFTPM2_NO_WRAPPER */

#ifndef NO_MAIN_DRIVER
int main(int argc, char *argv[])
{
    int rc = NOT_COMPILED_IN;

#ifndef WOLFTPM2_NO_WRAPPER
    rc = wolfTPM2_Size_Keygen_Example(NULL, argc, argv);
#else
    printf("KeyGen code not compiled in\n");
    (void)argc;
    (void)argv;
#endif

    return rc;
}
#endif
