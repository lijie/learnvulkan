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

### 使用 lldb 调试
- 我发现不需要在网上搜索的那么复杂, 写各种配置文件
- 首先启用clangd扩展
- 设置环境变量 PYTHONPATH=C:\Program Files\WindowsApps\PythonSoftwareFoundation.Python.3.10_3.10.3056.0_x64__qbz5n2kfra8p0\lib
- 重启vscode就可以了

### Vulkan 的坐标系
- 右手坐标系
- Y-up, Z-forward, X-right
- Camera 看 +z 方向

### vertex input data

- 顶点数据不一定是 `interleaved`
- 比如:
  ```C++
  struct vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
  };
  vertex[10] = { /* ... */ }
  ```
- 也可能是:
  ```C++
  vec3 position[10] = {};
  vec3 normal[10] = {};
  vec3 uv[10] = {};
  ```
- 大多数vulkan sample都是 `interleaved` 顶点作为例子, 但实践中, 可能 `non-interleaved` 更常见? 比如gltf的数据就是.

[参考这里](https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/vertex_input_data_processing.adoc)

### VK_WHOLE_SIZE

`VK_WHOLE_SIZE` is a special value indicating that the entire remaining length of a buffer following a given offset should be used.

### draw multiple objects
- `vkCmdBindDescriptorSets` 可以设置 uniform 的偏移数组, 对于使用了 dynamic uniform 的情况, 可以对每个 object 的每个 binding 设置不同的偏移读取 uniform memory 中不同的位置

### uniform buffer alignment
- 似乎 uniform buffer struct 必须是 16B 对齐的, 所以引入 vec3 会导致一些很费解的对齐问题. 都使用 vec4/mat4 ?