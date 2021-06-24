/* keygen.c
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

/* Tool and example for creating, storing and loading keys using TPM2.0 */

#include <wolftpm/tpm2_wrap.h>

#include <examples/keygen/keygen.h>
#include <examples/tpm_io.h>
#include <examples/tpm_test.h>
#include <examples/tpm_test_keys.h>

#include <stdio.h>
#include <stdlib.h> /* atoi */
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#ifndef WOLFTPM2_NO_WRAPPER

#define SYM_EXTRA_OPTS_LEN 14 /* 5 chars for "-sym=" and 9 for extra options */
#define SYM_EXTRA_OPTS_POS 4  /* Array pos of the equal sign for extra opts */
#define SYM_EXTRA_OPTS_AES_MODE_POS 8
#define SYM_EXTRA_OPTS_KEY_BITS_POS 11



/******************************************************************************/
/* --- BEGIN TPM Keygen Example -- */
/******************************************************************************/
static void usage(void)
{
    printf("Expected usage:\n");
    printf("./examples/keygen/keygen [keyblob.bin] [-ecc/-rsa/-sym] [-t] [-aes/xor]\n");
    printf("* -rsa: Use RSA for asymmetric key generation (DEFAULT)\n");
    printf("* -ecc: Use ECC for asymmetric key generation \n");
    printf("* -sym: Use Symmetric Cypher for key generation\n");
    printf("\tDefault Symmetric Cypher is AES CTR with 256 bits\n");
    printf("* -t: Use default template (otherwise AIK)\n");
    printf("* -ssh: Use template for SSH key (only RSA algorithm)\n");
    printf("\tStores public key in id_rsa.pub\n");
    printf("\tStores private key in keyblob.bin\n");
    printf("* -aes/xor: Use Parameter Encryption\n");
    printf("Example usage:\n");
    printf("\t* RSA, default template\n");
    printf("\t\t keygen -t\n");
    printf("\t* ECC, Attestation Key template "\
           "with AES CFB parameter encryption\n");
    printf("\t\t keygen -ecc -aes\n");
    printf("\t* Symmetric key, AES, CTR mode, 128 bits\n");
    printf("\t\t keygen -sym=aesctr128\n");
    printf("\t* Symmetric key, AES, CFB mode, 256 bits\n");
    printf("\t\t keygen -sym=aescfb256\n");
    printf("\t* Symmetric key, AES, CBC mode, 128 bits, "\
           "with XOR parameter encryption\n");
    printf("\t\t keygen -sym=aescbc256 -xor\n");
}

static int symChoice(const char* arg, TPM_ALG_ID* algSym, int* keyBits,
                     char* symMode)
{
    size_t len = XSTRLEN(arg);

    if (len != SYM_EXTRA_OPTS_LEN) {
        return TPM_RC_FAILURE;
    }
    if (XSTRNCMP(&arg[SYM_EXTRA_OPTS_POS+1], "aes", 3)) {
        return TPM_RC_FAILURE;
    }

    /* Copy string for user information later */
    XMEMCPY(symMode, &arg[SYM_EXTRA_OPTS_POS+1], 6);

    if (XSTRNCMP(&arg[SYM_EXTRA_OPTS_AES_MODE_POS], "cfb", 3) == 0) {
        *algSym = TPM_ALG_CFB;
    }
    else if (XSTRNCMP(&arg[SYM_EXTRA_OPTS_AES_MODE_POS], "ctr", 3) == 0) {
        *algSym = TPM_ALG_CTR;
    }
    else if (XSTRNCMP(&arg[SYM_EXTRA_OPTS_AES_MODE_POS], "cbc", 3) == 0) {
        *algSym = TPM_ALG_CBC;
    }
    else {
        return TPM_RC_FAILURE;
    }

    *keyBits = atoi(&arg[SYM_EXTRA_OPTS_KEY_BITS_POS]);
    if(*keyBits != 128 && *keyBits != 192 && *keyBits != 256) {
        return TPM_RC_FAILURE;
    }

    return TPM_RC_SUCCESS;
}

static int writeKeyPubSsh(const char *filename, const byte *buf, word32 buf_size)
{
    int rc = TPM_RC_FAILURE;
#if !defined(WOLFTPM2_NO_WOLFCRYPT) && !defined(NO_FILESYSTEM)
    XFILE fp = NULL;
    size_t fileSz = 0;

    if (filename == NULL || buf == NULL)
        return BAD_FUNC_ARG;

    fp = XFOPEN(filename, "wt");
    if (fp != XBADFILE) {
        fileSz = XFWRITE(buf, 1, buf_size, fp);
        /* sanity check */
        if (fileSz == buf_size) {
            rc = TPM_RC_SUCCESS;
        }
#ifdef DEBUG_WOLFTPM
        printf("Public PEM file size = %zu\n", fileSz);
        TPM2_PrintBin(buf, buf_size);
#endif
        XFCLOSE(fp);
    }
#endif
    return rc;

}

int TPM2_Keygen_Example(void* userCtx, int argc, char *argv[])
{
    int rc;
    WOLFTPM2_DEV dev;
    WOLFTPM2_KEY storage; /* SRK */
    WOLFTPM2_KEY aesKey; /* Symmetric key */
    WOLFTPM2_KEYBLOB newKey;
    TPMT_PUBLIC publicTemplate;
    TPMI_ALG_PUBLIC alg = TPM_ALG_RSA; /* default, see usage() for options */
    TPM_ALG_ID algSym = TPM_ALG_CTR; /* default Symmetric Cypher, see usage */
    TPM_ALG_ID paramEncAlg = TPM_ALG_NULL;
    WOLFTPM2_SESSION tpmSession;
    TPM2B_AUTH auth;
    int bAIK = 1;
    int bSSH = 0;
    int keyBits = 256;
    const char* outputFile = "keyblob.bin";
    size_t len = 0;
    char symMode[] = "aesctr";

    if (argc >= 2) {
        if (XSTRNCMP(argv[1], "-?", 2) == 0 ||
            XSTRNCMP(argv[1], "-h", 2) == 0 ||
            XSTRNCMP(argv[1], "--help", 6) == 0) {
            usage();
            return 0;
        }
        if (argv[1][0] != '-')
            outputFile = argv[1];
    }
    while (argc > 1) {
        if (XSTRNCMP(argv[argc-1], "-rsa", 4) == 0) {
            alg = TPM_ALG_RSA;
        }
        if (XSTRNCMP(argv[argc-1], "-ecc", 4) == 0) {
            alg = TPM_ALG_ECC;
        }
        if (XSTRNCMP(argv[argc-1], "-sym", 4) == 0) {
            len = XSTRLEN(argv[argc-1]);
            if (len >= SYM_EXTRA_OPTS_LEN) {
                /* Did the user provide specific options? */
                if (argv[argc-1][SYM_EXTRA_OPTS_POS] == '=') {
                    rc = symChoice(argv[argc-1], &algSym, &keyBits, symMode);
                    /* In case of incorrect extra options, abort execution */
                    if (rc != TPM_RC_SUCCESS) {
                        usage();
                        return 0;
                    }
                }
                /* Otherwise, defaults are used: AES CTR, 256 key bits */
            }
            alg = TPM_ALG_SYMCIPHER;
            bAIK = 0;
        }
        if (XSTRNCMP(argv[argc-1], "-t", 2) == 0) {
            bAIK = 0;
        }
        if (XSTRNCMP(argv[argc-1], "-ssh", 4) == 0) {
            bSSH = 1;
            bAIK = 0;
        }
        if (XSTRNCMP(argv[argc-1], "-aes", 4) == 0) {
            paramEncAlg = TPM_ALG_CFB;
        }
        if (XSTRNCMP(argv[argc-1], "-xor", 4) == 0) {
            paramEncAlg = TPM_ALG_XOR;
        }
        argc--;
    }

    XMEMSET(&storage, 0, sizeof(storage));
    XMEMSET(&newKey, 0, sizeof(newKey));
    XMEMSET(&aesKey, 0, sizeof(aesKey));
    XMEMSET(&tpmSession, 0, sizeof(tpmSession));
    XMEMSET(&auth, 0, sizeof(auth));

    printf("TPM2.0 Key generation example\n");
    printf("\tKey Blob: %s\n", outputFile);
    printf("\tAlgorithm: %s\n", TPM2_GetAlgName(alg));
    if(alg == TPM_ALG_SYMCIPHER) {
        printf("\t\t %s mode, %d keybits\n", symMode, keyBits);
    }
    printf("\tTemplate: %s\n", bAIK ? "AIK" : "Default");
    printf("\tUse Parameter Encryption: %s\n", TPM2_GetAlgName(paramEncAlg));

    rc = wolfTPM2_Init(&dev, TPM2_IoCb, userCtx);
    if (rc != TPM_RC_SUCCESS) {
        printf("\nwolfTPM2_Init failed\n");
        goto exit;
    }

    /* get SRK */
    rc = getPrimaryStoragekey(&dev, &storage, TPM_ALG_RSA);
    if (rc != 0) goto exit;

    if (paramEncAlg != TPM_ALG_NULL) {
        /* Start an authenticated session (salted / unbound) with parameter encryption */
        rc = wolfTPM2_StartSession(&dev, &tpmSession, &storage, NULL,
            TPM_SE_HMAC, paramEncAlg);
        if (rc != 0) goto exit;
        printf("TPM2_StartAuthSession: sessionHandle 0x%x\n",
            (word32)tpmSession.handle.hndl);

        /* set session for authorization of the storage key */
        rc = wolfTPM2_SetAuthSession(&dev, 1, &tpmSession,
            (TPMA_SESSION_decrypt | TPMA_SESSION_encrypt | TPMA_SESSION_continueSession));
        if (rc != 0) goto exit;
    }

    /* Create new key */
    if (bAIK) {
        if (alg == TPM_ALG_RSA) {
            printf("RSA AIK template\n");
            rc = wolfTPM2_GetKeyTemplate_RSA_AIK(&publicTemplate);
        }
        else if (alg == TPM_ALG_ECC) {
            printf("ECC AIK template\n");
            rc = wolfTPM2_GetKeyTemplate_ECC_AIK(&publicTemplate);
        }
        else if (alg == TPM_ALG_SYMCIPHER) {
            printf("AIK are expected to be RSA or ECC, not symmetric keys.\n");
            rc = BAD_FUNC_ARG;
        }
        else {
            rc = BAD_FUNC_ARG;
        }

        /* set session for authorization key */
        auth.size = (int)sizeof(gAiKeyAuth)-1;
        XMEMCPY(auth.buffer, gAiKeyAuth, auth.size);

    }
    else if (bSSH) {
        if (alg == TPM_ALG_RSA) {
            printf("SSH template for RSA key\n");
            rc = wolfTPM2_GetKeyTemplate_RSA_SSH(&publicTemplate, TPM_ALG_SHA1);

            /* set session for authorization key */
            auth.size = (int)sizeof(gKeyAuth)-1;
            XMEMCPY(auth.buffer, gKeyAuth, auth.size);
        }
        else {
            printf("SSH template for ECC key not implemented\n");
            rc = BAD_FUNC_ARG;
        }
    }
    else {
        if (alg == TPM_ALG_RSA) {
            printf("RSA template\n");
            rc = wolfTPM2_GetKeyTemplate_RSA(&publicTemplate,
                     TPMA_OBJECT_sensitiveDataOrigin | TPMA_OBJECT_userWithAuth |
                     TPMA_OBJECT_decrypt | TPMA_OBJECT_sign | TPMA_OBJECT_noDA);
        }
        else if (alg == TPM_ALG_ECC) {
            printf("ECC template\n");
            rc = wolfTPM2_GetKeyTemplate_ECC(&publicTemplate,
                     TPMA_OBJECT_sensitiveDataOrigin | TPMA_OBJECT_userWithAuth |
                     TPMA_OBJECT_sign | TPMA_OBJECT_noDA,
                     TPM_ECC_NIST_P256, TPM_ALG_ECDSA);
        }
        else if (alg == TPM_ALG_SYMCIPHER) {
            printf("Symmetric template\n");
            rc = wolfTPM2_GetKeyTemplate_Symmetric(&publicTemplate, keyBits,
                    algSym, YES, YES);
        }
        else {
            rc = BAD_FUNC_ARG;
        }

        /* set session for authorization key */
        auth.size = (int)sizeof(gKeyAuth)-1;
        XMEMCPY(auth.buffer, gKeyAuth, auth.size);
    }
    if (rc != 0) goto exit;

    printf("Creating new %s key...\n", TPM2_GetAlgName(alg));
    rc = wolfTPM2_CreateKey(&dev, &newKey, &storage.handle,
                            &publicTemplate, auth.buffer, auth.size);
    if (rc != TPM_RC_SUCCESS) {
        printf("wolfTPM2_CreateKey failed\n");
        goto exit;
    }
    printf("Created new key (pub %d, priv %d bytes)\n",
        newKey.pub.size, newKey.priv.size);

    /* Save key as encrypted blob to the disk */
#if !defined(WOLFTPM2_NO_WOLFCRYPT) && !defined(NO_FILESYSTEM)
    rc = writeKeyBlob(outputFile, &newKey);
    if (rc != 0) {
        printf("Failure to store key blob\n");
        goto exit;
    }

    if (bSSH) {
        WOLFTPM2_KEY tpmKey;
        RsaKey rsaKey;
        byte der[MAX_RSA_KEY_BYTES], pem[MAX_RSA_KEY_BYTES];
        int  derSz, pemSz;

        /* Prepare wolfCrypt key structure */
        rc = wc_InitRsaKey(&rsaKey, NULL);
        if (rc != 0) goto exit;
        /* Prepare wolfTPM key structure */
        XMEMCPY(&tpmKey.handle, &newKey.handle, sizeof(tpmKey.handle));
        XMEMCPY(&tpmKey.pub, &newKey.pub, sizeof(tpmKey.pub));
        /* Convert the wolfTPM key to wolfCrypt format */
        rc = wolfTPM2_RsaKey_TpmToWolf(&dev, &tpmKey, &rsaKey);
        if (rc != 0) goto exit;
        /* Convert the wolfCrypt key to DER format */
        rc = wc_RsaKeyToPublicDer(&rsaKey, der, sizeof(der));
        if (rc <= 0) goto exit;
        derSz = rc;
        /* Convert the DER key to PEM format */
        rc = wc_DerToPem(der, derSz, pem, sizeof(pem), PUBLICKEY_TYPE);
        if (rc <= 0) goto exit;
        pemSz = rc;
        /* Store PEM output to file */
        rc = writeKeyPubSsh("id_rsa.pub", pem, (word32)pemSz);
    }
#else
    if(alg == TPM_ALG_SYMCIPHER) {
        printf("The Public Part of a symmetric key contains only meta data\n");
    }
    printf("Key Public Blob %d\n", newKey.pub.size);
    TPM2_PrintBin((const byte*)&newKey.pub.publicArea, newKey.pub.size);
    printf("Key Private Blob %d\n", newKey.priv.size);
    TPM2_PrintBin(newKey.priv.buffer, newKey.priv.size);
#endif

exit:

    if (rc != 0) {
        printf("\nFailure 0x%x: %s\n\n", rc, wolfTPM2_GetRCString(rc));
    }

    /* Close handles */
    wolfTPM2_UnloadHandle(&dev, &storage.handle);
    wolfTPM2_UnloadHandle(&dev, &newKey.handle);
    wolfTPM2_UnloadHandle(&dev, &tpmSession.handle);

    wolfTPM2_Cleanup(&dev);
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
    rc = TPM2_Keygen_Example(NULL, argc, argv);
#else
    printf("KeyGen code not compiled in\n");
    (void)argc;
    (void)argv;
#endif

    return rc;
}
#endif
