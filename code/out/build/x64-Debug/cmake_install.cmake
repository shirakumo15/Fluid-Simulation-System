# Install script for directory: E:/CG/Fluid-Simulation-System/code

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "E:/CG/Fluid-Simulation-System/code/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("E:/CG/Fluid-Simulation-System/code/out/build/x64-Debug/third_party/imgui/cmake_install.cmake")
  include("E:/CG/Fluid-Simulation-System/code/out/build/x64-Debug/third_party/glad/cmake_install.cmake")
  include("E:/CG/Fluid-Simulation-System/code/out/build/x64-Debug/third_party/glm/cmake_install.cmake")
  include("E:/CG/Fluid-Simulation-System/code/out/build/x64-Debug/common/cmake_install.cmake")
  include("E:/CG/Fluid-Simulation-System/code/out/build/x64-Debug/fluid2d/cmake_install.cmake")
  include("E:/CG/Fluid-Simulation-System/code/out/build/x64-Debug/fluid3d/cmake_install.cmake")
  include("E:/CG/Fluid-Simulation-System/code/out/build/x64-Debug/ui/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "E:/CG/Fluid-Simulation-System/code/out/build/x64-Debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
