# Development

## Setting up build system

Create a CMakeUserPresets.json file in the root directory of your project:

```json
{
  "version": 2,
  "configurePresets": [
    {
      "name": "default",
      "inherits": "release",
      "environment": {
        "VCPKG_ROOT": "~/vcpkg"
      }
    }
  ]
}
```

Then run `cmake --preset default` to generate the build system and `cmake --build --preset default` to build the project.

To build debug version, run `cmake --preset debug` and then `cmake --build --preset debug`.
