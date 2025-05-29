cmake_minimum_required(VERSION 3.10)
project(deltabase-c C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set (SOURCES
main.c
errors.c
data/src/header/header-io.c
data/src/token/token-io.c
data/src/row/row-io.c
data/src/utils/utils.c
data/src/utils/initializers.c
sql/src/token.c
sql/src/lexer.c
sql/src/parse-select.c
sql/src/parse-where.c
sql/src/parser.c
sql/src/constants.c
utils/stream-utils/stream-utils.c
utils/string-utils/string-utils.c
)

add_executable(exe ${SOURCES})
