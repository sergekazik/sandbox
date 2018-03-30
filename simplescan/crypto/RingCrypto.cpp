#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sodium.h>
#include "RingCrypto.hh"

using namespace Ring;
using namespace Ring::Ble;
using namespace Ring::Ble::Crypto;

static int _encrypt(Ring::SodiumGlue *sgl, char *aSrc, int aSrcLen, char *aDest, int &aDestLen);
static int _decrypt(Ring::SodiumGlue *sgl, char *aSrc, int aSrcLen, char *aDest, int &aDestLen);

Client::Client()
{
    sgl = new SodiumGlue();
}

///
/// \brief GetPublicKey
/// \param aKey - [out] buffer to copy
/// \param aLength [in/out]: [in] max aKey [out] lenght of the returned public key
/// \return pointer to aKey
///
char* Client::GetPublicKey(char *aKey, int &aLength)
{
    if (!sgl || !aKey || !aLength || (aLength < (int) sgl->m_local_public_key.size()))
        return NULL;

    memcpy(aKey, sgl->m_local_public_key.data(), aLength = sgl->m_local_public_key.size());
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
    if (!sgl || !aPayload || !aLength)
        return Crypto::Error::INVALID_ARGUMENT;
    auto payload = ByteArr(aPayload, aPayload + aLength);
    sgl->processPayload(payload);

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
    return _encrypt(this->sgl, aSrc, aSrcLen, aDest, aDestLen);
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
    return _decrypt(this->sgl, aSrc, aSrcLen, aDest, aDestLen);
}

// ---------------------------------------------------------------------------------
// SERVER Class
//----------------------------------------------------------------------------------

Server::Server(char *aClientPublicKey, int aLength)
{
    if (aClientPublicKey && aLength)
    {
        auto payload = ByteArr(aClientPublicKey, aClientPublicKey + aLength);
        sgl = new Ring::SodiumGlue(payload);
    }
}

///
/// \brief GetPublicPayload
/// \param aPayload - [out] buffer to copy
/// \param aLength [in/out]: [in] max aPayload [out] lenght of the returned aPayload
/// \return pointer to aPayload
///
char* Server::GetPublicPayload(char *aPayload, int &aLength)
{
    if (!sgl || !aPayload || !aLength || (aLength < (int) sgl->m_local_public_key.size()))
        return NULL;

    memcpy(aPayload, sgl->m_local_public_key.data(), aLength = sgl->m_local_public_key.size());
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
    return _encrypt(this->sgl, aSrc, aSrcLen, aDest, aDestLen);
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
    return _decrypt(this->sgl, aSrc, aSrcLen, aDest, aDestLen);
}

///
/// \brief Debug::DumpArray
/// \param name
/// \param arr
///

bool Debug::bSuppress = false;
void Debug::DumpArray(const char* name, const ByteArr& arr)
{
    for (int i = 0; !bSuppress;)
    {
        printf("%s (%zu bytes):\n", name, arr.size());
        for (auto c: arr) printf("%02x%c", uint8_t(c), (++i % 32)?' ':'\n');
        printf("\n");break;
    }
}

///
/// \brief _encrypt
/// \param sgl
/// \param aSrc
/// \param aSrcLen
/// \param aDest
/// \param aDestLen
/// \return
///
static int _encrypt(Ring::SodiumGlue *sgl, char *aSrc, int aSrcLen, char *aDest, int &aDestLen)
{
    if (!aSrc || !aSrcLen || !aDest || !aDestLen)
    return Crypto::Error::INVALID_ARGUMENT;

    auto payload = ByteArr(aSrc, aSrc + aSrcLen);
    auto res = sgl->encrypt(payload);

    if (aDestLen < (int) res.size())
    {
        Crypto::Debug::DumpArray("[C] aDestLen < res.size", res);
        return Crypto::Error::INVALID_SIZE;
    }

    memcpy(aDest, res.data(), aDestLen = res.size());
    return Crypto::Error::NO_ERROR;
}

///
////// \brief _decrypt
////// \param sgl
////// \param aSrc
////// \param aSrcLen
////// \param aDest
////// \param aDestLen
////// \return
///
static int _decrypt(Ring::SodiumGlue *sgl, char *aSrc, int aSrcLen, char *aDest, int &aDestLen)
{
    if (!aSrc || !aSrcLen || !aDest || !aDestLen)
        return Crypto::Error::INVALID_ARGUMENT;

    auto payload = ByteArr(aSrc, aSrc + aSrcLen);
    auto res = sgl->decrypt(payload);

    if (aDestLen < (int) res.size())
    {
        Crypto::Debug::DumpArray("[S] aDestLen < res.size", res);
        return Crypto::Error::INVALID_SIZE;
    }

    memcpy(aDest, res.data(), aDestLen = res.size());
    return Crypto::Error::NO_ERROR;
}

