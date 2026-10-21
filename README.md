# Sun / Earth / Moon

使用 **C++17 + OpenGL 3.3 Core** 实现的日地月运动模型，计算机图形学课程作业。

作业一实现圆形公转轨道、自转、月球轨道倾角和交互观察；**作业二的纹理与光照尚未实现**。
当前球体的经向色带仅用于观察自转，不是纹理贴图或光照。

![日地月全景](docs/screenshots/overview.png)

![地月近景](docs/screenshots/earth-moon.png)

## 构建与运行

需要 CMake 3.24+、Git、C++17 编译器和支持 OpenGL 3.3 的显卡驱动。
首次配置会下载固定版本的 GLFW 和 GLM，需要网络；运行程序不需要网络。

### macOS

使用 Xcode 或 Command Line Tools 提供的 Clang 和 SDK：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
ctest --test-dir build --output-on-failure
open build/sun-earth-moon.app
```

如果完整 Xcode 尚未配置，但已安装独立 Command Line Tools，可以只为本次命令指定工具链，不修改全局设置：

```sh
export DEVELOPER_DIR=/Library/Developer/CommandLineTools
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
```

### Windows

安装 Visual Studio 2022 的“使用 C++ 的桌面开发”工作负载，以及 CMake、Git：

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\sun-earth-moon.exe
```

Windows 构建配置使用静态 GLFW 和 MSVC 运行库，不依赖单独安装 GLFW DLL。
仍需要显卡厂商的 OpenGL 3.3+ 驱动，不能将系统显卡驱动打进应用。
**Windows 尚未实机验证，不能把跨平台源码视为已经验证的 Windows 成品。**

### Linux（可选）

安装 C++ 编译器、CMake、Git 以及 GLFW 所需的系统开发包。例如 Ubuntu 的 X11 路径需要 `xorg-dev`：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGLFW_BUILD_WAYLAND=OFF
cmake --build build -j 8
ctest --test-dir build --output-on-failure
./build/sun-earth-moon
```

Linux 尚未实机验证。

## 操作

| 操作 | 功能 |
| --- | --- |
| 鼠标左键拖动 / 滚轮 | 环绕观察 / 缩放 |
| Space | 暂停、继续 |
| `+` / `-` | 加速、减速（模拟天 / 真实秒） |
| `1` / `2` / `3` | 全景 / 跟随地球 / 俯视 |
| `P` | 显示、隐藏月球轨道平面 |
| `R` | 重置时间、速度和视角（保留暂停状态） |
| F12 | 向当前工作目录写入 `screenshot.png`，覆盖同名文件 |
| Esc | 退出 |

全景适合展示地球公转；跟随地球适合观察月球绕地、倾斜轨道以及自转。
macOS 的 F12 可能被系统快捷键占用，可使用下述截图参数。

## 运动模型与教学比例

采用右手坐标系，Y 轴向上，地球公转轨道位于 XZ 平面。
月球局部轨道绕 X 轴倾斜 **5.145°**，且轨道中心随地球移动，而不是绕世界原点运动：

```text
Earth(t) = (R cos(theta_e), 0, -R sin(theta_e))
Moon(t)  = Earth(t) + RotateX(5.145°) * (r cos(theta_m), 0, -r sin(theta_m))
theta    = 2*pi*t / period
```

- 地球公转周期：365.256 模拟天；月球绕地周期：27.322 模拟天。
- 地球自转周期：0.99727 模拟天，自转轴倾角：23.44°，轴方向在世界空间中固定。
- 月球同步自转：其局部 -X 方向始终朝向地球。
- 太阳示意自转周期：25.4 模拟天；不模拟差异自转。
- 时间由真实帧间隔推进，暂停或改变速度不会改变相对周期。单帧间隔最多计入 0.1 秒，避免窗口阻塞后的巨大跳变。
- 默认 5 模拟天/秒：一年约 73 秒；观看地球自转时建议减速。

**这是圆轨道运动学教学模型，不是引力数值模拟。** 不模拟偏心率、轨道进动或地月质心修正。
球体半径为太阳 1.65、地球 0.63、月球 0.23；轨道半径为地球 10、月球 2.25。
尺寸和距离分别压缩以便观察，不代表真实比例，周期和倾角保留上述近似值。

## 截图与打包

命令行可固定时间、视角并在指定帧数后退出。截图由实际 OpenGL 帧缓冲输出，而非示意图：

```sh
# macOS；Windows / Linux 将程序路径替换为对应可执行文件
build/sun-earth-moon.app/Contents/MacOS/sun-earth-moon \
  --days 48 --view 1 --frames 3 --screenshot docs/screenshots/overview.png
build/sun-earth-moon.app/Contents/MacOS/sun-earth-moon \
  --days 48 --view 2 --frames 3 --screenshot docs/screenshots/earth-moon.png

# 构建完成后打包，产物在 build/ 下
(cd build && cpack -C Release)
```

`--days` 固定模拟时间；省略时正常播放。`--screenshot` 未指定帧数时默认运行 3 帧后退出。
macOS 包为 `.app`，Windows 包为 `.exe`；压缩包同时包含说明和依赖许可。
macOS 当前构建只面向本机架构，不是 Universal Binary；未做 Apple Developer ID 签名或公证，其他 Mac 的 Gatekeeper 可能要求用户确认。

## 源码导览

- `src/orbits.hpp`：轨道、自转与姿态计算，不依赖窗口和 GPU。
- `src/renderer.cpp`：球网格、轨道、星空、文字及 PNG 截图；使用 VAO/VBO 和 GLSL 330 着色器。
- `src/main.cpp`：窗口、时间推进、相机、输入和界面。
- `tests/orbit_tests.cpp`：轨道半径、层级关系、非共面、周期、同步自转和地轴倾角检查。

## 后续：作业二

在现有运动模型上增加球体 UV、地球纹理和太阳位置的光源；必要时加入仅照亮太阳的侧后方辅助灯。
引入纹理时记录来源和可再分发许可，保留作业一版本。

## 许可

原创代码采用 [MIT License](LICENSE)。依赖及资源保留各自许可，详见 [THIRD_PARTY.md](THIRD_PARTY.md)。
