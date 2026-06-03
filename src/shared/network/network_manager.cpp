#include "network_manager.h"

#include <iostream>
#include <stdexcept>

namespace Network {

NetworkManager::NetworkManager() {
    if (enet_initialize() != 0)
        throw std::runtime_error("Failed to initialise ENet");
}

NetworkManager::~NetworkManager() {
    if (host_) {
        if (isClient() && serverPeer_)
            enet_peer_disconnect_now(serverPeer_, 0);
        enet_host_destroy(host_);
    }
    enet_deinitialize();
}

bool NetworkManager::startServer(uint16_t port, std::size_t maxPeers) {
    role_ = Role::Server;

    ENetAddress addr{};
    addr.host = ENET_HOST_ANY;
    addr.port = port;

    host_ = enet_host_create(&addr, maxPeers, NUM_CHANNELS, 0, 0);
    if (!host_) {
        std::cerr << "[Net] Failed to create server host on port " << port << "\n";
        return false;
    }
    std::cout << "[Net] Server listening on port " << port << "\n";
    serverConnected_ = true; // server is always "connected" to itself
    return true;
}

bool NetworkManager::connectToServer(const std::string &host, uint16_t port) {
    role_ = Role::Client;

    host_ = enet_host_create(nullptr, 1, NUM_CHANNELS, 0, 0);
    if (!host_) {
        std::cerr << "[Net] Failed to create client host\n";
        return false;
    }

    ENetAddress addr{};
    if (enet_address_set_host(&addr, host.c_str()) != 0) {
        std::cerr << "[Net] Could not resolve host: " << host << "\n";
        return false;
    }
    addr.port = port;

    serverPeer_ = enet_host_connect(host_, &addr, NUM_CHANNELS, 0);
    if (!serverPeer_) {
        std::cerr << "[Net] No available peers for connection\n";
        return false;
    }
    std::cout << "[Net] Connecting to " << host << ":" << port << "...\n";
    return true;
}

void NetworkManager::poll(const PacketCallback &onPacket, const ConnectCallback &onConnect, const DisconnectCallback &onDisconnect) {
    if (!host_)
        return;

    ENetEvent event;
    while (enet_host_service(host_, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            if (isServer()) {
                peers_.push_back(event.peer);
                std::cout << "[Net] Client connected (" << peers_.size() << " total)\n";
            } else {
                serverConnected_ = true;
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
                auto it = std::find(peers_.begin(), peers_.end(), event.peer);
                if (it != peers_.end())
                    peers_.erase(it);
                std::cout << "[Net] Client disconnected (" << peers_.size() << " remaining)\n";
            } else {
                serverConnected_ = false;
                serverPeer_ = nullptr;
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
    for (auto *peer : peers_)
        sendPacket(peer, data, CHAN_UNRELIABLE, false);
}

void NetworkManager::broadcastReliable(const std::vector<uint8_t> &data) {
    for (auto *peer : peers_)
        sendPacket(peer, data, CHAN_RELIABLE, true);
}

void NetworkManager::sendReliableTo(ENetPeer *peer, const std::vector<uint8_t> &data) { sendPacket(peer, data, CHAN_RELIABLE, true); }

void NetworkManager::sendToServerUnreliable(const std::vector<uint8_t> &data) {
    if (serverPeer_)
        sendPacket(serverPeer_, data, CHAN_UNRELIABLE, false);
}

void NetworkManager::sendToServerReliable(const std::vector<uint8_t> &data) {
    if (serverPeer_)
        sendPacket(serverPeer_, data, CHAN_RELIABLE, true);
}

} // namespace Network
