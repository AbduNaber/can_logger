cmake_minimum_required(VERSION 3.0.2)
project(can_logger)


find_package(catkin REQUIRED)
find_package(yaml-cpp REQUIRED)


catkin_package(
    DEPENDS yaml-cpp
)
find_package(yaml-cpp REQUIRED)

include_directories(
    include
    ${catkin_INCLUDE_DIRS}
    /usr/include/
)

add_executable(can_logger_node src/can_logger.cpp)
target_link_libraries(can_logger_node ${YAML_CPP_LIBRARIES} pthread)
target_include_directories(can_logger_node PRIVATE ${YAML_CPP_INCLUDE_DIRS})
install(TARGETS
    can_logger_node
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
