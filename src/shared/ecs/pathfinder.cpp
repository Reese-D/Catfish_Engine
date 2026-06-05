#include "pathfinder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

namespace Systems {

Pathfinder::Pathfinder(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize) : m_cellSize(cellSize), m_worldMin(worldMin) {
    m_dims = {
        static_cast<int>(std::ceil((worldMax.x - worldMin.x) / cellSize)),
        static_cast<int>(std::ceil((worldMax.y - worldMin.y) / cellSize)),
    };
    m_blocked.assign(static_cast<std::size_t>(m_dims.x * m_dims.y), false);
}

glm::ivec2 Pathfinder::toCell(glm::vec2 worldPos) const {
    return {
        static_cast<int>((worldPos.x - m_worldMin.x) / m_cellSize),
        static_cast<int>((worldPos.y - m_worldMin.y) / m_cellSize),
    };
}

glm::vec2 Pathfinder::toWorld(glm::ivec2 cell) const {
    return {
        m_worldMin.x + (static_cast<float>(cell.x) + 0.5f) * m_cellSize,
        m_worldMin.y + (static_cast<float>(cell.y) + 0.5f) * m_cellSize,
    };
}

bool Pathfinder::inBounds(glm::ivec2 cell) const { return cell.x >= 0 && cell.x < m_dims.x && cell.y >= 0 && cell.y < m_dims.y; }

bool Pathfinder::isWalkable(glm::ivec2 cell) const { return inBounds(cell) && !m_blocked[cellIndex(cell)]; }

std::size_t Pathfinder::cellIndex(glm::ivec2 cell) const { return static_cast<std::size_t>(cell.y * m_dims.x + cell.x); }

void Pathfinder::setBlocked(glm::vec2 worldPos, bool blocked) {
    auto cell = toCell(worldPos);
    if (inBounds(cell))
        m_blocked[cellIndex(cell)] = blocked;
}

namespace {

float octile(glm::ivec2 a, glm::ivec2 b) {
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    int mn = std::min(dx, dy);
    return static_cast<float>(dx + dy) + (1.41421356f - 2.0f) * static_cast<float>(mn);
}

} // namespace

std::vector<glm::vec3> Pathfinder::findPath(glm::vec2 start, glm::vec2 end) const {
    auto startCell = toCell(start);
    auto endCell = toCell(end);

    if (!inBounds(startCell) || !inBounds(endCell))
        return {};
    if (startCell == endCell)
        return {{end.x, end.y, 0.0f}};

    const std::size_t N = static_cast<std::size_t>(m_dims.x * m_dims.y);
    const std::size_t startIdx = cellIndex(startCell);
    const std::size_t endIdx = cellIndex(endCell);
    const std::size_t NONE = std::numeric_limits<std::size_t>::max();

    std::vector<float> gScore(N, std::numeric_limits<float>::infinity());
    std::vector<std::size_t> cameFrom(N, NONE);
    std::vector<bool> closed(N, false);

    gScore[startIdx] = 0.0f;

    using Entry = std::pair<float, std::size_t>; // (f, idx)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> openSet;
    openSet.push({octile(startCell, endCell), startIdx});

    constexpr int DX[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    constexpr int DY[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    constexpr float COST[8] = {1.41421356f, 1.0f, 1.41421356f, 1.0f, 1.0f, 1.41421356f, 1.0f, 1.41421356f};

    while (!openSet.empty()) {
        auto [f, currIdx] = openSet.top();
        openSet.pop();

        if (closed[currIdx])
            continue;
        closed[currIdx] = true;

        if (currIdx == endIdx) {
            std::vector<glm::vec3> path;
            std::size_t idx = endIdx;
            while (idx != startIdx) {
                int x = static_cast<int>(idx) % m_dims.x;
                int y = static_cast<int>(idx) / m_dims.x;
                auto w = toWorld({x, y});
                path.push_back({w.x, w.y, 0.0f});
                idx = cameFrom[idx];
                if (idx == NONE)
                    break;
            }
            if (!path.empty())
                path.front() = {end.x, end.y, 0.0f};
            std::reverse(path.begin(), path.end());
            return smooth(path);
        }

        int cx = static_cast<int>(currIdx) % m_dims.x;
        int cy = static_cast<int>(currIdx) / m_dims.x;

        for (int i = 0; i < 8; ++i) {
            glm::ivec2 nb{cx + DX[i], cy + DY[i]};
            if (!isWalkable(nb))
                continue;

            // Don't cut through diagonal gaps
            if (DX[i] != 0 && DY[i] != 0) {
                if (!isWalkable({cx + DX[i], cy}) || !isWalkable({cx, cy + DY[i]}))
                    continue;
            }

            std::size_t nbIdx = cellIndex(nb);
            float tentativeG = gScore[currIdx] + COST[i];
            if (tentativeG < gScore[nbIdx]) {
                gScore[nbIdx] = tentativeG;
                cameFrom[nbIdx] = currIdx;
                openSet.push({tentativeG + octile(nb, endCell), nbIdx});
            }
        }
    }

    return {};
}

bool Pathfinder::hasLineOfSight(glm::vec2 a, glm::vec2 b) const {
    auto ca = toCell(a);
    auto cb = toCell(b);

    int x = ca.x, y = ca.y;
    int dx = std::abs(cb.x - ca.x);
    int dy = std::abs(cb.y - ca.y);
    int sx = (cb.x > ca.x) ? 1 : -1;
    int sy = (cb.y > ca.y) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (!isWalkable({x, y}))
            return false;
        if (x == cb.x && y == cb.y)
            break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
    return true;
}

std::vector<glm::vec3> Pathfinder::smooth(const std::vector<glm::vec3> &raw) const {
    if (raw.size() <= 2)
        return raw;

    std::vector<glm::vec3> result;
    result.push_back(raw.front());

    std::size_t i = 0;
    while (i < raw.size() - 1) {
        std::size_t j = raw.size() - 1;
        while (j > i + 1 && !hasLineOfSight({raw[i].x, raw[i].y}, {raw[j].x, raw[j].y}))
            --j;
        result.push_back(raw[j]);
        i = j;
    }
    return result;
}

} // namespace Systems
