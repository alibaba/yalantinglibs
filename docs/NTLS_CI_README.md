# NTLS CI 测试说明

本文档说明如何使用GitHub Actions CI来测试NTLS（国密SSL）功能。

## 概述

由于NTLS功能依赖于Tongsuo库（而不是标准的OpenSSL），原仓库的CI配置无法测试NTLS相关功能。因此，我们创建了专门的CI工作流来测试NTLS功能。

## CI工作流

### 文件位置

CI配置文件位于：`.github/workflows/ntls_test.yml`

### 触发条件

该工作流会在以下情况触发：
- 推送到 `main` 或 `develop` 分支
- 创建或更新PR到 `main` 或 `develop` 分支
- 手动触发（workflow_dispatch）
- 当相关文件发生变化时（包括NTLS相关代码、CMake配置、证书文件等）

### 测试内容

CI工作流会执行以下步骤：

1. **安装Tongsuo库**
   - 从源码编译安装Tongsuo（支持NTLS的OpenSSL分支）
   - 启用NTLS、SM2、SM3、SM4等国密算法支持

2. **配置和编译**
   - 使用CMake配置项目，启用SSL和NTLS支持
   - 编译NTLS相关的示例程序（`ntls_server`, `ntls_client`等）

3. **验证**
   - 检查NTLS示例程序是否成功编译
   - 验证Tongsuo库中的NTLS符号
   - 运行基本测试

### 支持的编译器

- GCC
- Clang 17

### 构建模式

- Debug模式

## 使用方法

### 在Fork的仓库中

1. **将CI文件添加到你的fork**
   ```bash
   git add .github/workflows/ntls_test.yml
   git commit -m "Add NTLS CI workflow"
   git push
   ```

2. **创建或更新PR**
   - 当你创建PR时，CI会自动运行
   - 你可以在PR页面看到CI运行状态

3. **查看CI结果**
   - 在PR页面点击"Checks"标签
   - 查看"NTLS Test (Tongsuo)"工作流的运行结果

### 如何确认CI使用了你的配置

#### 方法1：查看PR中的CI运行状态

1. **在PR页面查看**
   - 打开你的PR页面
   - 向下滚动，你会看到"Checks"部分
   - 如果看到"NTLS Test (Tongsuo)"工作流，说明CI正在运行
   - 点击工作流名称可以查看详细信息

2. **检查工作流名称**
   - 如果看到 `NTLS Test (Tongsuo)` 这个工作流名称，说明使用了你的配置
   - 主仓库的CI不会包含这个工作流（除非他们也添加了）

#### 方法2：在Actions页面查看

1. **在你的fork仓库中**
   - 进入你的fork仓库（不是主仓库）
   - 点击顶部的"Actions"标签
   - 在左侧工作流列表中找到"NTLS Test (Tongsuo)"
   - 如果能看到这个工作流，说明配置已生效

2. **查看运行历史**
   - 点击工作流名称
   - 查看运行历史，确认是否有新的运行记录
   - 每次推送代码或更新PR时，应该会触发新的运行

#### 方法3：检查工作流文件是否存在

在你的fork仓库中检查：
```bash
# 在你的fork仓库中运行
ls -la .github/workflows/ntls_test.yml
```

如果文件存在，CI应该会使用它。

#### 方法4：查看CI日志确认

1. **打开CI运行详情**
   - 在PR页面点击"Checks"标签
   - 点击"NTLS Test (Tongsuo)"工作流
   - 点击具体的job（如"ntls_test_ubuntu"）

2. **检查关键步骤**
   - 查看"Build and install Tongsuo"步骤
   - 查看"Configure CMake with NTLS"步骤
   - 如果这些步骤存在且成功，说明使用了你的CI配置

3. **查看CMake配置输出**
   - 在"Configure CMake with NTLS"步骤的日志中
   - 应该能看到 `YLT_ENABLE_NTLS: ON` 的输出
   - 这证明CMake使用了你的配置

#### 方法5：通过GitHub API检查（高级）

如果你想通过API检查，可以使用：
```bash
# 获取工作流列表
curl -H "Authorization: token YOUR_TOKEN" \
  https://api.github.com/repos/YOUR_USERNAME/yalantinglibs/actions/workflows
```

#### 常见问题排查

**问题1：看不到CI运行**
- 确认 `.github/workflows/ntls_test.yml` 文件已提交到你的fork
- 检查文件路径是否正确（`.github/workflows/` 目录）
- 确认文件格式正确（YAML语法）

**问题2：CI运行但失败**
- 查看CI日志中的错误信息
- 检查Tongsuo编译是否成功
- 确认CMake配置是否正确

**问题3：CI没有自动触发**
- 检查工作流的触发条件（`on:` 部分）
- 确认你修改的文件在触发路径中
- 可以手动触发（workflow_dispatch）

**问题4：不确定是否使用了fork的CI还是主仓库的CI**
- Fork的CI会在你的fork仓库的Actions页面显示
- PR中的CI检查会显示来源仓库
- 如果主仓库没有这个工作流，那么运行的肯定是你的配置

### 手动触发

如果需要手动触发CI：

1. **在你的fork仓库中**
   - 进入你的fork仓库（不是主仓库）
   - 点击"Actions"标签
   - 选择"NTLS Test (Tongsuo)"工作流
   - 点击"Run workflow"按钮
   - 选择分支（通常是你的PR分支）
   - 点击绿色的"Run workflow"按钮

2. **在PR中触发**
   - 如果PR已经创建，可以重新推送代码来触发
   - 或者关闭并重新打开PR

## 本地测试

如果你想在本地测试NTLS功能，可以按照以下步骤：

### 1. 安装Tongsuo

```bash
# 克隆Tongsuo仓库
git clone https://github.com/Tongsuo-Project/Tongsuo.git
cd Tongsuo

# 配置和编译
./config --prefix=/usr/local/tongsuo \
         --openssldir=/usr/local/tongsuo/ssl \
         shared \
         -fPIC \
         enable-ntls \
         enable-sm2 \
         enable-sm3 \
         enable-sm4 \
         enable-tls1_3

make -j$(nproc)
sudo make install
sudo ldconfig
```

### 2. 配置环境变量

```bash
export PATH=/usr/local/tongsuo/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/tongsuo/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=/usr/local/tongsuo/lib/pkgconfig:$PKG_CONFIG_PATH
```

### 3. 编译项目

```bash
mkdir build && cd build
cmake .. \
    -DYLT_ENABLE_SSL=ON \
    -DYLT_ENABLE_NTLS=ON \
    -DCMAKE_PREFIX_PATH=/usr/local/tongsuo \
    -DOPENSSL_ROOT_DIR=/usr/local/tongsuo

make -j$(nproc)
```

### 4. 运行示例

```bash
# 运行NTLS服务器示例
./src/coro_rpc/examples/base_examples/ntls_server

# 在另一个终端运行客户端
./src/coro_rpc/examples/base_examples/ntls_client
```

## 关于Fork和PR的说明

### 权限问题

- **你不能直接修改主仓库的CI配置**
- 但是，**你可以在自己的fork中创建CI配置**
- 当你提交PR时，fork中的CI会运行，结果会显示在PR中

### 最佳实践

1. **在fork中创建CI配置**
   - 将`.github/workflows/ntls_test.yml`添加到你的fork
   - 这样每次推送代码时，CI会自动运行

2. **在PR中说明**
   - 在PR描述中说明你添加了NTLS CI测试
   - 提供CI运行结果的链接

3. **与维护者沟通**
   - 如果主仓库维护者希望合并你的CI配置，他们可以接受你的PR
   - 或者，他们可以参考你的CI配置，在主仓库中创建类似的配置

## 故障排除

### CI失败常见原因

1. **Tongsuo编译失败**
   - 检查Tongsuo版本是否可用
   - 确认所有依赖都已安装

2. **CMake找不到Tongsuo**
   - 检查`CMAKE_PREFIX_PATH`和`OPENSSL_ROOT_DIR`设置
   - 确认库文件路径正确

3. **链接错误**
   - 确认`LD_LIBRARY_PATH`包含Tongsuo库路径
   - 检查库文件是否存在

### 调试建议

1. 查看CI日志中的详细错误信息
2. 在本地复现相同的环境进行测试
3. 检查Tongsuo版本兼容性

## 相关资源

- [Tongsuo项目](https://github.com/Tongsuo-Project/Tongsuo)
- [NTLS文档](https://github.com/Tongsuo-Project/Tongsuo/wiki)
- [国密SSL标准](https://www.oscca.gov.cn/)

## 关于测试环境的选择

### 当前CI配置

当前的NTLS CI配置测试了以下环境：
- ✅ Ubuntu 22.04 + GCC
- ✅ Ubuntu 22.04 + Clang 17

### 是否需要测试所有环境？

**简短回答：不需要。** 对于PR测试，测试主要环境就足够了。

### 为什么不需要所有环境？

1. **Tongsuo库的平台支持**
   - Tongsuo主要在Linux平台上使用和测试
   - Linux是NTLS功能的主要部署环境
   - Mac和Windows上的Tongsuo支持可能有限或需要额外配置

2. **PR测试的目的**
   - PR中的CI主要用于验证代码能正常编译和基本功能
   - 不需要覆盖所有可能的部署环境
   - 主仓库的CI会处理其他环境的测试

3. **资源效率**
   - 测试所有环境会消耗大量CI资源
   - 增加CI运行时间，影响开发效率
   - 对于fork的PR，资源可能有限

4. **编译器差异**
   - 当前配置已经测试了GCC和Clang两个主要编译器
   - 这已经覆盖了大部分编译问题
   - MSVC的测试可以在主仓库合并后进行

### 建议的测试策略

#### 最小配置（推荐用于PR）
- ✅ Ubuntu + GCC（主要编译器）
- ✅ Ubuntu + Clang（验证C++标准兼容性）

**理由：**
- 覆盖了主要的Linux部署环境
- 测试了两个主要编译器
- 资源消耗合理
- 足以验证NTLS功能的基本正确性

#### 完整配置（如果资源充足）
如果主仓库维护者希望，可以添加：
- Ubuntu + GCC
- Ubuntu + Clang
- ~~Mac + Clang~~（Tongsuo在Mac上支持有限）
- ~~Windows + MSVC~~（Tongsuo在Windows上支持有限）

**注意：** Mac和Windows上的Tongsuo支持可能需要额外配置，不建议在PR阶段测试。

### 什么时候需要添加更多环境？

1. **主仓库要求**
   - 如果维护者明确要求测试特定环境
   - 如果项目有跨平台部署需求

2. **功能需要**
   - 如果你的NTLS功能有平台特定的代码
   - 如果需要验证特定平台的兼容性

3. **资源充足**
   - 如果你的fork有充足的CI资源
   - 如果测试时间不是问题

### 当前配置是否足够？

**是的，当前配置已经足够用于PR测试：**

✅ **优点：**
- 测试了主要部署平台（Linux）
- 覆盖了两个主要编译器（GCC、Clang）
- 资源消耗合理
- 足以验证NTLS功能的基本正确性

✅ **验证内容：**
- NTLS代码能够编译
- Tongsuo库能够正确链接
- CMake配置正确
- 基本功能正常

### 与维护者沟通

在PR中，你可以说明：
- 当前CI测试了Linux上的GCC和Clang
- 这是NTLS功能的主要部署环境
- 如果需要，可以在主仓库合并后添加其他环境的测试
- 或者，如果维护者需要，可以添加更多环境的测试

### 总结

**对于PR测试：**
- ✅ 测试主要环境（Linux + GCC/Clang）就足够了
- ❌ 不需要测试所有环境（Mac、Windows等）
- ✅ 当前配置已经合理且足够

**如果主仓库合并后：**
- 维护者可以根据需要添加其他环境的测试
- 或者你可以在后续PR中添加更多环境的支持

## 更新日志

- 2025-01-XX: 初始版本，支持Ubuntu 22.04上的GCC和Clang测试

