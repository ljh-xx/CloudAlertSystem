# Git 速查表

## 📖 目录
- [基础操作](#基础操作)
- [分支操作](#分支操作)
- [远程操作](#远程操作)
- [撤销操作](#撤销操作)
- [查看信息](#查看信息)
- [协作流程](#协作流程)

---

## 基础操作

### 配置
```bash
# 配置用户名
git config --global user.name "你的名字"

# 配置邮箱
git config --global user.email "你的邮箱"

# 查看配置
git config --global --list
```

### 初始化与克隆
```bash
# 初始化仓库
git init

# 克隆远程仓库
git clone https://github.com/用户名/仓库名.git

# 克隆到指定目录
git clone https://github.com/用户名/仓库名.git my-project
```

### 提交更改
```bash
# 查看状态
git status

# 查看修改
git diff

# 添加文件到暂存区
git add 文件名
git add .                  # 添加所有文件
git add *.cpp              # 添加所有 cpp 文件

# 提交
git commit -m "提交信息"

# 添加并提交（已追踪的文件）
git commit -am "提交信息"
```

---

## 分支操作

### 查看分支
```bash
# 查看本地分支
git branch

# 查看所有分支（包括远程）
git branch -a

# 查看远程分支
git branch -r
```

### 创建与切换
```bash
# 创建分支
git branch 分支名

# 创建并切换到新分支
git checkout -b 分支名
git switch -c 分支名       # Git 2.23+

# 切换分支
git checkout 分支名
git switch 分支名          # Git 2.23+

# 切换到上一个分支
git checkout -
```

### 删除分支
```bash
# 删除已合并的分支
git branch -d 分支名

# 强制删除分支（未合并也删除）
git branch -D 分支名

# 删除远程分支
git push origin --delete 分支名
```

### 合并分支
```bash
# 合并分支到当前分支
git merge 分支名

# 合并但不自动提交
git merge --no-commit 分支名

# 中止合并
git merge --abort
```

---

## 远程操作

### 远程仓库
```bash
# 查看远程仓库
git remote -v

# 添加远程仓库
git remote add origin https://github.com/用户名/仓库名.git

# 修改远程仓库地址
git remote set-url origin https://github.com/用户名/新仓库名.git

# 移除远程仓库
git remote remove origin
```

### 拉取与推送
```bash
# 拉取远程代码并合并
git pull origin main

# 仅拉取，不合并
git fetch origin

# 推送到远程
git push origin 分支名

# 推送所有分支
git push origin --all

# 推送标签
git push origin --tags
```

---

## 撤销操作

### 撤销工作区修改
```bash
# 撤销未暂存的修改
git checkout -- 文件名
git restore 文件名           # Git 2.23+

# 撤销所有未暂存的修改
git checkout .
```

### 撤销暂存区
```bash
# 从暂存区移除（保留工作区修改）
git reset HEAD 文件名
git restore --staged 文件名  # Git 2.23+

# 撤销所有暂存
git reset HEAD .
```

### 撤销提交
```bash
# 撤销上一次提交（保留更改）
git reset --soft HEAD~1

# 撤销上一次提交（丢弃更改）
git reset --hard HEAD~1

# 撤销到指定提交
git reset --hard commit-id

# 撤销但保留更改在暂存区
git reset --mixed HEAD~1
```

### 撤销合并
```bash
# 合并冲突时中止
git merge --abort

# 撤销已合并的提交
git revert -m 1 commit-id
```

---

## 查看信息

### 查看提交历史
```bash
# 查看提交历史
git log

# 单行显示
git log --oneline

# 图形化显示
git log --graph --oneline --all

# 显示最近 N 条
git log -n 10

# 显示文件变更
git log --stat

# 搜索提交信息
git log --grep="关键词"
```

### 查看差异
```bash
# 查看工作区与暂存区差异
git diff

# 查看暂存区与上一次提交差异
git diff --cached
git diff --staged

# 查看工作区与上一次提交差异
git diff HEAD

# 查看两个提交之间的差异
git diff commit1 commit2

# 查看特定文件的差异
git diff 文件名
```

### 查看文件状态
```bash
# 查看详细状态
git status

# 简洁状态
git status -s
```

---

## 协作流程

### 完整开发流程
```bash
# 1. 拉取最新代码
git checkout main
git pull origin main

# 2. 创建功能分支
git checkout -b feature/新功能

# 3. 开发...
# 修改文件

# 4. 提交
git add .
git commit -m "feat: 添加新功能"

# 5. 推送到远程
git push origin feature/新功能

# 6. GitHub 上创建 PR

# 7. 合并后清理
git checkout main
git pull origin main
git branch -d feature/新功能
git push origin --delete feature/新功能
```

### 解决冲突
```bash
# 1. 拉取 main
git checkout main
git pull origin main

# 2. 切换到你的分支
git checkout feature/你的分支

# 3. 合并 main
git merge main

# 4. 打开冲突文件，手动解决

# 5. 标记解决
git add .

# 6. 完成合并
git commit

# 7. 推送
git push origin feature/你的分支
```

### 更新你的分支
```bash
# 方式 1：合并
git checkout main
git pull origin main
git checkout feature/你的分支
git merge main

# 方式 2：变基（更清晰的历史）
git checkout main
git pull origin main
git checkout feature/你的分支
git rebase main
```

---

## 常用技巧

### 暂存当前工作
```bash
# 暂存当前工作
git stash

# 查看暂存列表
git stash list

# 恢复最近一次暂存
git stash pop

# 恢复指定暂存
git stash apply stash@{0}

# 删除暂存
git stash drop stash@{0}

# 清空所有暂存
git stash clear
```

### 标签
```bash
# 创建标签
git tag v1.0.0

# 创建带注释的标签
git tag -a v1.0.0 -m "版本 1.0.0"

# 查看标签
git tag

# 推送标签
git push origin v1.0.0

# 推送所有标签
git push origin --tags

# 删除本地标签
git tag -d v1.0.0

# 删除远程标签
git push origin --delete v1.0.0
```

### 查看特定提交
```bash
# 查看提交详情
git show commit-id

# 查看提交中的文件列表
git show --name-only commit-id

# 查看提交中的特定文件
git show commit-id:文件名
```

---

## 实用别名（可选）

在 `~/.gitconfig` 中添加：

```ini
[alias]
    co = checkout
    br = branch
    ci = commit
    st = status
    unstage = reset HEAD --
    last = log -1 HEAD
    lg = log --graph --oneline --all --decorate
```

使用方式：
```bash
git co main           # git checkout main
git br                # git branch
git ci -m "msg"       # git commit -m "msg"
git st                # git status
git lg                # 漂亮的日志
```

---

## 帮助

```bash
# 查看命令帮助
git help 命令名
git 命令名 --help

# 查看 Git 版本
git --version
```

---

需要了解更多？访问 [Git 官方文档](https://git-scm.com/doc)
