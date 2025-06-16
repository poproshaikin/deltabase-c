#!/bin/bash
set -e

# --- Settings ---
CXXFLAGS="-std=c++17 -Wall -Wextra -O2"
CFLAGS="-std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L"
BUILD_DIR="build"
BIN_DIR="$BUILD_DIR/bin"
LIB_DIR="$BUILD_DIR/lib"
OBJ_DIR="$BUILD_DIR/obj"

MODULES=(core executor sql server misc cli)
UUID_CFLAGS=$(pkg-config --cflags uuid)
UUID_LIBS=$(pkg-config --libs uuid)

mkdir -p "$BIN_DIR" "$LIB_DIR" "$OBJ_DIR"

# --- Build core (C only with gcc) ---
echo "Building core..."
CORE_OBJS=""
for src in $(find src/core -name '*.c'); do
    obj="$OBJ_DIR/${src%.c}.o"
    obj="${obj//src\//}"  # flatten path
    obj="$OBJ_DIR/$obj"
    mkdir -p "$(dirname "$obj")"
    gcc $CFLAGS $UUID_CFLAGS -Isrc/core/include -c "$src" -o "$obj"
    CORE_OBJS="$CORE_OBJS $obj"
done

ar rcs "$LIB_DIR/libcore.a" $CORE_OBJS

# --- Build C++ modules (g++) ---
for mod in "${MODULES[@]:1}"; do
    echo "Building $mod..."
    OBJS=""
    for src in $(find "src/$mod" -name '*.cpp' -o -name '*.cc' -o -name '*.cxx'); do
        obj="$OBJ_DIR/${src%.*}.o"
        obj="${obj//src\//}"
        obj="$OBJ_DIR/$obj"
        mkdir -p "$(dirname "$obj")"
        g++ $CXXFLAGS -Isrc/$mod/include -c "$src" -o "$obj"
        OBJS="$OBJS $obj"
    done
    ar rcs "$LIB_DIR/lib$mod.a" $OBJS
done

# --- Build test.exe ---
echo "Linking test.exe..."
TEST_OBJS=""
for src in $(find tests -name '*.cpp' -o -name '*.cc' -o -name '*.cxx'); do
    obj="$OBJ_DIR/${src%.*}.o"
    obj="${obj//tests\//}"
    obj="$OBJ_DIR/tests/$obj"
    mkdir -p "$(dirname "$obj")"
    g++ $CXXFLAGS -c "$src" -o "$obj"
    TEST_OBJS="$TEST_OBJS $obj"
done

g++ $CXXFLAGS -o "$BIN_DIR/test.exe" $TEST_OBJS \
    $LIB_DIR/libcli.a \
    $LIB_DIR/libmisc.a \
    $LIB_DIR/libexecutor.a \
    $LIB_DIR/libsql.a \
    $LIB_DIR/libserver.a \
    $LIB_DIR/libcore.a \
    $UUID_LIBS

# --- Build main.exe ---
echo "Linking main.exe..."
MAIN_OBJ="$OBJ_DIR/main.o"
g++ $CXXFLAGS -c main.cpp -o "$MAIN_OBJ"

g++ $CXXFLAGS -o "$BIN_DIR/main.exe" "$MAIN_OBJ" \
    $LIB_DIR/libcli.a \
    $LIB_DIR/libmisc.a \
    $LIB_DIR/libexecutor.a \
    $LIB_DIR/libsql.a \
    $LIB_DIR/libserver.a \
    $LIB_DIR/libcore.a \
    $UUID_LIBS

echo "✅ Build complete. Binaries are in $BIN_DIR"
