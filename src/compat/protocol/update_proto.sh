#!/bin/bash

# 优先使用系统安装的protoc
if which protoc >/dev/null 2>&1; then
    protoc=$(which protoc)
    echo "Using system installed protoc: $protoc"
else
    protobuf_dir="${PWD}/../../../3rdparty/protobuf"
    protoc="${protobuf_dir}/protoc/linux-x86_64/protoc"
    
    if [ ! -f "$protoc" ]; then
        echo "Error: protoc not found in system PATH nor in $protobuf_dir"
        exit 1
    fi
    echo "Using local protoc: $protoc"
fi

filename=${1:-"message.proto"}
cppout=${2:-${PWD}}

if [ -f $protoc ]; then
    echo "Running C++ protocol buffer compiler on ${filename} out: ${cppout}"
    $protoc --cpp_out ${cppout} ${filename}
else
    echo "the protoc is not exist:$protoc"
fi
