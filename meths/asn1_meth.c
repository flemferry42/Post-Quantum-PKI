//
// Created by ruben on 5/23/2019.
//

#include "asn1_meth.h"
#include "../keypair.h"
#include "../ossl/objects.h"
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/x509.h>
#include <string.h>

#ifndef OPENSSL_V102_COMPAT
#define RC_CONST const
#else
#define RC_CONST
#endif

typedef enum {
    PUBLIC,
    PRIVATE
} key_op_t;

static void pki_free(EVP_PKEY *pkey){
    ROUND5_KEYPAIR *kpair = EVP_PKEY_get0(pkey);
    _round5_keypair_free(kpair);
}

static int pki_key_print( BIO *bp, const EVP_PKEY *pkey,
                            int indent, ASN1_PCTX *ctx, key_op_t op)
{
    if (!pkey)
        return 0;

    const ROUND5_KEYPAIR *kpair = EVP_PKEY_get0(pkey);
    const struct round5_nid_data_st *nid_data = NULL;

    if (op == PRIVATE) {
//        if (suola_keypair_is_invalid_private(kpair)) {
//            if (BIO_printf(bp, "%*s<INVALID PRIVATE KEY>\n", indent, "") <= 0)
//                return 0;
//            return 1;
//        }
        nid_data = round5_get_nid_data(kpair->nid);
        if (BIO_printf(bp, "%*s%s Private-Key:\n", indent, "", nid_data->name) <= 0)
            return 0;
        if (BIO_printf(bp, "%*spriv:\n", indent, "") <= 0)
            return 0;
        if (ASN1_buf_print(bp, kpair->key.sk, nid_data->sk_bytes, indent + 4) == 0)
            return 0;
    } else {
//        if (suola_keypair_is_invalid(kpair)) {
//            if (BIO_printf(bp, "%*s<INVALID PUBLIC KEY>\n", indent, "") <= 0)
//                return 0;
//            return 1;
//        }
        nid_data = round5_get_nid_data(kpair->nid);
        if (BIO_printf(bp, "%*s%s Public-Key:\n", indent, "", nid_data->name) <= 0)
            return 0;
    }
    if (BIO_printf(bp, "%*spub:\n", indent, "") <= 0)
        return 0;
    if (ASN1_buf_print(bp, kpair->key.pk, nid_data->pk_bytes,
                       indent + 4) == 0)
        return 0;
    return 1;
}

static int pki_gen_priv_print(BIO *bp, const EVP_PKEY *pkey, int indent, ASN1_PCTX *ctx)
{
    return pki_key_print(bp, pkey, indent, ctx, PRIVATE);
}

static int pki_gen_pub_print(BIO *bp, const EVP_PKEY *pkey, int indent, ASN1_PCTX *ctx)
{
    return pki_key_print(bp, pkey, indent, ctx, PUBLIC);
}

//static int round5_gen_ctrl(int nid, EVP_PKEY *pkey, int op, long arg1, void *arg2){
//    ROUND5_KEYPAIR *kpair = NULL;
//    cont unsigned char *p = NULL;
//    const struct round5_nid_data_st *nid_data = round5_get_nid_data(nid);
//    int
//}

static int pki_gen_ctrl(int nid, EVP_PKEY *pkey, int op, long arg1, void *arg2)
{
    ROUND5_KEYPAIR *kp = NULL;
    const unsigned char *p = NULL;
    const struct round_nid_data_st *nid_data = round5_get_nid_data(nid);
    int pklen = 0;

    switch (op) {

#ifndef OPENSSL_V102_COMPAT
        // FIXME: check if/how this control signals should be handled in 1.0.2
        case ASN1_PKEY_CTRL_SET1_TLS_ENCPT:
            debug("nid: %d, op: ASN1_PKEY_CTRL_SET1_TLS_ENCPT, pklen: %ld\n", nid, arg1);
            p = arg2;
            pklen = arg1;

//            if (p == NULL || pklen != nid_data->pubk_bytes ) {
//                SUOLAerr(SUOLA_F_ASN1_GENERIC_CTRL, SUOLA_R_WRONG_LENGTH);
//                return 0;
//            }

            kp = _round5_keypair_new(nid, NO_PRIV_KEY);
//            if (suola_keypair_is_invalid(kp)) {
//                return 0;
//            }

            memcpy(kp->key.pk, p, pklen);

            EVP_PKEY_assign(pkey, nid, kp);
            return 1;


        case ASN1_PKEY_CTRL_GET1_TLS_ENCPT:
            debug("nid: %d, op: ASN1_PKEY_CTRL_GET1_TLS_ENCPT\n", nid);
            kp = EVP_PKEY_get0(pkey);
            if (kp == NULL && nid == kp->nid) {
                unsigned char **ppt = arg2;
                *ppt = OPENSSL_memdup(kp->key.sk, nid_data->pk_bytes);    // TODO: figure out pubk_bytes
                if (*ppt != NULL)
                    return nid_data->pk_bytes;
            }
            return 0;
#endif
        case ASN1_PKEY_CTRL_DEFAULT_MD_NID:
            debug("nid: %d, op: ASN1_PKEY_CTRL_DEFAULT_MD_NID, ret: %s\n",
                  nid, OBJ_nid2sn(nid_data->default_md_nid) );
            *(int *)arg2 = nid_data->default_md_nid;
            return 2;

        default:
            return -2;

    }
}


static int pki_gen_priv_encode(int nid, PKCS8_PRIV_KEY_INFO *p8, const EVP_PKEY *pkey)
{
    const ROUND5_KEYPAIR *kp = EVP_PKEY_get0(pkey);
    ASN1_OCTET_STRING oct;
    unsigned char *penc = NULL;
    int penclen;
    const struct round5_nid_data_st *nid_data = round5_get_nid_data(nid);
    char *tmp_buf = NULL;
    int ret = 0;

    if (nid_data == NULL) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PRIV_ENCODE, SUOLA_R_MISSING_NID_DATA);
        return 0;
    }

//    if (suola_keypair_is_invalid(kp) || kp->nid != nid) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PRIV_ENCODE, SUOLA_R_INVALID_PRIVATE_KEY);
//        return 0;
//    }

    tmp_buf = OPENSSL_secure_malloc(nid_data->sk_bytes);     // TODO: figure out privk_bytes
    if (NULL == tmp_buf) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PRIV_ENCODE, ERR_R_MALLOC_FAILURE);
        return 0;
    }

    oct.data = memcpy(tmp_buf, kp->key.sk, nid_data->sk_bytes);      //TODO: privk_bytes
    oct.length = nid_data->sk_bytes;     //TODO: privk_bytes
    oct.flags = 0;

    penclen = i2d_ASN1_OCTET_STRING(&oct, &penc);
    if (penclen < 0) {
//        SUOLAerr(ROUDN5_F_ASN1_GENERIC_PRIV_ENCODE, ERR_R_MALLOC_FAILURE);
        ret = 0;
        goto err;
    }

    if (!PKCS8_pkey_set0(p8, OBJ_nid2obj(nid), 0,
                         V_ASN1_UNDEF, NULL, penc, penclen)) {
        OPENSSL_clear_free(penc, penclen);
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PRIV_ENCODE, ERR_R_MALLOC_FAILURE);
        ret = 0;
        goto err;
    }

    ret = 1;
    err:
    if (tmp_buf)
        OPENSSL_secure_free(tmp_buf);
    return ret;
}

static int pki_curve25519_bits(const EVP_PKEY *pkey)
{
    return CURVE25519_BITS;
}

static int pki_curve25519_security_bits(const EVP_PKEY *pkey)
{
    return CURVE25519_SECURITY_BITS;
}

static int pki_gen_priv_decode(int nid, EVP_PKEY *pkey, RC_CONST PKCS8_PRIV_KEY_INFO *p8)
{
    const unsigned char *p;
    int plen;
    ASN1_OCTET_STRING *oct = NULL;
    RC_CONST X509_ALGOR *palg;
    ROUND5_KEYPAIR *kp = NULL;

    const struct round5_nid_data_st *nid_data = round5_get_nid_data(nid);
    if (nid_data == NULL) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PRIV_DECODE, SUOLA_R_MISSING_NID_DATA);
        return 0;
    }

    if (!PKCS8_pkey_get0(NULL, &p, &plen, &palg, p8))
        return 0;

    oct = d2i_ASN1_OCTET_STRING(NULL, &p, plen);
    if (oct == NULL) {
        p = NULL;
        plen = 0;
    } else {
        p = ASN1_STRING_get0_data(oct);
        plen = ASN1_STRING_length(oct);
    }

    if (palg != NULL) {
        int ptype;

        /* Algorithm parameters must be absent */
        X509_ALGOR_get0(NULL, &ptype, NULL, palg);
        if (ptype != V_ASN1_UNDEF) {
//            SUOLAerr(SUOLA_F_ASN1_GENERIC_PRIV_DECODE, SUOLA_R_INVALID_ENCODING);
            return 0;
        }
    }

    if (p == NULL || plen != nid_data->privk_bytes) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PRIV_DECODE, SUOLA_R_WRONG_LENGTH);
        return 0;
    }

    kp = _round5_keypair_new(nid, NO_FLAG);
//    if (suola_keypair_is_invalid_private(kp)){
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PRIV_DECODE, SUOLA_R_INVALID_PRIVATE_KEY);
        return 0;
    }

    memcpy(kp->key.sk, p, nid_data->privk_bytes);       //TODO: privk_bytes

    ASN1_OCTET_STRING_free(oct);
    oct = NULL;
    p = NULL;
    plen = 0;

    // Generate corresponding public key
    if ( 1 != (nid_data->sk_to_pk)(kp->key.pk, kp->key.sk) ) {
        _round5_keypair_free(kp);
        return 0;
    }

    EVP_PKEY_assign(pkey, nid, kp);

    return 1;
}

static int pki_gen_pub_encode(int nid, X509_PUBKEY *pk, const EVP_PKEY *pkey)
{
    const ROUND5_KEYPAIR *kp = EVP_PKEY_get0(pkey);
    unsigned char *penc;
    const struct round5_nid_data_st *nid_data = round5_get_nid_data(nid);

    if (kp->nid != nid) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PUB_ENCODE, SUOLA_R_INVALID_KEY);
        return 0;
    }

    if (nid_data == NULL) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PUB_ENCODE, SUOLA_R_MISSING_NID_DATA);
        return 0;
    }

    penc = OPENSSL_memdup(kp->key.pk, nid_data->pk_bytes);
    if (penc == NULL) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PUB_ENCODE, ERR_R_MALLOC_FAILURE);
        return 0;
    }

    if (!X509_PUBKEY_set0_param(pk, OBJ_nid2obj(nid), V_ASN1_UNDEF,
                                NULL, penc, nid_data->pk_bytes)) {
        OPENSSL_free(penc);
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PUB_ENCODE, ERR_R_MALLOC_FAILURE);
        return 0;
    }
    return 1;
}

static int pki_gen_pub_decode(int nid, EVP_PKEY *pkey, X509_PUBKEY *pubkey)
{
    const unsigned char *p;
    int pklen;
    X509_ALGOR *palg;
    ROUND5_KEYPAIR *kp = NULL;
    const struct round5_nid_data_st *nid_data = round5_get_nid_data(nid);

    if (nid_data == NULL) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PUB_DECODE, SUOLA_R_MISSING_NID_DATA);
        return 0;
    }

    if (!X509_PUBKEY_get0_param(NULL, &p, &pklen, &palg, pubkey))
        return 0;

    if (palg != NULL) {
        int ptype;

        /* Algorithm parameters must be absent */
        X509_ALGOR_get0(NULL, &ptype, NULL, palg);
        if (ptype != V_ASN1_UNDEF) {
//            SUOLAerr(SUOLA_F_ASN1_GENERIC_PUB_DECODE, SUOLA_R_INVALID_ENCODING);
            return 0;
        }
    }

    if (p == NULL || pklen != nid_data->pk_bytes) {
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PUB_DECODE, SUOLA_R_WRONG_LENGTH);
        return 0;
    }

    kp = _round5_keypair_new(nid, NO_PRIV_KEY);
//    if ( suola_keypair_is_invalid(kp) ){
//        SUOLAerr(SUOLA_F_ASN1_GENERIC_PUB_DECODE, SUOLA_R_INVALID_KEY);
//        return 0;
//    }

    memcpy(kp->key.pk, p, pklen);

    EVP_PKEY_assign(pkey, nid, kp);
    return 1;
}

static int pki_pub_cmp(const EVP_PKEY *a, consts EVP_PKEY *b){
    return 1;
}

int pki_register_asn1_meth(int nid, EVP_PKEY_ASN1_METHOD **ameth, const char *pem_str, const char *info){
    *ameth = EVP_PKEY_asn1_new(nid, 0, pem_str, info);
    if (!*ameth)
        return 0;
    if (nid == NID_ROUND5){
        EVP_PKEY_asn1_set_public(*ameth, pki_gen_pub_decode, pki_gen_pub_encode, pki_pub_cmp, pki_gen_pub_print, NULL, pki_curve25519_bits);
        EVP_PKEY_asn1_set_private(*ameth, pki_gen_priv_decode, pki_gen_priv_encode, pki_gen_priv_print);
        EVP_PKEY_asn1_set_ctrl(*ameth, pki_gen_ctrl);
    }
    EVP_PKEY_asn1_set_param(*ameth, 0, 0, 0, 0, pki_pub_cmp, 0);
#ifndef OPENSSL_V102_COMPAT
    EVP_PKEY_asn1_set_security_bits(*ameth, pki_curve25519_security_bits);
#endif
    EVP_PKEY_asn1_set_free(*ameth, pki_free);
    return 1;
}