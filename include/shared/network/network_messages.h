#ifndef NETWORK_MESSAGES_H
#define NETWORK_MESSAGES_H

#include <cstdint>
#include <cstring>
#include <vector>

// All multi-byte fields are in native byte order (both peers are assumed to be
// the same architecture for now; add bswap if cross-platform is needed later).

// Bump whenever the packet layout changes in a breaking way.
constexpr uint16_t kProtocolVersion = 5;

enum class MessageType : uint8_t {
    Snapshot = 0x01,           // server → client, unreliable
    Input = 0x02,              // client → server, unreliable
    PlayerAssignment = 0x03,   // server → client, reliable, sent once on connect
    Disconnect = 0x04,         // server → client, reliable
    ConnectionRejected = 0x05, // server → client, reliable
    Hello = 0x06,              // client → server, reliable, sent once on connect
};

enum class DisconnectReason : uint8_t {
    ServerShuttingDown = 0x01,
    Kicked = 0x02,
    GameOver = 0x03,
};

enum class RejectionReason : uint8_t {
    ServerFull = 0x01,
    VersionMismatch = 0x02,
};

// ---- Per-entity / per-projectile data inside a Snapshot --------------------

#pragma pack(push, 1)

struct EntitySnapshot {
    uint32_t netId;
    uint8_t faction; // 0 = Player, 1 = Enemy
    float x, y, z;
    float health;
    float maxHealth;
};

struct ProjectileSnapshot {
    uint32_t netId;
    uint8_t faction;  // owner faction
    uint8_t type;     // 0=fireball, 1=gravityWell, 2=lightning
    float x, y, z;
    float vx, vy, vz;
    float yaw;        // Z-axis rotation (radians); used by lightning bolt mesh
};

struct RockSnapshot {
    uint32_t netId;
    float x, y, z;
};

// Fixed-size header; followed by entityCount EntitySnapshots then
// projectileCount ProjectileSnapshots in the raw packet buffer.
struct SnapshotHeader {
    uint8_t msgType; // MessageType::Snapshot
    uint32_t tick;
    float lavaRadius;
    uint8_t entityCount;
    uint8_t projectileCount;
    uint8_t rockCount;
};

namespace InputFlags {
constexpr uint8_t kMoveOrder = 0x01;
constexpr uint8_t kFireAbility = 0x02;
} // namespace InputFlags

struct InputPacket {
    uint8_t msgType; // MessageType::Input
    uint32_t tick;
    uint8_t flags;       // InputFlags bitmask
    uint8_t abilitySlot; // which AbilitySet slot to fire (when FireAbility set)
    float moveX, moveY, moveZ;
    float abilityX, abilityY, abilityZ;
};

struct PlayerAssignmentPacket {
    uint8_t msgType;        // MessageType::PlayerAssignment
    uint32_t yourNetworkId; // the NetworkId of the entity the client controls
};

struct HelloPacket {
    uint8_t msgType;          // MessageType::Hello
    uint16_t protocolVersion; // must equal PROTOCOL_VERSION
};

struct DisconnectPacket {
    uint8_t msgType; // MessageType::Disconnect
    uint8_t reason;  // DisconnectReason
};

struct ConnectionRejectedPacket {
    uint8_t msgType; // MessageType::ConnectionRejected
    uint8_t reason;  // RejectionReason
};

#pragma pack(pop)

// ---- Simple byte-buffer helpers --------------------------------------------

class BufWriter {
  public:
    template <typename T> void write(const T &v) {
        const auto *p = reinterpret_cast<const uint8_t *>(&v);
        m_buf.insert(m_buf.end(), p, p + sizeof(T));
    }
    const std::vector<uint8_t> &buf() const { return m_buf; }

  private:
    std::vector<uint8_t> m_buf;
};

class BufReader {
  public:
    BufReader(const uint8_t *data, std::size_t size) : m_data(data), m_size(size) {}

    template <typename T> bool read(T &v) {
        if (m_pos + sizeof(T) > m_size)
            return false;
        std::memcpy(&v, m_data + m_pos, sizeof(T));
        m_pos += sizeof(T);
        return true;
    }

    bool ok() const { return m_pos <= m_size; }
    bool done() const { return m_pos == m_size; }

  private:
    const uint8_t *m_data;
    std::size_t m_size;
    std::size_t m_pos{0};
};

#endif // NETWORK_MESSAGES_H
