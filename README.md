# Development

## Dependencies

- [vcpkg](https://github.com/microsoft/vcpkg)

## Setting up build system

Set `VCPKG_ROOT` environment variable to the path of your vcpkg installation:

```sh
export VCPKG_ROOT=~/vcpkg
```

Then run `cmake --preset release` to generate the build system and `cmake --build --preset release` to build the project.

To build debug version, run `cmake --preset debug` and then `cmake --build --preset debug`.
