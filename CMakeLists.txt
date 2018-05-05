cmake_minimum_required(VERSION 2.8.3)

include(cmake/HunterGate.cmake)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.20.68.tar.gz"
    SHA1 "8bbef873746f3e47595550751c052098aa8d202e"
)

project(rospack)

hunter_add_package(catkin)
hunter_add_package(Boost COMPONENTS filesystem program_options system)
hunter_add_package(tinyxml2)

find_package(catkin REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS filesystem program_options system)
set(Python_ADDITIONAL_VERSIONS "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
find_package(PythonLibs "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
set(Boost_LINK_TARGETS Boost::filesystem Boost::program_options Boost::system)
set(TinyXML2_LIBRARIES "tinyxml2")

set(PROJECT_INSTALLSPACE_LIBRARIES ros::rospack)
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES rospack ${PYTHON_LIBRARIES}
  DEPENDS "Boost COMPONENTS filesystem program_options system" tinyxml2
)

#add_definitions(-Wall)

set(API_BACKCOMPAT_V1 "YES" CACHE BOOL "Whether to enable backwards compatibility with old C++ API")
if(API_BACKCOMPAT_V1)
  add_definitions(-DROSPACK_API_BACKCOMPAT_V1)
  set(backcompat_source src/rospack_backcompat.cpp)
endif()

include_directories(include ${TinyXML2_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})

add_library(rospack
  src/rospack.cpp
  ${backcompat_source}
  src/rospack_cmdline.cpp
  src/utils.cpp
)
target_link_libraries(rospack ${TinyXML2_LIBRARIES} ${Boost_LINK_TARGETS} ${PYTHON_LIBRARIES})

add_executable(rospackexe src/rospack_main.cpp)
# Set the name, and make it a "global" executable
set_target_properties(rospackexe PROPERTIES
  OUTPUT_NAME rospack
  RUNTIME_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/bin)
target_link_libraries(rospackexe rospack ${Boost_LINK_TARGETS})
add_executable(rosstackexe src/rosstack_main.cpp)
target_link_libraries(rosstackexe rospack ${Boost_LINK_TARGETS})
# Set the name, and make it a "global" executable
set_target_properties(rosstackexe PROPERTIES
  OUTPUT_NAME rosstack
  RUNTIME_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/bin)

install(TARGETS rospack
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION})
install(TARGETS rospackexe rosstackexe
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION})
install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  FILES_MATCHING PATTERN "*.h")

install(EXPORT ${PROJECT_NAME}Targets
    DESTINATION share/${PROJECT_NAME}/cmake
    NAMESPACE ros::
)

# uninstall target
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

if(CATKIN_ENABLE_TESTING)
  add_subdirectory(test)
endif()

if(DOXYGEN_FOUND)
  add_custom_target(rospack-docs
    COMMAND doxygen Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()
