#include "filewatcher.h"
#include <QDir>

FileWatcher::FileWatcher(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
{
    // 当监听到文件变更，转发信号
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &FileWatcher::fileChanged);

    // 当目录内容变化（新增/删除文件），重新注册监听
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, [this] {
        // 重新扫描目录，把新文件也加入监听
        if (!m_workspacePath.isEmpty())
            watchWorkspace(m_workspacePath);
    });
}

void FileWatcher::watchWorkspace(const QString &path)
{
    stopWatching();
    m_workspacePath = path;

    // 添加工作区根目录
    m_watcher->addPath(path);

    // 递归添加所有子文件夹（章节文件夹及其内部文件夹）
    QDir dir(path);
    QStringList entries = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        QString fullPath = path + "/" + entry;
        m_watcher->addPath(fullPath);

        // 二级子目录（例如 ch01/images/）
        QFileInfo fi(fullPath);
        if (fi.isDir()) {
            QDir subDir(fullPath);
            QStringList subEntries = subDir.entryList(
                QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
            for (const QString &sub : subEntries) {
                m_watcher->addPath(fullPath + "/" + sub);
            }
        }
    }
}

void FileWatcher::stopWatching()
{
    if (!m_workspacePath.isEmpty()) {
        QStringList allPaths = m_watcher->files() + m_watcher->directories();
        if (!allPaths.isEmpty())
            m_watcher->removePaths(allPaths);
    }
    m_workspacePath.clear();
}
