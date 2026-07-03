# 协作开发指南

欢迎参与 CloudAlertSystem 项目的开发！本文档将帮助你快速上手项目协作。

## 📋 目录

- [开发环境准备](#开发环境准备)
- [Git 工作流](#git-工作流)
- [分支策略](#分支策略)
- [提交规范](#提交规范)
- [Pull Request 流程](#pull-request-流程)
- [代码审查](#代码审查)

---

## 开发环境准备

### 1. 安装必要工具

- **Visual Studio 2022** (v145 工具集)
- **Git** (最新版本)
- **GitHub Desktop** (可选，图形化界面)

### 2. 配置 Git

```bash
# 配置用户信息
git config --global user.name "你的名字"
git config --global user.email "你的邮箱"

# 查看配置
git config --global --list
```

### 3. 克隆仓库

```bash
# 克隆远程仓库到本地
git clone https://github.com/你的用户名/CloudAlertSystem.git

# 进入项目目录
cd CloudAlertSystem

# 查看远程仓库
git remote -v
```

### 4. 同步最新代码

```bash
# 拉取远程最新代码
git pull origin main
```

---

## Git 工作流

我们使用 **GitHub Flow** 工作流，简单高效。

### 完整开发流程

```
1. 拉取最新代码 → 2. 创建功能分支 → 3. 开发 → 4. 提交 → 5. 推送到远程 → 6. 创建 PR → 7. 代码审查 → 8. 合并
```

### 详细步骤

#### 1. 确保本地代码是最新的

```bash
# 切换到 main 分支
git checkout main

# 拉取远程最新代码
git pull origin main
```

#### 2. 创建新的功能分支

```bash
# 从 main 创建新分支
git checkout -b feature/你的功能名称

# 示例
git checkout -b feature/email-optimization
git checkout -b fix/bug-login
git checkout -b docs/update-readme
```

#### 3. 进行开发

在 Visual Studio 中打开项目，进行代码修改。

#### 4. 查看和提交更改

```bash
# 查看更改状态
git status

# 查看具体修改
git diff

# 添加修改的文件
git add .
# 或者添加特定文件
git add Server/Notifier.cpp

# 提交更改（请遵循提交规范）
git commit -m "feat: 优化邮件发送模块，添加重试机制"
```

#### 5. 推送到远程仓库

```bash
# 推送分支到远程
git push origin feature/你的功能名称

# 示例
git push origin feature/email-optimization
```

#### 6. 创建 Pull Request

1. 访问你的 GitHub 仓库页面
2. 点击 **Compare & pull request** 按钮
3. 填写 PR 描述（使用模板）
4. 点击 **Create pull request**

#### 7. 代码审查与合并

- 等待项目维护者审查
- 根据反馈进行修改（如果需要）
- 审核通过后，由维护者合并到 main 分支

#### 8. 清理分支（合并后）

```bash
# 切换到 main
git checkout main

# 拉取最新代码
git pull origin main

# 删除本地分支
git branch -d feature/你的功能名称

# 删除远程分支
git push origin --delete feature/你的功能名称
```

---

## 分支策略

### 分支命名规范

| 分支类型 | 前缀 | 示例 | 说明 |
|---------|------|------|------|
| 功能开发 | `feature/` | `feature/user-auth` | 新功能开发 |
| Bug 修复 | `fix/` | `fix/login-error` | 修复 Bug |
| 文档更新 | `docs/` | `docs/update-readme` | 更新文档 |
| 性能优化 | `perf/` | `perf/db-optimize` | 性能优化 |
| 重构 | `refactor/` | `refactor/code-structure` | 代码重构 |
| 测试 | `test/` | `test/add-unit-tests` | 添加测试 |

### 主分支

- **main**: 稳定版本，随时可部署
- 所有合并到 main 的代码都必须经过代码审查

---

## 提交规范

### Commit Message 格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type 类型

| 类型 | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | 修复 Bug |
| `docs` | 文档更新 |
| `style` | 代码格式调整（不影响功能） |
| `refactor` | 重构 |
| `perf` | 性能优化 |
| `test` | 测试相关 |
| `chore` | 构建/工具链相关 |

### 示例

```bash
# 新功能
git commit -m "feat(notifier): 添加邮件发送重试机制"

# 修复 Bug
git commit -m "fix(ctp): 修复行情断线重连问题"

# 文档
git commit -m "docs: 更新 README，添加协作指南"

# 重构
git commit -m "refactor(db): 重构数据库访问层"
```

---

## Pull Request 流程

### 创建 PR 前检查清单

- [ ] 代码已在本地编译通过，无错误
- [ ] 已测试功能正常
- [ ] Commit Message 符合规范
- [ ] 已同步 main 分支的最新代码，无冲突

### PR 描述模板

创建 PR 时，请使用以下模板：

```markdown
## 变更类型
- [ ] 新功能 (feat)
- [ ] Bug 修复 (fix)
- [ ] 文档更新 (docs)
- [ ] 代码重构 (refactor)
- [ ] 性能优化 (perf)
- [ ] 其他

## 变更描述
请简要描述本次变更的内容...

## 相关 Issue
Closes #123

## 测试方式
- [ ] 单元测试
- [ ] 手动测试
- [ ] 无需测试

## 截图（如有）
```

### PR 审查要点

1. 代码质量和可读性
2. 是否符合项目架构
3. 是否有充分的测试
4. Commit 历史是否清晰

---

## 代码审查

### 审查者职责

- 及时审查分配给自己的 PR
- 给出具体、建设性的反馈
- 确保代码质量和项目一致性

### 审查建议

- 使用 GitHub 的"建议更改"功能
- 保持友善和尊重
- 关注重要问题，不拘泥于细节
- 优先考虑架构和可维护性

---

## 常见问题

### Q: 如何解决代码冲突？

```bash
# 1. 切换到你的分支
git checkout feature/你的分支

# 2. 拉取 main 最新代码
git fetch origin main

# 3. 合并 main 到你的分支
git merge origin/main

# 4. 解决冲突文件
# 5. 标记为已解决
git add .

# 6. 继续合并
git commit

# 7. 推送到远程
git push origin feature/你的分支
```

### Q: 如何撤销上一次提交？

```bash
# 保留更改，仅撤销 commit
git reset --soft HEAD~1

# 完全撤销，丢弃更改
git reset --hard HEAD~1
```

### Q: 如何同步其他开发者的分支？

```bash
# 添加其他开发者的仓库为远程源
git remote add other-dev https://github.com/其他开发者/CloudAlertSystem.git

# 拉取其他开发者的分支
git fetch other-dev
git checkout -b other-feature other-dev/feature-xxx
```

---

## 联系方式

如有问题，请：
- 提交 Issue 讨论
- 联系项目维护者
- 在 PR 中讨论

---

感谢你的贡献！ 🎉
