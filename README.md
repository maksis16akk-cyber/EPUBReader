📖 EPUBReader — читалка EPUB с ночным режимом
Умная читалка электронных книг в формате EPUB с поддержкой ночного режима, настройкой шрифта, закладками и прогрессом чтения.
Реализована на 7 языках программирования для демонстрации различных подходов к работе с текстовыми данными и EPUB-форматом.

https://img.shields.io/github/repo-size/yourname/epubreader
https://img.shields.io/github/stars/yourname/epubreader?style=social
https://img.shields.io/badge/License-MIT-blue.svg

🧠 Концепция
EPUBReader — это не просто читалка. Это инструмент для комфортного чтения электронных книг:

✅ Открытие EPUB-файлов — распаковка и парсинг содержимого.

✅ Отображение текста с разбивкой на страницы (или прокруткой).

✅ Ночной режим — переключение между светлой и тёмной темой.

✅ Настройка шрифта — размер, тип (в GUI-версиях).

✅ Закладки — сохранение позиции чтения.

✅ Поиск по тексту (в некоторых версиях).

✅ Прогресс чтения — отображение прочитанной части.

✅ Сохранение состояния — автоматическое восстановление после перезапуска.

✅ Горячие клавиши — управление без мыши.

🚀 Как запустить
Каждая версия использует соответствующие библиотеки для работы с EPUB (см. инструкции по установке).
Для консольных версий доступен интерактивный режим с командами.

bash
# Python (GUI)
pip install ebooklib tkinter
python epubreader_python.py

# C++ (Qt + minizip + tinyxml2)
# Требуется сборка с Qt и зависимостями
qmake && make
./epubreader_cpp

# Java (Swing + epublib)
javac -cp .:epublib-core.jar epubreader_java.java && java epubreader_java

# C# (WPF + EpubSharp)
dotnet add package EpubSharp
dotnet run

# Go (консоль + go-epub)
go mod tidy
go run epubreader_go.go

# Rust (консоль + epub + zip)
cargo build --release && ./target/release/epubreader_rs

# Node.js (консоль + epub-parser)
npm install epub-parser
node epubreader_js.js
🧩 Пример сессии (консольная версия)
text
📖 EPUBReader v2.0
Открыт файл: book.epub
Заголовок: Приключения Шерлока Холмса
Автор: Артур Конан Дойл
[Ночной режим: выкл]

Страница 1/245
Глава 1. Мистер Шерлок Холмс
В 1878 году я получил степень доктора медицины...
[Нажмите Enter для следующей страницы, n - ночной режим, b - закладка, q - выход]
📦 Содержимое репозитория
Файл	Язык	Особенности
epubreader_python.py	Python	Tkinter GUI, ночной режим, выбор шрифта, закладки
epubreader_cpp.cpp	C++	Qt Widgets, WebEngineView для отображения HTML (EPUB), ночной режим через CSS
epubreader_java.java	Java	Swing + JEditorPane, ночной режим через свойства
epubreader_cs.cs	C#	WPF + WebBrowser, ночной режим, закладки
epubreader_go.go	Go	консоль, парсинг EPUB, постраничный вывод с цветами
epubreader_rs.rs	Rust	консоль, парсинг EPUB через epub crate, цветной вывод
epubreader_js.js	JavaScript	Node.js, парсинг EPUB, интерактивный консольный режим
🔮 Расширенные функции
Текст в голос (TTS) — в планах (не реализовано).

Поддержка изображений — в GUI-версиях.

Синхронизация с облаком (опционально).

📜 Лицензия
MIT — свободно используйте, модифицируйте и распространяйте.

🤝 Вклад
Приветствуются пул-реквесты с улучшениями, новыми языками и поддержкой мобильных платформ.

⭐ Если проект помогает вам читать с комфортом — поставьте звёздочку!
