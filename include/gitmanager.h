#ifndef GITMANAGER_H
#define GITMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>

// 描述一次 commit 的数据结构
struct CommitInfo {
    QString hash;           // commit 的完整 SHA 值
    QString author;         // 提交者
    QDateTime dateTime;     // 提交时间
    QString message;        // commit message
    QStringList files;      // 本次提交涉及的所有文件路径
};

class GitManager : public QObject
{
    Q_OBJECT
public:
    explicit GitManager(QObject *parent = nullptr);

    // 设置/获取工作区路径（即 git 仓库根目录）
    void setWorkspacePath(const QString &path);
    QString workspacePath() const;

    // 仓库操作
    bool initRepo();        // git init
    bool isRepo() const;    // 检查 .git 目录是否存在

    // 暂存与提交
    bool stageAll();        // git add -A
    bool commit(const QString &message);              // git commit -m "..."
    bool stageAndCommit(const QString &message);       // add + commit 一步完成

    // 查看历史
    // folderFilter: 按章节文件夹过滤 (如 "ch01-引言")
    // 传空串则返回全部 commit
    QList<CommitInfo> log(const QString &folderFilter = QString());

    // 章节回退: 将指定文件夹恢复到某个历史版本
    // relativePath: 章节文件夹名 (如 "ch01-引言")
    // hash: 目标 commit 的 SHA
    bool restoreFolder(const QString &relativePath, const QString &hash);

    // 文件夹操作 (用于章节重命名/删除)
    bool moveFolder(const QString &oldPath, const QString &newPath);  // git mv
    bool removeFolder(const QString &relativePath);                   // git rm -r

    // 状态查询
    bool hasUncommittedChanges(const QString &folder = {}); // 有未提交变更
    QString currentBranch();          // 当前分支名

    // 远程仓库
    bool hasRemote() const;                  // 是否已配置 origin
    bool addRemote(const QString &url);      // git remote add origin <url>
    bool push(const QString &branch = {});   // git push -u origin <branch>
    bool cloneRepo(const QString &url, const QString &targetPath); // git clone

    // 分支管理
    QStringList listBranches();              // git branch → 分支名列表
    QString mergeBase(const QString &a, const QString &b); // git merge-base
    bool createBranch(const QString &name);  // git branch <name>
    bool switchBranch(const QString &name);  // git switch <name>
    bool mergeBranch(const QString &name);   // git merge <name>
    bool deleteBranch(const QString &name);  // git branch -d <name>

    // 解析 git log 的原始输出
    static QList<CommitInfo> parseLogOutput(const QByteArray &output);

    // 环境检测与配置（静态方法，不依赖工作区）
    static bool isGitInstalled();                              // git --version
    static bool installGit();                                  // winget install Git.Git
    static bool configureUser(const QString &name,             // git config --global
                              const QString &email);

signals:
    // 发生错误时发出信号
    void errorOccurred(const QString &operation, const QString &errorDetail);

private:
    // 所有 git 命令的统一执行入口
    // 返回 stdout 内容；通过 ok 和 errorMsg 传出执行结果
    QByteArray runGit(const QStringList &args, bool *ok = nullptr,
                      QString *errorMsg = nullptr);

    QString m_workspacePath;      // git 仓库根目录
    int m_timeoutMs = 30000;      // 超时时间 30 秒
};

#endif // GITMANAGER_H
