cmake_minimum_required(VERSION 3.20)

if(ESP_PLATFORM)
   # Build as an ESP-IDF component (header-only)
   idf_component_register(
      SRCS "src/dsmr_parser/component.cpp"
      INCLUDE_DIRS "src"
   )
   return()
endif()

project(dsmr_parser VERSION 1.0.0)
