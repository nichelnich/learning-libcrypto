#include <openssl/provider.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void error_and_exit(const char* msg) {
  printf("%s\n", msg);
  char buf[256];
  int err = ERR_get_error();
  ERR_error_string_n(err, buf, sizeof(buf));
  printf("errno: %d, %s\n", err, buf);
}

void* get_ec_key(void* args) {
  EVP_PKEY* pkey = (EVP_PKEY*) args;
  printf("[%u] get_ec_keys: pkey: %p\n", pthread_self(), pkey);
  EC_KEY* ec_key = EVP_PKEY_get0_EC_KEY(pkey);
  printf("[%u] get_ec_keys: ec_key: %p\n", pthread_self(), ec_key);
  pthread_exit(NULL);
}

void* get_pkcs8(void* args) {
  EVP_PKEY* pkey = (EVP_PKEY*) args;
  printf("[%u] get_pkcs8: pkey: %p\n", pthread_self(), pkey);

  PKCS8_PRIV_KEY_INFO* pkcs8 = EVP_PKEY2PKCS8(pkey);
  BIO* pkcs8_bio = BIO_new(BIO_s_mem());
  if  (i2d_PKCS8_PRIV_KEY_INFO_bio(pkcs8_bio, pkcs8) <= 0) {
    error_and_exit("Could not convert to PKCS8 format");
  }
  pthread_exit(NULL);
}

int main(int arc, char *argv[]) {
  //const char* curve_name = "secp384r1";
  const char* curve_name = "P-384";
  printf("Elliptic Curve keygen (%s) example \n", curve_name);
  int curve_nid = EC_curve_nist2nid(curve_name);
  if (curve_nid == NID_undef) {
    // try converting the shortname (sn) to nid (numberic id)
    curve_nid = OBJ_sn2nid(curve_name);
  }
  printf("curve_nid of %s: %d\n", curve_name, curve_nid);

  // The last argument is the ENGINE*.
  EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
  // The following function is for generating parameters
  if (EVP_PKEY_paramgen_init(ctx) <= 0) {
    error_and_exit("Could not initialize the parameters for key generation");
  }

  // Set the curve as there are no default curves.
  if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, curve_nid) <= 0) {
    error_and_exit("Could not set the param curve nid");
  }

  // Set the parameter encoding which can be either OPENSSL_EC_EXPLICIT_CURVE
  // or OPENSSL_EC_NAMED_CURVE. The default for OpenSSL 3.x is named curve
  printf("Set ec_param_encoding to %d\n", OPENSSL_EC_NAMED_CURVE);
  int ret = EVP_PKEY_CTX_set_ec_param_enc(ctx, OPENSSL_EC_NAMED_CURVE);
  if (ret  <= 0) {
    printf("EVP_PKEY_CTX_set_ec_param_enc retuned: %d\n", ret);
    error_and_exit("EVP_PKEY_CTX_set_ec_param_enc failed");
  } 

  EVP_PKEY* params = NULL;
  // Generate the parameters.
  if (EVP_PKEY_paramgen(ctx, &params) <= 0) {
    error_and_exit("Could not generate the paremeters");
  }

  EVP_PKEY_CTX* key_ctx = EVP_PKEY_CTX_new(params, NULL);

  if (EVP_PKEY_keygen_init(key_ctx) <= 0) {
    error_and_exit("Could not initialize the keygen context the paremeters");
  }

  EVP_PKEY* pkey = NULL;
  if (EVP_PKEY_keygen(key_ctx, &pkey) <= 0) {
    error_and_exit("Could not generate the private key");
  }

  printf("created EVP_PKEY, now create threads and pass reference as args\n");
  pthread_t get_pkcs8_t;
  pthread_t get_ec_key_t;
  pthread_create(&get_pkcs8_t, NULL, get_pkcs8, pkey);
  pthread_create(&get_ec_key_t, NULL, get_ec_key, pkey);

  void* status;
  pthread_join(get_pkcs8_t, &status);
  pthread_join(get_ec_key_t, &status);

  //EVP_PKEY_CTX_free(ctx);
  exit(EXIT_SUCCESS);
}
