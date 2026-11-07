#include "SSLCert.hpp"
#include <mbedtls/oid.h>

namespace httpsserver {

SSLCert::SSLCert(unsigned char * certData, uint16_t certLength, unsigned char * pkData, uint16_t pkLength):
  _certLength(certLength),
  _certData(certData),
  _pkLength(pkLength),
  _pkData(pkData) {

}

SSLCert::~SSLCert() {
  // TODO Auto-generated destructor stub
}


uint16_t SSLCert::getCertLength() {
  return _certLength;
}

uint16_t SSLCert::getPKLength() {
  return _pkLength;
}

unsigned char * SSLCert::getCertData() {
  return _certData;
}

unsigned char * SSLCert::getPKData() {
  return _pkData;
}

void SSLCert::setPK(unsigned char * pkData, uint16_t length) {
  _pkData = pkData;
  _pkLength = length;
}

void SSLCert::setCert(unsigned char * certData, uint16_t length) {
  _certData = certData;
  _certLength = length;
}

void SSLCert::clear() {
  // Arrays from new[] (key and certificate generation): wipe, then delete[] (was delete).
  if (_certData != NULL) {
    for(uint16_t i = 0; i < _certLength; i++) _certData[i]=0;
    delete[] _certData;
    _certData = NULL;
  }
  _certLength = 0;

  if (_pkData != NULL) {
    for(uint16_t i = 0; i < _pkLength; i++) _pkData[i] = 0;
    delete[] _pkData;
    _pkData = NULL;
  }
  _pkLength = 0;
}

#ifndef HTTPS_DISABLE_SELFSIGNING

/**
 * Function to create the key for a self-signed certificate.
 * 
 * Writes private key as DER in certCtx
 * 
 * Based on programs/pkey/gen_key.c
 */
static int gen_key(SSLCert &certCtx, SSLKeySize keySize) {

  // Initialize the entropy source
  mbedtls_entropy_context entropy;
  mbedtls_entropy_init( &entropy );

  // Initialize the RNG
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ctr_drbg_init( &ctr_drbg );
  int rngRes = mbedtls_ctr_drbg_seed(
    &ctr_drbg, mbedtls_entropy_func, &entropy,
    NULL, 0
  );
  if (rngRes != 0) {
    mbedtls_entropy_free( &entropy );
    return HTTPS_SERVER_ERROR_KEYGEN_RNG;
  }

  // Initialize the private key
  mbedtls_pk_context key;
  mbedtls_pk_init( &key );
  bool ecdsa = keySize == KEYTYPE_ECDSA_P256;
  int resPkSetup = mbedtls_pk_setup( &key, mbedtls_pk_info_from_type( ecdsa ? MBEDTLS_PK_ECKEY : MBEDTLS_PK_RSA ) );
  if ( resPkSetup != 0) {
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );
    return HTTPS_SERVER_ERROR_KEYGEN_SETUP_PK;
  }

  // Actual key generation 
  int resPkGen = ecdsa
    ? mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec( key ), mbedtls_ctr_drbg_random, &ctr_drbg)
    : mbedtls_rsa_gen_key(
        mbedtls_pk_rsa( key ),
        mbedtls_ctr_drbg_random,
        &ctr_drbg,
        keySize,
        65537
      );
  if ( resPkGen != 0) {
    mbedtls_pk_free( &key );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );
    return HTTPS_SERVER_ERROR_KEYGEN_GEN_PK;
  }

  // Free the entropy source and the RNG as they are no longer needed
  mbedtls_ctr_drbg_free( &ctr_drbg );
  mbedtls_entropy_free( &entropy );

  // Allocate the space on the heap, as stack size is quite limited
  unsigned char * output_buf = new unsigned char[4096];
  if (output_buf == NULL) {
    mbedtls_pk_free( &key );
    return HTTPS_SERVER_ERROR_KEY_OUT_OF_MEM;
  }
  memset(output_buf, 0, 4096);

  // Write the key to the temporary buffer and determine its length
  int resPkWrite = mbedtls_pk_write_key_der( &key, output_buf, 4096 );
  if (resPkWrite < 0) {
    delete[] output_buf;
    mbedtls_pk_free( &key );
    return HTTPS_SERVER_ERROR_KEY_WRITE_PK;
  }
  size_t pkLength = resPkWrite;
  unsigned char *pkOffset = output_buf + sizeof(unsigned char) * 4096 - pkLength;

  // Copy the key into a new, fitting space on the heap
  unsigned char * output_pk = new unsigned char[pkLength];
  if (output_pk == NULL) {
    delete[] output_buf;
    mbedtls_pk_free( &key );
    return HTTPS_SERVER_ERROR_KEY_WRITE_PK;
  }
  memcpy(output_pk, pkOffset, pkLength);

  // Clean up the temporary buffer and clear the key context
  delete[] output_buf;
  mbedtls_pk_free( &key );

  // Set the private key in the context
  certCtx.setPK(output_pk, pkLength);

  return 0;
}

/**
 * Function to generate the X509 certificate data for a private key
 * 
 * Writes certificate in certCtx
 *
 * Based on programs/x509/cert_write.c
 */
// DER for subjectAltName: SEQUENCE { [2] dNSName ..., [7] iPAddress ... }
static std::vector<unsigned char> encodeSubjectAltName(const std::vector<std::string> &dnsNames, const std::vector<uint32_t> &ipv4) {
  std::vector<unsigned char> body;
  for (const auto &name : dnsNames) {
    if (name.empty() || name.length() > 127) continue;
    body.push_back(0x82);
    body.push_back((unsigned char)name.length());
    body.insert(body.end(), name.begin(), name.end());
  }
  for (uint32_t ip : ipv4) {
    const unsigned char* b = (const unsigned char*)&ip; // network order a.b.c.d
    body.push_back(0x87); body.push_back(4);
    body.insert(body.end(), b, b + 4);
  }
  std::vector<unsigned char> der;
  der.push_back(0x30);
  if (body.size() < 128) {
    der.push_back((unsigned char)body.size());
  } else {
    der.push_back(0x82);
    der.push_back((unsigned char)(body.size() >> 8));
    der.push_back((unsigned char)(body.size() & 0xFF));
  }
  der.insert(der.end(), body.begin(), body.end());
  return der;
}

static int cert_write(SSLCert &certCtx, std::string dn, std::string validityFrom, std::string validityTo,
                      const std::vector<std::string> &altDnsNames, const std::vector<uint32_t> &altIPv4) {
  int funcRes = 0;
  int stepRes = 0;

  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_pk_context key;
  mbedtls_x509write_cert crt;
  mbedtls_mpi serial;
  unsigned char * primary_buffer;
  unsigned char *certOffset;
  unsigned char * output_buffer;
  size_t certLength;

  // Make a C-friendly version of the distinguished name
  char dn_cstr[dn.length()+1];
  strcpy(dn_cstr, dn.c_str());

  // Initialize the entropy source
  mbedtls_entropy_init( &entropy );

  // Initialize the RNG
  mbedtls_ctr_drbg_init( &ctr_drbg );
  stepRes = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0 );
  if (stepRes != 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_RNG;
    goto error_after_entropy;
  }

  mbedtls_pk_init( &key );
  stepRes = mbedtls_pk_parse_key( &key, certCtx.getPKData(), certCtx.getPKLength(), NULL, 0 );
  if (stepRes != 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_READKEY;
    goto error_after_key;
  }

  // Start configuring the certificate
  mbedtls_x509write_crt_init( &crt );
  // Set version and hash algorithm
  mbedtls_x509write_crt_set_version( &crt, MBEDTLS_X509_CRT_VERSION_3 );
  mbedtls_x509write_crt_set_md_alg( &crt, MBEDTLS_MD_SHA256 );

  // Set the keys (same key as we self-sign)
  mbedtls_x509write_crt_set_subject_key( &crt, &key );
  mbedtls_x509write_crt_set_issuer_key( &crt, &key );

  // Set issuer and subject (same, as we self-sign)
  stepRes = mbedtls_x509write_crt_set_subject_name( &crt, dn_cstr );
  if (stepRes != 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_NAME;
    goto error_after_cert;
  }
  stepRes = mbedtls_x509write_crt_set_issuer_name( &crt, dn_cstr );
  if (stepRes != 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_NAME;
    goto error_after_cert;
  }

  // Set the validity of the certificate. At the moment, it's fixed from 2019 to end of 2029.
  stepRes = mbedtls_x509write_crt_set_validity( &crt, validityFrom.c_str(), validityTo.c_str());
  if (stepRes != 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_VALIDITY;
    goto error_after_cert;
  }

  // A server (leaf) certificate, not a CA: browsers treat a CA certificate presented by a
  // server as an error on top of it being self-signed.
  stepRes = mbedtls_x509write_crt_set_basic_constraints( &crt, 0, -1 );
  if (stepRes != 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_VALIDITY;
    goto error_after_cert;
  }
  {
    bool ecdsa = mbedtls_pk_get_type(&key) == MBEDTLS_PK_ECKEY || mbedtls_pk_get_type(&key) == MBEDTLS_PK_ECDSA;
    unsigned int keyUsage = MBEDTLS_X509_KU_DIGITAL_SIGNATURE | (ecdsa ? 0 : MBEDTLS_X509_KU_KEY_ENCIPHERMENT);
    stepRes = mbedtls_x509write_crt_set_key_usage( &crt, keyUsage );
    if (stepRes != 0) {
      funcRes = HTTPS_SERVER_ERROR_CERTGEN_VALIDITY;
      goto error_after_cert;
    }
    // extendedKeyUsage: serverAuth
    static const unsigned char ekuServerAuth[] = { 0x30, 0x0A, 0x06, 0x08, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01 };
    stepRes = mbedtls_x509write_crt_set_extension( &crt, MBEDTLS_OID_EXTENDED_KEY_USAGE, MBEDTLS_OID_SIZE(MBEDTLS_OID_EXTENDED_KEY_USAGE),
      0, ekuServerAuth, sizeof(ekuServerAuth) );
    if (stepRes != 0) {
      funcRes = HTTPS_SERVER_ERROR_CERTGEN_VALIDITY;
      goto error_after_cert;
    }
    if (!altDnsNames.empty() || !altIPv4.empty()) {
      std::vector<unsigned char> san = encodeSubjectAltName(altDnsNames, altIPv4);
      stepRes = mbedtls_x509write_crt_set_extension( &crt, MBEDTLS_OID_SUBJECT_ALT_NAME, MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME),
        0, san.data(), san.size() );
      if (stepRes != 0) {
        funcRes = HTTPS_SERVER_ERROR_CERTGEN_VALIDITY;
        goto error_after_cert;
      }
    }
  }

  // Random serial number. A fixed serial ("1") with the same issuer name on every new
  // certificate makes Firefox refuse the new one outright (SEC_ERROR_REUSED_ISSUER_AND_SERIAL).
  mbedtls_mpi_init( &serial );
  {
    unsigned char serialBytes[16];
    stepRes = mbedtls_ctr_drbg_random( &ctr_drbg, serialBytes, sizeof(serialBytes) );
    serialBytes[0] &= 0x7F; // positive
    serialBytes[0] |= 0x01; // and never zero
    if (stepRes == 0) stepRes = mbedtls_mpi_read_binary( &serial, serialBytes, sizeof(serialBytes) );
  }
  if (stepRes != 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_SERIAL;
    goto error_after_cert_serial;
  }
  stepRes = mbedtls_x509write_crt_set_serial( &crt, &serial );
  if (stepRes != 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_SERIAL;
    goto error_after_cert_serial;
  }

  // Create buffer to write the certificate
  primary_buffer = new unsigned char[4096];
  if (primary_buffer == NULL) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_OUT_OF_MEM;
    goto error_after_cert_serial;
  }

  // Write the actual certificate
  stepRes = mbedtls_x509write_crt_der(&crt, primary_buffer, 4096, mbedtls_ctr_drbg_random, &ctr_drbg );
  if (stepRes < 0) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_WRITE;
    goto error_after_primary_buffer;
  }

  // Create a matching buffer
  certLength = stepRes;
  certOffset = primary_buffer + sizeof(unsigned char) * 4096 - certLength;

  // Copy the cert into a new, fitting space on the heap
  output_buffer = new unsigned char[certLength];
  if (output_buffer == NULL) {
    funcRes = HTTPS_SERVER_ERROR_CERTGEN_OUT_OF_MEM;
    goto error_after_primary_buffer;
  }
  memcpy(output_buffer, certOffset, certLength);

  // Configure the cert in the context
  certCtx.setCert(output_buffer, certLength);

  // Run through the cleanup process
error_after_primary_buffer:
  delete[] primary_buffer;

error_after_cert_serial:
  mbedtls_mpi_free( &serial );

error_after_cert:
  mbedtls_x509write_crt_free( &crt );

error_after_key:
  mbedtls_pk_free(&key);

error_after_entropy:
  mbedtls_ctr_drbg_free( &ctr_drbg );
  mbedtls_entropy_free( &entropy );
  return funcRes;
}

int createSelfSignedCert(SSLCert &certCtx, SSLKeySize keySize, std::string dn, std::string validFrom, std::string validUntil) {
  return createSelfSignedCert(certCtx, keySize, dn, validFrom, validUntil, std::vector<std::string>(), std::vector<uint32_t>());
}

int createSelfSignedCert(SSLCert &certCtx, SSLKeySize keySize, std::string dn, std::string validFrom, std::string validUntil,
  const std::vector<std::string> &altDnsNames, const std::vector<uint32_t> &altIPv4) {

  // Add the private key
  int keyRes = gen_key(certCtx, keySize);
  if (keyRes != 0) {
    // Key-generation failed, return the failure code
    return keyRes;
  }

  // Add the self-signed certificate
  int certRes = cert_write(certCtx, dn, validFrom, validUntil, altDnsNames, altIPv4);
  if (certRes != 0) {
    // Cert writing failed, reset the pk and return failure code
    certCtx.setPK(NULL, 0);
    return certRes;
  }

  // If all went well, return 0
  return 0;
}

#endif // !HTTPS_DISABLE_SELFSIGNING

} /* namespace httpsserver */
