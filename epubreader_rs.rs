// epubreader_rs.rs — Читалка EPUB с ночным режимом на Rust (консоль + termion)

use std::fs::File;
use std::io::{self, Write, BufRead, Read};
use std::path::Path;
use zip::ZipArchive;
use regex::Regex;
use termion::{color, style};

struct Reader {
    full_text: String,
    pages: Vec<String>,
    current: usize,
    night_mode: bool,
}

impl Reader {
    fn new() -> Self {
        Reader {
            full_text: String::new(),
            pages: Vec::new(),
            current: 0,
            night_mode: false,
        }
    }

    fn load_epub(&mut self, filename: &str) -> Result<(), Box<dyn std::error::Error>> {
        let file = File::open(filename)?;
        let mut archive = ZipArchive::new(file)?;
        let mut text_builder = String::new();
        let html_regex = Regex::new(r".*\.(html|xhtml|htm)$")?;
        let tag_regex = Regex::new(r"<[^>]+>")?;
        for i in 0..archive.len() {
            let mut entry = archive.by_index(i)?;
            let name = entry.name();
            if html_regex.is_match(name) {
                let mut content = String::new();
                entry.read_to_string(&mut content)?;
                let text = tag_regex.replace_all(&content, " ");
                text_builder.push_str(&text);
                text_builder.push(' ');
            }
        }
        self.full_text = Regex::new(r"\s+")?.replace_all(&text_builder, " ").trim().to_string();
        self.pages.clear();
        let page_size = 2000;
        let chars: Vec<char> = self.full_text.chars().collect();
        for i in (0..chars.len()).step_by(page_size) {
            let end = (i + page_size).min(chars.len());
            let page: String = chars[i..end].iter().collect();
            self.pages.push(page);
        }
        self.current = 0;
        Ok(())
    }

    fn show_page(&self) {
        if self.pages.is_empty() {
            println!("Книга не загружена");
            return;
        }
        let total = self.pages.len();
        let idx = self.current.min(total - 1);
        println!("\n{}--- Страница {}/{} ---{}", color::Fg(color::Blue), idx+1, total, style::Reset);
        if self.night_mode {
            print!("{}", color::Bg(color::White));
            print!("{}", color::Fg(color::Black));
        }
        println!("{}", self.pages[idx]);
        if self.night_mode {
            print!("{}", style::Reset);
        }
        println!("{}---{}", color::Fg(color::Blue), style::Reset);
    }

    fn next(&mut self) {
        if self.current < self.pages.len() - 1 {
            self.current += 1;
            self.show_page();
        } else {
            println!("Это последняя страница");
        }
    }

    fn prev(&mut self) {
        if self.current > 0 {
            self.current -= 1;
            self.show_page();
        } else {
            println!("Это первая страница");
        }
    }

    fn toggle_night(&mut self) {
        self.night_mode = !self.night_mode;
        println!("Ночной режим: {}", if self.night_mode { "включён" } else { "выключен" });
        self.show_page();
    }
}

fn main() {
    let mut reader = Reader::new();
    let stdin = io::stdin();
    let mut stdin_lock = stdin.lock();
    println!("{}📖 EPUBReader — Rust Edition{}", color::Fg(color::Cyan), style::Reset);
    println!("Команды: open <filename>, next, prev, night, exit");
    loop {
        print!("{}> {}", color::Fg(color::Yellow), style::Reset);
        io::stdout().flush().unwrap();
        let mut line = String::new();
        if stdin_lock.read_line(&mut line).is_err() { break; }
        let line = line.trim();
        if line.is_empty() { continue; }
        let parts: Vec<&str> = line.splitn(2, ' ').collect();
        let cmd = parts[0];
        let arg = if parts.len() > 1 { parts[1] } else { "" };
        match cmd {
            "open" => {
                if arg.is_empty() {
                    println!("Укажите имя файла");
                    continue;
                }
                if let Err(e) = reader.load_epub(arg) {
                    println!("Ошибка: {}", e);
                } else {
                    println!("Книга загружена, страниц: {}", reader.pages.len());
                    reader.show_page();
                }
            }
            "next" => reader.next(),
            "prev" => reader.prev(),
            "night" => reader.toggle_night(),
            "exit" => {
                println!("До свидания!");
                break;
            }
            _ => println!("Неизвестная команда"),
        }
    }
}
