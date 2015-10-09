cmake_minimum_required (VERSION 2.8)
include_directories (${CMAKE_SOURCE_DIR}
                     ${CMAKE_SOURCE_DIR}/src
                     ${CMAKE_SOURCE_DIR}/src/commands
                     ${CMAKE_SOURCE_DIR}/src/columns
                     ${TASK_INCLUDE_DIRS})

set (task_SRCS CLI.cpp CLI.h
               Color.cpp Color.h
               Config.cpp Config.h
               Context.cpp Context.h
               DOM.cpp DOM.h
               Date.cpp Date.h
               Dates.cpp Dates.h
               Duration.cpp Duration.h
               Eval.cpp Eval.h
               iostream.cpp
               Filter.cpp Filter.h
               Hooks.cpp Hooks.h
               ISO8601.cpp ISO8601.h
               JSON.cpp JSON.h
               Lexer.cpp Lexer.h
               Msg.cpp Msg.h
               Nibbler.cpp Nibbler.h
               RX.cpp RX.h
               TDB2.cpp TDB2.h
               Task.cpp Task.h
               Timer.cpp Timer.h
               TLSClient.cpp TLSClient.h
               Variant.cpp Variant.h
               ViewTask.cpp ViewTask.h
               ViewText.cpp ViewText.h
               dependency.cpp
               feedback.cpp
               i18n.h
               legacy.cpp
               recur.cpp
               rules.cpp
               sort.cpp
               text.cpp text.h
               utf8.cpp utf8.h
               util.cpp util.h
               wcwidth6.cpp)

if (WINDOWS)
  set(task_SRCS ${task_SRCS}
    windows/Directory.cpp windows/Directory.h
    windows/File.cpp windows/File.h
    windows/Path.cpp windows/Path.h
    windows/interactive.cpp)
else (WINDOWS)
  set(task_SRCS ${task_SRCS}
    posix/Directory.cpp posix/Directory.h
    posix/File.cpp posix/File.h
    posix/Path.cpp posix/Path.h
    posix/interactive.cpp)
endif (WINDOWS)

add_library (task STATIC ${task_SRCS})
add_executable (task_executable main.cpp)
add_executable (calc_executable calc.cpp)
add_executable (lex_executable lex.cpp)

# Yes, 'task' is included twice, otherwise linking fails on assorted OSes.
target_link_libraries (task_executable task commands columns task ${TASK_LIBRARIES})
target_link_libraries (calc_executable task commands columns task ${TASK_LIBRARIES})
target_link_libraries (lex_executable task commands columns task ${TASK_LIBRARIES})

set_property (TARGET task_executable PROPERTY OUTPUT_NAME "task")

install (TARGETS task_executable DESTINATION ${TASK_BINDIR})

set_property (TARGET calc_executable PROPERTY OUTPUT_NAME "calc")
set_property (TARGET lex_executable PROPERTY OUTPUT_NAME "lex")

#SET(CMAKE_BUILD_TYPE gcov)
#SET(CMAKE_CXX_FLAGS_GCOV "--coverage")
#SET(CMAKE_C_FLAGS_GCOV "--coverage")
#SET(CMAKE_EXE_LINKER_FLAGS_GCOV "--coverage")
