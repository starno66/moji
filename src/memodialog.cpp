#include "memodialog.h"
#include "gitmanager.h"
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <QWebEngineView>

static const char *kListStyle =
    "QListWidget{border:1px solid #e2e8f0;border-radius:10px;background:#fff;"
    "outline:none;font-size:14px;}"
    "QListWidget::item{padding:10px 14px;border-bottom:1px solid #f1f5f9;}"
    "QListWidget::item:hover{background:#f8fafc;}"
    "QListWidget::item:selected{background:#eef2ff;color:#4338ca;}";

static QString normalizePath(const QString &path)
{
    QString n = path;
    n.replace('\\', '/');
    return n;
}

// ── Storage ──
QString MemoDialog::notesFilePath() const { return m_workspacePath + "/.moji/notes.json"; }

QJsonObject MemoDialog::readNotesFile() const
{
    QFile f(notesFilePath());
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

void MemoDialog::writeNotesFile(const QJsonObject &obj)
{
    QDir().mkpath(m_workspacePath + "/.moji");
    QFile f(notesFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(obj).toJson());
    f.close();

    if (m_gitManager) {
        QProcess proc;
        proc.setWorkingDirectory(m_workspacePath);
        proc.start("git", {"add", ".moji/"});
        proc.waitForFinished(5000);
        m_gitManager->commit(QString::fromUtf8("随记: 更新笔记"));
    }
}

// ── Constructor ──
MemoDialog::MemoDialog(const QString &workspacePath, GitManager *git,
                       QWidget *parent)
    : QDialog(parent), m_gitManager(git), m_workspacePath(workspacePath)
{
    setWindowTitle("随记");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint |
                   Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    resize(960, 680);
    setMinimumSize(600, 420);
    auto *lay = new QVBoxLayout(this);

    m_stack = new QStackedWidget(this);

    // ═══════ Page 0: 章节总览 ═══════
    auto *page0 = new QWidget();
    auto *p0 = new QVBoxLayout(page0);
    p0->setSpacing(10);

    auto *headerRow = new QHBoxLayout();
    auto *chapLabel = new QLabel("章节:");
    QFont lblF = chapLabel->font(); lblF.setBold(true);
    chapLabel->setFont(lblF);
    headerRow->addWidget(chapLabel);

    m_chapterCombo = new QComboBox();
    m_chapterCombo->setMinimumWidth(320);
    m_chapterCombo->setStyleSheet(
        "QComboBox{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;"
        "padding:6px 14px;font-size:14px;}"
        "QComboBox:hover{background:#f1f5f9;}"
        "QComboBox::drop-down{border:none;width:20px;}"
        "QComboBox QAbstractItemView{background:#fff;border:1px solid #e2e8f0;"
        "border-radius:8px;padding:4px;outline:none;font-size:14px;}"
        "QComboBox QAbstractItemView::item{padding:8px 12px;border-radius:6px;}"
        "QComboBox QAbstractItemView::item:hover{background:#f1f5f9;}");
    connect(m_chapterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onChapterChanged(); });
    headerRow->addWidget(m_chapterCombo, 1);

    m_addFileBtn = new QPushButton("+ 添加文件");
    m_addFileBtn->setStyleSheet(
        "QPushButton{background:#fff;color:#1e40af;border:1px dashed #93c5fd;"
        "border-radius:8px;padding:7px 16px;font-weight:600;font-size:13px;}"
        "QPushButton:hover{background:#eef2ff;border-color:#1e40af;}");
    connect(m_addFileBtn, &QPushButton::clicked, this, &MemoDialog::onAddFile);
    headerRow->addWidget(m_addFileBtn);
    p0->addLayout(headerRow);

    m_overviewList = new QListWidget();
    m_overviewList->setStyleSheet(kListStyle);
    m_overviewList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_overviewList, &QListWidget::itemDoubleClicked,
            this, &MemoDialog::onFileClicked);
    connect(m_overviewList, &QListWidget::customContextMenuRequested,
            this, &MemoDialog::onOverviewContextMenu);
    p0->addWidget(m_overviewList, 1);

    m_stack->addWidget(page0);

    // ═══════ Page 1: 文件笔记 ═══════
    auto *page1 = new QWidget();
    auto *p1 = new QVBoxLayout(page1);

    auto *topBar = new QHBoxLayout();
    m_backBtn = new QPushButton("← 返回总览");
    m_backBtn->setStyleSheet(
        "QPushButton{background:transparent;color:#475569;border:none;"
        "padding:4px 8px;font-weight:600;}"
        "QPushButton:hover{color:#1e40af;}");
    connect(m_backBtn, &QPushButton::clicked, this, &MemoDialog::onBackToOverview);
    topBar->addWidget(m_backBtn);
    m_fileLabel = new QLabel();
    QFont fl = m_fileLabel->font(); fl.setBold(true);
    m_fileLabel->setFont(fl);
    m_fileLabel->setStyleSheet("color:#1e293b;");
    topBar->addWidget(m_fileLabel, 1);
    p1->addLayout(topBar);

    auto *hSplit = new QSplitter(Qt::Horizontal);

    auto *leftW = new QWidget();
    auto *leftL = new QVBoxLayout(leftW);
    leftL->setContentsMargins(0, 0, 0, 0);
    m_noteList = new QListWidget();
    m_noteList->setStyleSheet(kListStyle);
    connect(m_noteList, &QListWidget::currentRowChanged,
            this, [this](int) { onNoteSelected(); });
    leftL->addWidget(m_noteList, 1);
    leftW->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    hSplit->addWidget(leftW);

    // 右侧：工具栏 + 左右分栏编辑器/预览
    auto *rightW = new QWidget();
    auto *rightL = new QVBoxLayout(rightW);
    rightL->setContentsMargins(0, 0, 0, 0);
    rightL->setSpacing(4);

    // 快捷输入工具栏
    auto *mdBar = new QHBoxLayout();
    mdBar->setSpacing(2);
    auto makeBtn = [&](const QString &text, const QString &tip,
                       const QString &before, const QString &after = {}) {
        auto *btn = new QPushButton(text);
        btn->setToolTip(tip);
        btn->setFixedSize(28, 26);
        btn->setStyleSheet(
            "QPushButton{background:#f1f5f9;color:#475569;border:1px solid #e2e8f0;"
            "border-radius:4px;font-size:12px;font-weight:bold;padding:0;}"
            "QPushButton:hover{background:#e2e8f0;border-color:#94a3b8;}");
        connect(btn, &QPushButton::clicked, this, [this, before, after]() {
            insertMarkdown(before, after);
        });
        return btn;
    };
    mdBar->addWidget(makeBtn("H1", "一级标题", "# "));
    mdBar->addWidget(makeBtn("H2", "二级标题", "## "));
    mdBar->addWidget(makeBtn("H3", "三级标题", "### "));
    mdBar->addWidget(makeBtn("B", "粗体", "**", "**"));
    mdBar->addWidget(makeBtn("I", "斜体", "*", "*"));
    mdBar->addWidget(makeBtn("`", "行内代码", "`", "`"));
    mdBar->addWidget(makeBtn("-", "列表项", "- "));
    mdBar->addWidget(makeBtn(">", "引用", "> "));
    mdBar->addWidget(makeBtn("$", "行内公式", "$", "$"));
    mdBar->addWidget(makeBtn("$$", "公式块", "$$\n", "\n$$"));
    mdBar->addWidget(makeBtn("---", "分隔线", "\n---\n"));
    mdBar->addWidget(makeBtn("[ ]", "链接 [text](url)", "[", "](url)"));
    mdBar->addStretch();
    rightL->addLayout(mdBar);

    m_editSplitter = new QSplitter(Qt::Horizontal);
    m_editor = new QTextEdit();
    m_editor->setPlaceholderText("输入 Markdown...");
    m_editor->setStyleSheet(
        "QTextEdit{border:1px solid #e2e8f0;border-radius:8px;padding:10px;"
        "font-family:'Cascadia Code',Consolas,monospace;font-size:14px;background:#fafbfc;}");

    m_preview = new QWebEngineView();
    m_preview->setAttribute(Qt::WA_InputMethodEnabled, false);
    m_preview->setFocusPolicy(Qt::NoFocus);
    m_preview->setStyleSheet(
        "QWebEngineView{border:1px solid #e2e8f0;border-radius:8px;background:#fff;}");
    m_preview->load(QUrl("qrc:/mathjax.html"));

    m_editSplitter->addWidget(m_editor);
    m_editSplitter->addWidget(m_preview);
    m_editSplitter->setSizes({400, 420});
    rightL->addWidget(m_editSplitter, 1);

    hSplit->addWidget(rightW);
    hSplit->setStretchFactor(0, 0);
    hSplit->setStretchFactor(1, 1);
    hSplit->setSizes({200, 700});

    p1->addWidget(hSplit, 1);

    auto *botBar = new QHBoxLayout();
    m_newNoteBtn = new QPushButton("+ 新建笔记");
    connect(m_newNoteBtn, &QPushButton::clicked, this, &MemoDialog::onNewNote);
    m_delNoteBtn = new QPushButton("删除笔记");
    m_delNoteBtn->setStyleSheet(
        "QPushButton{background:#fff;color:#ef4444;border:1px solid #fecaca;"
        "border-radius:8px;padding:6px 14px;font-weight:600;}"
        "QPushButton:hover{background:#fef2f2;}");
    connect(m_delNoteBtn, &QPushButton::clicked, this, &MemoDialog::onDeleteNote);
    botBar->addWidget(m_newNoteBtn);
    botBar->addWidget(m_delNoteBtn);
    botBar->addStretch();
    p1->addLayout(botBar);

    m_stack->addWidget(page1);

    lay->addWidget(m_stack);
    m_stack->setCurrentIndex(0);

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(250);
    connect(m_debounce, &QTimer::timeout, this, &MemoDialog::updatePreview);
    connect(m_editor, &QTextEdit::textChanged, this, &MemoDialog::onEditorChanged);

    refreshOverview();
}

// ── Overview ──
void MemoDialog::refreshOverview()
{
    m_chapterCombo->blockSignals(true);
    m_chapterCombo->clear();

    QSet<QString> chapterSet;
    QDir wsDir(m_workspacePath);
    QStringList dirs = wsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &d : dirs) {
        if (d == ".git" || d == ".moji") continue;
        chapterSet.insert(d);
    }
    QJsonObject all = readNotesFile();
    for (auto it = all.begin(); it != all.end(); ++it) {
        QString key = normalizePath(it.key());
        int slash = key.indexOf('/');
        if (slash > 0)
            chapterSet.insert(key.left(slash));
    }

    QStringList chapters = chapterSet.values();
    chapters.sort();
    for (const QString &ch : chapters)
        m_chapterCombo->addItem(ch, ch);

    m_chapterCombo->blockSignals(false);

    if (m_chapterCombo->count() == 0) {
        m_overviewList->clear();
        m_addFileBtn->setEnabled(false);
    } else {
        m_addFileBtn->setEnabled(true);
        onChapterChanged();
    }
}

void MemoDialog::onChapterChanged()
{
    QString chapter = m_chapterCombo->currentData().toString();
    m_overviewList->clear();

    if (chapter.isEmpty()) return;

    QJsonObject all = readNotesFile();
    QStringList files = all.keys();
    files.sort();

    for (const QString &f : files) {
        QString nf = normalizePath(f);
        if (!nf.startsWith(chapter + "/")) continue;

        QJsonArray notes = all[f].toArray();
        QString latestTitle;
        if (!notes.isEmpty())
            latestTitle = notes.first().toObject()["title"].toString();

        auto *item = new QListWidgetItem(
            QString("%1\n%2 条笔记 · %3")
                .arg(nf.mid(chapter.length() + 1))
                .arg(notes.size())
                .arg(latestTitle.isEmpty() ? "暂无" : latestTitle));
        item->setData(Qt::UserRole, f);
        m_overviewList->addItem(item);
    }

    if (m_overviewList->count() == 0) {
        auto *item = new QListWidgetItem(
            QString::fromUtf8("该章节暂无随记, 点击 [添加文件] 来创建"));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_overviewList->addItem(item);
    }
}

// ── Navigation ──
void MemoDialog::onFileClicked(QListWidgetItem *item)
{
    QString filePath = item->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) return;
    switchToFileView(filePath);
}

void MemoDialog::onOverviewContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_overviewList->itemAt(pos);
    if (!item || item->data(Qt::UserRole).toString().isEmpty()) return;

    QMenu menu(m_overviewList);
    QAction *delAction = menu.addAction("删除此文件及所有随记");
    QAction *chosen = menu.exec(m_overviewList->viewport()->mapToGlobal(pos));
    if (chosen != delAction) return;

    QString fp = item->data(Qt::UserRole).toString();
    if (fp == m_currentFilePath && m_stack->currentIndex() == 1)
        onBackToOverview();
    QJsonObject all = readNotesFile();
    all.remove(fp);
    writeNotesFile(all);
    refreshOverview();
}

void MemoDialog::switchToFileView(const QString &filePath)
{
    QJsonObject all = readNotesFile();
    if (!all.contains(filePath)) {
        all[filePath] = QJsonArray();
        writeNotesFile(all);
    }
    m_fileLabel->setText(filePath);
    m_stack->setCurrentIndex(1);
    loadNotes(filePath);
}

void MemoDialog::onBackToOverview()
{
    saveCurrentNote();
    refreshOverview();
    m_stack->setCurrentIndex(0);
}

void MemoDialog::selectFile(const QString &filePath)
{
    switchToFileView(normalizePath(filePath));
}

// ── Add file ──
void MemoDialog::onAddFile()
{
    QString chapter = m_chapterCombo->currentData().toString();
    if (chapter.isEmpty()) return;

    QString chapterDir = m_workspacePath + "/" + chapter;
    QString path = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("选择 %1 章节内的文件").arg(chapter),
        chapterDir, "所有文件 (*.*)");
    if (path.isEmpty()) return;

    QString absPath = QDir::fromNativeSeparators(path);
    QString wsPath  = QDir::fromNativeSeparators(m_workspacePath);
    if (absPath.startsWith(wsPath + "/"))
        absPath = absPath.mid(wsPath.length() + 1);
    else if (absPath.startsWith(wsPath))
        absPath = absPath.mid(wsPath.length());
    if (absPath.isEmpty()) return;

    if (!absPath.startsWith(chapter + "/")) {
        QMessageBox::warning(this, "无效文件",
            QString::fromUtf8("只能为 \"%1\" 章节内的文件创建随记。").arg(chapter));
        return;
    }

    QJsonObject all = readNotesFile();
    if (!all.contains(absPath)) {
        all[absPath] = QJsonArray();
        writeNotesFile(all);
        refreshOverview();
    }
    switchToFileView(absPath);
}

// ── Load notes ──
void MemoDialog::loadNotes(const QString &filePath)
{
    m_currentFilePath = filePath;
    m_noteList->blockSignals(true);
    m_noteList->clear();
    m_editor->clear();
    m_preview->page()->runJavaScript("render('')");
    m_currentNoteId.clear();
    m_delNoteBtn->setEnabled(false);

    QJsonObject all = readNotesFile();
    QJsonArray notes = all.value(filePath).toArray();

    for (int i = 0; i < notes.size(); ++i) {
        QJsonObject n = notes[i].toObject();
        QDateTime t = QDateTime::fromString(n["updated"].toString(), Qt::ISODate);
        auto *li = new QListWidgetItem(
            (n["title"].toString().isEmpty() ? "无标题" : n["title"].toString())
            + "\n" + t.toLocalTime().toString("MM-dd hh:mm"));
        li->setData(Qt::UserRole, n["id"].toString());
        li->setData(Qt::UserRole + 1, n["content"].toString());
        m_noteList->addItem(li);
    }
    m_noteList->blockSignals(false);
    if (m_noteList->count() > 0)
        m_noteList->setCurrentRow(0);
}

// ── Note ops ──
void MemoDialog::onNoteSelected()
{
    saveCurrentNote();
    auto *item = m_noteList->currentItem();
    if (!item) {
        m_editor->clear(); m_preview->page()->runJavaScript("render('')");
        m_currentNoteId.clear();
        m_delNoteBtn->setEnabled(false); return;
    }
    m_currentNoteId = item->data(Qt::UserRole).toString();
    m_editor->blockSignals(true);
    m_editor->setPlainText(item->data(Qt::UserRole + 1).toString());
    m_editor->blockSignals(false);
    m_delNoteBtn->setEnabled(true);
    updatePreview();
}

void MemoDialog::saveCurrentNote()
{
    if (m_currentFilePath.isEmpty() || m_currentNoteId.isEmpty()) return;
    QJsonObject all = readNotesFile();
    QJsonArray notes = all.value(m_currentFilePath).toArray();
    for (int i = 0; i < notes.size(); ++i) {
        QJsonObject n = notes[i].toObject();
        if (n["id"].toString() == m_currentNoteId) {
            QString content = m_editor->toPlainText();
            n["content"] = content;
            QString title = content.section('\n', 0, 0).trimmed();
            title.remove(QRegularExpression("^#+\\s*"));
            if (title.isEmpty()) title = "无标题";
            n["title"] = title.left(60);
            n["updated"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            notes[i] = n; break;
        }
    }
    all[m_currentFilePath] = notes;
    writeNotesFile(all);
}

void MemoDialog::onNewNote()
{
    if (m_currentFilePath.isEmpty()) return;
    saveCurrentNote();
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject note;
    note["id"] = id; note["title"] = "新笔记"; note["content"] = "";
    note["updated"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    QJsonObject all = readNotesFile();
    QJsonArray notes = all.value(m_currentFilePath).toArray();
    notes.prepend(note);
    all[m_currentFilePath] = notes;
    writeNotesFile(all);
    loadNotes(m_currentFilePath);
    for (int i = 0; i < m_noteList->count(); ++i) {
        if (m_noteList->item(i)->data(Qt::UserRole).toString() == id) {
            m_noteList->setCurrentRow(i); break;
        }
    }
    m_editor->setFocus();
}

void MemoDialog::onDeleteNote()
{
    if (m_currentFilePath.isEmpty() || m_currentNoteId.isEmpty()) return;
    QJsonObject all = readNotesFile();
    QJsonArray notes = all.value(m_currentFilePath).toArray();
    QJsonArray filtered;
    for (const auto &v : notes) {
        if (v.toObject()["id"].toString() != m_currentNoteId) filtered.append(v);
    }
    if (filtered.isEmpty()) all.remove(m_currentFilePath);
    else all[m_currentFilePath] = filtered;
    writeNotesFile(all);
    m_currentNoteId.clear(); m_editor->clear();
    m_preview->page()->runJavaScript("render('')");
    loadNotes(m_currentFilePath);
}

// ── Editor ──
void MemoDialog::onEditorChanged() { m_debounce->start(); }

void MemoDialog::updatePreview()
{
    QString md = m_editor->toPlainText();
    QByteArray b64 = md.toUtf8().toBase64();
    m_preview->page()->runJavaScript(
        QString("render(decodeURIComponent(escape(atob('%1'))))")
            .arg(QString::fromLatin1(b64)));
}

void MemoDialog::insertMarkdown(const QString &before, const QString &after)
{
    QTextCursor cur = m_editor->textCursor();
    if (cur.hasSelection()) {
        int start = cur.selectionStart();
        int end   = cur.selectionEnd();
        cur.setPosition(start);
        cur.insertText(before);
        cur.setPosition(end + before.length());
        cur.insertText(after);
    } else {
        cur.insertText(before + after);
        if (!after.isEmpty())
            cur.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, after.length());
    }
    m_editor->setTextCursor(cur);
    m_editor->setFocus();
    updatePreview();
}

void MemoDialog::wrapSelection(const QString &before, const QString &after)
{
    insertMarkdown(before, after);
}
