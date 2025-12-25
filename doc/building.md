# Building

This repo uses a superbuild to fetch and build SDL, bgfx, and Dear ImGui.
The examples below assume a Unix Makefiles generator and a Release build.

## Superbuild configure

```bash
cmake -B build/release-make -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUPERBUILD=ON \
  -DBGFX_BUILD_EXAMPLES=ON
```

## Build the superbuild

```bash
cmake --build build/release-make
```

## Build bgfx examples

The bgfx examples are built inside the ExternalProject build tree.

```bash
cmake --build third-party/build/src/bgfx-build/Release --target examples
```

## Example output path

Example binaries land here:

```text
third-party/build/src/bgfx-build/Release/cmake/bgfx/example-00-helloworld
```

Run them from the bgfx runtime directory so assets resolve:

```bash
cd third-party/build/src/bgfx/bgfx/examples/runtime
../../../../build/src/bgfx-build/Release/cmake/bgfx/example-00-helloworld
```

## Reminder

This layout can be improved by adding a nicer output directory (for example,
copying or installing example binaries next to `examples/runtime`).
