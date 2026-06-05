#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <enet/enet.h>

namespace Network {

// Channels
constexpr enet_uint8 kChanUnreliable = 0;
constexpr enet_uint8 kChanReliable = 1;
constexpr std::size_t kNumChannels = 2;

class NetworkManager {
  public:
    NetworkManager();
    ~NetworkManager();

    NetworkManager(const NetworkManager &) = delete;
    NetworkManager &operator=(const NetworkManager &) = delete;

    // Server: listen on port, accept up to maxPeers clients.
    bool startServer(uint16_t port, std::size_t maxPeers = 4);

    // Client: connect to host:port. Returns false if the address can't be resolved.
    // The actual connection is confirmed asynchronously via the first poll() call.
    bool connectToServer(const std::string &host, uint16_t port);

    // Process all pending ENet events.
    // onPacket  — called once per received packet.
    // onConnect — called when a new peer connects (server: new client; client: server ack).
    using PacketCallback = std::function<void(const uint8_t *, std::size_t, ENetPeer *)>;
    using ConnectCallback = std::function<void(ENetPeer *)>;
    using DisconnectCallback = std::function<void(ENetPeer *)>;
    void poll(const PacketCallback &onPacket, const ConnectCallback &onConnect = nullptr, const DisconnectCallback &onDisconnect = nullptr);

    // Server helpers
    void broadcastUnreliable(const std::vector<uint8_t> &data);
    void broadcastReliable(const std::vector<uint8_t> &data);
    void sendReliableTo(ENetPeer *peer, const std::vector<uint8_t> &data);

    // Client helpers
    void sendToServerUnreliable(const std::vector<uint8_t> &data);
    void sendToServerReliable(const std::vector<uint8_t> &data);

    bool isServer() const { return m_role == Role::Server; }
    bool isClient() const { return m_role == Role::Client; }
    bool isConnected() const { return m_serverConnected; }
    int peerCount() const { return static_cast<int>(m_peers.size()); }

  private:
    void sendPacket(ENetPeer *peer, const std::vector<uint8_t> &data, enet_uint8 channel, bool reliable);

    enum class Role { None, Server, Client } m_role{Role::None};

    ENetHost *m_host{nullptr};
    ENetPeer *m_serverPeer{nullptr}; // client's connection to the server
    std::vector<ENetPeer *> m_peers; // server's connected clients
    bool m_serverConnected{false};
};

} // namespace Network

#endif // NETWORK_MANAGER_H
