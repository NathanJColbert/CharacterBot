# AI ChatBot For Discord

## Description

Making a reactive AI ChatBot in voice calls on discord. The build environment is MSYS2 MINGW64. I build using cmake.

## Table of Contents

- [Source Architecture](#source-architecture)
- [Dependencies](#dependencies)
- [Installation & Setup](#installation--setup)
- [Usage](#usage)

## Source Architecture

Running on Windows 10, using MSYS2 MINGW64.

## Dependencies

DPP can be covered by opus, fmt and gcc. You need git (obviously). ffmpeg is a residual library from older versions of this project. I intend to use it eventaully for the audio streaming.

```bash
pacman -Syu
pacman -S \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-fmt \
    mingw-w64-x86_64-opus \
    mingw-w64-x86_64-ffmpeg \
    git
```

DPP is used for the discord bot api. I had to modify some of the src files in the source dpp repository to run on windows using cmake (In the MINGW64 env). So, this project uses a submodule copied from the main DPP branch

## Installation & Setup

```bash
# Clone the repo
git clone --recurse-submodules https://github.com/NathanJColbert/CharacterBot.git
cd CharacterBot

# OR
git clone https://github.com/NathanJColbert/CharacterBot.git
cd CharacterBot
git submodule update --init --recursive

# Build the source
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
./AIBot.exe
```