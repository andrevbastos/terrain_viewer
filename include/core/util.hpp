#pragma once

#include <array>
#include <cmath>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ifcg/ifcg.hpp>
#include <ifcg/graphics/mesh.hpp>

#include "core/noise_gen.hpp"
#include "transvoxel/transvoxel.hpp"

using namespace ifcg;

struct Color {
    float r, g, b, a;

    Color operator*(float f) const {
        return {r * f, g * f, b * f, a};
    }
};

struct VertexPositionHash {
    std::size_t operator()(const Vertex& vertex) const {
        std::size_t x = std::hash<float>{}(vertex.x);
        std::size_t y = std::hash<float>{}(vertex.y);
        std::size_t z = std::hash<float>{}(vertex.z);

        return x ^ (y << 1) ^ (z << 2);
    }
};

struct VertexPositionEqual {
    bool operator()(const Vertex& a, const Vertex& b) const {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

inline std::pair<std::vector<Vertex>, std::vector<GLuint>> getMarchingCubeData(
    const std::vector<float>& noise,
    int width,
    int height,
    int depth,
    Color color = {1.0f, 1.0f, 1.0f, 1.0f}
) {
    if (width < 2 || height < 2 || depth < 2 || noise.size() < static_cast<size_t>(width) * static_cast<size_t>(depth)) {
        return {{}, {}};
    }

    const std::array<Vertex, 8> corners {
        Vertex{0.0f, 0.0f, 0.0f, color.r, color.g, color.b, color.a},
        Vertex{1.0f, 0.0f, 0.0f, color.r, color.g, color.b, color.a},
        Vertex{0.0f, 1.0f, 0.0f, color.r, color.g, color.b, color.a},
        Vertex{1.0f, 1.0f, 0.0f, color.r, color.g, color.b, color.a},
        Vertex{0.0f, 0.0f, 1.0f, color.r, color.g, color.b, color.a},
        Vertex{1.0f, 0.0f, 1.0f, color.r, color.g, color.b, color.a},
        Vertex{0.0f, 1.0f, 1.0f, color.r, color.g, color.b, color.a},
        Vertex{1.0f, 1.0f, 1.0f, color.r, color.g, color.b, color.a}
    };

    const std::array<uint, 8> cornerDx {0, 1, 0, 1, 0, 1, 0, 1};
    const std::array<uint, 8> cornerDy {0, 0, 1, 1, 0, 0, 1, 1};
    const std::array<uint, 8> cornerDz {0, 0, 0, 0, 1, 1, 1, 1};

    std::vector<uint> columnHeights(static_cast<size_t>(width) * static_cast<size_t>(depth));
    for (uint z = 0; z < static_cast<uint>(depth); ++z) {
        for (uint x = 0; x < static_cast<uint>(width); ++x) {
            const uint noiseIndex = (z * width) + x;
            columnHeights[noiseIndex] = static_cast<uint>(std::round(1.0f + noise[noiseIndex] * (static_cast<float>(height) - 1.0f)));
        }
    }

    auto isVoxelFilled = [&](uint x, uint y, uint z) {
        return y < columnHeights[(z * width) + x];
    };

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::unordered_map<Vertex, GLuint, VertexPositionHash, VertexPositionEqual> vertexMap;

    for (uint z = 0; z < static_cast<uint>(depth - 1); ++z) {
        for (uint y = 0; y < static_cast<uint>(height); ++y) {
            for (uint x = 0; x < static_cast<uint>(width - 1); ++x) {
                uint caseIndex = 0;

                for (int i = 0; i < 8; ++i) {
                    if (isVoxelFilled(x + cornerDx[i], y + cornerDy[i], z + cornerDz[i])) {
                        caseIndex |= (1 << i);
                    }
                }

                if (caseIndex != 0 && caseIndex != 255) {
                    auto classIndex = regularCellClass[caseIndex];
                    auto cellData = regularCellData[classIndex];
                    auto vertexCount = cellData.GetVertexCount();
                    auto triangleCount = cellData.GetTriangleCount();

                    std::vector<GLuint> cellVertexIndices(vertexCount);

                    for (int i = 0; i < vertexCount; i++) {
                        auto edgeInfo = regularVertexData[caseIndex][i];
                        auto lowByte = edgeInfo & 0xFF;
                        auto a = lowByte >> 4;
                        auto b = lowByte & 0x0F;

                        auto dim = static_cast<float>(y) / static_cast<float>(height);

                        auto pos = corners[a] % corners[b];
                        pos = (pos + Vertex{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 0.0f, 0.0f, 0.0f, 0.0f}) * Vertex{1.0f, 1.0f, 1.0f, dim, dim, dim, 1.0f};

                        auto found = vertexMap.find(pos);
                        if (found == vertexMap.end()) {
                            GLuint vertexIndex = vertices.size();
                            vertexMap[pos] = vertexIndex;
                            vertices.push_back(pos);
                            cellVertexIndices[i] = vertexIndex;
                        } else {
                            cellVertexIndices[i] = found->second;
                        }
                    }
                    for (int i = 0; i < (triangleCount * 3); i++) {
                        indices.push_back(cellVertexIndices[cellData.vertexIndex[i]]);
                    }
                }
            }
        }
    }

    if (!vertices.empty() && !indices.empty()) {
        return {vertices, indices};
    }
    return {{}, {}};
}
