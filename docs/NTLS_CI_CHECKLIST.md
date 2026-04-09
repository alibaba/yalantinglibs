# NTLS CI 快速检查清单

## ✅ 如何确认CI使用了你的配置

### 步骤1：确认文件已提交
```bash
# 在你的fork仓库中检查
git log --oneline --all -- .github/workflows/ntls_test.yml
# 应该能看到你提交的记录
```

### 步骤2：在GitHub上检查

#### 方法A：在PR页面查看（推荐）
1. 打开你的PR页面
2. 向下滚动到"Checks"部分
3. **如果看到 "NTLS Test (Tongsuo)" 工作流** ✅
   - 说明CI正在使用你的配置
   - 点击可以查看详细运行情况

#### 方法B：在Actions页面查看
1. 进入**你的fork仓库**（不是主仓库）
2. 点击顶部的 **"Actions"** 标签
3. **如果左侧列表中有 "NTLS Test (Tongsuo)"** ✅
   - 说明配置已生效
4. 点击工作流名称，查看运行历史

#### 方法C：查看CI日志确认
1. 在PR的"Checks"部分，点击 "NTLS Test (Tongsuo)"
2. 点击具体的job（如 "ntls_test_ubuntu"）
3. 展开 "Configure CMake with NTLS" 步骤
4. **在日志中查找 `YLT_ENABLE_NTLS: ON`** ✅
   - 如果看到这个输出，说明使用了你的配置

### 步骤3：验证关键特征

你的CI配置应该包含以下特征，在日志中可以验证：

- ✅ **编译Tongsuo库** - 日志中应该有 "Build and install Tongsuo" 步骤
- ✅ **配置CMake启用NTLS** - 日志中应该有 `-DYLT_ENABLE_NTLS=ON`
- ✅ **编译NTLS示例** - 日志中应该有 "Verify NTLS examples are built" 步骤
- ✅ **使用Tongsuo路径** - 日志中应该有 `/usr/local/tongsuo` 相关路径

## 🔍 如何区分fork的CI和主仓库的CI

### 关键区别

| 特征 | Fork的CI | 主仓库的CI |
|------|---------|-----------|
| 工作流名称 | "NTLS Test (Tongsuo)" | 不存在（除非他们也添加了） |
| 显示位置 | 你的fork仓库的Actions页面 | 主仓库的Actions页面 |
| PR中的显示 | 会显示在你的PR中 | 不会显示（除非主仓库有） |

### 确认方法

1. **查看工作流来源**
   - 在PR的"Checks"部分，每个检查旁边会显示来源
   - 如果显示你的用户名，说明是fork的CI
   - 如果显示主仓库名，说明是主仓库的CI

2. **检查工作流文件位置**
   - Fork的CI：`.github/workflows/ntls_test.yml` 在你的fork中
   - 主仓库的CI：`.github/workflows/ntls_test.yml` 在主仓库中

## 🚨 常见问题

### Q: 看不到CI运行怎么办？

**检查清单：**
- [ ] 确认 `.github/workflows/ntls_test.yml` 文件已提交
- [ ] 确认文件路径正确（`.github/workflows/` 目录）
- [ ] 检查YAML语法是否正确（可以在线验证）
- [ ] 确认你修改的文件符合触发条件（`paths:` 部分）

**解决方法：**
```bash
# 重新提交CI文件
git add .github/workflows/ntls_test.yml
git commit -m "Add NTLS CI workflow"
git push
```

### Q: CI运行但失败了？

**查看日志：**
1. 点击失败的CI运行
2. 查看红色标记的步骤
3. 展开查看详细错误信息

**常见原因：**
- Tongsuo编译失败（网络问题、依赖缺失）
- CMake找不到Tongsuo（路径配置问题）
- 编译错误（代码问题）

### Q: 如何手动触发CI？

1. 进入你的fork仓库
2. 点击 "Actions" 标签
3. 选择 "NTLS Test (Tongsuo)" 工作流
4. 点击 "Run workflow" 按钮
5. 选择分支，点击绿色按钮

### Q: 不确定是否使用了我的配置？

**验证方法：**
1. 查看CI日志中的CMake配置步骤
2. 确认看到 `YLT_ENABLE_NTLS: ON`
3. 确认看到Tongsuo相关的路径和配置
4. 如果看到这些，说明使用了你的配置 ✅

## 📝 快速验证命令

在你的fork仓库中运行：

```bash
# 1. 检查文件是否存在
ls -la .github/workflows/ntls_test.yml

# 2. 检查文件内容
head -20 .github/workflows/ntls_test.yml

# 3. 检查是否已提交
git ls-files .github/workflows/ntls_test.yml

# 4. 查看提交历史
git log --oneline -- .github/workflows/ntls_test.yml
```

## 🎯 成功标志

如果以下所有条件都满足，说明CI正确使用了你的配置：

- ✅ PR的"Checks"部分显示 "NTLS Test (Tongsuo)" 工作流
- ✅ 工作流运行成功（绿色对勾）
- ✅ CI日志中显示 `YLT_ENABLE_NTLS: ON`
- ✅ CI日志中显示Tongsuo编译和安装步骤
- ✅ CI日志中显示NTLS示例编译成功

## 📞 需要帮助？

如果仍然无法确认，可以：
1. 查看完整的CI日志
2. 检查GitHub Actions的运行历史
3. 在主仓库的PR中询问维护者

