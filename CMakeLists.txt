file(GLOB COMMS_SRC CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(comms ${COMMS_SRC})

target_include_directories(comms PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# TODO: Use TimeInterface instead of ZephyrTime to remove zephyr_abs dep
target_link_libraries(comms PUBLIC zephyr_abs)
