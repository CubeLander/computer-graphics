# Build and preview on the development Mac

Use when compiling, running or capturing this native OpenGL project.

Observed on 2026-10-21: the selected full Xcode toolchain refused `xcrun`
because its license had not been accepted. The independently installed Command
Line Tools worked with `DEVELOPER_DIR=/Library/Developer/CommandLineTools`.
Use that process-local environment for CMake configure and build if the full
Xcode remains blocked; do not change `xcode-select` globally or accept a license
on the user's behalf. This is local machine state, not a project requirement.

The app uses the actual macOS OpenGL context, not browser WebGL. On this M1 Max,
a native 4.1 Core context reported `4.1 Metal - 90.5` and GLSL 4.10. Keep source
within the OpenGL 3.3 / GLSL 330 contract for the planned Windows build.

Build commands and deterministic capture flags live in README.md. For CLI
captures, run the binary inside the `.app`, not `open --args`, so exit status
and shader/context errors remain observable. A successful compile alone does
not validate a rendered frame: inspect a captured PNG and check process status.

Windows is not locally validated. Keep that boundary explicit until a Windows
build and graphics runtime have actually been exercised.
