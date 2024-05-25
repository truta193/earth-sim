if (WIN32)
    cmake_minimum_required(VERSION 3.0)
    project(atestat C)

    set(CMAKE_C_STANDARD 11)

    add_executable(atestat main.c external/glad/src/glad.c)

    find_package(OpenGL REQUIRED)

    include_directories(external/glad/include ${OPENGL_INCLUDE_DIRS} ${X11_INCLUDE_DIR} external/stb_image m)

    target_link_libraries(atestat ${OPENGL_LIBRARIES} ${X11_LIBRARIES} m gdi32 winmm)

else()

    cmake_minimum_required(VERSION 3.0)
    project(atestat C)

    set(CMAKE_C_STANDARD 11)

    add_executable(atestat main.c external/glad/src/glad.c)

    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)


    include_directories(external/glad/include ${OPENGL_INCLUDE_DIRS} ${X11_INCLUDE_DIR} external/stb_image m)
    target_link_libraries(atestat ${OPENGL_LIBRARIES} ${X11_LIBRARIES} m dl)

endif (WIN32)