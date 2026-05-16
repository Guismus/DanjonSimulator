cmake_minimum_required(VERSION 3.20)
project(DanjonSimulator LANGUAGES CXX)

# 1. On désactive la gestion automatique du standard par CMake pour éviter l'erreur
# set(CMAKE_CXX_STANDARD 26) # <-- NE PAS UTILISER ÇA, ça fait planter ton CMake

# 2. On définit nos fichiers sources
file(GLOB SOURCES "*.cpp" "./back/core/*.cpp")
add_executable(DanjonSimulator ${SOURCES})

# 3. On force manuellement le flag C++26 (latest) pour MSVC
if(MSVC)
    target_compile_options(DanjonSimulator PRIVATE /std:c++latest)
    # Optionnel : active aussi les rapports de fonctionnalités pour vérifier
    target_compile_definitions(DanjonSimulator PRIVATE _HAS_CXX26=1)
else()
    # Si jamais tu passes sur GCC ou Clang plus tard
    target_compile_options(DanjonSimulator PRIVATE -std=c++26)
endif()