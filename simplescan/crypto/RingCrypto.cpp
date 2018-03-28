#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sodium.h>
#include "RingCrypto.hh"

using namespace Ring;
using namespace Ring::Ble;
using namespace Ring::Ble::Crypto;

// std::pair<ByteArr, ByteArr> GenSignPK();

Client::Client(char *aShared, int aLenght) :
    mNonceCount(0)
{
    (void) sodium_init();

    //  genearate private/public key pair
    mLocalPublicKey = ByteArr (crypto_box_PUBLICKEYBYTES);
    mLocalPrivateKey = ByteArr (crypto_box_SECRETKEYBYTES);
    mSignaturePublic = ByteArr(aShared, aShared + aLenght);

    crypto_box_keypair(mLocalPublicKey.data(), mLocalPrivateKey.data());

    Crypto::Debug::DumpArray("[C] local pub", mLocalPublicKey);
    Crypto::Debug::DumpArray("[C] local priv", mLocalPrivateKey);
}

///
/// \brief GetPublicKey
/// \param aKey - [out] buffer to copy
/// \param aLength [in/out]: [in] max aKey [out] lenght of the returned public key
/// \return pointer to aKey
///
char* Client::GetPublicKey(char *aKey, int &aLength)
{
    if (!aKey || !aLength || (aLength < (int) mLocalPublicKey.size()))
    {
        return NULL;
    }

    memcpy(aKey, mLocalPublicKey.data(), aLength = mLocalPublicKey.size());
    return aKey;
}

///
/// \brief ProcessPublicPayload
/// \param aPayload
/// \param aLength [in] is length of aPayload to process
/// \return error code
///
int Client::ProcessPublicPayload(char *aPayload, int aLength)
{

    // parse public payload
    ByteArr payload(aPayload, aPayload + aLength);

    if (payload.size() != Crypto::Size::PublicKey + Crypto::Size::Signature + Crypto::Size::Nonce)
    {
        return Crypto::Error::INVALID_SIZE;
    }

    ByteArr remote_pub_key(payload.data(), payload.data() + Crypto::Size::PublicKey);
    ByteArr sign (payload.data() + Crypto::Size::PublicKey, payload.data() + Crypto::Size::PublicKey + Crypto::Size::Signature);

    mNonce = ByteArr(payload.data() + Crypto::Size::PublicKey + Crypto::Size::Signature, payload.data() + Crypto::Size::PublicKey + Crypto::Size::Signature + Crypto::Size::Nonce);

    Crypto::Debug::DumpArray("[C] received pub", remote_pub_key);
    Crypto::Debug::DumpArray("[C] received sign", sign);
    Crypto::Debug::DumpArray("[C] received nonce", mNonce);

    //  check signature
    auto full_sign = sign;
    full_sign.insert(end(full_sign), begin(remote_pub_key), end(remote_pub_key));

    auto msg = ByteArr(crypto_box_PUBLICKEYBYTES);
    if (crypto_sign_open(msg.data(), nullptr, full_sign.data(), full_sign.size(), mSignaturePublic.data()) != 0)
    {
        Crypto::Debug::DumpArray("[C] wrong signature", full_sign);
        return Crypto::Error::INVALID_SIGNATURE;
    }
    Crypto::Debug::DumpArray("[C] unsigned", msg);

    //  generate true shared key
    mSharedSecret.resize(crypto_box_BEFORENMBYTES);
    (void)crypto_box_beforenm(mSharedSecret.data(), remote_pub_key.data(), mLocalPrivateKey.data());
    Crypto::Debug::DumpArray("[C] shared key", mSharedSecret);

    sodium_memzero(mLocalPrivateKey.data(), mLocalPrivateKey.size());
    return Crypto::Error::NO_ERROR;
}

///
/// \brief Encrypt
/// \param aSrc [in] buffer to encrypt
/// \param aSrcLen [in] lenght of aSrc
/// \param aDest [out] buffer to copy encrypted output
/// \param aDestLen [in/out]: [in] max aDest [out] lenght of the encrypted output
/// \return error
///
int Client::Encrypt(char *aSrc, int aSrcLen, char *aDest, int &aDestLen)
{
    if (!aSrc || !aSrcLen || !aDest || !aDestLen)
        return Crypto::Error::INVALID_ARGUMENT;

    ++mNonceCount;
    ByteArr nonce = mNonce;
    nonce.insert(end(nonce), (uint8_t*)(&mNonceCount), (uint8_t*)((&mNonceCount)+1));
    Crypto::Debug::DumpArray("[C] nonce", nonce);

    ByteArr tmp_data(aSrc, aSrc + aSrcLen);
    tmp_data.insert(begin(tmp_data), Crypto::Size::DataHeader, uint8_t{0});

    ByteArr res(tmp_data.size());
    crypto_box_afternm(res.data(), tmp_data.data(), tmp_data.size(), nonce.data(), mSharedSecret.data());

    auto restored_res = ByteArr{};
    restored_res.insert(end(restored_res), begin(res)+crypto_box_BOXZEROBYTES, end(res));
    res = restored_res;
    res.insert(begin(res), (uint8_t*)(&mNonceCount), (uint8_t*)((&mNonceCount)+1));

    if (aDestLen < (int) res.size())
    {
        Crypto::Debug::DumpArray("[C] aDestLen < res.size", res);
        return Crypto::Error::INVALID_SIZE;
    }

    memcpy(aDest, res.data(), aDestLen = res.size());
    return Crypto::Error::NO_ERROR;
}

///
/// \brief Decrypt
/// \param aSrc [in] buffer to decrypt
/// \param aSrcLen [in] lenght of aSrc
/// \param aDest [out] buffer to copy decrypted output
/// \param aDestLen [in/out]: [in] max aDest [out] lenght of the decrypted output
/// \return pointer to aDest
///
int Client::Decrypt(char *aSrc, int aSrcLen, char *aDest, int &aDestLen)
{
    if (!aSrc || !aSrcLen || !aDest || !aDestLen)
        return Crypto::Error::INVALID_ARGUMENT;

    ByteArr data(aSrc, aSrc + aSrcLen);
    auto nonce = mNonce;
    nonce.insert(end(nonce), begin(data), begin(data) + 4);

    auto prep_data = ByteArr{};
    prep_data.insert(begin(prep_data), crypto_box_BOXZEROBYTES, uint8_t{0});
    prep_data.insert(end(prep_data), begin(data)+4, end(data));

    auto res = ByteArr(data.size()-4 + crypto_box_ZEROBYTES);
    (void)crypto_box_open_afternm(res.data(), prep_data.data(), prep_data.size(), nonce.data(), mSharedSecret.data());

    auto restored_res = ByteArr{};
    restored_res.insert(end(restored_res), begin(res)+crypto_box_ZEROBYTES, end(res) - crypto_box_BOXZEROBYTES);
    res = restored_res;

    if (aDestLen < (int) res.size())
    {
        Crypto::Debug::DumpArray("[S] aDestLen < res.size", res);
        return Crypto::Error::INVALID_SIZE;
    }

    memcpy(aDest, res.data(), aDestLen = res.size());
    return Crypto::Error::NO_ERROR;
}

void Client::SaveSecrets()
{
    FILE* fout = fopen("cryptoclient.cfg", "wb");
    if (fout)
    {
        int sssize = mSharedSecret.size();
        fwrite(&sssize, sizeof(int), 1, fout);
        fwrite(mSharedSecret.data(), sizeof(char), mSharedSecret.size(), fout);

        sssize = mNonce.size();
        fwrite(&sssize, sizeof(int), 1, fout);
        fwrite(mNonce.data(), sizeof(char), mNonce.size(), fout);
        fwrite(&mNonceCount, sizeof(int), 1, fout);

        fclose(fout);
    }
}

void Client::RestoreSecrets()
{
    FILE* fin = fopen("cryptoclient.cfg", "rb");
    if (fin)
    {
        int sssize;
        char intmp[0xff];
        fread(&sssize, sizeof(int), 1, fin);
        fread(intmp, sizeof(char), sssize, fin);
        mSharedSecret = ByteArr(intmp, intmp + sssize);

        fread(&sssize, sizeof(int), 1, fin);
        fread(intmp, sizeof(char), sssize, fin);
        mNonce = ByteArr(intmp, intmp + sssize);

        fread(&mNonceCount, sizeof(int), 1, fin);

        fclose(fin);
    }
}

// ---------------------------------------------------------------------------------
// SERVER Class
//----------------------------------------------------------------------------------

Server::Server(char *aClientPublicKey, int aLength, const unsigned char *aPirvateSignature) :
    mNonceCount(0)

{
    (void)sodium_init();

    //  genearate private/public key pair
    ByteArr local_pub_key(crypto_box_PUBLICKEYBYTES);
    ByteArr local_priv_key(crypto_box_SECRETKEYBYTES);

    crypto_box_keypair(local_pub_key.data(), local_priv_key.data());

    Crypto::Debug::DumpArray("[S] local priv", local_priv_key);
    Crypto::Debug::DumpArray("[S] local pub", local_pub_key);

    //  generate nonce start
    auto nonce_start = ByteArr(Crypto::Size::Nonce);
    randombytes_buf(nonce_start.data(), nonce_start.size());

    //  prepare public payload
    auto sig = ByteArr(crypto_sign_BYTES + crypto_box_PUBLICKEYBYTES);
    crypto_sign(sig.data(), nullptr, local_pub_key.data(), local_pub_key.size(), (const unsigned char*) aPirvateSignature);

    mPublicPayload = local_pub_key;
    mPublicPayload.insert(std::end(mPublicPayload), std::begin(sig), std::begin(sig)+crypto_sign_BYTES);
    mPublicPayload.insert(std::end(mPublicPayload), std::begin(nonce_start), std::end(nonce_start));
    mNonce = nonce_start;

    Crypto::Debug::DumpArray("[S] payload", mPublicPayload);

    //  generate true shared key
    mSharedSecret.resize(crypto_box_BEFORENMBYTES);
    ByteArr public_key(aClientPublicKey, aClientPublicKey + aLength);
    (void)crypto_box_beforenm(mSharedSecret.data(), public_key.data(), local_priv_key.data());
    Crypto::Debug::DumpArray("[S] shared key", mSharedSecret);

    sodium_memzero(local_priv_key.data(), local_priv_key.size());
}

///
/// \brief GetPublicPayload
/// \param aPayload - [out] buffer to copy
/// \param aLength [in/out]: [in] max aPayload [out] lenght of the returned aPayload
/// \return pointer to aPayload
///
char* Server::GetPublicPayload(char *aPayload, int &aLength)
{
    if (!aPayload || !aLength || (aLength < (int) mPublicPayload.size()))
    {
        return NULL;
    }

    memcpy(aPayload, mPublicPayload.data(), aLength = mPublicPayload.size());
    return aPayload;
}

///
/// \brief Encrypt
/// \param aSrc [in] buffer to encrypt
/// \param aSrcLen [in] lenght of aSrc
/// \param aDest [out] buffer to copy encrypted output
/// \param aDestLen [in/out]: [in] max aDest [out] lenght of the encrypted output
/// \return error
///
int Server::Encrypt(char *aSrc, int aSrcLen, char *aDest, int &aDestLen)
{
    if (!aSrc || !aSrcLen || !aDest || !aDestLen)
        return Crypto::Error::INVALID_ARGUMENT;

    ++mNonceCount;
    ByteArr nonce = mNonce;
    nonce.insert(end(nonce), (uint8_t*)(&mNonceCount), (uint8_t*)((&mNonceCount)+1));
    Crypto::Debug::DumpArray("[C] nonce", nonce);

    ByteArr tmp_data(aSrc, aSrc + aSrcLen);
    tmp_data.insert(begin(tmp_data), Crypto::Size::DataHeader, uint8_t{0});

    ByteArr res(tmp_data.size());
    crypto_box_afternm(res.data(), tmp_data.data(), tmp_data.size(), nonce.data(), mSharedSecret.data());

    auto restored_res = ByteArr{};
    restored_res.insert(end(restored_res), begin(res)+crypto_box_BOXZEROBYTES, end(res));
    res = restored_res;
    res.insert(begin(res), (uint8_t*)(&mNonceCount), (uint8_t*)((&mNonceCount)+1));

    if (aDestLen < (int) res.size())
    {
        Crypto::Debug::DumpArray("[C] aDestLen < res.size", res);
        return Crypto::Error::INVALID_SIZE;
    }

    memcpy(aDest, res.data(), aDestLen = res.size());
    return Crypto::Error::NO_ERROR;
}

///
/// \brief Decrypt
/// \param aSrc [in] buffer to decrypt
/// \param aSrcLen [in] lenght of aSrc
/// \param aDest [out] buffer to copy decrypted output
/// \param aDestLen [in/out]: [in] max aDest [out] lenght of the decrypted output
/// \return pointer to aDest
///
int Server::Decrypt(char *aSrc, int aSrcLen, char *aDest, int &aDestLen)
{
    if (!aSrc || !aSrcLen || !aDest || !aDestLen)
        return Crypto::Error::INVALID_ARGUMENT;

    ByteArr data(aSrc, aSrc + aSrcLen);
    auto nonce = mNonce;
    nonce.insert(end(nonce), begin(data), begin(data) + 4);

    auto prep_data = ByteArr{};
    prep_data.insert(begin(prep_data), crypto_box_BOXZEROBYTES, uint8_t{0});
    prep_data.insert(end(prep_data), begin(data)+4, end(data));

    auto res = ByteArr(data.size()-4 + crypto_box_ZEROBYTES);
    (void)crypto_box_open_afternm(res.data(), prep_data.data(), prep_data.size(), nonce.data(), mSharedSecret.data());

    auto restored_res = ByteArr{};
    restored_res.insert(end(restored_res), begin(res)+crypto_box_ZEROBYTES, end(res) - crypto_box_BOXZEROBYTES);
    res = restored_res;

    if (aDestLen < (int) res.size())
    {
        Crypto::Debug::DumpArray("[S] aDestLen < res.size", res);
        return Crypto::Error::INVALID_SIZE;
    }

    memcpy(aDest, res.data(), aDestLen = res.size());
    return Crypto::Error::NO_ERROR;

}

bool Debug::bSuppress = false;
void Debug::DumpArray(const char* name, const ByteArr& arr)
{
    if (!bSuppress)
    {
        printf("%s (%zu bytes):\n", name, arr.size());
        int i = 0;
        for (auto c: arr)
        {
            printf("%02x%c", uint8_t(c), (++i % 16)?' ':'\n');
        }
        printf("\n");
    }
}
