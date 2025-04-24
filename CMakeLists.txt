# Minimum CMake versiyonu
cmake_minimum_required(VERSION 3.10)

# Proje adı ve dili
project(DesktopViewer VERSION 1.0 LANGUAGES CXX)

# C++ standardı
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# OpenGL ve Assimp kütüphanelerini bul
find_package(OpenGL REQUIRED)
find_package(ASSIMP REQUIRED)

# Kaynak dosyalar
set(SOURCES
    src/main.cpp
    src/DesktopViewer.cpp
)

# Başlık dosyalar
set(HEADERS
    src/DesktopViewer.h
)

# Assets ve diğer kaynak dosyalar
set(MODELS_DIR assets/models)
set(METADATA_DIR assets/metadata)

# Include dizinlerini ayarlama
include_directories(${ASSIMP_INCLUDE_DIRS})
include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(src)

# Derleme hedefi (executable) oluşturma
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# OpenGL ve Assimp kütüphanelerini linkleme
target_link_libraries(${PROJECT_NAME} PRIVATE ${ASSIMP_LIBRARIES} ${OPENGL_LIBRARIES})

# (Opsiyonel) assets dizinlerini derleme hedeflerine dahil et
# Bu adım sadece gerekli model ve metadata dosyalarına erişim sağlamak için
file(COPY ${MODELS_DIR} DESTINATION ${CMAKE_BINARY_DIR}/models)
file(COPY ${METADATA_DIR} DESTINATION ${CMAKE_BINARY_DIR}/metadata)
