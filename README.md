<!--
SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# shadNet PS4

This is a GoldHEN plugin designed to bring shadNet to jailbroken PS4s. This is still extremely early in development, expect bugs.

## Compiling this project

This project relies on the [OpenOrbis PS4 Toolchain](https://github.com/StevenMiller123/OpenOrbis-PS4-Toolchain).

### Setting up the OpenOrbis PS4 Toolchain
Download the toolchain from https://github.com/StevenMiller123/OpenOrbis-PS4-Toolchain/releases/latest/download/openorbis_toolchain.tar.gz, \
then extract the toolchain and set the OO_PS4_TOOLCHAIN environment variable to the "PS4Toolchain" folder in the extracted contents.

### Windows instructions

#### Building the plugin

```pwsh
git clone https://github.com/StevenMiller123/shadNet-PS4.git --recursive
cd ./shadNet-PS4
cmake -S . -B build -G Ninja
cmake --build build --parallel $env:NUMBER_OF_PROCESSORS
```

### Linux instructions

#### Building the plugin

```bash
git clone https://github.com/StevenMiller123/shadNet-PS4.git --recursive
cd ./shadNet-PS4
cmake -S . -B build
cmake --build build --parallel $(nproc)
```

## Installing the plugin

After that, the compiled GoldHEN plugin should be in ./bin/plugins/prx_final.
Copy the .prx from there to your PS4. \
For GoldHEN users, plugins typically belong in the /data/GoldHEN/plugins directory. \
You'll also need to go to /data/GoldHEN and add an entry in plugins.ini for the shadNet-PS4 plugin.

```
[CUSAXXXXX]
/data/GoldHEN/plugins/shadNet-PS4.prx
```
Add that entry to the ini, where [CUSAXXXXX] is the game's serial number. \
Alternatively, replace [CUSAXXXXX] with [default] if you want the plugin to load for all installed apps (not recommended).
