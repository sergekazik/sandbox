#ifndef _SODIUM_GLUE_DEFINE_H_
#define _SODIUM_GLUE_DEFINE_H_

#include <utility>
#include <vector>
#include <cstdint>
#include <functional>
#include <utility>

namespace Ring {

using ByteArr = std::vector<uint8_t>;
std::pair<ByteArr, ByteArr> GenSignPK();

class SodiumGlue
{
public:
    /// "Send data to peer" callback
    ///  - Used by handshake initiator to send public key
    ///  - Used by server to send public payload
    using SendHandler = std::function<void(const ByteArr&)>;

    /// "Wait for data from peer" callback
    ///  - Used by initiator to get public payload
    using ReceiveHandler = std::function<ByteArr()>;

    explicit SodiumGlue(SendHandler sh, ReceiveHandler rh = nullptr)
        : m_sendHandler(std::move(sh)), m_receiveHandler(std::move(rh))
    {
    }
    SodiumGlue() {doHandshake();}
    SodiumGlue(const ByteArr &pub_key) {doHandshake(pub_key);}

    /// Perform encryption handshake
    ///
    /// \note This is a blocking call
    ///
    /// \return
    bool doHandshake();
    bool doHandshake(const ByteArr &pub_key);
    bool processPayload(ByteArr &payload);

    ByteArr encrypt(const ByteArr& data);
    ByteArr decrypt(const ByteArr &data);

    static ByteArr convert_to_array(const char* in, bool hex);
    static char* deconvert_from_array(ByteArr &in);

    ByteArr m_local_public_key;

private:
    SendHandler m_sendHandler = NULL;
    ReceiveHandler m_receiveHandler = NULL;
    ByteArr m_sharedSecret;
    ByteArr m_nonceStart;
    ByteArr m_local_priv_key;
    int m_nonceCounter = 0;

};}

#endif //_SODIUM_GLUE_DEFINE_H_
