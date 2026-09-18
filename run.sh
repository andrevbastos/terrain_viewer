#!/bin/bash

# Encontra a pasta do script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# 1. Compila o projeto C++ focado no terrain_viewer
echo -e "\e[1;34m[Build]\e[0m Compilando o terrain_viewer..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
if ! cmake --build . --target terrain_viewer; then
    echo -e "\e[1;31m[Build] Erro na compilação do terrain_viewer!\e[0m"
    exit 1
fi
cd ..

# 2. Inicia o painel de controles em Python no background
echo -e "\e[1;32m[Python]\e[0m Iniciando a interface de controles..."
python3 controls.py &
PYTHON_PID=$!

# Garante que o processo Python seja encerrado ao fechar a aplicação C++
cleanup() {
    echo -e "\e[1;33m[Cleanup]\e[0m Encerrando a interface de controles..."
    kill $PYTHON_PID 2>/dev/null
    exit
}
trap cleanup EXIT INT TERM

# 3. Executa o visualizador C++ de terrenos de dentro da pasta build/ (necessário para os shaders e preview)
echo -e "\e[1;32m[C++]\e[0m Iniciando o visualizador de terrenos 3D..."
cd build
./terrain_viewer
