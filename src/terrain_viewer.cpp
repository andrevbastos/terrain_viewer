#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifcg/ifcg.hpp>
#include <ifcg/graphics/mesh.hpp>

#include "core/util.hpp"

int setupUDPSocket(int port) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::cerr << "[UDP] Erro ao criar socket" << std::endl;
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "[UDP] Erro ao configurar socket como O_NONBLOCK" << std::endl;
        close(fd);
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[UDP] Erro no bind na porta " << port << std::endl;
        close(fd);
        return -1;
    }

    std::cout << "[UDP] Escutando comandos na porta " << port << "..." << std::endl;
    return fd;
}

int main() {
    const int port = 12345;
    int socket_fd = setupUDPSocket(port);
    if (socket_fd < 0) {
        return 1;
    }

    Engine::init(1200, 800, "Gerador de Terrenos Interativo - IFCG");
    Engine::setup3D();

    auto& renderer {Engine::getRenderer()};
    auto& input {Engine::getInputHandler()};
    GLuint shader {renderer.getShaderID()};

    auto& camera {renderer.getCamera()};
    camera.setPosition(glm::vec3(-50.0f, 150.0f, -50.0f));
    camera.setOrientation(glm::vec3(0.6f, -0.5f, 0.6f));
    renderer.setFarPlane(2000.0f);
    camera.setSpeed(1.0f);

    input.addKeyCallback(Key::SHIFT_L, KeyAction::HELD, [&camera]() {
        camera.setSpeed(3.0f);
    });

    input.addKeyCallback(Key::SHIFT_L, KeyAction::RELEASE, [&camera]() {
        camera.setSpeed(1.0f);
    });


    noise2D::NoiseConfig config {
        .width = 256,
        .height = 256,
        .wave = 100,
        .lacunarity = 2.0f,
        .persistence = 0.5f,
        .exp = 1.0f,
        .seed = 42,
        .octaves = 3
    };
    float intensity = 100.0f;

    Color terrainColor = {0.3f, 0.6f, 0.3f, 1.0f};

    std::cout << "[Terrain] Gerando malha inicial de tamanho " << config.width << "x" << config.height << "..." << std::endl;
    std::vector<float> noiseMap = noise2D::generateNoiseMap(config);
    noise2D::saveNoiseAsPNG("./noise_preview.png", noiseMap, config.width, config.height);
    auto [vertices, indices] = getMarchingCubeData(noiseMap, config.width, intensity, config.height, terrainColor);
    
    std::shared_ptr<MeshBase> terrainMesh = std::make_shared<Mesh>(std::move(vertices), std::move(indices), shader, GL_TRIANGLES);
    renderer.addMesh(terrainMesh);

    bool needsUpdate = false;

    LoopConfig loopConfig {
        .loopBody = [&]() {
            char buffer[1024];
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            
            bool dataReceived = false;
            int new_w = config.width;
            int new_h = config.height;
            float new_wave = static_cast<float>(config.wave);
            float new_lacunarity = config.lacunarity;
            float new_persistence = config.persistence;
            float new_exp = config.exp;
            unsigned int new_seed = config.seed;
            unsigned int new_octaves = config.octaves;
            float new_intensity = intensity;

            while (true) {
                ssize_t bytes = recvfrom(socket_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
                if (bytes <= 0) {
                    break;
                }
                
                buffer[bytes] = '\0';
                std::string msg(buffer);
                std::stringstream ss(msg);
                
                if (ss >> new_w >> new_h >> new_wave >> new_lacunarity >> new_persistence >> new_exp >> new_seed >> new_octaves >> new_intensity) {
                    dataReceived = true;
                }
            }

            if (dataReceived) {
                if (config.width != new_w || config.height != new_h ||
                    config.wave != static_cast<int>(new_wave) || config.lacunarity != new_lacunarity ||
                    config.persistence != new_persistence || config.exp != new_exp ||
                    config.seed != new_seed || config.octaves != new_octaves ||
                    intensity != new_intensity) {
                    
                    config.width = new_w;
                    config.height = new_h;
                    config.wave = static_cast<int>(new_wave);
                    config.lacunarity = new_lacunarity;
                    config.persistence = new_persistence;
                    config.exp = new_exp;
                    config.seed = new_seed;
                    config.octaves = new_octaves;
                    intensity = new_intensity;
                    needsUpdate = true;
                }
            }

            if (needsUpdate) {
                std::vector<float> newNoiseMap = noise2D::generateNoiseMap(config);
                noise2D::saveNoiseAsPNG("./noise_preview.png", newNoiseMap, config.width, config.height);
                auto [newVertices, newIndices] = getMarchingCubeData(newNoiseMap, config.width, intensity, config.height, terrainColor);
                
                if (!newVertices.empty()) {
                    auto newMesh = std::make_shared<Mesh>(std::move(newVertices), std::move(newIndices), shader, GL_TRIANGLES);
                    renderer.removeMesh(terrainMesh);
                    renderer.addMesh(newMesh);
                    terrainMesh = newMesh;
                }
                
                needsUpdate = false;
            }
        },
        .onExit = [&]() {
            if (socket_fd >= 0) {
                close(socket_fd);
            }
            std::cout << "[Engine] Visualizador encerrado." << std::endl;
        }
    };

    Engine::loop(loopConfig);

    renderer.removeMesh(terrainMesh);
    terrainMesh.reset();

    Engine::terminate();

    return 0;
}
