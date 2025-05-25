cmake_minimum_required(VERSION 3.18)

project(threedee C)

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")  
    set(USE_FLAGS "-s USE_SDL=3 -s USE_SDL_IMAGE=3 -s SDL3_IMAGE_FORMATS=\"[\"\"png\"\"]\" -s USE_SDL_TTF=3 -s USE_SDL_MIXER=3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

include_directories(cJSON/include)
include_directories(threedee/include)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")  
    include_directories(SDL/include)
    include_directories(SDL_image/include)
    include_directories(SDL_ttf/include)
    include_directories(SDL_mixer/include)
endif()

set(SOURCES
    cJSON/src/cJSON.c
    threedee/src/animation.c
    threedee/src/app.c
    threedee/src/camera.c
    threedee/src/collider.c
    threedee/src/component.c
    threedee/src/game.c
    threedee/src/grid.c
    threedee/src/heap.c
    threedee/src/image.c
    threedee/src/input.c
    threedee/src/interface.c
    threedee/src/linalg.c
    threedee/src/list.c
    threedee/src/menu.c
    threedee/src/navigation.c
    threedee/src/particle.c
    threedee/src/perlin.c
    threedee/src/physics.c
    threedee/src/player.c
    threedee/src/raycast.c
    threedee/src/settings.c
    threedee/src/sound.c
    threedee/src/util.c
    threedee/src/threedee.c
    threedee/src/widget.c
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")  
    add_executable(threedee ${SOURCES})
    set_target_properties(threedee PROPERTIES LINK_FLAGS "--preload-file=data -s ALLOW_MEMORY_GROWTH=1 --shell-file ../shell.html")

    file(COPY threedee/data
        DESTINATION ${CMAKE_BINARY_DIR}
        FILES_MATCHING 
            PATTERN "*.png"
            PATTERN "*.ttf"
            PATTERN "*.wav"
            PATTERN "*.json"
            PATTERN "*.ogg"
    )   
else()
    # Enable console by removing WIN32 flag
    add_executable(threedee ${SOURCES})
endif()

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/threedee/data/shaders/source)
set(SHADER_BIN_DIR ${CMAKE_BINARY_DIR}/data/shaders/compiled/SPIRV)

file(GLOB SHADERS "${SHADER_SRC_DIR}/*.vert.hlsl" "${SHADER_SRC_DIR}/*.frag.hlsl")

foreach(SHADER ${SHADERS})
    message(STATUS "Compiling shader: ${SHADER}")
    get_filename_component(FILE_NAME ${SHADER} NAME_WLE)
    set(SPIRV ${SHADER_BIN_DIR}/${FILE_NAME}.spv)
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BIN_DIR}
            COMMAND glslangValidator -V -e main ${SHADER} -o ${SPIRV}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${FILE_NAME}"
    )
    list(APPEND SPIRV_SHADERS ${SPIRV})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SPIRV_SHADERS})
add_dependencies(threedee compile_shaders)

install(TARGETS threedee
    DESTINATION ${CMAKE_BINARY_DIR})

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(LIBS
        ${CMAKE_SOURCE_DIR}/SDL/lib/x64/sdl3.lib
        ${CMAKE_SOURCE_DIR}/SDL_image/lib/x64/sdl3_image.lib
        ${CMAKE_SOURCE_DIR}/SDL_ttf/lib/x64/sdl3_ttf.lib
        ${CMAKE_SOURCE_DIR}/SDL_mixer/lib/x64/sdl3_mixer.lib
    )

    target_link_libraries(threedee ${LIBS})

    set(DLLS
        ${CMAKE_SOURCE_DIR}/SDL/lib/x64/sdl3.dll
        ${CMAKE_SOURCE_DIR}/SDL_image/lib/x64/sdl3_image.dll
        ${CMAKE_SOURCE_DIR}/SDL_ttf/lib/x64/sdl3_ttf.dll
        ${CMAKE_SOURCE_DIR}/SDL_mixer/lib/x64/sdl3_mixer.dll
    )

    install(FILES ${DLLS}
        DESTINATION ${CMAKE_BINARY_DIR}
    )

    install(DIRECTORY threedee/data
        DESTINATION ${CMAKE_BINARY_DIR}
        FILES_MATCHING 
            PATTERN "*.png"
            PATTERN "*.ttf"
            PATTERN "*.wav"
            PATTERN "*.json"
            PATTERN "*.ogg"
    )

    install(FILES ${EXTERNAL_DLLS}
        DESTINATION ${CMAKE_BINARY_DIR}
    )

    set(CONFIG 
        ${CMAKE_SOURCE_DIR}/threedee/config.cfg
    )

    install(FILES ${CONFIG}
        DESTINATION ${CMAKE_BINARY_DIR}
    )
endif()
