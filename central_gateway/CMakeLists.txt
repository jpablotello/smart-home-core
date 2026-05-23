# CMakeLists.txt del Gateway Central
cmake_minimum_required(VERSION 3.16)

# Agrega la ruta de componentes compartidos
set(EXTRA_COMPONENT_DIRS "../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(central_gateway)
