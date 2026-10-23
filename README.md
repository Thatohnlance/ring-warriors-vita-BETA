# Ring Warriors Vita

A homebrew wrestling game starter for PS Vita, built with VitaSDK + vita2d.

## What is included
- Playable 2D ring match prototype with movement, strikes, grapple, pin attempt, opponent AI, match result screen.
- Six original wrestler names and a simple career purse saved to `ux0:data/RingWarriors/career.dat`.
- CMake project and GitHub Actions workflow that builds a `.vpk` using VitaSDK.
- No copyrighted game assets are included; all ring and wrestler art is drawn from simple shapes.

## Build a VPK
The current chat environment does not have VitaSDK installed, so it cannot compile the native Vita executable here. Use a VitaSDK environment or the included GitHub Actions workflow.

1. Create/push these files to a GitHub repository.
2. Ensure GitHub Actions is enabled.
3. Run **Build PS Vita VPK** from the Actions tab, or push to `main`.
4. Download the `RingWarriors-vpk` artifact from the completed workflow.

## Controls
- D-pad / left stick: move
- Cross: strike
- Square: grapple / throw when close
- Circle: pin attempt when opponent is down
- Start: pause / resume
- Triangle: return to title from result screen

## Important
The supplied `vita_wrestling_prototype.vpk` is only a packaged executable (`eboot.bin`) and `sce_sys/param.sfo`; it does not contain the original C/C++ source or game assets. It cannot be safely modified into a larger game directly. This project is a clean source starting point and does not reuse that executable.
