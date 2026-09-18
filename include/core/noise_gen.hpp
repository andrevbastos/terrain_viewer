#include <iostream>
#include <math.h>
#include <random>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cerrno>
#include <cstring>

#include "stb_image.h"
#include "stb_image_write.h"

namespace fs = std::filesystem;

namespace noise2D {
    struct Vector2D {
        float x, y;
    };

    struct NoiseConfig {
        int width, height;
        int wave = 100;
        float lacunarity = 2.0f;
        float persistence = 0.5f;
        float exp = 1.0f;
        unsigned int seed = 0;
        unsigned int octaves = 5;
    };

    Vector2D randomGradient(int ix, int iy, unsigned int seed) {
        const unsigned w {8 * sizeof(unsigned)};
        const unsigned s {w / 2}; 
        unsigned a = ix, b = iy;
        a ^= seed;
        a *= 3284157443;
    
        b ^= a << s | a >> w - s;
        b *= 1911520717;
    
        a ^= b << s | b >> w - s;
        a *= 2048419325;
        float random = a * (3.14159265 / ~(~0u >> 1));
        
        Vector2D v;
        v.x = sin(random);
        v.y = cos(random);
    
        return v;
    }

    float dotGridGradient(int ix, int iy, float x, float y, unsigned int seed) {
        auto gradient {randomGradient(ix, iy, seed)};

        float dx {x - (float)ix};
        float dy {y - (float)iy};

        return (dx * gradient.x + dy * gradient.y);
    }

    float interpolate(float a0, float a1, float w) {
        return (a1 - a0) * (3.0f - w * 2.0f) * w * w + a0;
    }

    float perlin(float x, float y, unsigned int seed) {
        int x0 {(int)x};
        int x1 {x0 + 1};
        int y0 {(int)y};
        int y1 {y0 + 1};

        float sx {x - (float)x0};
        float sy {y - (float)y0};

        float n0 {dotGridGradient(x0, y0, x, y, seed)};
        float n1 {dotGridGradient(x1, y0, x, y, seed)};
        float ix0 {interpolate(n0, n1, sx)};

        n0 = dotGridGradient(x0, y1, x, y, seed);
        n1 = dotGridGradient(x1, y1, x, y, seed);
        float ix1 {interpolate(n0, n1, sx)};

        float result {interpolate(ix0, ix1, sy)};
        
        return result;
    }

    std::vector<float> generateNoiseMap(NoiseConfig config) {
        std::vector<float> noiseMap(config.width * config.height);

        for (int y{0}; y < config.height; ++y) {
            for (int x{0}; x < config.width; ++x) {
                int index {y * config.width + x};

                float val {0.0f};
                float amplitudeSum {0.0f};
                float frequency {1.0f};
                float amplitude {1.0f};

                for (unsigned int i{0}; i < config.octaves; ++i) {
                    val += perlin(x * frequency / config.wave, y * frequency / config.wave, config.seed) * amplitude;
                    amplitudeSum += amplitude;
                    frequency *= config.lacunarity;
                    amplitude *= config.persistence;
                }

                if (amplitudeSum > 0.0f) val /= amplitudeSum;

                if (val > 1.0f) val = 1.0f;
                else if (val < -1.0f) val = -1.0f;
                
                val = (val + 1.0f) * 0.5f;
                val = pow(val, config.exp);
                
                noiseMap[index] = val;
            }
        }

        return noiseMap;
    }

    void saveNoiseAsPNG(const std::string& filename, const std::vector<float>& noiseMap, int width, int height) {
        std::vector<unsigned char> imageData(width * height);
        
        for (size_t i = 0; i < noiseMap.size(); ++i) {
            imageData[i] = static_cast<unsigned char>(noiseMap[i] * 255);
        }

        fs::create_directories(fs::path(filename).parent_path());
        if (!stbi_write_png(filename.c_str(), width, height, 1, imageData.data(), width)) {
            std::cerr << "Failed to save noise map as PNG: " << filename << " (" << std::strerror(errno) << ")" << std::endl;
        }
    }
}

namespace noise3D {
    struct Vector3D {
        float x, y, z;
    };
    
    struct NoiseConfig {
        int width, height, depth;
        int wave = 100;
        float lacunarity = 2.0f;
        float persistence = 0.5f;
        float exp = 1.0f;
        unsigned int seed = 0;
        unsigned int octaves = 5;
    };

    Vector3D randomGradient(int ix, int iy, int iz, unsigned int seed) {
        const unsigned w {8 * sizeof(unsigned)};
        const unsigned s {w / 2}; 
        unsigned a = ix, b = iy, c = iz;
        a ^= seed;
        a *= 3284157443;

        b ^= a << s | a >> (w - s);
        b *= 1911520717;

        c ^= b << s | b >> (w - s);
        c *= 2048419325;

        a ^= c << s | c >> (w - s);
        a *= 668265263;

        float theta = a * (3.14159265 / ~(~0u >> 1));
        float phi = b * (3.14159265 / ~(~0u >> 1));

        Vector3D v;
        v.x = sin(theta) * cos(phi);
        v.y = sin(theta) * sin(phi);
        v.z = cos(theta);

        return v;
    };

    float dotGridGradient(int ix, int iy, int iz, float x, float y, float z, unsigned int seed) {
        auto gradient {randomGradient(ix, iy, iz, seed)};

        float dx {x - (float)ix};
        float dy {y - (float)iy};
        float dz {z - (float)iz};

        return (dx * gradient.x + dy * gradient.y + dz * gradient.z);
    };

    float interpolate(float a0, float a1, float w) {
        return (a1 - a0) * (3.0f - w * 2.0f) * w * w + a0;
    };
    
    float perlin(float x, float y, float z, unsigned int seed) {
        int x0 {(int)x};
        int x1 {x0 + 1};

        int y0 {(int)y};
        int y1 {y0 + 1};

        int z0 {(int)z};
        int z1 {z0 + 1};

        float sx {x - (float)x0};
        float sy {y - (float)y0};
        float sz {z - (float)z0};

        float n000 = dotGridGradient(x0, y0, z0, x, y, z, seed);
        float n100 = dotGridGradient(x1, y0, z0, x, y, z, seed);
        float n010 = dotGridGradient(x0, y1, z0, x, y, z, seed);
        float n110 = dotGridGradient(x1, y1, z0, x, y, z, seed);
        float n001 = dotGridGradient(x0, y0, z1, x, y, z, seed);
        float n101 = dotGridGradient(x1, y0, z1, x, y, z, seed);
        float n011 = dotGridGradient(x0, y1, z1, x, y, z, seed);
        float n111 = dotGridGradient(x1, y1, z1, x, y, z, seed);

        float ix00 = interpolate(n000, n100, sx);
        float ix10 = interpolate(n010, n110, sx);
        float ix01 = interpolate(n001, n101, sx);
        float ix11 = interpolate(n011, n111, sx);

        float iy0 = interpolate(ix00, ix10, sy);
        float iy1 = interpolate(ix01, ix11, sy);

        float result = interpolate(iy0, iy1, sz);

        return result;
    };
    
    std::vector<float> generateNoiseField(NoiseConfig config) {
        std::vector<float> noiseField(config.width * config.height * config.depth);
        
        for (int z{0}; z < config.depth; ++z) {
            for (int y{0}; y < config.height; ++y) {
                for (int x{0}; x < config.width; ++x) {
                    int index {z * config.width * config.height + y * config.width + x};

                    float val {0.0f};
                    float amplitudeSum {0.0f};
                    float frequency {1.0f};
                    float amplitude {1.0f};

                    for (unsigned int i{0}; i < config.octaves; ++i) {
                        val += perlin(x * frequency / config.wave, y * frequency / config.wave, z * frequency / config.wave, config.seed) * amplitude;
                        amplitudeSum += amplitude;
                        frequency *= config.lacunarity;
                        amplitude *= config.persistence;
                    }

                    if (amplitudeSum > 0.0f) val /= amplitudeSum;

                    if (val > 1.0f) val = 1.0f;
                    else if (val < -1.0f) val = -1.0f;

                    val = (val + 1.0f) * 0.5f;
                    val = pow(val, config.exp);

                    noiseField[index] = val;
                }
            }
        }
        
        return noiseField;
    };
    
    void saveNoiseSliceAsPNG(const std::string& filename, const std::vector<float>& noiseField, int width, int height, int z) {
        std::vector<unsigned char> imageData(width * height);

        for (int y{0}; y < height; ++y) {
            for (int x{0}; x < width; ++x) {
                int fieldIndex = z * width * height + y * width + x;
                int imageIndex = y * width + x;

                imageData[imageIndex] = static_cast<unsigned char>(noiseField[fieldIndex] * 255);
            }
        }

        fs::create_directories(fs::path(filename).parent_path());

        if (!stbi_write_png(filename.c_str(), width, height, 1, imageData.data(), width)) {
            std::cerr << "Failed to save noise slice as PNG: " << filename << " (" << std::strerror(errno) << ")" << std::endl;
        }
    };
}