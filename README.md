![clock_v3_cpp](screenshot.png)

# Environment Variables

Set the `CEREBRAS_API_KEY` environment variable to your Cerebras API key if you want to get clothing advice from the Cerebras API.

# Building

Run `cmake --preset release` to generate the build system and `cmake --build --preset release` to build the project.

To build debug version, run `cmake --preset debug` and then `cmake --build --preset debug`.

# Building for Raspberry Pi

## Preparing cross-compilation environment

Set up `libcurl` with openSSL support on your Raspberry Pi (cpr is gonna link against it).

You need to setup `./pi_sysroot` folder. Connect your RPi SD card to your computer and mount it.
And no, you can't just copy the files from the SD card img file you downloaded from the Raspberry Pi website.
You need the actual files from the SD card you're running on. _Alternatively, you can run `./pi_sysroot.sh <username> <hostname>` to
do the below over the network,_ but plugging in the SD card and mounting it is sometimes faster.

Then run while being inside `/` on the `rootfs` partition (replace `~/clock_v3_cpp/pi_sysroot` with the actual path):

```sh
rsync -avz --rsync-path="sudo rsync" ./lib ~/clock_v3_cpp/pi_sysroot/
rsync -avz --rsync-path="sudo rsync" ./usr/lib ~/clock_v3_cpp/pi_sysroot/usr/
rsync -avz --rsync-path="sudo rsync" ./usr/include ~/clock_v3_cpp/pi_sysroot/usr/
```

then use `sudo symlinks -rc ~/clock_v3_cpp/pi_sysroot` to fix symlinks.

## Cross-compiling

```sh
cmake --preset release-arm
cmake --build --preset release-arm
```

# Attribution

- BellotaText Bold font used in this project is licensed under the [Open Font License](https://openfontlicense.org).
