#!/bin/bash

# compile shaders

mkdir -p shader/build

# simple shader
./third-party/build/bin/shaderc \
-f shader/v_simple.sc -o shader/build/v_simple.bin \
--platform linux --type vertex --verbose -i ./ -p spirv

./third-party/build/bin/shaderc \
-f shader/f_simple.sc -o shader/build/f_simple.bin \
--platform linux --type fragment --verbose -i ./ -p spirv

# sphere instanced shader (note: shader/sphere first so it finds the correct varying.def.sc)
./third-party/build/bin/shaderc \
-f shader/sphere/vs_sphere_instanced.sc -o shader/build/vs_sphere_instanced.bin \
--platform linux --type vertex --verbose -i shader/sphere -i third-party/build/include/bgfx -p spirv

./third-party/build/bin/shaderc \
-f shader/sphere/fs_sphere_textured.sc -o shader/build/fs_sphere_textured.bin \
--platform linux --type fragment --verbose -i shader/sphere -i third-party/build/include/bgfx -p spirv
