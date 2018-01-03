#include "test_handshake.h"
#include <sodium.h>
#include <iostream>
#include <array>

using namespace std;
using Ring::ByteArr;

static const uint8_t sign_priv[] = {
    0x2b,0xf8,0x4f,0x99,0x6a,0xfa,0xb6,0x53,0xd0,0x57,0x9f,0xb0,0x5a,0x7b,0xa2,0xd1,
    0xca,0xd2,0xa4,0x80,0x13,0x2d,0x98,0xf9,0x82,0xe0,0x1c,0x49,0x7c,0x84,0xa0,0x49,
    0xbb,0x30,0x38,0xee,0x8c,0x71,0x2b,0x47,0x86,0xfb,0x57,0x5c,0xa0,0x57,0xf6,0x81,
    0xa5,0xef,0x24,0xfa,0x49,0x8f,0x25,0xf4,0x8d,0xe1,0xe0,0xfa,0x49,0x70,0xc4,0x97
};
static const uint8_t sign_pub[] = {
    0xbb,0x30,0x38,0xee,0x8c,0x71,0x2b,0x47,0x86,0xfb,0x57,0x5c,0xa0,0x57,0xf6,0x81,
    0xa5,0xef,0x24,0xfa,0x49,0x8f,0x25,0xf4,0x8d,0xe1,0xe0,0xfa,0x49,0x70,0xc4,0x97
};

static const auto PUBK_SZ = 32u;
static const auto SIGN_SZ = 64u;
static const auto NONCE_SZ = 20u;

template <typename Array>
void debug_print(const char* name, const Array& arr)
{
    printf("%s (%zu bytes): ", name, arr.size());
    for (auto c: arr)
    {
        printf("%02x", uint8_t(c));
    }
    printf("\n");
}

bool Ring::EncHandshake::doHandshake()
{
    (void)sodium_init();

    //  genearate private/public key pair and send public part
    ByteArr local_pub_key(crypto_box_PUBLICKEYBYTES);
    ByteArr local_priv_key(crypto_box_SECRETKEYBYTES);
    crypto_box_keypair(local_pub_key.data(), local_priv_key.data());
    debug_print("[C] local priv", local_priv_key);
    debug_print("[C] local pub", local_pub_key);
    m_sendHandler(local_pub_key);

    //  wait and parse public payload
    auto payload = m_receiveHandler();
    if (payload.size() != PUBK_SZ + SIGN_SZ + NONCE_SZ)
    {
        cout << "Error. Wrong payload: " << payload.size() << "bytes" << endl;
        return false;
    }
    auto remote_pub_key = ByteArr(payload.data(), payload.data() + PUBK_SZ);
    auto sign = ByteArr(payload.data() + PUBK_SZ, payload.data() + PUBK_SZ + SIGN_SZ);
    auto nonce = ByteArr(payload.data() + PUBK_SZ + SIGN_SZ, payload.data() + PUBK_SZ + SIGN_SZ + NONCE_SZ);
    m_nonceStart = nonce;
    debug_print("[C] received pub", remote_pub_key);
    debug_print("[C] received sign", sign);
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
    (void)crypto_box_beforenm(m_sharedSecret.data(), remote_pub_key.data(), local_priv_key.data());
    debug_print("[C] shared key", m_sharedSecret);

    sodium_memzero(local_priv_key.data(), local_priv_key.size());
    return true;
}

bool Ring::EncHandshake::doHandshake(const Ring::ByteArr &pub_key)
{
    (void)sodium_init();

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
    m_sendHandler(payload);

    //  generate true shared key
    m_sharedSecret.resize(crypto_box_BEFORENMBYTES);
    (void)crypto_box_beforenm(m_sharedSecret.data(), pub_key.data(), local_priv_key.data());
    debug_print("[S] shared key", m_sharedSecret);

    sodium_memzero(local_priv_key.data(), local_priv_key.size());
    return true;
}

bool Ring::EncHandshake::valid()
{
    return false;
}

ByteArr Ring::EncHandshake::encrypt(const ByteArr &data)
{
    //sodium_increment((uint8_t*)(&m_nonceCounter), sizeof(m_nonceCounter));
    ++m_nonceCounter;
    auto nonce = m_nonceStart;
    nonce.insert(end(nonce), (uint8_t*)(&m_nonceCounter), (uint8_t*)((&m_nonceCounter)+1));
    debug_print("[e] nonce", nonce);

    auto tmp_data = data;
    tmp_data.insert(begin(tmp_data), 32, uint8_t{0});

    auto res = ByteArr(tmp_data.size());
    crypto_box_afternm(res.data(), tmp_data.data(), tmp_data.size(), nonce.data(), m_sharedSecret.data());

    auto restored_res = ByteArr{};
    restored_res.insert(end(restored_res), begin(res)+crypto_box_BOXZEROBYTES, end(res));
    res = restored_res;
    res.insert(begin(res), (uint8_t*)(&m_nonceCounter), (uint8_t*)((&m_nonceCounter)+1));

    return res;
}

ByteArr Ring::EncHandshake::decrypt(const ByteArr &data)
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

std::pair<ByteArr, ByteArr> Ring::GenSignPK()
{
    ByteArr pk(crypto_sign_PUBLICKEYBYTES);
    ByteArr sk(crypto_sign_SECRETKEYBYTES);
    crypto_sign_keypair(pk.data(), sk.data());

    printf("public: ");
    for (auto c: pk) {
        printf("0x%2x,", uint8_t(c));
    }
    printf("\nprivate: ");
    for (auto c: sk) {
        printf("0x%2x,", uint8_t(c));
    }
    printf("\n");

    return make_pair(pk, sk);
}
