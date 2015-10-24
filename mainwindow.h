#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QStringListModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QLineEdit>
#include <QMediaPlayer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum FILE_TYPES {
        TYPE_UNKNOWN,
        TYPE_IMAGE,
        TYPE_VIDEO,
        TYPE_GIF
    };

    QString targetPath;
    QDir *targetDir;
    QStringListModel *listModel;
    QItemSelectionModel *selection;

    QString currentFileName;
    QModelIndex currentFileIndex;

    QMediaPlayer *player;

    bool suppressNameChange;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void updateTargetDir();
    void updateFileList();
    void updateCurrentFile();
    void setCurrentFile(QModelIndex index);
    void updateFilePreview();
    void commitName();
    bool moveCurrentFile(QString newPath);
    void clearRow(QModelIndex index);
    void selectByIndex(QModelIndex index);
    void selectFileName();
    int findExtensionStart(QString str);
    void autocompleteDir(QLineEdit *input);
    void resetInput(QLineEdit *input);
    void tryFilenameInsert();

    QString getCurrentFullPath();
    QString dirPathWithSeparator();
    FILE_TYPES findFileType();
    bool isFileValid();

protected:
    bool eventFilter(QObject *, QEvent *);

private slots:
    void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);

    void on_baseField_textChanged(const QString &arg1);

    void on_nameField_returnPressed();

    void on_nameField_selectionChanged();

    void on_nameField_cursorPositionChanged(int arg1, int arg2);

    void on_nameField_textEdited(const QString &arg1);

    void on_baseField_returnPressed();

    void on_nameField_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
