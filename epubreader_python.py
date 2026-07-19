# epubreader_python.py — Читалка EPUB с ночным режимом на Python (Tkinter)

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import os
import json
import zipfile
import xml.etree.ElementTree as ET
from ebooklib import epub
import re
import html

class EpubReader:
    def __init__(self, root):
        self.root = root
        self.root.title("📖 EPUBReader — Python")
        self.root.geometry("900x700")
        self.book = None
        self.items = []
        self.current_item = 0
        self.current_pos = 0  # позиция в тексте
        self.bookmarks = {}
        self.night_mode = False
        self.font_size = 12
        self.font_family = "Arial"
        self.filename = "reader_state.json"
        self.load_state()
        self.create_widgets()
        self.set_colors()
        self.root.protocol("WM_DELETE_WINDOW", self.save_state)

    def create_widgets(self):
        # Верхняя панель
        toolbar = tk.Frame(self.root)
        toolbar.pack(fill=tk.X, pady=5)
        tk.Button(toolbar, text="Открыть книгу", command=self.open_book).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Закладка", command=self.add_bookmark).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Перейти к закладке", command=self.goto_bookmark).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Ночной режим", command=self.toggle_night_mode).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Увеличить шрифт", command=self.inc_font).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Уменьшить шрифт", command=self.dec_font).pack(side=tk.LEFT, padx=5)

        # Информация о книге
        self.info_label = tk.Label(self.root, text="Книга не загружена", anchor=tk.W)
        self.info_label.pack(fill=tk.X, padx=10)

        # Текст
        self.text_widget = scrolledtext.ScrolledText(self.root, wrap=tk.WORD, font=(self.font_family, self.font_size))
        self.text_widget.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        self.text_widget.config(state=tk.DISABLED)

        # Статус
        self.status = tk.Label(self.root, text="Готов", anchor=tk.W)
        self.status.pack(fill=tk.X, padx=10)

        # Горячие клавиши
        self.root.bind("<Control-o>", lambda e: self.open_book())
        self.root.bind("<Control-b>", lambda e: self.add_bookmark())
        self.root.bind("<Control-n>", lambda e: self.toggle_night_mode())
        self.root.bind("<Control-=>", lambda e: self.inc_font())
        self.root.bind("<Control-minus>", lambda e: self.dec_font())
        self.root.bind("<Right>", lambda e: self.next_page())
        self.root.bind("<Left>", lambda e: self.prev_page())

    def open_book(self):
        filename = filedialog.askopenfilename(filetypes=[("EPUB files", "*.epub")])
        if not filename:
            return
        try:
            self.book = epub.read_epub(filename)
            self.items = []
            for item in self.book.get_items():
                if item.get_type() == epub.ITEM_DOCUMENT:
                    self.items.append(item)
            if not self.items:
                messagebox.showerror("Ошибка", "Не найдено текстовых документов в EPUB")
                return
            self.current_item = 0
            self.current_pos = 0
            self.bookmarks = {}
            self.info_label.config(text=f"Книга: {self.book.get_metadata('DC', 'title')[0][0] if self.book.get_metadata('DC', 'title') else 'Без названия'}")
            self.display_item()
            self.status.config(text=f"Загружено: {os.path.basename(filename)}")
        except Exception as e:
            messagebox.showerror("Ошибка", f"Не удалось открыть EPUB: {e}")

    def display_item(self):
        if not self.items or self.current_item >= len(self.items):
            return
        item = self.items[self.current_item]
        content = item.get_content().decode('utf-8', errors='ignore')
        # Удаляем теги HTML, оставляем текст
        text = self.extract_text(content)
        # Разбиваем на страницы (по 2000 символов для удобства)
        self.pages = [text[i:i+2000] for i in range(0, len(text), 2000)]
        self.current_page = 0
        self.show_page()

    def extract_text(self, html_content):
        # Простейшее извлечение текста из HTML
        # Удаляем теги <script> и <style>
        html_content = re.sub(r'<script[^>]*>.*?</script>', '', html_content, flags=re.DOTALL)
        html_content = re.sub(r'<style[^>]*>.*?</style>', '', html_content, flags=re.DOTALL)
        # Заменяем теги на пробелы
        text = re.sub(r'<[^>]+>', ' ', html_content)
        # Удаляем лишние пробелы и переносы
        text = re.sub(r'\s+', ' ', text).strip()
        # Декодируем HTML-сущности
        text = html.unescape(text)
        return text

    def show_page(self):
        if not hasattr(self, 'pages') or not self.pages:
            return
        if self.current_page >= len(self.pages):
            self.current_page = len(self.pages) - 1
        if self.current_page < 0:
            self.current_page = 0
        text = self.pages[self.current_page]
        self.text_widget.config(state=tk.NORMAL)
        self.text_widget.delete(1.0, tk.END)
        self.text_widget.insert(tk.END, text)
        self.text_widget.config(state=tk.DISABLED)
        total_pages = len(self.pages)
        self.status.config(text=f"Страница {self.current_page+1}/{total_pages} | Всего: {sum(len(p) for p in self.pages)} символов")

    def next_page(self):
        if hasattr(self, 'pages') and self.pages:
            if self.current_page < len(self.pages) - 1:
                self.current_page += 1
                self.show_page()

    def prev_page(self):
        if hasattr(self, 'pages') and self.pages:
            if self.current_page > 0:
                self.current_page -= 1
                self.show_page()

    def add_bookmark(self):
        if not self.book:
            return
        pos = self.current_item * 10000 + self.current_page  # грубый идентификатор
        name = tk.simpledialog.askstring("Закладка", "Введите название закладки:")
        if name:
            self.bookmarks[name] = (self.current_item, self.current_page)
            self.status.config(text=f"Закладка '{name}' добавлена")

    def goto_bookmark(self):
        if not self.bookmarks:
            messagebox.showinfo("Информация", "Нет закладок")
            return
        names = list(self.bookmarks.keys())
        name = tk.simpledialog.askstring("Перейти к закладке", "Введите имя закладки:", initialvalue=names[0] if names else "")
        if name and name in self.bookmarks:
            item, page = self.bookmarks[name]
            self.current_item = item
            self.current_page = page
            self.display_item()
            self.show_page()
            self.status.config(text=f"Переход к закладке '{name}'")

    def toggle_night_mode(self):
        self.night_mode = not self.night_mode
        self.set_colors()
        self.status.config(text="Ночной режим " + ("включён" if self.night_mode else "выключен"))

    def set_colors(self):
        if self.night_mode:
            bg = "#1e1e1e"
            fg = "#d4d4d4"
            insert = "#ffffff"
        else:
            bg = "#ffffff"
            fg = "#000000"
            insert = "#000000"
        self.text_widget.config(bg=bg, fg=fg, insertbackground=insert)
        self.root.config(bg=bg)
        self.info_label.config(bg=bg, fg=fg)
        self.status.config(bg=bg, fg=fg)

    def inc_font(self):
        self.font_size += 2
        self.text_widget.config(font=(self.font_family, self.font_size))

    def dec_font(self):
        if self.font_size > 8:
            self.font_size -= 2
            self.text_widget.config(font=(self.font_family, self.font_size))

    def save_state(self):
        data = {
            "bookmarks": self.bookmarks,
            "font_size": self.font_size,
            "night_mode": self.night_mode
        }
        with open(self.filename, 'w') as f:
            json.dump(data, f)

    def load_state(self):
        if os.path.exists(self.filename):
            with open(self.filename, 'r') as f:
                data = json.load(f)
                self.bookmarks = data.get("bookmarks", {})
                self.font_size = data.get("font_size", 12)
                self.night_mode = data.get("night_mode", False)

if __name__ == "__main__":
    root = tk.Tk()
    app = EpubReader(root)
    root.mainloop()
