#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QString>

/*
 * QFileSystemWatcher 是 Qt 提供的文件系统监控类。
 * 它能监控指定文件/目录的变更（修改、重命名、删除等），并发出信号。
 *
 * 我们用它来检测用户用外部编辑器（LaTeX/Word）修改章节文件后，
 * 及时更新状态栏的提示。
 */

class FileWatcher : public QObject
{
    Q_OBJECT
public:
    explicit FileWatcher(QObject *parent = nullptr);

    void watchWorkspace(const QString &path);   // 开始监控整个工作区
    void stopWatching();                         // 停止监控

signals:
    // 当工作区有任何文件变更时发出
    void fileChanged(const QString &path);

private:
    QFileSystemWatcher *m_watcher;
    QString m_workspacePath;
};

#endif // FILEWATCHER_H
