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

There's a lot (Thanks dpp...).

DPP is used for the discord bot api. I had to modify some of the src files in the source dpp repository to run on windows using cmake.

## Installation & Setup

```bash
# something like...
git clone https://github.com/NathanJColbert/CharacterBot.git
cd CharacterBot
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
./AIBot.exe
```