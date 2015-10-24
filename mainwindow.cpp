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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->player = new QMediaPlayer(this);
    this->listModel = new QStringListModel();
    this->ui->fileList->setModel(this->listModel);
    this->suppressNameChange = false;

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

    if (!this->targetDir->exists())
    {
        this->listModel->setStringList(QStringList());
    }
    else
    {
        this->listModel->setStringList(this->targetDir->entryList(QDir::Files, QDir::Time));
        this->selectByIndex(this->listModel->index(0, 0));
    }

    this->updateCurrentFile();
}

void MainWindow::updateCurrentFile()
{
    // this just grabs the first item in the list
    this->setCurrentFile(this->listModel->index(0));
}

void MainWindow::setCurrentFile(QModelIndex index)
{
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
    QString value = this->ui->nameField->text();
    std::cout << value.toStdString() << std::endl;

    QModelIndex index = this->listModel->index(this->currentFileIndex.row() + 1);
    QModelIndex oldIndex = QModelIndex(this->currentFileIndex);

    std::cout << "comparing: " << value.toStdString() << " == " << this->currentFileName.toStdString() << std::endl;
    if (value != this->currentFileName)
    {
        if (!this->moveCurrentFile(value))
        {
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
    std::cout << "Pretending to move file to: " << fullPath.toStdString() << std::endl;

    if (newFile.exists())
    {
        std::cout << "File already exists: " << fullPath.toStdString() << std::endl;
        return false;
    }

    int idx = newPath.lastIndexOf(QDir::separator());

    if (idx == -1)
    {
        idx = 0;
    }

    QString dirPath = newPath.left(idx);

    if (dirPath.length() > 0) {
        if (!this->targetDir->mkpath(dirPath))
        {
            std::cout << "mkpath failed: " << dirPath.toStdString() << std::endl;
            return false;
        }

        QDir fullDir(this->dirPathWithSeparator() + dirPath);
        if (!fullDir.exists())
        {
            std::cout << "fullDir doesn't exist for some reason: " << fullDir.absolutePath().toStdString() << std::endl;
            return false;
        }
    }

    QFile oldFile(this->targetDir->filePath(this->currentFileName));
    if (!oldFile.rename(fullPath))
    {
        std::cout << "rename failed: " << oldFile.fileName().toStdString() << std::endl;
    }

    return true;
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

    QGraphicsPixmapItem *imageItem;

    QLabel *gifLabel;
    QMovie *gifMovie;
    QGraphicsProxyWidget *gifProxy;

    QGraphicsVideoItem *videoItem;
    QMediaPlaylist *playlist;

    // stop any videos which may be playing
    this->player->stop();

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
        imageItem = new QGraphicsPixmapItem(QPixmap(this->getCurrentFullPath()));
        scene->addItem(imageItem);

        fitTarget = (QGraphicsItem*)imageItem;
        break;
    case TYPE_GIF:
        // gif is a special case. apparently the only real way to play an animated
        // gif in Qt is with a QLabel
        gifLabel = new QLabel();
        gifMovie = new QMovie(this->getCurrentFullPath());
        gifLabel->setMovie(gifMovie);
        gifMovie->start();
        gifProxy = scene->addWidget(gifLabel);

        fitTarget = (QGraphicsItem*)gifProxy;
        break;
    case TYPE_VIDEO:
        videoItem = new QGraphicsVideoItem();
        playlist = new QMediaPlaylist();

        // apparently we need to explicitly set a size or the call to
        // fitInView later won't know how big the QGraphicsItem is
        videoItem->setSize(QSizeF(720,480));

        this->player->setVideoOutput(videoItem);
        scene->addItem(videoItem);

        // using a playlist seems to be the easiest way to loop a video
        playlist->addMedia(QMediaContent(QUrl::fromLocalFile(this->getCurrentFullPath())));
        playlist->setPlaybackMode(QMediaPlaylist::Loop);
        this->player->setPlaylist(playlist);

        this->player->play();

        fitTarget = (QGraphicsItem*)videoItem;
        break;
    default:
        break;
    }

    if (fitTarget != NULL)
    {
        this->ui->graphicsView->fitInView(fitTarget, Qt::KeepAspectRatio);
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

    if (!path.endsWith(QDir::separator())) {
        path.append(QDir::separator());
    }

    return path;
}

bool MainWindow::isFileValid()
{
    std::cout << this->getCurrentFullPath().toStdString() << std::endl;
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
    int idx = str.lastIndexOf(QChar('.'));
}

void MainWindow::autocompleteDir(QLineEdit *input)
{
    std::cout << "trying autocomplete for: " << input->text().toStdString() << std::endl;
    QString value = input->text();
    QString basePath("");

    if (input == this->ui->nameField) {
        basePath = this->targetDir->absolutePath() + QDir::separator();
    }

    int cursorPos = input->cursorPosition();
    int sepPos = value.lastIndexOf(QDir::separator());

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

    std::cout << leftPart.toStdString() << " : " << pathPart.toStdString() << " : " << dirPart.toStdString() << " : " << sepPos << std::endl;

    dir.setPath(basePath + pathPart);

    if (!dir.exists()) {
        std::cout << dir.absolutePath().toStdString() << " isn't a dir, bailing" << std::endl;
        return;
    }

    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QString findPart = QString(dirPart);
    if (findPart.startsWith(QDir::separator()))
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
        completion.append(QDir::separator());
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

    std::cout << rightPart.toStdString() << " : " << extPart.toStdString() << " : " << leftPart.toStdString() << " : " << this->currentFileName.toStdString() << std::endl;

    if (rightPart != extPart || !leftPart.endsWith(QDir::separator()))
    {
        return;
    }

    std::cout << "gonna set text to " << (leftPart + this->currentFileName).toStdString() << std::endl;

    this->suppressNameChange = true;
    this->ui->nameField->setText(leftPart + this->currentFileName);
    this->suppressNameChange = false;
    this->selectFileName();
    std::cout << curPos << std::endl;
    this->ui->nameField->setCursorPosition(curPos);
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
