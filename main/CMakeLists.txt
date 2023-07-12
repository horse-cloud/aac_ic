# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

file(GLOB_RECURSE DRIVER_RT903_SRCS ./driver/rt903/*.c)
file(GLOB_RECURSE DRIVER_UCS10100_SRCS ./driver/ucs10100/*.c)
file(GLOB_RECURSE SERVICES_SRCS ./services/*.c)


idf_component_register(
    SRCS main.c   ${DRIVER_RT903_SRCS} ${DRIVER_UCS10100_SRCS} ${SERVICES_SRCS}    # list the source files of this component
    INCLUDE_DIRS  "include"   # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES      "driver" "freertos" "esp_timer"       # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
