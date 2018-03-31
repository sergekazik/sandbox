#include "sodium_glue.h"
#include <sodium.h>
#include <iostream>
#include <array>

#include "memory.h"

using namespace std;
using Ring::ByteArr;

static const uint8_t sign_priv[] = {
    0x2b,0x3a,0x3d,0x35,0x75,0xe5,0x94,0xec,0x77,0xf5,0xeb,0x96,0xf3,0xb9,0xda,0xc6,
    0x8a,0x00,0x21,0xdd,0x5a,0x9c,0x15,0xf0,0x0e,0xa7,0x46,0xc0,0xf8,0x21,0x22,0x38,
    0x9f,0x9c,0x2c,0x9a,0xc1,0xbd,0x07,0x7d,0xd9,0x2f,0xeb,0xa3,0x89,0x34,0x5e,0x0a,
    0x11,0xa3,0x85,0x72,0x84,0x66,0x7a,0xc4,0x85,0x56,0xf9,0x2d,0x36,0x0f,0xdf,0x97,
};
static const uint8_t sign_pub[] = {
    0x9f,0x9c,0x2c,0x9a,0xc1,0xbd,0x07,0x7d,0xd9,0x2f,0xeb,0xa3,0x89,0x34,0x5e,0x0a,
    0x11,0xa3,0x85,0x72,0x84,0x66,0x7a,0xc4,0x85,0x56,0xf9,0x2d,0x36,0x0f,0xdf,0x97,
};

static const auto PUBK_SZ = 32u;
static const auto SIGN_SZ = 64u;
static const auto NONCE_SZ = 20u;

// global for debug only
bool gbSodiumGlueDebugOn = false;

template <typename Array>
void debug_print(const char* name, const Array& arr)
{
    if (!gbSodiumGlueDebugOn) return;
    printf("%s (%zu bytes)= ", name, arr.size());
    for (auto c: arr)
    {
        printf("%02X", uint8_t(c));
    }
    printf("\n");
}

#define IO_MSG_BUFFER_SIZE 2000
void dump_hex_pl(const char* pref, ByteArr &pl)
{
    if (!gbSodiumGlueDebugOn) return;
    char outstr[IO_MSG_BUFFER_SIZE];
    memset(outstr, 0, IO_MSG_BUFFER_SIZE);

    int offset = sprintf(outstr, "%s|", pref);
    for (unsigned int i = 0; i < pl.size(); i++)
        offset += sprintf(&outstr[offset], "%02X", (unsigned char) pl.data()[i]);
    sprintf(&outstr[offset], "|%s", pref);
    printf("%s\n", outstr);

}

bool Ring::SodiumGlue::processPayload(ByteArr &payload)
{
    if (payload.size() != PUBK_SZ + SIGN_SZ + NONCE_SZ)
    {
        cout << "Error. Wrong payload: " << payload.size() << "bytes" << endl;
        return false;
    }
    debug_print("[C] ppp", payload);
    auto remote_pub_key = ByteArr(payload.data(), payload.data() + PUBK_SZ);
    auto sign = ByteArr(payload.data() + PUBK_SZ, payload.data() + PUBK_SZ + SIGN_SZ);
    auto nonce = ByteArr(payload.data() + PUBK_SZ + SIGN_SZ, payload.data() + PUBK_SZ + SIGN_SZ + NONCE_SZ);
    m_nonceStart = nonce;
    debug_print("[C] received publc", remote_pub_key);
    debug_print("[C] received sign ", sign);
    debug_print("[C] received nonce", nonce);

    //  check signature
    auto full_sign = sign;
    full_sign.insert(end(full_sign), begin(remote_pub_key), end(remote_pub_key));

    auto msg = ByteArr(crypto_box_PUBLICKEYBYTES);
    if (crypto_sign_open(msg.data(), nullptr, full_sign.data(), full_sign.size(), sign_pub) != 0)
    {
        debug_print("[C] wrong sign", full_sign);
        return false;
    }
    debug_print("[C] unsigned", msg);

    //  generate true shared key
    m_sharedSecret.resize(crypto_box_BEFORENMBYTES);
    (void)crypto_box_beforenm(m_sharedSecret.data(), remote_pub_key.data(), m_local_priv_key.data());
    debug_print("[C] shared key", m_sharedSecret);

    sodium_memzero(m_local_priv_key.data(), m_local_priv_key.size());
    return true;
}

bool Ring::SodiumGlue::doHandshake()
{
    (void)sodium_init();

    //  genearate private/public key pair and send public part
    ByteArr local_pub_key(crypto_box_PUBLICKEYBYTES);
    ByteArr local_priv_key(crypto_box_SECRETKEYBYTES);
    crypto_box_keypair(local_pub_key.data(), local_priv_key.data());
    debug_print("[C] local privt", local_priv_key);
    debug_print("[C] local publc", local_pub_key);

    m_local_priv_key = local_priv_key;
    m_local_public_key = local_pub_key;

    if (m_sendHandler)
        m_sendHandler(local_pub_key);
//    else
//        dump_hex_pl("KEY", local_pub_key);

    if (m_receiveHandler)
    {   //  wait and parse public payload
        auto payload = m_receiveHandler();
        return Ring::SodiumGlue::processPayload(payload);
    }
    return true;
}

bool Ring::SodiumGlue::doHandshake(const Ring::ByteArr &pub_key)
{
    (void)sodium_init();

    debug_print("[S] pub_key", pub_key);

    //  genearate private/public key pair
    ByteArr local_pub_key(crypto_box_PUBLICKEYBYTES);
    ByteArr local_priv_key(crypto_box_SECRETKEYBYTES);
    crypto_box_keypair(local_pub_key.data(), local_priv_key.data());
    debug_print("[S] local priv", local_priv_key);
    debug_print("[S] local pub", local_pub_key);

    //  generate nonce start
    auto nonce_start = ByteArr(NONCE_SZ);
    randombytes_buf(nonce_start.data(), nonce_start.size());

    //  prepare payload and send
    auto sig = ByteArr(crypto_sign_BYTES + crypto_box_PUBLICKEYBYTES);
    crypto_sign(sig.data(), nullptr, local_pub_key.data(), local_pub_key.size(), sign_priv);
    auto payload = local_pub_key;
    payload.insert(std::end(payload), std::begin(sig), std::begin(sig)+crypto_sign_BYTES);
    payload.insert(std::end(payload), std::begin(nonce_start), std::end(nonce_start));
    m_nonceStart = nonce_start;

    debug_print("[S] payload", payload);
    m_local_public_key = payload;

    if (m_sendHandler)
        m_sendHandler(payload);

    //  generate true shared key
    m_sharedSecret.resize(crypto_box_BEFORENMBYTES);
    (void)crypto_box_beforenm(m_sharedSecret.data(), pub_key.data(), local_priv_key.data());
    debug_print("[S] shared key", m_sharedSecret);

    sodium_memzero(local_priv_key.data(), local_priv_key.size());
    return true;
}

ByteArr Ring::SodiumGlue::encrypt(const ByteArr &data)
{
    //sodium_increment((uint8_t*)(&m_nonceCounter), sizeof(m_nonceCounter));
    ++m_nonceCounter;
    auto nonce = m_nonceStart;
    nonce.insert(end(nonce), (uint8_t*)(&m_nonceCounter), (uint8_t*)((&m_nonceCounter)+1));
    // debug_print("[e] nonce", nonce);

    auto tmp_data = data;
    tmp_data.insert(begin(tmp_data), 32, uint8_t{0});

    auto res = ByteArr(tmp_data.size());
    crypto_box_afternm(res.data(), tmp_data.data(), tmp_data.size(), nonce.data(), m_sharedSecret.data());

    auto restored_res = ByteArr{};
    restored_res.insert(end(restored_res), begin(res)+crypto_box_BOXZEROBYTES, end(res));
    res = restored_res;
    res.insert(begin(res), (uint8_t*)(&m_nonceCounter), (uint8_t*)((&m_nonceCounter)+1));

    debug_print("ENC=", res);

    return res;
}

ByteArr Ring::SodiumGlue::decrypt(const ByteArr &data)
{
    auto nonce = m_nonceStart;
    nonce.insert(end(nonce), begin(data), begin(data) + 4);

    auto prep_data = ByteArr{};
    prep_data.insert(begin(prep_data), crypto_box_BOXZEROBYTES, uint8_t{0});
    prep_data.insert(end(prep_data), begin(data)+4, end(data));

    auto res = ByteArr(data.size()-4 + crypto_box_ZEROBYTES);
    (void)crypto_box_open_afternm(res.data(), prep_data.data(), prep_data.size(), nonce.data(), m_sharedSecret.data());

    auto restored_res = ByteArr{};
    restored_res.insert(end(restored_res), begin(res)+crypto_box_ZEROBYTES, end(res) - crypto_box_BOXZEROBYTES);
    res = restored_res;

    return res;
}

char* Ring::SodiumGlue::deconvert_from_array(ByteArr &in)
{
    int i = 0;
    char *str = new char[in.size()];
    for (auto c: in)
        str[i++] = c;
    return str;
}

ByteArr Ring::SodiumGlue::convert_to_array(const char *in, bool hex)
{
    int length = hex ? strlen(in)/2 : strlen(in);
    ByteArr res = ByteArr(in, in + length);

    for (unsigned int idx = 0; idx < res.size(); idx++)
    {
        if (hex)
        {
            int val;
            sscanf(&in[idx*2], "%02X", &val);
            res.data()[idx] = val;
        }
        else
            res.data()[idx] = in[idx];

    }
    return res;
}

//std::pair<ByteArr, ByteArr> Ring::GenSignPK()
//{
//    ByteArr pk(crypto_sign_PUBLICKEYBYTES);
//    ByteArr sk(crypto_sign_SECRETKEYBYTES);
//    crypto_sign_keypair(pk.data(), sk.data());

//    printf("public: ");
//    for (auto c: pk) {
//        printf("0x%2x,", uint8_t(c));
//    }
//    printf("\nprivate: ");
//    for (auto c: sk) {
//        printf("0x%2x,", uint8_t(c));
//    }
//    printf("\n");

//    return make_pair(pk, sk);
//}
