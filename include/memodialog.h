#ifndef MEMODIALOG_H
#define MEMODIALOG_H

#include <QDialog>
#include <QJsonObject>

class QListWidget;
class QListWidgetItem;
class QTextEdit;
class QWebEngineView;
class QTimer;
class QSplitter;
class QLabel;
class QPushButton;
class QComboBox;
class QStackedWidget;
class GitManager;

class MemoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MemoDialog(const QString &workspacePath, GitManager *git = nullptr,
                        QWidget *parent = nullptr);
    void selectFile(const QString &filePath);

private slots:
    void onChapterChanged();
    void onFileClicked(QListWidgetItem *item);
    void onOverviewContextMenu(const QPoint &pos);
    void onAddFile();
    void onBackToOverview();
    void onNoteSelected();
    void onNewNote();
    void onDeleteNote();
    void onEditorChanged();
    void updatePreview();
    void insertMarkdown(const QString &before, const QString &after = {});
    void wrapSelection(const QString &before, const QString &after);

private:
    void refreshOverview();
    void loadNotes(const QString &filePath);
    void saveCurrentNote();
    void switchToFileView(const QString &filePath);
    QString notesFilePath() const;
    QJsonObject readNotesFile() const;
    void writeNotesFile(const QJsonObject &obj);
    GitManager *m_gitManager;
    QString m_workspacePath;
    QString m_currentFilePath;
    QString m_currentNoteId;

    QStackedWidget *m_stack;

    // Page 0: overview
    QComboBox  *m_chapterCombo;
    QListWidget *m_overviewList;
    QPushButton *m_addFileBtn;

    // Page 1: file notes
    QLabel      *m_fileLabel;
    QPushButton *m_backBtn;
    QListWidget *m_noteList;
    QTextEdit   *m_editor;
    QWebEngineView *m_preview;
    QSplitter   *m_editSplitter;
    QPushButton *m_newNoteBtn;
    QPushButton *m_delNoteBtn;
    QTimer      *m_debounce;
};

#endif // MEMODIALOG_H
