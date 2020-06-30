cmake_minimum_required(VERSION 3.3)
project(molecular-programgenerator)

set(CMAKE_CXX_STANDARD 14)

add_custom_target(  generate-parser
					COMMAND ./gen-lex-files molecular/programgenerator/GlslTranslator.l
					COMMAND ./gen-ypp-files molecular/programgenerator/GlslTranslator.ypp
					WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
					SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.l
						${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.ypp
)

#define explicitly files, that are going to be generated, but not present yet
# so that cmake can proceed generationg Makefiles
add_custom_command(OUTPUT
	${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.lex.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.lex.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.tab.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.tab.hpp
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(molecular-programgenerator
	molecular/programgenerator/CfgParser.cpp
	molecular/programgenerator/CfgParser.h
	${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.lex.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.lex.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.tab.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/molecular/programgenerator/GlslTranslator.tab.hpp
	molecular/programgenerator/ProgramGenerator.cpp
	molecular/programgenerator/ProgramGenerator.h
	molecular/programgenerator/ProgramFile.cpp
	molecular/programgenerator/ProgramFile.h
)
target_link_libraries(molecular-programgenerator PUBLIC molecular::util)
add_dependencies(molecular-programgenerator generate-parser)
add_library(molecular::programgenerator ALIAS molecular-programgenerator)
target_include_directories(molecular-programgenerator PUBLIC .)
