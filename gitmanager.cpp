#include "gitmanager.h"
#include <QProcess>
#include <QDir>

GitManager::GitManager(QObject *parent)
    : QObject(parent)
{
}

// ==================== 基础设置 ====================

void GitManager::setWorkspacePath(const QString &path)
{
    m_workspacePath = path;
}

QString GitManager::workspacePath() const
{
    return m_workspacePath;
}

bool GitManager::initRepo()
{
    bool ok;
    runGit({"init"}, &ok);
    return ok;
}

bool GitManager::isRepo() const
{
    if (m_workspacePath.isEmpty())
        return false;
    // 检查工作区目录下是否存在 .git 文件夹
    return QDir(m_workspacePath + "/.git").exists();
}

// ==================== 暂存与提交 ====================

bool GitManager::stageAll()
{
    bool ok;
    runGit({"add", "-A"}, &ok);  // -A 表示暂存所有变更（新增+修改+删除）
    return ok;
}

bool GitManager::commit(const QString &message)
{
    bool ok;
    runGit({"commit", "-m", message}, &ok);
    return ok;
}

bool GitManager::stageAndCommit(const QString &message)
{
    if (!stageAll()) return false;
    // 如果 commit 失败（比如没有实际变更），不算错误
    bool ok;
    runGit({"commit", "-m", message}, &ok);
    return ok;
}

// ==================== 查看历史 ====================

QList<CommitInfo> GitManager::log(const QString &folderFilter)
{
    /*
     * git log 命令解析:
     *
     * --format="%H|%an|%aI|%s"  用 | 分隔输出: hash|作者|时间|message
     * --name-only               每个 commit 后列出涉及的文件名
     * -- (folder)下文件         只显示影响该文件夹的 commit
     *
     * 输出示例:
     *   abc123...|An Hao|2025-01-15T10:30:00+08:00|ch01: 修改引言
     *   ch01-引言/main.tex
     *   ch01-引言/refs.bib
     *
     *   def456...|An Hao|2025-01-14T09:00:00+08:00|ch02: 增加图表
     *   ch02-方法/images/fig1.png
     */

    QStringList args = {"log", "--format=%H|%an|%aI|%s", "--name-only"};
    if (!folderFilter.isEmpty()) {
        // 只查影响该章节文件夹的 commit
        args << "--" << (folderFilter + "/*");
    }

    bool ok;
    QByteArray output = runGit(args, &ok);
    if (!ok)
        return {};

    return parseLogOutput(output);
}

// ==================== 章节回退 ====================

bool GitManager::restoreFolder(const QString &relativePath, const QString &hash)
{
    /*
     * git checkout <hash> -- <path>
     * 将指定路径的所有文件恢复到 <hash> 版本
     * 注意: 这会直接覆盖工作区文件，不会产生冲突
     */
    bool ok;
    runGit({"checkout", hash, "--", relativePath}, &ok);
    return ok;
}

// ==================== 文件夹操作 ====================

bool GitManager::moveFolder(const QString &oldPath, const QString &newPath)
{
    bool ok;
    runGit({"mv", oldPath, newPath}, &ok);
    return ok;
}

bool GitManager::removeFolder(const QString &relativePath)
{
    // git rm -r: 递归删除整个文件夹
    bool ok;
    runGit({"rm", "-r", relativePath}, &ok);
    return ok;
}

// ==================== 状态查询 ====================

bool GitManager::hasUncommittedChanges()
{
    /*
     * git status --porcelain
     * 如果有未提交的变更，会输出变更列表
     * 没有变更则输出为空
     */
    bool ok;
    QByteArray output = runGit({"status", "--porcelain"}, &ok);
    if (!ok) return false;
    return !output.trimmed().isEmpty();
}

QString GitManager::currentBranch()
{
    bool ok;
    QByteArray output = runGit({"branch", "--show-current"}, &ok);
    if (!ok || output.isEmpty())
        return "unknown";
    return QString::fromUtf8(output).trimmed();
}

// ==================== 日志解析（静态方法） ====================

QList<CommitInfo> GitManager::parseLogOutput(const QByteArray &output)
{
    QList<CommitInfo> result;
    QString text = QString::fromUtf8(output).trimmed();
    if (text.isEmpty())
        return result;

    QStringList lines = text.split('\n');
    CommitInfo current;

    for (const QString &line : lines) {
        if (line.isEmpty()) {
            // 空行 = commit 分隔符，保存当前 commit 并准备下一个
            if (!current.hash.isEmpty()) {
                result.append(current);
                current = CommitInfo{};  // 重置为默认值
            }
            continue;
        }

        if (line.contains('|')) {
            // 包含 | = commit 头行: hash|author|date|message
            QStringList parts = line.split('|');
            if (parts.size() >= 4) {
                current.hash = parts[0];
                current.author = parts[1];
                current.dateTime = QDateTime::fromString(parts[2], Qt::ISODate);
                // message 可能包含 |，所以用 mid(3) 把剩余部分拼回去
                current.message = parts.mid(3).join('|');
            }
        } else {
            // 不包含 | = 文件路径行
            current.files.append(line.trimmed());
        }
    }

    // 最后一个 commit（没有空行结尾时）
    if (!current.hash.isEmpty())
        result.append(current);

    return result;
}

// ==================== 核心：执行 Git 命令 ====================

QByteArray GitManager::runGit(const QStringList &args, bool *ok, QString *errorMsg)
{
    /*
     * QProcess: Qt 提供的外部进程调用类
     *
     * 使用模式:
     *   1. 设置工作目录 (setWorkingDirectory)
     *   2. 启动进程 (start)
     *   3. 等待启动 (waitForStarted)
     *   4. 等待完成 (waitForFinished)
     *   5. 检查退出码
     *   6. 读取输出
     */

    QProcess process;
    process.setWorkingDirectory(m_workspacePath);

    // start("git", {"log", "--format=...", "--name-only"})
    // 等效于在命令行执行: git log --format=... --name-only
    process.start("git", args);

    if (!process.waitForStarted(m_timeoutMs)) {
        if (ok) *ok = false;
        if (errorMsg) *errorMsg = "git 启动失败，请确认已安装 Git 并加入 PATH";
        emit errorOccurred("启动", "git 启动失败");
        return {};
    }

    if (!process.waitForFinished(m_timeoutMs)) {
        process.kill();  // 超时则强制结束
        if (ok) *ok = false;
        if (errorMsg) *errorMsg = "git 命令超时";
        emit errorOccurred("超时", "git 命令执行超时");
        return {};
    }

    if (process.exitCode() != 0) {
        // git 命令返回非 0 = 执行出错
        if (ok) *ok = false;
        QString stderrStr = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (errorMsg) *errorMsg = stderrStr;
        emit errorOccurred(args.join(' '), stderrStr);
        return {};
    }

    if (ok) *ok = true;
    return process.readAllStandardOutput();
}
