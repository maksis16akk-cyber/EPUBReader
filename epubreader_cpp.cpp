// epubreader_cpp.cpp — Читалка EPUB с ночным режимом на C++ (Qt + minizip)

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenuBar>
#include <QAction>
#include <QKeySequence>
#include <QSettings>
#include <QTextStream>
#include <QByteArray>
#include <QBuffer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QTemporaryFile>
#include <QProcess>
#include <QWebEngineView>
#include <QWebEnginePage>

// Для упрощения используем QWebEngineView для отображения EPUB (распакованного содержимого)
// В реальности нужна библиотека для работы с EPUB, но для демонстрации используем распаковку zip.

class EpubReader : public QMainWindow {
    Q_OBJECT
public:
    EpubReader(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("📖 EPUBReader — C++");
        resize(900, 700);
        createUI();
        loadState();
        applyTheme();
    }

private slots:
    void openBook() {
        QString filename = QFileDialog::getOpenFileName(this, "Открыть EPUB", "", "EPUB (*.epub)");
        if (filename.isEmpty()) return;
        // Распаковываем во временную папку
        QString tempDir = QDir::tempPath() + "/epub_temp_" + QString::number(QDateTime::currentSecsSinceEpoch());
        QDir dir;
        if (!dir.mkpath(tempDir)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось создать временную папку");
            return;
        }
        // Распаковка через unzip (системный) — упрощённо
        QProcess unzip;
        unzip.start("unzip", QStringList() << filename << "-d" << tempDir);
        unzip.waitForFinished();
        if (unzip.exitCode() != 0) {
            QMessageBox::warning(this, "Ошибка", "Не удалось распаковать EPUB (нужен unzip)");
            return;
        }
        // Ищем файл .opf (metadata)
        QDir opfDir(tempDir);
        QStringList opfFiles = opfDir.entryList(QStringList() << "*.opf", QDir::Files | QDir::Readable, QDir::Name);
        QString opfPath;
        if (opfFiles.isEmpty()) {
            // рекурсивный поиск
            QDirIterator it(tempDir, QStringList() << "*.opf", QDir::Files, QDirIterator::Subdirectories);
            if (it.hasNext()) {
                opfPath = it.filePath();
            } else {
                QMessageBox::warning(this, "Ошибка", "Не найден файл .opf");
                return;
            }
        } else {
            opfPath = tempDir + "/" + opfFiles.first();
        }
        // Сохраняем путь и загружаем содержание
        currentBookPath = tempDir;
        // Получаем содержимое из файлов (упрощённо - просто ищем все HTML/ XHTML)
        // Для простоты загружаем все HTML в один QTextEdit
        QString text;
        QDirIterator htmlIt(tempDir, QStringList() << "*.html" << "*.xhtml", QDir::Files, QDirIterator::Subdirectories);
        while (htmlIt.hasNext()) {
            QString filePath = htmlIt.next();
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                QString content = stream.readAll();
                // Удаляем HTML теги
                content.remove(QRegExp("<[^>]*>"));
                text += content + "\n\n";
                file.close();
            }
        }
        if (text.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Не найдено текстовое содержимое");
            return;
        }
        // Сохраняем текст и разбиваем на страницы
        fullText = text;
        pages.clear();
        int pageSize = 2000;
        for (int i = 0; i < fullText.length(); i += pageSize) {
            pages.append(fullText.mid(i, pageSize));
        }
        currentPage = 0;
        showPage();
        statusLabel->setText("Загружено: " + QFileInfo(filename).fileName());
    }

    void showPage() {
        if (pages.isEmpty()) return;
        if (currentPage >= pages.size()) currentPage = pages.size() - 1;
        if (currentPage < 0) currentPage = 0;
        textEdit->setPlainText(pages[currentPage]);
        statusLabel->setText(QString("Страница %1/%2").arg(currentPage+1).arg(pages.size()));
    }

    void nextPage() {
        if (currentPage < pages.size() - 1) {
            currentPage++;
            showPage();
        }
    }

    void prevPage() {
        if (currentPage > 0) {
            currentPage--;
            showPage();
        }
    }

    void toggleNightMode() {
        nightMode = !nightMode;
        applyTheme();
    }

    void applyTheme() {
        if (nightMode) {
            textEdit->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
        } else {
            textEdit->setStyleSheet("background-color: #ffffff; color: #000000;");
        }
        // Сохраняем состояние
        QSettings settings("MyApp", "EpubReader");
        settings.setValue("nightMode", nightMode);
    }

    void addBookmark() {
        bool ok;
        QString name = QInputDialog::getText(this, "Закладка", "Введите название:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            bookmarks[name] = qMakePair(currentPage, 0);
            statusLabel->setText("Закладка добавлена: " + name);
        }
    }

    void gotoBookmark() {
        if (bookmarks.isEmpty()) {
            QMessageBox::information(this, "Информация", "Нет закладок");
            return;
        }
        QStringList names = bookmarks.keys();
        bool ok;
        QString name = QInputDialog::getItem(this, "Перейти к закладке", "Выберите:", names, 0, false, &ok);
        if (ok && !name.isEmpty()) {
            QPair<int,int> pos = bookmarks[name];
            currentPage = pos.first;
            showPage();
            statusLabel->setText("Переход к закладке: " + name);
        }
    }

private:
    QTextEdit *textEdit;
    QLabel *statusLabel;
    QString currentBookPath;
    QString fullText;
    QStringList pages;
    int currentPage = 0;
    bool nightMode = false;
    QMap<QString, QPair<int,int>> bookmarks;

    void createUI() {
        QWidget *central = new QWidget(this);
        setCentralWidget(central);
        QVBoxLayout *mainLayout = new QVBoxLayout(central);

        // Toolbar
        QHBoxLayout *toolbar = new QHBoxLayout();
        QPushButton *openBtn = new QPushButton("Открыть");
        QPushButton *bookmarkBtn = new QPushButton("Закладка");
        QPushButton *gotoBtn = new QPushButton("Перейти к закладке");
        QPushButton *nightBtn = new QPushButton("Ночной режим");
        toolbar->addWidget(openBtn);
        toolbar->addWidget(bookmarkBtn);
        toolbar->addWidget(gotoBtn);
        toolbar->addWidget(nightBtn);
        mainLayout->addLayout(toolbar);

        // Инфо
        statusLabel = new QLabel("Книга не загружена");
        mainLayout->addWidget(statusLabel);

        // Текст
        textEdit = new QTextEdit();
        textEdit->setReadOnly(true);
        mainLayout->addWidget(textEdit);

        // Статус
        QLabel *status = new QLabel("Готов");
        mainLayout->addWidget(status);

        connect(openBtn, &QPushButton::clicked, this, &EpubReader::openBook);
        connect(bookmarkBtn, &QPushButton::clicked, this, &EpubReader::addBookmark);
        connect(gotoBtn, &QPushButton::clicked, this, &EpubReader::gotoBookmark);
        connect(nightBtn, &QPushButton::clicked, this, &EpubReader::toggleNightMode);

        // Горячие клавиши
        QAction *nextAction = new QAction(this);
        nextAction->setShortcut(QKeySequence::MoveToNextPage);
        connect(nextAction, &QAction::triggered, this, &EpubReader::nextPage);
        addAction(nextAction);

        QAction *prevAction = new QAction(this);
        prevAction->setShortcut(QKeySequence::MoveToPreviousPage);
        connect(prevAction, &QAction::triggered, this, &EpubReader::prevPage);
        addAction(prevAction);
    }

    void loadState() {
        QSettings settings("MyApp", "EpubReader");
        nightMode = settings.value("nightMode", false).toBool();
        // Можно загрузить закладки, но для простоты оставим
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    EpubReader reader;
    reader.show();
    return app.exec();
}

#include "epubreader_cpp.moc"
