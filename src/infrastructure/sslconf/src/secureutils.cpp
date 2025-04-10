// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "secureutils.h"
#include "confstring.h"
#include "finally.h"
#include "filesystem.h"

#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#if SYSAPI_WIN32
// Windows builds require a shim that makes it possible to link to different
// versions of the Win32 C runtime. See OpenSSL FAQ.
#include <openssl/applink.c>
#endif

namespace sslconf {

const EVP_MD* getDigestbyType(FingerprintType type)
{
    switch (type) {
        case FingerprintType::SHA1: 
            return EVP_sha1();
        case FingerprintType::SHA256: 
            return EVP_sha256();
        default:
            throw std::runtime_error("Invalid fingerprint type: " + std::to_string(static_cast<int>(type)));
    }
}

std::string format_ssl_fingerprint(const std::vector<uint8_t>& fingerprint, bool separator)
{
    std::string result = sslconf::string::to_hex(fingerprint, 2);

    // all uppercase
    sslconf::string::uppercase(result);

    if (separator) {
        // add colon to separate each 2 characters
        size_t separators = result.size() / 2;
        for (size_t i = 1; i < separators; i++) {
            result.insert(i * 3 - 1, ":");
        }
    }
    return result;
}

std::string format_ssl_fingerprint_columns(const std::vector<uint8_t>& fingerprint)
{
    auto max_columns = 8;

    std::string hex = sslconf::string::to_hex(fingerprint, 2);
    sslconf::string::uppercase(hex);
    if (hex.empty() || hex.size() % 2 != 0) {
        return hex;
    }

    std::string separated;
    for (std::size_t i = 0; i < hex.size(); i += max_columns * 2) {
        for (std::size_t j = i; j < i + 16 && j < hex.size() - 1; j += 2) {
            separated.push_back(hex[j]);
            separated.push_back(hex[j + 1]);
            separated.push_back(':');
        }
        separated.push_back('\n');
    }
    separated.pop_back(); // we don't need last newline character
    return separated;
}

FingerprintData get_ssl_cert_fingerprint(X509* cert, FingerprintType type)
{
    if (!cert) {
        throw std::runtime_error("certificate is null");
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_length = 0;
    int result = X509_digest(cert, getDigestbyType(type), digest, &digest_length);

    if (result <= 0) {
        throw std::runtime_error("failed to calculate fingerprint, digest result: " +
                                 std::to_string(result));
    }

    std::vector<std::uint8_t> digest_vec;
    digest_vec.assign(reinterpret_cast<std::uint8_t*>(digest),
                      reinterpret_cast<std::uint8_t*>(digest) + digest_length);
    return {fingerprint_type_to_string(type), digest_vec};
}

FingerprintData get_pem_file_cert_fingerprint(const std::string& path, FingerprintType type)
{
    auto fp = sslconf::fopen_utf8_path(path, "r");
    if (!fp) {
        throw std::runtime_error("Could not open certificate path");
    }
    auto file_close = sslconf::finally([fp]() { std::fclose(fp); });

    X509* cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!cert) {
        throw std::runtime_error("Certificate could not be parsed");
    }
    auto cert_free = sslconf::finally([cert]() { X509_free(cert); });

    return get_ssl_cert_fingerprint(cert, type);
}

void generate_pem_self_signed_cert(const std::string& path)
{
    auto expiration_days = 365;

    auto* private_key = EVP_PKEY_new();
    if (!private_key) {
        throw std::runtime_error("Could not allocate private key for certificate");
    }
    auto private_key_free = sslconf::finally([private_key](){ EVP_PKEY_free(private_key); });

    auto* rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);
    if (!rsa) {
        throw std::runtime_error("Failed to generate RSA key");
    }
    EVP_PKEY_assign_RSA(private_key, rsa);

    auto* cert = X509_new();
    if (!cert) {
        throw std::runtime_error("Could not allocate certificate");
    }
    auto cert_free = sslconf::finally([cert]() { X509_free(cert); });

    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), expiration_days * 24 * 3600);
    X509_set_pubkey(cert, private_key);

    auto* name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char *>("sslconf"), -1, -1, 0);
    X509_set_issuer_name(cert, name);

    X509_sign(cert, private_key, EVP_sha256());

    auto fp = sslconf::fopen_utf8_path(path.c_str(), "w");
    if (!fp) {
        throw std::runtime_error("Could not open certificate output path");
    }
    auto file_close = sslconf::finally([fp]() { std::fclose(fp); });

    PEM_write_PrivateKey(fp, private_key, nullptr, nullptr, 0, nullptr, nullptr);
    PEM_write_X509(fp, cert);
}

/*
    Draw an ASCII-Art representing the fingerprint so human brain can
    profit from its built-in pattern recognition ability.
    This technique is called "random art" and can be found in some
    scientific publications like this original paper:

    "Hash Visualization: a New Technique to improve Real-World Security",
    Perrig A. and Song D., 1999, International Workshop on Cryptographic
    Techniques and E-Commerce (CrypTEC '99)
    sparrow.ece.cmu.edu/~adrian/projects/validation/validation.pdf

    The subject came up in a talk by Dan Kaminsky, too.

    If you see the picture is different, the key is different.
    If the picture looks the same, you still know nothing.

    The algorithm used here is a worm crawling over a discrete plane,
    leaving a trace (augmenting the field) everywhere it goes.
    Movement is taken from dgst_raw 2bit-wise.  Bumping into walls
    makes the respective movement vector be ignored for this turn.
    Graphs are not unambiguous, because circles in graphs can be
walked in either direction.
 */

/*
    Field sizes for the random art.  Have to be odd, so the starting point
    can be in the exact middle of the picture, and FLDBASE should be >=8 .
    Else pictures would be too dense, and drawing the frame would
    fail, too, because the key type would not fit in anymore.
*/
#define	FLDBASE		8
#define	FLDSIZE_Y	(FLDBASE + 1)
#define	FLDSIZE_X	(FLDBASE * 2 + 1)

std::string create_fingerprint_randomart(const std::vector<std::uint8_t>& dgst_raw)
{
    /*
     * Chars to be used after each other every time the worm
     * intersects with itself.  Matter of taste.
     */
    const char* augmentation_string = " .o+=*BOX@%&#/^SE";
    // char *p;
    std::uint8_t field[FLDSIZE_X][FLDSIZE_Y];
    std::size_t i;
    std::uint32_t b;
    int	 x, y;
    std::size_t len = strlen(augmentation_string) - 1;

    std::vector<char> retval;
    retval.reserve((FLDSIZE_X + 3) * (FLDSIZE_Y + 2));

    auto add_char = [&retval](char ch) { retval.push_back(ch); };

    /* initialize field */
    std::memset(field, 0, FLDSIZE_X * FLDSIZE_Y * sizeof(char));
    x = FLDSIZE_X / 2;
    y = FLDSIZE_Y / 2;

    /* process raw key */
    for (i = 0; i < dgst_raw.size(); i++) {
        /* each byte conveys four 2-bit move commands */
        int input = dgst_raw[i];
        for (b = 0; b < 4; b++) {
            /* evaluate 2 bit, rest is shifted later */
            x += (input & 0x1) ? 1 : -1;
            y += (input & 0x2) ? 1 : -1;

            /* assure we are still in bounds */
            x = std::max(x, 0);
            y = std::max(y, 0);
            x = std::min(x, FLDSIZE_X - 1);
            y = std::min(y, FLDSIZE_Y - 1);

            /* augment the field */
            if (field[x][y] < len - 2)
                field[x][y]++;
            input = input >> 2;
        }
    }

    /* mark starting point and end point*/
    field[FLDSIZE_X / 2][FLDSIZE_Y / 2] = len - 1;
    field[x][y] = len;

    /* output upper border */
    add_char('+');
    for (i = 0; i < FLDSIZE_X; i++)
        add_char('-');
    add_char('+');
    add_char('\n');

    /* output content */
    for (y = 0; y < FLDSIZE_Y; y++) {
        add_char('|');
        for (x = 0; x < FLDSIZE_X; x++)
            add_char(augmentation_string[std::min<int>(field[x][y], len)]);
        add_char('|');
        add_char('\n');
    }

    /* output lower border */
    add_char('+');
    for (i = 0; i < FLDSIZE_X; i++)
        add_char('-');
    add_char('+');

    return std::string{retval.data(), retval.size()};
}

} // namespace sslconf
