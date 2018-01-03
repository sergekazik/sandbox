#ifndef PROJECT_ENCRYPTEDHANDSHAKE_HPP
#define PROJECT_ENCRYPTEDHANDSHAKE_HPP

#include <utility>
#include <vector>
#include <cstdint>
#include <functional>
#include <utility>

namespace Ring {

using ByteArr = std::vector<uint8_t>;
std::pair<ByteArr, ByteArr> GenSignPK();

class EncHandshake
{
public:
    /// "Send data to peer" callback
    ///  - Used by handshake initiator to send public key
    ///  - Used by server to send public payload
    using SendHandler = std::function<void(const ByteArr&)>;

    /// "Wait for data from peer" callback
    ///  - Used by initiator to get public payload
    using ReceiveHandler = std::function<ByteArr()>;

    explicit EncHandshake(SendHandler sh, ReceiveHandler rh = nullptr)
        : m_sendHandler(std::move(sh)), m_receiveHandler(std::move(rh))
    {
    }

    /// Perform encryption handshake
    ///
    /// \note This is a blocking call
    ///
    /// \return
    bool doHandshake();
    bool doHandshake(const ByteArr &pub_key);

    bool valid();

    ByteArr encrypt(const ByteArr& data);
    ByteArr decrypt(const ByteArr &data);

private:
    SendHandler m_sendHandler;
    ReceiveHandler m_receiveHandler;
    ByteArr m_sharedSecret;
    ByteArr m_nonceStart;
    int m_nonceCounter = 0;
};

}

#endif //PROJECT_ENCRYPTEDHANDSHAKE_HPP
