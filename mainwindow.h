#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QStringListModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QLabel>

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
    QGraphicsScene *previewScene;

    QGraphicsPixmapItem *imageItem;

    QLabel *gifLabel;
    QMovie *gifMovie;
    QGraphicsProxyWidget *gifProxy;

    QGraphicsVideoItem *videoItem;
    QMediaPlaylist *playlist;

    bool suppressNameChange;
    bool numericFiles;
    bool autoIncrement;

    QFileDialog *browseDialog;

    int volume;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void updateTargetDir();
    void updateFileList();
    void updateCurrentFile();
    void previousRow();
    void setCurrentFile(QModelIndex index);
    void updateFilePreview();
    void cleanupFilePreview();
    void commitName();
    bool moveCurrentFile(QString newPath);
    void clearRow(QModelIndex index);
    void selectByIndex(QModelIndex index);
    void selectFileName();
    int findExtensionStart(QString str);
    void autocompleteDir(QLineEdit *input);
    void resetInput(QLineEdit *input);
    void tryFilenameInsert();
    void showMessage(QString message);
    void setVolume(int volume);
    QString findAutoIncrementName(QString targetPath, QString basePath);

    QString sep();
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

    void on_actionExit_triggered();

    void on_actionBrowse_for_base_folder_triggered();

    void on_volumeSlider_valueChanged(int value);

    void on_fileList_clicked(const QModelIndex &index);

    void on_checkBox_2_stateChanged(int checkedness);

    void on_autoIncCheckbox_stateChanged(int checkedness);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
