#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStandardPaths>
#include <iostream>
#include <QMimeDatabase>
#include <QMimeType>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsVideoItem>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QMovie>
#include <QLabel>
#include <QGraphicsProxyWidget>
#include <QRegExp>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->listModel = new QStringListModel();
    this->ui->fileList->setModel(this->listModel);
    this->suppressNameChange = false;
    this->browseDialog = new QFileDialog(this, "Pick Target Directory");

    this->player = NULL;
    this->previewScene = NULL;
    this->imageItem = NULL;
    this->gifLabel = NULL;
    this->gifMovie = NULL;
    this->gifProxy = NULL;
    this->videoItem = NULL;
    this->playlist = NULL;

    this->browseDialog->setFileMode(QFileDialog::Directory);
    this->browseDialog->setOptions(QFileDialog::ShowDirsOnly);

    this->volume = 50;
    this->numericFiles = false;
    this->autoIncrement = false;

    this->resetInput(this->ui->baseField);

    this->selection = this->ui->fileList->selectionModel();
    QObject::connect(
        this->selection, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionChanged(QItemSelection,QItemSelection))
    );

    this->ui->nameField->installEventFilter(this);
    this->ui->baseField->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    delete this->browseDialog;
    delete this->listModel;

    this->cleanupFilePreview();
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == this->ui->nameField || obj == this->ui->baseField) && event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Tab) {
            this->autocompleteDir((QLineEdit*)obj);
            if (obj == this->ui->nameField)
            {
                this->tryFilenameInsert();
            }
            return true;
        }

        if (key->key() == Qt::Key_Escape) {
            this->resetInput((QLineEdit*)obj);
            return true;
        }

        if (obj == this->ui->nameField && key->key() == Qt::Key_Return && key->modifiers() == Qt::SHIFT)
        {
            this->previousRow();
            return true;
        }
    }
    return false;
}

void MainWindow::updateTargetDir()
{
    this->targetDir = new QDir(this->targetPath);
    this->updateFileList();
}

void MainWindow::updateFileList()
{
    QModelIndex index;
    QStringList files;

    if (!this->targetDir->exists())
    {
        this->listModel->setStringList(QStringList());
    }
    else
    {
        files = this->targetDir->entryList(QDir::Files, QDir::Time);
        if (this->numericFiles)
        {
            files = files.filter(QRegularExpression(QString("^\\d+\\.\\w+$")));
        }
        this->listModel->setStringList(files);
        this->selectByIndex(this->listModel->index(0, 0));
    }

    this->updateCurrentFile();
}

void MainWindow::updateCurrentFile()
{
    // this just grabs the first item in the list
    this->setCurrentFile(this->listModel->index(0));
}

void MainWindow::previousRow()
{
    QModelIndex index = this->listModel->index(this->currentFileIndex.row() - 1);
    if (index.isValid())
    {
        this->selectByIndex(index);
    }
}

void MainWindow::setCurrentFile(QModelIndex index)
{
    this->cleanupFilePreview();

    this->currentFileName = QString("");
    this->currentFileIndex = index;
    QVariant row = this->listModel->data(this->currentFileIndex, 0);

    if (row.isValid() && !row.isNull()) {
        this->currentFileName = row.toString();
    }

    this->ui->nameField->setText(this->currentFileName);
    this->selectFileName();

    this->updateFilePreview();
}

void MainWindow::commitName()
{
    this->cleanupFilePreview();

    QString value = this->ui->nameField->text();

    QModelIndex index = this->listModel->index(this->currentFileIndex.row() + 1);
    QModelIndex oldIndex = QModelIndex(this->currentFileIndex);

    if (value != this->currentFileName)
    {
        if (!this->moveCurrentFile(value))
        {
            this->updateFilePreview();
            return;
        }

        index = this->ui->fileList->selectionModel()->selectedIndexes().first();
        this->clearRow(oldIndex);
        this->selectByIndex(oldIndex);
    }
    else
    {
        this->selectByIndex(index);
    }
}

bool MainWindow::moveCurrentFile(QString newPath)
{
    QString fullPath = this->dirPathWithSeparator().append(newPath);
    QFile newFile(fullPath);

    if (newFile.exists())
    {
        if (!this->autoIncrement)
        {
            this->showMessage(tr("File already exists: ") + fullPath);
            return false;
        }
        else
        {
            newPath = this->findAutoIncrementName(newPath, this->dirPathWithSeparator());
            fullPath = this->dirPathWithSeparator().append(newPath);
        }
    }

    int idx = newPath.lastIndexOf(this->sep());

    if (idx == -1)
    {
        idx = 0;
    }

    QString dirPath = newPath.left(idx);

    if (dirPath.length() > 0) {
        if (!this->targetDir->mkpath(dirPath))
        {
            this->showMessage(tr("Failure trying to create path: ") + dirPath);
            return false;
        }

        QDir fullDir(this->dirPathWithSeparator() + dirPath);
        if (!fullDir.exists())
        {
            this->showMessage(tr("The directory we created doesn't exist for some reason: ") + fullDir.absolutePath());
            return false;
        }
    }

    QFile oldFile(this->targetDir->filePath(this->currentFileName));
    if (!oldFile.rename(fullPath))
    {
        this->showMessage(tr("Rename operation failed for some reason: ") + oldFile.fileName() + " to " + fullPath);
        return false;
    }

    this->showMessage(tr("Renamed to: ") + newPath);

    return true;
}

QString MainWindow::findAutoIncrementName(QString targetPath, QString basePath)
{
    QString newName;
    QRegExp rx = QRegExp("\\/?(.+)\\.(\\w+)$");
    QString name;
    QString ext;
    int offset = 2;

    int lastSlash = targetPath.lastIndexOf(QChar('/'));

    if (lastSlash == -1)
    {
        lastSlash = 0;
    }
    else
    {
        lastSlash++;
    }

    rx.indexIn(targetPath, lastSlash);


    name = rx.cap(1);
    ext = rx.cap(2);

    do {
        newName = QString("");
        newName.append(targetPath.left(lastSlash));
        newName.append(name);
        newName.append(QString(" (%1)."));
        newName.append(ext);
        newName = newName.arg(offset++);
    } while(QFile(basePath + newName).exists());

    return newName;
}

void MainWindow::clearRow(QModelIndex index)
{
    if (index.isValid())
    {
        this->listModel->removeRow(index.row());
    }
}

void MainWindow::selectByIndex(QModelIndex index)
{
    if (index.isValid())
    {
        this->ui->fileList->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        this->ui->fileList->scrollTo(index);
    }
}

void MainWindow::updateFilePreview()
{
    QGraphicsScene *scene;
    QGraphicsItem *fitTarget = NULL;

    // setup the scene and clear current preview first
    if (this->ui->graphicsView->scene())
    {
        // there's already a scene, use that
        scene = this->ui->graphicsView->scene();
        scene->clear();
    }
    else
    {
        // there's no scene yet, make a new one
        scene = new QGraphicsScene();
        this->ui->graphicsView->setScene(scene);
    }

    if (!this->isFileValid()) {
        return;
    }

    // build new preview based on type
    switch(this->findFileType())
    {
    case TYPE_IMAGE:
        this->imageItem = new QGraphicsPixmapItem(QPixmap(this->getCurrentFullPath()));
        scene->addItem(this->imageItem);

        fitTarget = (QGraphicsItem*)imageItem;
        break;
    case TYPE_GIF:
        // gif is a special case. apparently the only real way to play an animated
        // gif in Qt is with a QLabel
        this->gifLabel = new QLabel();
        this->gifMovie = new QMovie(this->getCurrentFullPath());
        this->gifLabel->setMovie(this->gifMovie);
        this->gifMovie->start();
        this->gifProxy = scene->addWidget(this->gifLabel);

        fitTarget = (QGraphicsItem*)this->gifProxy;
        break;
    case TYPE_VIDEO:
        this->videoItem = new QGraphicsVideoItem();
        this->playlist = new QMediaPlaylist();
        this->player = new QMediaPlayer();
        this->player->setVolume(this->volume);

        // apparently we need to explicitly set a size or the call to
        // fitInView later won't know how big the QGraphicsItem is
        this->videoItem->setSize(QSizeF(720,480));

        this->player->setVideoOutput(this->videoItem);
        scene->addItem(videoItem);

        // using a playlist seems to be the easiest way to loop a video
        this->playlist->addMedia(QMediaContent(QUrl::fromLocalFile(this->getCurrentFullPath())));
        this->playlist->setPlaybackMode(QMediaPlaylist::Loop);
        this->player->setPlaylist(this->playlist);

        this->player->play();

        fitTarget = (QGraphicsItem*)this->videoItem;
        break;
    default:
        break;
    }

    if (fitTarget != NULL)
    {
        this->ui->graphicsView->fitInView(fitTarget, Qt::KeepAspectRatio);
    }
}

void MainWindow::cleanupFilePreview()
{

    if (this->player)
    {
        this->player->stop();
        delete this->player;
    }

    if (this->playlist)
    {
        delete this->playlist;
    }

    if (this->videoItem)
    {
        delete this->videoItem;
    }

    if (this->imageItem)
    {
        delete this->imageItem;
    }

    if (this->gifMovie)
    {
        this->gifMovie->stop();
        delete this->gifMovie;
    }

    if (this->gifLabel != NULL)
    {
        delete this->gifLabel;
    }

    if (this->gifProxy)
    {
        //delete this->gifProxy; // segfault. i guess this gets deleted somewhere else.
    }

    this->player = NULL;
    this->previewScene = NULL;
    this->imageItem = NULL;
    this->gifLabel = NULL;
    this->gifMovie = NULL;
    this->gifProxy = NULL;
    this->videoItem = NULL;
    this->playlist = NULL;

    if (this->ui->graphicsView->scene())
    {
        this->ui->graphicsView->scene()->clear();
    }
}

QString MainWindow::getCurrentFullPath()
{
    QString path = this->dirPathWithSeparator();

    path.append(this->currentFileName);

    return path;
}

QString MainWindow::dirPathWithSeparator()
{
    QString path = QString(this->targetDir->path());

    if (!path.endsWith(this->sep())) {
        path.append(this->sep());
    }

    return path;
}

bool MainWindow::isFileValid()
{
    QFile *file = new QFile(this->getCurrentFullPath());

    if (
            this->currentFileName.length() == 0 ||
            !file->exists()
       )
    {
        delete file;
        return false;
    }

    delete file;
    return true;
}

MainWindow::FILE_TYPES MainWindow::findFileType()
{
    QMimeDatabase mimeDatabase;
    QMimeType mimeType;
    QFile *file = new QFile(this->getCurrentFullPath());

    mimeType = mimeDatabase.mimeTypeForFile(QFileInfo(*file));

    delete file;

    if (
            mimeType.inherits("video/webm") ||
            mimeType.inherits("video/ogg") ||
            mimeType.inherits("video/quicktime") ||
            mimeType.inherits("video/x-msvideo") ||
            mimeType.inherits("video/mp4") ||
            mimeType.inherits("video/x-flv")
       )
    {
        return TYPE_VIDEO;
    }

    if (
            mimeType.inherits("image/jpg") ||
            mimeType.inherits("image/jpeg") ||
            mimeType.inherits("image/png") ||
            mimeType.inherits("image/bmp") ||
            mimeType.inherits("image/pbm")
       )
    {
        return TYPE_IMAGE;
    }

    if (mimeType.inherits("image/gif"))
    {
        // gif is a dum special case, see updateFilePreview
        return TYPE_GIF;
    }

    return TYPE_UNKNOWN;
}

void MainWindow::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (selected.indexes().length() == 0) {
        this->updateCurrentFile();
        return;
    }
    else
    {
        this->setCurrentFile(selected.indexes().first());
    }
}

void MainWindow::selectFileName()
{
    if (this->suppressNameChange)
    {
        return;
    }

    QString value = this->ui->nameField->text();

    int idx = value.lastIndexOf(this->currentFileName);

    if (idx == -1) {
        return;
    }

    int extIdx = this->findExtensionStart(value);
    if (extIdx == -1) {
        extIdx = value.length();
    }

    this->suppressNameChange = true;
    this->ui->nameField->setSelection(idx, extIdx - idx);
    this->suppressNameChange = false;
}

int MainWindow::findExtensionStart(QString str)
{
    return str.lastIndexOf(QChar('.'));
}

void MainWindow::autocompleteDir(QLineEdit *input)
{
    QString value = input->text();
    QString basePath("");

    if (input == this->ui->nameField) {
        basePath = this->targetDir->absolutePath() + this->sep();
    }

    int cursorPos = input->cursorPosition();
    int sepPos = value.lastIndexOf(this->sep());

    if (sepPos > cursorPos) {
        return;
    }

    if (sepPos == -1) {
        sepPos = 0;
    }

    QString leftPart = value.left(cursorPos);
    QString pathPart = value.left(sepPos);
    QString dirPart = leftPart.right(leftPart.length() - sepPos);
    QDir dir;

    dir.setPath(basePath + pathPart);

    if (!dir.exists()) {
        return;
    }

    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QString findPart = QString(dirPart);
    if (findPart.startsWith(this->sep()))
    {
        findPart = findPart.right(findPart.length() - 1);
    }
    QRegExp rx(QString("^").append(findPart));
    dirs = dirs.filter(rx);

    if (dirs.length() == 0)
    {
        return;
    }

    QString completionDir("");

    if (dirs.length() == 1)
    {
        completionDir = dirs.first();
    }
    else
    {
        bool moreStuff = true;
        int charPos = 0;
        QChar currentChar;
        while(moreStuff)
        {
            for (int i = 0; i < dirs.length(); i++)
            {
                if (charPos >= dirs[i].length())
                {
                    moreStuff = false;
                    break;
                }

                if (i == 0) {
                    currentChar = dirs[i][charPos];
                }

                if (dirs[i][charPos] != currentChar) {
                    moreStuff = false;
                    break;
                }
            }

            charPos++;

            if (moreStuff)
            {
                completionDir.append(currentChar);
            }
        }
    }

    QString completion = completionDir.right(completionDir.length() - findPart.length());

    if (dirs.length() == 1) {
        completion.append(this->sep());
    }

    value.insert(cursorPos, completion);
    input->setText(value);

    input->setCursorPosition(cursorPos + completion.length());
}

void MainWindow::resetInput(QLineEdit *input)
{
    if (input == this->ui->nameField)
    {
        this->ui->nameField->setText(this->currentFileName);

        return;
    }

    if (input == this->ui->baseField) {
        this->targetPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
        this->ui->baseField->setText(this->targetPath);

        return;
    }
}

void MainWindow::tryFilenameInsert()
{
    if (this->suppressNameChange)
    {
        return;
    }

    QString value = this->ui->nameField->text();
    int curPos = this->ui->nameField->cursorPosition();
    QString rightPart = value.right(value.length() - curPos);
    QString leftPart = value.left(curPos);
    QString extPart = this->currentFileName.right(this->currentFileName.length() - this->findExtensionStart(this->currentFileName));

    if (rightPart != extPart || !leftPart.endsWith(this->sep()))
    {
        return;
    }

    this->suppressNameChange = true;
    this->ui->nameField->setText(leftPart + this->currentFileName);
    this->suppressNameChange = false;
    this->selectFileName();
    this->ui->nameField->setCursorPosition(curPos);
}

QString MainWindow::sep()
{
    // so I guess I'll just always use / because I get them from QDir and shit in windows
    // return QDir::separator();
    return QString("/");
}

void MainWindow::showMessage(QString message)
{
    std::cout << message.toStdString() << std::endl;
    this->ui->statusBar->showMessage(message);
}

void MainWindow::setVolume(int volume)
{
    this->volume = volume;
    if (this->player)
    {
        this->player->setVolume(this->volume);
    }
}

void MainWindow::on_baseField_textChanged(const QString &path)
{
    this->targetPath = path;
    this->updateTargetDir();
}

void MainWindow::on_nameField_returnPressed()
{
    this->commitName();
}

void MainWindow::on_nameField_selectionChanged()
{
    this->selectFileName();
}

void MainWindow::on_nameField_cursorPositionChanged(int arg1, int arg2)
{
    this->selectFileName();
}

void MainWindow::on_baseField_returnPressed()
{
    this->ui->nameField->setFocus();
}

void MainWindow::on_nameField_textChanged(const QString &arg1)
{
    this->tryFilenameInsert();
}

void MainWindow::on_nameField_textEdited(const QString &arg1)
{

}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_actionBrowse_for_base_folder_triggered()
{
    this->browseDialog->setDirectory(this->targetDir->absolutePath());
    this->browseDialog->exec();
    QStringList dirs = this->browseDialog->selectedFiles();

    if (dirs.length() == 0)
    {
        return;
    }

    this->ui->baseField->setText(dirs.first());
}

void MainWindow::on_volumeSlider_valueChanged(int value)
{
    this->setVolume(value);
}

void MainWindow::on_fileList_clicked(const QModelIndex &index)
{
    this->ui->nameField->setFocus();
}

void MainWindow::on_checkBox_2_stateChanged(int checkedness)
{
    this->numericFiles = (checkedness == Qt::Checked);
    this->updateFileList();
}

void MainWindow::on_autoIncCheckbox_stateChanged(int checkedness)
{
    this->autoIncrement = (checkedness == Qt::Checked);
}
