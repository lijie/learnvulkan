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

### 关于画面上下翻转
- Vulkan 有着奇怪的NDC坐标, y轴是负的.
- 不同的 sample 有着不同的解决方案
  - 比如 gl_Position = -gl_Position
  - 比如 vulkan sample 加载模型时直接翻转了模型顶点的 y 和法线的 y
  - 还有例子是翻转project投影矩阵
  - vulkan api 1.1 之后可以设置 viewport.height 为负数来实现翻转
[参考这里](https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/)

### 强制 glm 使用右手坐标系
- 在makefile中定义 GLM_FORCE_DEPTH_ZERO_TO_ONE

### sType & pNext
* vulkan 的 struct, 很多都包含两个特殊的字段 `sType` 和 `pNext`
* sType 保存了结构体的类型id, 类似网络协议中的 cmd
* pNext 用于扩展 struct, 当需要对 struct 新增字段时, 为了保持向前兼容, 新增的内容可以用一个新的 struct 描述, 并存储在 pNext 指针中.
```C++
// 一开始 VkDeviceCreateInfo 是这样的:
typedef struct VkDeviceCreateInfo {
    VkStructureType                    sType;
    const void*                        pNext;
    VkDeviceCreateFlags                flags;
    uint32_t                           queueCreateInfoCount;
    const VkDeviceQueueCreateInfo*     pQueueCreateInfos;
    uint32_t                           enabledLayerCount;
    const char* const*                 ppEnabledLayerNames;
    uint32_t                           enabledExtensionCount;
    const char* const*                 ppEnabledExtensionNames;
    const VkPhysicalDeviceFeatures*    pEnabledFeatures;
} VkDeviceCreateInfo;

// 后来我们想在创建逻辑设备时指定 features, 但是出于对兼容的考虑, 不能修改 VkDeviceCreateInfo 了, 于是创建了一个新的 struct:
typedef struct VkPhysicalDeviceFeatures2 {
    VkStructureType             sType;
    void*                       pNext;
    VkPhysicalDeviceFeatures    features;
} VkPhysicalDeviceFeatures2;

// 于是我们创建 VkDeviceCreateInfo 时变成这样:
auto CreateInfo = VkDeviceCreateInfo();
auto Feature = VkPhysicalDeviceFeatures2();
CreateInfo.pNext = &Feature

// 这样我们扩展了 VkDeviceCreateInfo 的功能, 但是也不修改 VkDeviceCreateInfo 本身. 旧代码可以保持兼容, 新代码可以识别新的数据并处理.
```

### Command Pool & Command Buffer

* Command Pool 就是 Command Buffer 的分配器
  * vulkan 中几乎所有的 Pool 都是不建议跨线程使用的. [see here](https://github.com/ARM-software/vulkan_best_practice_for_mobile_developers/blob/master/samples/performance/command_buffer_usage/command_buffer_usage_tutorial.md)

#### 为什么会需要创建多个 Command Buffer ?
* 比如 Sasha Williams 的 vulkan sample, Command Buffer 的数量总是等于 swap chain image count 的数量.
* [参考链接](https://community.khronos.org/t/why-need-create-framebuffers-for-swapchain-count-images/6911)

### Vulkan 一般初始化流程
* 使用 vkCreateInstance 创建 Instance
  * extension 和 layer 在 VkInstanceCreateInfo 中指定
* 使用 vkCreateDebugUtilsMessengerEXT 创建 debug message object
* 使用 vkCreateDevice 创建逻辑设备(logical device)
  * VkDeviceCreateInfo 描述了 logical device 的创建信息
  * 需要使用 vkEnumeratePhysicalDevices 获取所有物理设备(VkPhysicalDevice), 并使用其中一个来创建logical device
  * 创建 logical device 同时也需要场景关联的 queues
    * VkDeviceQueueCreateInfo 描述了需要创建的 queue 信息
    * 最常用的 queue 有 VK_QUEUE_GRAPHICS_BIT 和 VK_QUEUE_COMPUTE_BIT
    * 通过 VkPhysicalDeviceFeatures2 指定需要开启的 feature
  * 为 Graphics queue 创建 Command Pool
    * 使用 vkCreateCommandPool, VkCommandPoolCreateInfo 创建 Command Pool