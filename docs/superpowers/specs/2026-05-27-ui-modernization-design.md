# UI 视觉现代化 — 设计规格

## 目标

对"墨迹"桌面应用进行全面视觉现代化升级，以现代极简主义为设计方向，保持现有功能逻辑不变，仅改造视觉层（QSS 样式表 + 少量代码调整）。

## 设计方向

- **视觉语言**：现代极简 (Modern Minimalist)
- **配色主题**：墨蓝 · 冷静学术 — 深藏青顶栏 + 紫蓝强调色
- **组件风格**：精致通透 — 大间距、大圆角、舒适视觉

---

## 全局设计语言

### 色彩系统

| Token | 色值 | 用途 |
|-------|------|------|
| `--bg-topbar` | `#1e293b` | 菜单栏背景 |
| `--fg-topbar` | `#e2e8f0` | 菜单栏文字 |
| `--fg-topbar-dim` | `#94a3b8` | 菜单栏次级文字 |
| `--bg-page` | `#f5f7fa` | 主窗口底色 |
| `--bg-card` | `#ffffff` | 面板/列表/详情区背景 |
| `--border-card` | `#e2e8f0` | 面板边框/分割线 |
| `--accent-primary` | `#1e40af` | 主按钮背景 (blue-800) |
| `--accent-primary-hover` | `#1e3a8a` | 主按钮悬停 (blue-900) |
| `--accent-secondary` | `#4338ca` | 选中态文字/链接 (indigo-700) |
| `--bg-selected` | `#eef2ff` | 列表项选中背景 (indigo-50) |
| `--fg-danger` | `#ef4444` | 删除按钮文字 (red-500) |
| `--border-danger` | `#fecaca` | 删除按钮边框 (red-200) |
| `--fg-text` | `#1e293b` | 主文字 |
| `--fg-text-secondary` | `#475569` | 次级文字 |
| `--fg-muted` | `#64748b` | 辅助文字/元信息 |
| `--fg-placeholder` | `#94a3b8` | 占位文字 |
| `--bg-item` | `#f8fafc` | 列表项默认背景 |
| `--bg-item-hover` | `#f1f5f9` | 列表项悬停背景 |
| `--dot-warning` | `#f59e0b` | 未提交变更指示点 |

### 间距系统 (基于 4px 网格)

| Token | 值 | 应用场景 |
|-------|-----|---------|
| `space-xs` | 4px | 列表项间距、紧凑元素 |
| `space-sm` | 6px | 按钮组间距、标签间距 |
| `space-md` | 8px | 面板内边距、元素间 |
| `space-lg` | 12px | 面板间距、section 间距 |
| `space-xl` | 16px | 面板 padding |

### 圆角系统

| Token | 值 | 应用场景 |
|-------|-----|---------|
| `radius-sm` | 6px | 小按钮、列表项、输入框、菜单项 |
| `radius-md` | 8px | 普通按钮、标签 |
| `radius-lg` | 10px | 面板卡片、详情区 |
| `radius-xl` | 12px | 主面板容器、弹窗 |

### 阴影系统

| Token | 值 | 应用 |
|-------|-----|------|
| `shadow-card` | `0 1px 3px rgba(0,0,0,0.04)` | 面板卡片默认 |
| `shadow-hover` | `0 2px 8px rgba(0,0,0,0.08)` | 悬停状态 |
| `shadow-menu` | `0 4px 12px rgba(0,0,0,0.10)` | 下拉菜单 |

### 字体

- 字体栈：`-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif`
- 主字号：14px（正文），15px（小标题），13px（辅助信息），12px（元信息/代码）
- Commit 详情代码区：`"Cascadia Code", "Consolas", monospace`

---

## 逐区域设计

### 1. 菜单栏

```
┌──────────────────────────────────────────────────────────┐
│  墨迹    文件(F)   章节(C)   远程(R)   AI(A)         ─ ✕  │
└──────────────────────────────────────────────────────────┘
```

- 背景 `--bg-topbar`，文字 `--fg-topbar`，字号 15px bold（brand）/ 14px（菜单项）
- 菜单项 padding：6px 10px，圆角 `radius-sm`，hover 背景 `rgba(255,255,255,0.1)`
- 下拉菜单：白底 `--bg-card`，圆角 `radius-md`，`shadow-menu`，菜单项 hover `--bg-item-hover`
- 分隔线：`--border-card`
- **不改动**：菜单项内容、快捷键绑定、existing enabled/disabled 逻辑

### 2. 左侧章节面板

```
┌──────────────────────────┐
│  章节列表                │  ← 15px bold, --fg-text
│                          │
│  ┌── ch01 - 引言 ──────┐ │  ← bg #f8fafc, 圆角 8px, 间距 6px
│  │                     │ │
│  │  ch02 - 方法        │ │
│  │                     │ │
│  ▎ ch03 - 实验结果 ●   │ │  ← 选中：bg #eef2ff, 左侧3px #4338ca 色条
│  └─────────────────────┘ │      未提交：右侧橙色圆点 (6px, #f59e0b)
│                          │
│  [新建] [重命名] [删除]   │  ← 主/次/危险按钮
└──────────────────────────┘
```

- 每个章节项为独立圆角卡片（bg `--bg-item`，圆角 `radius-md`，margin 6px）
- 选中态：bg `--bg-selected`，左侧 3px `--accent-secondary` 竖条，文字 `--accent-secondary`
- 未提交标记：章节名右侧 6px 橙色圆点 `--dot-warning`，hover tooltip 提示（替代当前红色文字+后缀）
- 按钮：
  - 新建：填充 `--accent-primary`，白字
  - 重命名：白底 + `--border-card` 描边，`--fg-text-secondary` 文字
  - 删除：白底 + `--border-danger` 描边，`--fg-danger` 文字
- **不改动**：按钮 enabled/disabled 逻辑（无选中章节时重命名/删除 disabled，无工作区时新建 disabled）
- 面板背景 `--bg-card`，边框 `--border-card`

### 3. 右侧 Commit 面板

```
┌──────────────────────────────────────────────┐
│  Commit 历史          筛选章节: [全部  ▾]     │
├──────────────────────────────────────────────┤
│  当前分支: [main  ▾]  [新建分支] [合并到当前]  │
├──────────────────────────────────────────────┤
│  ┌ [1] 05-20 10:30  ch01: 新建引言 ────────┐ │
│  │ [2] 05-20 14:22  更新内容              │ │
│  ▎ [3] 05-21 09:15  ch02: 新增方法        │ │  ← 选中 #eef2ff
│  │ [4] 05-22 16:40  ch03: 回退至 [2]  🔴  │ │  ← 非main分支独有(红色)
│  └─────────────────────────────────────────┘ │
│                                              │
│  [提交变更]    [回退章节到此版本]              │
├──────────────────────────────────────────────┤
│  Commit 详情                                 │
│  ┌────────────────────────────────────────┐  │
│  │  作者: An Hao     时间: 2026-05-21...  │  │
│  │  Message: ch02: 新增参考方法            │  │
│  │  涉及文件: main.tex, images/fig1.png   │  │
│  └────────────────────────────────────────┘  │
└──────────────────────────────────────────────┘
```

- 标题行：15px bold，筛选下拉框：bg `--bg-item`，圆角 `radius-sm`，字号 13px
- 分支栏：下拉框 min-width 200px，"新建分支"为描边次按钮，"合并到当前"为描边次按钮
- Commit 列表项：圆角 `radius-md` 卡片风格，间距 5px，默认 bg `--bg-item`，选中 bg `--bg-selected`
- **不改动**：CommitDelegate 的序号加粗逻辑、非 main 分支独有 commit 红色标识（`#cf222e`）、回退至 hash 的动态序号翻译
- 提交按钮：填充 `--accent-primary`，白字
- 回退按钮：白底 + `--accent-primary` 描边，`--accent-primary` 文字，disabled 时灰色
- Commit 详情区：白底卡片 `--bg-card`，HTML 表格渲染（保持现有结构），表格行高增加、边框色改为 `--border-card`
- **不改动**：文件链接点击打开逻辑

### 4. 状态栏

```
┌──────────────────────────────────────────────────────────┐
│  ✓ 工作区: ~/thesis  |  分支: main  |  已全部提交  |  章节: 3  │
└──────────────────────────────────────────────────────────┘
```

- 背景 `--bg-card`，顶部 1px `--border-card` 分隔线
- 文字 `--fg-muted`，字号 13px
- 有未提交变更时：左侧橙色圆点 + "有未提交变更" 文字
- **不改动**：状态栏信息内容、定时刷新逻辑

### 5. 弹窗统一规范

所有 QDialog（AI 助手、环境配置、QInputDialog、QMessageBox）：

- 最小宽度 480px，圆角 `radius-xl`
- 标题 16px bold，内容 14px
- QLineEdit：白底，border `--border-card`，圆角 `radius-md`，padding 8px 12px，focus 时 border 变 `--accent-secondary`
- QPushButton：统一使用主按钮/次按钮/危险按钮三套样式
- QMessageBox：自定义样式按钮，继承全局按钮规范

### 6. AI 对话框

- 聊天区 bg `--bg-item`，圆角 `radius-lg`
- 用户气泡：右对齐，蓝底白字（bg `--accent-primary`，文字 `#ffffff`），圆角 12px 12px 4px 12px
- AI 气泡：左对齐，白底（bg `--bg-card`，border `--border-card`），圆角 12px 12px 12px 4px
- 输入框：多行，底部固定，与发送按钮同行
- 发送按钮：填充 `--accent-primary`
- API Key 区域：可折叠（QToolButton 切换显隐）
- **不改动**：DeepSeek API 调用逻辑、消息累积逻辑、Markdown 渲染逻辑

### 7. 环境配置对话框

- 步骤式布局，序号圆圈标识
- Git 安装状态：已安装 = 绿色 ✓，未安装 = 灰色 ○
- 输入框统一样式（见弹窗规范）
- **不改动**：安装/配置逻辑、QSettings 读写

---

## 改动范围

### 需要修改的文件

| 文件 | 改动内容 |
|------|---------|
| `src/main.cpp` | 重写 GLOBAL_STYLE QSS 样式表 |
| `src/chapterlistmodel.cpp` | 修改 data() 将未提交标记改为特殊 role，不再在 DisplayRole 中追加后缀 |
| `src/commitmodel.cpp` | 调整 CommitDelegate paint() 适配新卡片风格 |
| `src/mainwindow.cpp` | 调整 initBranchBar() 中分支栏控件样式类名；可能需要少量代码调整以适配新 QSS |
| `src/aidialog.cpp` | 调整气泡渲染 HTML 结构 |
| `src/environmentsetupdialog.cpp` | 微调布局适配新样式 |
| `ui/mainwindow.ui` | 可能需要微调 widget objectName 以匹配新 QSS 选择器 |

### 不需要修改的文件

- `src/gitmanager.cpp` — 纯逻辑，不改
- `include/gitmanager.h` — 不改
- `src/filewatcher.cpp` — 不改
- `include/filewatcher.h` — 不改
- `src/commitdetailwidget.cpp` — 仅 QSS 控制外观，不改代码
- `include/` 目录下所有头文件 — 除 mainwindow.h 可能需要少量成员变量调整外不改

---

## 约束条件

1. **功能逻辑零改动**：所有现有功能（章节 CRUD、commit 提交回退、分支管理、文件监控、AI 对话）行为完全不变
2. **按钮启用/禁用逻辑不变**：保留现有的 enabled/disabled 条件判断
3. **Commit 红色标识逻辑不变**：保留 `CommitDelegate::s_branchOnlyHashes` 和 `s_currentBranch` 静态变量机制
4. **回退至 hash 的序号翻译不变**：保留 CommitModel::data() 和 CommitDetailWidget::resolveMessage() 中的动态替换逻辑
5. **QSS 全部集中在 main.cpp**：不创建额外的 .qss 文件，避免资源管理复杂度
