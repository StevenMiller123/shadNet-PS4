<!--
SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# shadNet PS4

This is a GoldHEN plugin designed to bring shadNet to jailbroken PS4s. This is still extremely early in development, expect bugs.

## Build instructions (Windows)

TODO

## Build instructions (Linux)

This project relies on the [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) and [GoldHEN Plugins SDK](https://github.com/GoldHEN/GoldHEN_Plugins_SDK).

### Setting up the OpenOrbis PS4 Toolchain
Go to https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/actions, click on the top-most "Build OpenOrbis Toolchain" option, then scroll to the bottom and click the "toolchain-llvm-18" option.
From there, extract the toolchain, and set the OO_PS4_TOOLCHAIN environment variable to the "PS4Toolchain folder in the extracted contents. 

### Compiling the GoldHEN Plugins SDK
```bash
git clone https://github.com/GoldHEN/GoldHEN_Plugins_SDK
cd ./GoldHEN_Plugins_SDK
make
export GOLDHEN_SDK=$(PWD)
```

### Compiling this project
To compile, just run
```bash
git clone https://github.com/StevenMiller123/shadNET-PS4.git
cd ./shadNET-PS4
make
```

After that, the compiled GoldHEN plugin should be in ./bin/plugins/prx_final.
Copy the .prx there to your PS4, typically to the console's /data/GoldHEN/plugins directory. Then go to /data/GoldHEN and open plugins.ini.
Add the following somewhere to the file, where [CUSAXXXXX] is the game's serial number. 
Alternatively, replace [CUSAXXXXX] with [default] if you want the plugin to load for all installed apps (not recommended).
```
[CUSAXXXXX]
/data/GoldHEN/plugins/shadnet-ps4-plugin.prx
```