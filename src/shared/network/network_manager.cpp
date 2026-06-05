#include "network_manager.h"

#include <iostream>
#include <stdexcept>

namespace Network {

NetworkManager::NetworkManager() {
    if (enet_initialize() != 0)
        throw std::runtime_error("Failed to initialise ENet");
}

NetworkManager::~NetworkManager() {
    if (m_host) {
        if (isClient() && m_serverPeer)
            enet_peer_disconnect_now(m_serverPeer, 0);
        enet_host_destroy(m_host);
    }
    enet_deinitialize();
}

bool NetworkManager::startServer(uint16_t port, std::size_t maxPeers) {
    m_role = Role::Server;

    ENetAddress addr{};
    addr.host = ENET_HOST_ANY;
    addr.port = port;

    m_host = enet_host_create(&addr, maxPeers, kNumChannels, 0, 0);
    if (!m_host) {
        std::cerr << "[Net] Failed to create server host on port " << port << "\n";
        return false;
    }
    std::cout << "[Net] Server listening on port " << port << "\n";
    m_serverConnected = true; // server is always "connected" to itself
    return true;
}

bool NetworkManager::connectToServer(const std::string &host, uint16_t port) {
    m_role = Role::Client;

    m_host = enet_host_create(nullptr, 1, kNumChannels, 0, 0);
    if (!m_host) {
        std::cerr << "[Net] Failed to create client host\n";
        return false;
    }

    ENetAddress addr{};
    if (enet_address_set_host(&addr, host.c_str()) != 0) {
        std::cerr << "[Net] Could not resolve host: " << host << "\n";
        return false;
    }
    addr.port = port;

    m_serverPeer = enet_host_connect(m_host, &addr, kNumChannels, 0);
    if (!m_serverPeer) {
        std::cerr << "[Net] No available peers for connection\n";
        return false;
    }
    std::cout << "[Net] Connecting to " << host << ":" << port << "...\n";
    return true;
}

void NetworkManager::poll(const PacketCallback &onPacket, const ConnectCallback &onConnect, const DisconnectCallback &onDisconnect) {
    if (!m_host)
        return;

    ENetEvent event;
    while (enet_host_service(m_host, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            if (isServer()) {
                m_peers.push_back(event.peer);
                std::cout << "[Net] Client connected (" << m_peers.size() << " total)\n";
            } else {
                m_serverConnected = true;
                std::cout << "[Net] Connected to server\n";
            }
            if (onConnect)
                onConnect(event.peer);
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            onPacket(reinterpret_cast<const uint8_t *>(event.packet->data), event.packet->dataLength, event.peer);
            enet_packet_destroy(event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            if (isServer()) {
                auto it = std::find(m_peers.begin(), m_peers.end(), event.peer);
                if (it != m_peers.end())
                    m_peers.erase(it);
                std::cout << "[Net] Client disconnected (" << m_peers.size() << " remaining)\n";
            } else {
                m_serverConnected = false;
                m_serverPeer = nullptr;
                std::cout << "[Net] Disconnected from server\n";
            }
            if (onDisconnect)
                onDisconnect(event.peer);
            break;

        default:
            break;
        }
    }
}

void NetworkManager::sendPacket(ENetPeer *peer, const std::vector<uint8_t> &data, enet_uint8 channel, bool reliable) {
    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
    ENetPacket *pkt = enet_packet_create(data.data(), data.size(), flags);
    enet_peer_send(peer, channel, pkt);
}

void NetworkManager::broadcastUnreliable(const std::vector<uint8_t> &data) {
    for (auto *peer : m_peers)
        sendPacket(peer, data, kChanUnreliable, false);
}

void NetworkManager::broadcastReliable(const std::vector<uint8_t> &data) {
    for (auto *peer : m_peers)
        sendPacket(peer, data, kChanReliable, true);
}

void NetworkManager::sendReliableTo(ENetPeer *peer, const std::vector<uint8_t> &data) { sendPacket(peer, data, kChanReliable, true); }

void NetworkManager::sendToServerUnreliable(const std::vector<uint8_t> &data) {
    if (m_serverPeer)
        sendPacket(m_serverPeer, data, kChanUnreliable, false);
}

void NetworkManager::sendToServerReliable(const std::vector<uint8_t> &data) {
    if (m_serverPeer)
        sendPacket(m_serverPeer, data, kChanReliable, true);
}

} // namespace Network
