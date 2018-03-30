#ifndef _RING_BLE_CRYPTO_H_
#define _RING_BLE_CRYPTO_H_

#include <utility>
#include <vector>
#include <cstdint>
#include <functional>

#include "sodium_glue.h"

namespace Ring { namespace Ble { namespace Crypto {
using ByteArr = std::vector<uint8_t>;

namespace Size { enum {
    PublicKey = 32u,
    Signature = 64u,
    Nonce = 20u,
    DataHeader = 32u,
};}

namespace Error { enum {
    NO_ERROR            =  0,
    INVALID_SIZE        = -1,
    INVALID_SIGNATURE   = -2,
    INVALID_ARGUMENT    = -3
};}

class Client
{
public:

    Client();

    ///
    /// \brief GetPublicKey
    /// \param aKey - [out] buffer to copy
    /// \param aLength [in/out]: [in] max aKey [out] lenght of the returned public key
    /// \return pointer to aKey
    ///
    char *GetPublicKey(char *aKey, int &aLength);

    ///
    /// \brief ProcessPublicPayload
    /// \param aPayload
    /// \param aLength [in] is length of aPayload to process
    /// \return error code
    ///
    int ProcessPublicPayload(char *aPayload, int aLength);

    ///
    /// \brief Encrypt
    /// \param aSrc [in] buffer to encrypt
    /// \param aSrcLen [in] lenght of aSrc
    /// \param aDest [out] buffer to copy encrypted output
    /// \param aDestLen [in/out]: [in] max aDest [out] lenght of the encrypted output
    /// \return error code
    ///
    int Encrypt(char *aSrc, int aSrcLen, char *aDest, int &aDestLen);

    ///
    /// \brief Decrypt
    /// \param aSrc [in] buffer to decrypt
    /// \param aSrcLen [in] lenght of aSrc
    /// \param aDest [out] buffer to copy decrypted output
    /// \param aDestLen [in/out]: [in] max aDest [out] lenght of the decrypted output
    /// \return error code
    ///
    int Decrypt(char *aSrc, int aSrcLen, char *aDest, int &aDestLen);

private:
    Ring::SodiumGlue *sgl = NULL;

#ifdef __x86_64__
public:
    void SaveSecrets();
    void RestoreSecrets();
#endif

};

class Server
{
public:

    Server(char *aClientPublicKey, int aLength);

    ///
    /// \brief GetPublicPayload
    /// \param aPayload - [out] buffer to copy
    /// \param aLength [in/out]: [in] max aPayload [out] lenght of the returned aPayload
    /// \return pointer to aPayload
    ///
    char *GetPublicPayload(char *aPayload, int &aLength);

    ///
    /// \brief Encrypt
    /// \param aSrc [in] buffer to encrypt
    /// \param aSrcLen [in] lenght of aSrc
    /// \param aDest [out] buffer to copy encrypted output
    /// \param aDestLen [in/out]: [in] max aDest [out] lenght of the encrypted output
    /// \return error code
    ///
    int Encrypt(char *aSrc, int aSrcLen, char *aDest, int &aDestLen);

    ///
    /// \brief Decrypt
    /// \param aSrc [in] buffer to decrypt
    /// \param aSrcLen [in] lenght of aSrc
    /// \param aDest [out] buffer to copy decrypted output
    /// \param aDestLen [in/out]: [in] max aDest [out] lenght of the decrypted output
    /// \return error code
    ///
    int Decrypt(char *aSrc, int aSrcLen, char *aDest, int &aDestLen);

private:
    Ring::SodiumGlue *sgl = NULL;
};

class Debug
{
public:
    static void Suppress(bool aSup) { bSuppress = aSup; }
    static void DumpArray(const char* name, const ByteArr& arr);
private:
    static bool bSuppress;
};

}}} /* namespace Ring::Ble::Crypto */


#endif // _RING_BLE_CRYPTO_H_
