# UI 视觉现代化 — 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将墨迹应用全面升级为现代极简视觉风格（A1 墨蓝配色 + 精致通透组件），所有功能逻辑不变。

**Architecture:** 核心改动是重写 `main.cpp` 中的 `GLOBAL_STYLE` QSS 全局样式表（占比 ~80%），其余文件做配套调整：章节未提交标记改为橙点、Commit 列表卡片化、AI 聊天气泡重构、环境配置对话框美化。不改动 GitManager、FileWatcher 等纯逻辑层。

**Tech Stack:** Qt 6 Widgets, QSS (Qt Style Sheets), C++17, qmake

---

### Task 1: 重写全局 QSS 样式表

**Files:**
- Modify: `src/main.cpp:6-67` (GLOBAL_STYLE 常量)

这是整个 UI 升级的核心——用新的设计 token 重写 QSS。

- [ ] **Step 1: 将 GLOBAL_STYLE 替换为新样式表**

将 `src/main.cpp` 中第 6 行的 `static const char *GLOBAL_STYLE = R"(...)"` 整个替换为以下内容：

```cpp
static const char *GLOBAL_STYLE = R"(
/* ====== 全局 ====== */
QMainWindow { background-color: #f5f7fa; }
QWidget { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
          font-size: 14px; color: #1e293b; }

/* ====== 菜单栏 ====== */
QMenuBar { background: #1e293b; color: #e2e8f0; padding: 2px 6px; font-size: 14px; }
QMenuBar::item { padding: 6px 10px; border-radius: 6px; }
QMenuBar::item:selected { background: rgba(255,255,255,0.1); }
QMenu { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;
        padding: 4px; font-size: 14px; }
QMenu::item { padding: 6px 24px 6px 12px; border-radius: 6px; }
QMenu::item:selected { background: #f1f5f9; }
QMenu::separator { height: 1px; background: #e2e8f0; margin: 4px 6px; }

/* ====== 面板标题 ====== */
QLabel#chapterTitleLabel, QLabel#historyTitleLabel, QLabel#commitDetailTitle {
  font-size: 15px; font-weight: bold; color: #1e293b; padding: 2px 0; }
QLabel#filterLabel { font-size: 14px; font-weight: bold; color: #1e293b; }

/* ====== 列表视图 —— 章节 & Commit ====== */
QListView { background: transparent; border: none; padding: 2px; outline: none; font-size: 14px; }
QListView::item { padding: 8px 10px; border-radius: 8px; margin: 3px 0;
                  background: #f8fafc; border: none; }
QListView::item:hover { background: #f1f5f9; }
QListView::item:selected { background: #eef2ff; color: #4338ca; }
QListView::item:selected:!active { background: #eef2ff; color: #4338ca; }

/* ====== 下拉框  ====== */
QComboBox { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px;
            padding: 5px 10px; min-width: 60px; font-size: 13px; }
QComboBox#branchCombo { min-width: 200px; }
QComboBox:hover { background: #f1f5f9; }
QComboBox::drop-down { border: none; width: 18px; subcontrol-position: right center; }
QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #e2e8f0;
    border-radius: 8px; padding: 4px; outline: none; font-size: 13px; }
QComboBox QAbstractItemView::item { padding: 6px 10px; border-radius: 6px; min-height: 24px; }
QComboBox QAbstractItemView::item:hover { background: #f1f5f9; }

/* ====== 按钮 ====== */
QPushButton { background-color: #1e40af; color: #ffffff;
              border: none; border-radius: 8px;
              padding: 6px 16px; font-size: 14px; font-weight: 600; }
QPushButton:hover { background-color: #1e3a8a; }
QPushButton:pressed { background-color: #1e3a8a; }
QPushButton:disabled { background-color: #cbd5e1; color: #94a3b8; }

/* 次按钮（描边风格）— 用于重命名、新建分支、合并 */
QPushButton#renameChapterBtn, QPushButton#newBranchBtn,
QPushButton#mergeBtn {
  background-color: #ffffff; color: #475569;
  border: 1px solid #cbd5e1; font-weight: 600;
}
QPushButton#renameChapterBtn:hover, QPushButton#newBranchBtn:hover,
QPushButton#mergeBtn:hover {
  background-color: #f8fafc; border-color: #94a3b8;
}

/* 回退按钮 — 描边主色 */
QPushButton#rollbackBtn {
  background-color: #ffffff; color: #1e40af;
  border: 1px solid #1e40af; font-weight: 600;
}
QPushButton#rollbackBtn:hover { background-color: #eef2ff; }
QPushButton#rollbackBtn:disabled {
  background-color: #ffffff; color: #94a3b8; border-color: #cbd5e1;
}

/* 删除按钮 — 描边危险色 */
QPushButton#deleteChapterBtn {
  background-color: #ffffff; color: #ef4444;
  border: 1px solid #fecaca; font-weight: 600;
}
QPushButton#deleteChapterBtn:hover {
  background-color: #fef2f2; border-color: #fca5a5;
}
QPushButton#deleteChapterBtn:disabled {
  background-color: #ffffff; color: #94a3b8; border-color: #cbd5e1;
}

/* ====== 文本浏览器（Commit 详情） ====== */
QTextBrowser { background: #ffffff; border: none; border-radius: 10px;
              padding: 14px; font-size: 14px; }

/* ====== 分割器 ====== */
QSplitter::handle { background: transparent; }

/* ====== 状态栏 ====== */
QStatusBar { background: #ffffff; border-top: 1px solid #e2e8f0;
            color: #64748b; font-size: 13px; padding: 3px 10px; }

/* ====== 通用元素 ====== */
QToolTip { background: #1e293b; color: #ffffff; border-radius: 6px;
          padding: 5px 10px; font-size: 13px; }
QLineEdit { background: #ffffff; border: 1px solid #cbd5e1; border-radius: 8px;
           padding: 8px 12px; font-size: 14px; }
QLineEdit:focus { border-color: #4338ca; }
QGroupBox { font-weight: bold; border: none; border-radius: 10px;
           margin-top: 10px; padding: 16px 12px 12px 12px; font-size: 14px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }

/* ====== 弹窗 ====== */
QDialog { min-width: 480px; }
QDialog QLabel { font-size: 14px; }
QDialog QLineEdit { font-size: 14px; padding: 8px 12px; }
QDialog QPushButton { font-size: 14px; padding: 8px 22px; }
)";
```

- [ ] **Step 2: 构建验证**

```bash
cd "E:/Study/程设大作业——墨迹/test/test" && qmake && make clean && make -j$(nproc)
```

预期：编译成功。

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "style: 重写全局QSS样式表为现代极简墨蓝主题"
```

---

### Task 2: 章节列表未提交标记改为橙点

**Files:**
- Modify: `src/chapterlistmodel.cpp:16-48` (data 方法)
- Modify: `include/chapterlistmodel.h:30-31` (添加自定义 role 常量)

根据设计规格，将有未提交的章节从"红色文字+后缀"改为"章节名右侧橙色圆点"。

- [ ] **Step 1: 在头文件中定义自定义 role**

在 `include/chapterlistmodel.h` 中，`chapterAt` 声明之前添加：

```cpp
    static constexpr int DirtyRole = Qt::UserRole + 1;
```

位置：第 28 行 `int rowCount` 之后。

- [ ] **Step 2: 修改 data() 逻辑**

将 `src/chapterlistmodel.cpp` 第 23-27 行的 `data()` 方法（DisplayRole 和 ForegroundRole 部分）替换为：

```cpp
    if (role == Qt::DisplayRole) {
        if (m_dirtyChapters.contains(name))
            return name + "  ●";
        return name;
    }

    if (role == Qt::ForegroundRole) {
        if (m_dirtyChapters.contains(name))
            return QColor("#f59e0b");
        return {};
    }
```

`setDirtyChapters()` 中第 46-47 行的 `dataChanged` signal 已包含 `Qt::ForegroundRole`，无需修改。

- [ ] **Step 3: 构建验证**

```bash
cd "E:/Study/程设大作业——墨迹/test/test" && qmake && make -j$(nproc)
```

- [ ] **Step 4: Commit**

```bash
git add include/chapterlistmodel.h src/chapterlistmodel.cpp
git commit -m "style: 章节未提交标记改为橙色文字+圆点"
```

---

### Task 3: CommitDelegate 卡片化绘制

**Files:**
- Modify: `src/commitmodel.cpp:76-134` (CommitDelegate::paint 方法)

将 Commit 列表项绘制调整为卡片风格（圆角背景、更好间距），保持序号加粗和红色标识逻辑完全不变。

- [ ] **Step 1: 调整 paint() 方法的绘制**

将 `src/commitmodel.cpp` 第 76-134 行的 `CommitDelegate::paint` 替换为：

```cpp
void CommitDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    if (text.isEmpty()) return;

    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    // --- 卡片背景 ---
    bool selected = option.state & QStyle::State_Selected;
    QRect cardRect = option.rect.adjusted(2, 2, -2, -2);

    if (selected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#eef2ff"));
        painter->drawRoundedRect(cardRect, 8, 8);
    }

    // --- 文字颜色 ---
    QColor textColor = option.palette.text().color();
    if (s_currentBranch != "main" && !s_branchOnlyHashes.isEmpty()) {
        const CommitModel *model = static_cast<const CommitModel *>(index.model());
        CommitInfo ci = model->commitAt(index.row());
        QString hash7 = ci.hash.left(7);
        for (const QString &h : s_branchOnlyHashes) {
            if (h.startsWith(hash7) || hash7 == h.left(7)) {
                textColor = QColor("#cf222e");
                break;
            }
        }
    }
    if (selected && textColor == option.palette.text().color())
        textColor = QColor("#4338ca");

    // --- 解析 [序号] 前缀 ---
    int bracketEnd = text.indexOf(']');
    if (bracketEnd == -1) {
        painter->setPen(textColor);
        painter->drawText(cardRect.adjusted(4, 0, -4, 0),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        painter->restore();
        return;
    }

    QString prefix = text.left(bracketEnd + 1);
    QString suffix = text.mid(bracketEnd + 1);

    QFont boldFont = option.font;
    boldFont.setBold(true);

    QRect textRect = cardRect.adjusted(4, 0, -4, 0);
    QFontMetrics fmBold(boldFont);
    int prefixWidth = fmBold.horizontalAdvance(prefix);

    // 画加粗序号
    QRect prefixRect = textRect;
    prefixRect.setWidth(prefixWidth);
    painter->setFont(boldFont);
    painter->setPen(textColor);
    painter->drawText(prefixRect, Qt::AlignLeft | Qt::AlignVCenter, prefix);

    // 画剩余文字
    QRect suffixRect = textRect;
    suffixRect.setLeft(textRect.left() + prefixWidth);
    painter->setFont(option.font);
    QString elided = option.fontMetrics.elidedText(suffix, Qt::ElideRight, suffixRect.width());
    painter->drawText(suffixRect, Qt::AlignLeft | Qt::AlignVCenter, elided);

    painter->restore();
}
```

- [ ] **Step 2: 构建验证**

```bash
cd "E:/Study/程设大作业——墨迹/test/test" && qmake && make -j$(nproc)
```

- [ ] **Step 3: Commit**

```bash
git add src/commitmodel.cpp
git commit -m "style: CommitDelegate卡片化绘制，选中态加圆角背景"
```

---

### Task 4: 分支栏控件样式调整

**Files:**
- Modify: `src/mainwindow.cpp:129-163` (initBranchBar 方法)

调整分支栏控件的 objectName 和样式类以匹配新 QSS 选择器。

- [ ] **Step 1: 确认分支栏按钮的 objectName 与新 QSS 匹配**

检查 `initBranchBar()` 中（`src/mainwindow.cpp:129-163`）：

- `m_newBranchBtn`：需设置 `setObjectName("newBranchBtn")` → 当前代码没有显式设置，需要添加
- `m_mergeBtn`：需设置 `setObjectName("mergeBtn")` → 同上

在第 152 行 "新建分支" 按钮创建后添加 objectName：

```cpp
    m_newBranchBtn = new QPushButton("新建分支");
    m_newBranchBtn->setObjectName("newBranchBtn");  // 新增
    branchBar->addWidget(m_newBranchBtn);
```

在第 156 行 "合并到当前" 按钮创建后添加 objectName：

```cpp
    m_mergeBtn = new QPushButton("合并到当前");
    m_mergeBtn->setObjectName("mergeBtn");  // 新增
    branchBar->addWidget(m_mergeBtn);
```

- [ ] **Step 2: 构建验证**

```bash
cd "E:/Study/程设大作业——墨迹/test/test" && qmake && make -j$(nproc)
```

- [ ] **Step 3: Commit**

```bash
git add src/mainwindow.cpp
git commit -m "style: 分支栏按钮添加objectName以匹配QSS选择器"
```

---

### Task 5: AI 对话框聊天气泡重构

**Files:**
- Modify: `src/aidialog.cpp:68-210` (addBubble 和 renderHtml 方法)

将 AI 对话框的消息从当前简单 div 布局改为聊天气泡风格。

- [ ] **Step 1: 修改 addBubble() — 使用气泡 HTML 结构**

将 `src/aidialog.cpp` 第 197-210 行的 `addBubble` 方法替换为：

```cpp
void AiDialog::addBubble(const QString &role, const QString &text)
{
    bool isUser = (role == "user");
    QString label = isUser ? "你" : "墨迹 AI";
    QString body  = isUser
        ? text.toHtmlEscaped().replace('\n', "<br>")
        : simpleMarkdown(text);

    // 气泡样式：用户右对齐蓝底白字，AI 左对齐白底
    QString bubbleStyle = isUser
        ? "background:#1e40af;color:#ffffff;border-radius:12px 12px 4px 12px;"
        : "background:#ffffff;border:1px solid #e2e8f0;border-radius:12px 12px 12px 4px;";
    QString align = isUser ? "flex-end" : "flex-start";
    QString bubbleLabel = isUser ? "👤 你" : "🤖 墨迹 AI";

    QString bubble = QString(
        "<div style='display:flex;flex-direction:column;align-items:%1;margin:10px 12px;'>"
        "<span style='font-size:12px;color:#94a3b8;margin-bottom:3px;'>%2</span>"
        "<div style='%3 padding:10px 14px;max-width:80%%;font-size:14px;line-height:1.6;'>"
        "%4</div></div>"
    ).arg(align, bubbleLabel, bubbleStyle, body);

    m_chatHtml += bubble;
    renderHtml();
}
```

- [ ] **Step 2: 修改 renderHtml() — 更新聊天区容器样式**

将 `src/aidialog.cpp` 第 212-226 行的 `renderHtml` 方法中的 CSS 更新为：

```cpp
void AiDialog::renderHtml()
{
    m_chatView->setHtml(
        "<style>"
        "body{font-size:14px;line-height:1.7;color:#1e293b;background:#f8fafc;margin:0;padding:8px;}"
        "b{font-weight:bold;}"
        "code{background:#f1f5f9;padding:1px 6px;border-radius:4px;"
        "font-family:'Cascadia Code',Consolas,monospace;font-size:13px;}"
        "ul{margin:4px 0 8px;padding-left:20px;}"
        "li{margin:4px 0;}"
        "p{margin:4px 0 8px;}"
        "</style>" + m_chatHtml);
    m_chatView->verticalScrollBar()->setValue(
        m_chatView->verticalScrollBar()->maximum());
}
```

- [ ] **Step 3: 修改 AI 欢迎消息文本**

`setCommitContext()` 中第 162 行的欢迎消息（`src/aidialog.cpp:162-163`），将 "AI 写作助手" 改为 "墨迹 AI"：

```cpp
    addBubble("assistant", "你好！我是墨迹 AI 写作助手。我已读取当前的 commit 历史，"
              "可以帮你分析修改趋势、提供写作建议、解读反馈。请随时提问！");
```

- [ ] **Step 4: 构建验证**

```bash
cd "E:/Study/程设大作业——墨迹/test/test" && qmake && make -j$(nproc)
```

- [ ] **Step 5: Commit**

```bash
git add src/aidialog.cpp
git commit -m "style: AI对话框聊天气泡重构，蓝底用户/白底AI"
```

---

### Task 6: 环境配置对话框美化

**Files:**
- Modify: `src/environmentsetupdialog.cpp` (完整文件)

添加步骤序号标识，微调布局以适配新弹窗样式。

- [ ] **Step 1: 修改标题区域 — 添加步骤序号**

将 `src/environmentsetupdialog.cpp` 第 25-30 行标题部分替换为：

```cpp
    // ── 标题 ──
    auto *titleLabel = new QLabel(
        "<style>"
        ".step{display:inline-block;width:24px;height:24px;border-radius:12px;"
        "background:#1e40af;color:#fff;text-align:center;line-height:24px;"
        "font-size:13px;font-weight:bold;margin-right:8px;}"
        "</style>"
        "<h3 style='margin-bottom:8px;'>欢迎使用墨迹</h3>"
        "<p style='color:#475569;line-height:1.6;'>"
        "<span class='step'>1</span> 安装 Git — 版本管理核心<br><br>"
        "<span class='step'>2</span> 配置作者信息<br><br>"
        "完成以上两步即可开始使用。</p>");
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);
```

- [ ] **Step 2: 调整状态标签颜色逻辑**

在 `updateState()` (`src/environmentsetupdialog.cpp:70-76`) 中，增强 Git 状态显示：

```cpp
void EnvironmentSetupDialog::updateState()
{
    m_installBtn->setEnabled(!m_gitInstalled);
    if (m_gitInstalled) {
        m_installBtn->setText("✓ Git 已安装");
        m_installBtn->setStyleSheet("background-color:#16a34a;color:#fff;border:none;"
            "border-radius:8px;padding:6px 16px;font-size:14px;font-weight:600;");
    }
    m_nameEdit->setEnabled(m_gitInstalled);
    m_emailEdit->setEnabled(m_gitInstalled);
    m_saveBtn->setEnabled(m_gitInstalled);
}
```

- [ ] **Step 3: 构建验证**

```bash
cd "E:/Study/程设大作业——墨迹/test/test" && qmake && make -j$(nproc)
```

- [ ] **Step 4: Commit**

```bash
git add src/environmentsetupdialog.cpp
git commit -m "style: 环境配置对话框添加步骤引导和状态色"
```

---

### Task 7: 最终构建验证

- [ ] **Step 1: 完整清理构建**

```bash
cd "E:/Study/程设大作业——墨迹/test/test" && qmake && make clean && make -j$(nproc)
```

预期：零错误、零警告编译通过。

- [ ] **Step 2: 验证功能逻辑完整性**

运行应用并检查：
1. 打开工作区 → 章节列表正常显示
2. 新建/重命名/删除章节 → 按钮状态正确切换
3. 提交变更 → commit 列表刷新
4. 章节回退 → 回退操作正常
5. 分支切换/创建/合并 → 分支栏正常
6. AI 对话框 → 聊天气泡正确渲染
7. 筛选下拉框 → commit 筛选正常

- [ ] **Step 3: 关闭视觉伴侣服务器**

```bash
bash "/c/Users/38862/.claude/plugins/cache/superpowers-marketplace/superpowers/5.0.7/skills/brainstorming/scripts/stop-server.sh" "E:/Study/程设大作业——墨迹/test/test/.superpowers/brainstorm/1880-1779874810"
```

- [ ] **Step 4: 最终提交**

```bash
git commit --allow-empty -m "chore: UI视觉现代化完成，构建验证通过"
```

---

## 验证清单

实施完成后逐项检查：

- [ ] 应用启动，主窗口显示新的墨蓝色菜单栏
- [ ] 章节列表项为独立圆角卡片，选中时紫蓝背景+左侧色条
- [ ] 有未提交变更的章节显示橙色圆点标记
- [ ] Commit 列表项为圆角卡片风格，序号加粗
- [ ] 选中 commit 时为紫蓝背景（#eef2ff）
- [ ] 非 main 分支独有 commit 保持红色文字
- [ ] 新建按钮为填充蓝色，重命名/新建分支/合并为描边按钮，删除按钮为红色描边
- [ ] 分支下拉框宽度适中，与筛选下拉框样式一致
- [ ] Commit 详情区白底卡片，文件链接正常点击
- [ ] 状态栏白底+顶部分隔线，文字灰色
- [ ] AI 对话框聊天气泡左右分布，用户蓝底白字
- [ ] 环境配置对话框步骤序号正常显示
- [ ] 所有按钮 disabled 状态为灰色
- [ ] 所有现有功能（章节CRUD、提交、回退、分支、AI）正常工作
