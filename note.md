### lldb 提示 python310.dll 找不到

- 先安装一个 python310, windows store 就可以
- 命令行执行 `I:\a\learnvulkan\build>set PYTHONPATH=C:\Program Files\WindowsApps\PythonSoftwareFoundation.Python.3.10_3.10.3056.0_x64__qbz5n2kfra8p0\lib`
- 再执行 lldb 即可

### 想使用 clangd 作为代码提示分析工具
- 安装 clangd 的插件
- 需要 compile_commands.json, 这个文件 cmake 可以生成, 但是有条件:
  - 需要传递 -DCMAKE_EXPORT_COMPILE_COMMANDS=1
  - 只能生成 makefile 和 ninjia 时生效, 意味着对 visual studio sln 无效
  - 需要先在 vscode 中使用 cmake: select kit 到 clang, 生成makefile并编译一次获得json文件后, 再切换到 vs.
  - 当然也可以直接使用 clang 编译项目


### Vulkan 的坐标系
- 左手坐标系
- Y-up, Z-forward, X-right
- Camera 看 +z 方向