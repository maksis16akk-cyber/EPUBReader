// epubreader_js.js — Читалка EPUB с ночным режимом на JavaScript (Node.js + readline)

const fs = require('fs');
const readline = require('readline');
const unzipper = require('unzipper');
const { createReadStream } = require('fs');
const path = require('path');
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    prompt: '> '
});

class Reader {
    constructor() {
        this.fullText = '';
        this.pages = [];
        this.current = 0;
        this.nightMode = false;
    }

    async loadEpub(filename) {
        if (!fs.existsSync(filename)) {
            throw new Error('Файл не найден');
        }
        const directory = await unzipper.Open.file(filename);
        let textBuilder = '';
        const htmlRegex = /.*\.(html|xhtml|htm)$/i;
        for (const file of directory.files) {
            if (htmlRegex.test(file.path)) {
                const content = await file.buffer();
                let text = content.toString('utf-8');
                // Удаляем HTML теги
                text = text.replace(/<[^>]+>/g, ' ');
                textBuilder += text + ' ';
            }
        }
        this.fullText = textBuilder.replace(/\s+/g, ' ').trim();
        // Разбиваем на страницы
        this.pages = [];
        const pageSize = 2000;
        for (let i = 0; i < this.fullText.length; i += pageSize) {
            this.pages.push(this.fullText.substring(i, i + pageSize));
        }
        this.current = 0;
        return this.pages.length;
    }

    showPage() {
        if (this.pages.length === 0) {
            console.log('Книга не загружена');
            return;
        }
        const total = this.pages.length;
        const idx = Math.min(this.current, total - 1);
        console.log(`\n--- Страница ${idx+1}/${total} ---`);
        if (this.nightMode) {
            // Инвертируем цвета через ANSI
            console.log('\x1b[30m\x1b[47m');
        }
        console.log(this.pages[idx]);
        if (this.nightMode) {
            console.log('\x1b[0m');
        }
        console.log('---');
    }

    next() {
        if (this.current < this.pages.length - 1) {
            this.current++;
            this.showPage();
        } else {
            console.log('Это последняя страница');
        }
    }

    prev() {
        if (this.current > 0) {
            this.current--;
            this.showPage();
        } else {
            console.log('Это первая страница');
        }
    }

    toggleNight() {
        this.nightMode = !this.nightMode;
        console.log(`Ночной режим: ${this.nightMode ? 'включён' : 'выключен'}`);
        this.showPage();
    }
}

const reader = new Reader();
console.log('📖 EPUBReader — JavaScript Edition');
console.log('Команды: open <filename>, next, prev, night, exit');
rl.prompt();

rl.on('line', async (line) => {
    const parts = line.trim().split(' ');
    const cmd = parts[0];
    const arg = parts.slice(1).join(' ');
    switch (cmd) {
        case 'open':
            if (!arg) { console.log('Укажите имя файла'); break; }
            try {
                const pages = await reader.loadEpub(arg);
                console.log(`Книга загружена, страниц: ${pages}`);
                reader.showPage();
            } catch (e) {
                console.log('Ошибка:', e.message);
            }
            break;
        case 'next':
            reader.next();
            break;
        case 'prev':
            reader.prev();
            break;
        case 'night':
            reader.toggleNight();
            break;
        case 'exit':
            console.log('До свидания!');
            rl.close();
            return;
        default:
            console.log('Неизвестная команда');
    }
    rl.prompt();
}).on('close', () => {
    process.exit(0);
});
