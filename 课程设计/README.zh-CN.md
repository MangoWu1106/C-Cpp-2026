# 五子棋 · 人机对战

## 开始游戏

在 Windows 10 或 11（64 位）上，双击 `dist/Gobang.exe`。如需移动游戏，请复制整个 `dist` 文件夹。程序从可执行文件旁的 `assets` 文件夹加载中文字体。

默认执黑，难度为普通。进入「游戏设置」可改为执白或选择其他难度。黑棋先手，横、竖或斜线连成五子及以上即获胜。双方均无禁手。

- 鼠标移到空交叉点可预览棋子，点击落子。也可用方向键移动棋盘焦点，按 Enter 落子。
- 按 Tab 或 Shift + Tab 在棋盘与按钮之间切换焦点，按 Enter 操作。
- 「悔棋」撤销你最近一次落子。如果电脑已应答，会一起撤销电脑的应答。对局结束后可选择「再来一局」。
- 重新开始、返回菜单或退出时，若会丢失进行中的棋局，界面会先要求确认。

## 用 VS Code 构建

在 VS Code 中打开本项目，安装 CMake 3.24 或以上版本、Ninja 和支持 C17 的 Windows 编译器。以下命令已用 MSYS2 UCRT64 GCC 构建验证。将 GCC（如 `C:\msys64\ucrt64\bin`）、CMake 和 Ninja 加入 PATH 后，在项目根目录的终端运行。

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix dist --component Game
```

首次配置会下载已固定版本和校验值的 raylib 5.5。已有解压后的官方源码时，可在配置命令中指定 `-DFETCHCONTENT_SOURCE_DIR_RAYLIB=绝对路径`。运行游戏无需联网。

## 源码与难度

| 文件 | 职责 |
| --- | --- |
| `game.c` | 棋盘、规则、历史与悔棋 |
| `ai.c` | 棋形评分、候选点、Minimax、α–β 剪枝及迭代加深 |
| `search_worker.c` | 后台搜索、棋盘副本、取消与结果交接 |
| `ui.c` | 中文界面、缩放布局、鼠标与键盘输入 |
| `main.c` | 菜单与对局流程、界面和搜索的协调 |
| `assets.c` | 读取可执行文件旁的字体资源 |

| 难度 | 最大搜索层数 | 时间预算 |
| --- | ---: | ---: |
| 简单 | 2 | 200 毫秒 |
| 普通 | 4 | 800 毫秒 |
| 困难 | 8 | 2000 毫秒 |

一层表示一方走一步。实际耗时随局面变化，有立即胜着时可提前结束。运行 `ctest --test-dir build -V` 可查看固定局面的深度、节点数、耗时与剪枝对照。

在开发电脑的一个 16 子中盘测试中，简单、普通、困难分别完成了 2、4、8 层，访问 40、285、81,663 个节点。另一固定局面的同树搜索得分均为 12,173，α–β 搜索访问了 45 个节点，未剪枝的 Minimax 访问了 156 个节点。

在同一四步开局（H8、I9、G8、H7）上交换执子，普通两次战胜简单，困难分别两次战胜普通和简单。记录到的最长困难搜索用时约 2000 毫秒，采用了已完整搜索到第 7 层的合法落点。

## 资源许可

程序静态链接 [raylib 5.5](https://github.com/raysan5/raylib/releases/tag/5.5) 和它内置的 GLFW。许可文件随包放在 `assets` 中。

界面使用 [Noto Sans CJK SC Regular](https://github.com/notofonts/noto-cjk/releases/tag/Sans2.004)，按 SIL Open Font License 1.1 分发，详见 `assets/OFL.txt`。
