#include "gitmanager.h"
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>

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

    QStringList args = {"-c", "core.quotepath=false", "log",
                        "--format=%H|%an|%aI|%s", "--name-only"};
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

bool GitManager::hasUncommittedChanges(const QString &folder)
{
    QStringList args = {"status", "--porcelain"};
    if (!folder.isEmpty())
        args << "--" << (folder + "/*");
    bool ok;
    QByteArray output = runGit(args, &ok);
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

// ==================== 远程仓库 ====================

bool GitManager::hasRemote() const
{
    QProcess process;
    process.setWorkingDirectory(m_workspacePath);
    process.start("git", {"remote", "get-url", "origin"});
    process.waitForFinished(5000);
    return process.exitCode() == 0;
}

bool GitManager::addRemote(const QString &url)
{
    QProcess process;
    process.setWorkingDirectory(m_workspacePath);
    process.start("git", {"remote", "add", "origin", url});
    if (!process.waitForStarted(10000))
        return false;
    if (!process.waitForFinished(10000))
        return false;
    return process.exitCode() == 0;
}

bool GitManager::push(const QString &branch)
{
    QString b = branch.isEmpty() ? currentBranch() : branch;
    QProcess process;
    process.setWorkingDirectory(m_workspacePath);
    process.start("git", {"push", "-u", "origin", b});
    if (!process.waitForStarted(10000))
        return false;
    if (!process.waitForFinished(60000)) {
        process.kill();
        return false;
    }
    if (process.exitCode() != 0) {
        emit errorOccurred("push", QString::fromUtf8(process.readAllStandardError()).trimmed());
        return false;
    }
    return true;
}

bool GitManager::cloneRepo(const QString &url, const QString &targetPath)
{
    QProcess process;
    process.start("git", {"clone", url, targetPath});
    if (!process.waitForStarted(10000))
        return false;
    if (!process.waitForFinished(300000)) {
        process.kill();
        return false;
    }
    if (process.exitCode() != 0) {
        emit errorOccurred("clone", QString::fromUtf8(process.readAllStandardError()).trimmed());
        return false;
    }
    return true;
}

// ==================== 分支管理 ====================

QStringList GitManager::listBranches()
{
    bool ok;
    QByteArray output = runGit({"branch"}, &ok);
    if (!ok) return {};

    QStringList branches;
    for (const QString &line : QString::fromUtf8(output).split('\n')) {
        QString name = line.trimmed();
        if (name.startsWith('*')) name = name.mid(1).trimmed();  // 去掉当前标记 *
        if (!name.isEmpty()) branches.append(name);
    }
    return branches;
}

bool GitManager::createBranch(const QString &name)
{
    bool ok;
    runGit({"branch", name}, &ok);
    return ok;
}

bool GitManager::switchBranch(const QString &name)
{
    bool ok;
    runGit({"switch", name}, &ok);
    return ok;
}

bool GitManager::mergeBranch(const QString &name)
{
    bool ok;
    runGit({"merge", name}, &ok);
    return ok;
}

bool GitManager::deleteBranch(const QString &name)
{
    bool ok;
    runGit({"branch", "-d", name}, &ok);
    return ok;
}

bool GitManager::dropCommit(const QString &hash)
{
    bool ok;
    runGit({"rebase", "--onto", hash + "~1", hash}, &ok);
    return ok;
}

bool GitManager::amendMessage(const QString &hash, const QString &newMessage)
{
    // 如果是最新 commit（HEAD），直接用 commit --amend
    QByteArray headHash = runGit({"rev-parse", "HEAD"});
    if (QString::fromUtf8(headHash).trimmed() == hash.trimmed()) {
        bool ok;
        runGit({"commit", "--amend", "-m", newMessage}, &ok);
        return ok;
    }

    // 非 HEAD：使用 rebase -i，通过挂载 GIT_SEQUENCE_EDITOR 和 GIT_EDITOR
    // 将消息写入临时文件，再用 cp 覆盖 rebase 给的编辑器文件
    QTemporaryFile msgFile;
    msgFile.open();
    QString msgPath = msgFile.fileName();
    msgFile.write(newMessage.toUtf8());
    msgFile.close();
    // Windows 路径转 Unix 格式，供 Git 自带的 sh 使用
    msgPath.replace('\\', '/');

    // GIT_SEQUENCE_EDITOR 脚本：将 todo 列表中的 pick HASH 改为 reword HASH
    QTemporaryFile seqScript;
    seqScript.open();
    QString seqPath = seqScript.fileName();
    seqScript.write(
        QString("#!/bin/sh\nsed 's/^pick %1/reword %1/' \"$1\" > \"$1.tmp\" && mv \"$1.tmp\" \"$1\"\n")
            .arg(hash.left(7)).toUtf8());
    seqScript.close();
    seqPath.replace('\\', '/');

    // GIT_EDITOR 脚本：将提前写好的消息文件复制到 rebase 给的文件
    QTemporaryFile editorScript;
    editorScript.open();
    QString editorPath = editorScript.fileName();
    editorScript.write(
        QString("#!/bin/sh\ncp \"%1\" \"$1\"\n").arg(msgPath).toUtf8());
    editorScript.close();
    editorPath.replace('\\', '/');

    QProcess process;
    process.setWorkingDirectory(m_workspacePath);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("GIT_SEQUENCE_EDITOR", "sh \"" + seqPath + "\"");
    env.insert("GIT_EDITOR", "sh \"" + editorPath + "\"");
    process.setProcessEnvironment(env);
    process.start("git", {"rebase", "-i", hash + "~1"});
    if (!process.waitForFinished(m_timeoutMs)) {
        process.kill();
        runGit({"rebase", "--abort"});
        return false;
    }
    if (process.exitCode() != 0) {
        runGit({"rebase", "--abort"});
        emit errorOccurred("amendMessage",
            QString::fromUtf8(process.readAllStandardError()).trimmed());
        return false;
    }
    return true;
}

QString GitManager::mergeBase(const QString &a, const QString &b)
{
    bool ok;
    QByteArray output = runGit({"merge-base", a, b}, &ok);
    return ok ? QString::fromUtf8(output).trimmed() : QString();
}

// ==================== 日志解析（静态方法） ====================

QList<CommitInfo> GitManager::parseLogOutput(const QByteArray &output)
{
    QList<CommitInfo> result;
    QString text = QString::fromUtf8(output);
    if (text.isEmpty())
        return result;

    text.replace("\r\n", "\n");
    text.replace('\r', '\n');

    QStringList lines = text.split('\n');
    CommitInfo current;

    for (const QString &line : lines) {
        QString t = line.trimmed();
        if (t.isEmpty())
            continue;  // 跳过空行（commit header 和文件列表之间有空行）

        if (t.contains('|')) {
            // 遇到新 commit header，先保存上一个 commit
            if (!current.hash.isEmpty()) {
                result.append(current);
                current = CommitInfo{};
            }
            QStringList parts = t.split('|');
            if (parts.size() >= 4) {
                current.hash = parts[0];
                current.author = parts[1];
                current.dateTime = QDateTime::fromString(parts[2], Qt::ISODate);
                current.message = parts.mid(3).join('|');
            }
        } else {
            current.files.append(t);
        }
    }

    if (!current.hash.isEmpty())
        result.append(current);

    return result;
}

// ==================== 环境检测与配置 ====================

bool GitManager::isGitInstalled()
{
    QProcess process;
    process.start("git", {"--version"});
    if (!process.waitForStarted(5000))
        return false;
    if (!process.waitForFinished(5000))
        return false;
    return process.exitCode() == 0;
}

bool GitManager::installGit()
{
#ifdef Q_OS_WIN
    QProcess process;
    // winget 是 Windows 10 1809+ 内置的包管理器
    process.start("winget", {"install", "--id", "Git.Git", "-e",
                             "--silent",
                             "--accept-source-agreements",
                             "--accept-package-agreements"});
    if (!process.waitForStarted(10000))
        return false;  // winget 不可用
    if (!process.waitForFinished(300000)) {  // 5 分钟超时
        process.kill();
        return false;
    }
    return process.exitCode() == 0;
#else
    return false;  // 非 Windows 平台暂不支持自动安装
#endif
}

bool GitManager::configureUser(const QString &name, const QString &email)
{
    QProcess process;

    process.start("git", {"config", "--global", "user.name", name});
    if (!process.waitForStarted(10000) || !process.waitForFinished(10000))
        return false;
    if (process.exitCode() != 0)
        return false;

    process.start("git", {"config", "--global", "user.email", email});
    if (!process.waitForStarted(10000) || !process.waitForFinished(10000))
        return false;
    return process.exitCode() == 0;
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
