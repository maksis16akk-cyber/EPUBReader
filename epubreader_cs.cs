// epubreader_cs.cs — Читалка EPUB с ночным режимом на C# (WPF)

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;
using System.Xml;

namespace EpubReaderWPF
{
    public partial class MainWindow : Window
    {
        private string fullText = "";
        private List<string> pages = new List<string>();
        private int currentPage = 0;
        private bool nightMode = false;
        private Dictionary<string, int> bookmarks = new Dictionary<string, int>();
        private int fontSize = 14;
        private string fontFamily = "Arial";
        private string stateFile = "reader_state.json";

        private TextBlock textBlock;
        private Label statusLabel, infoLabel;
        private ScrollViewer scrollViewer;

        public MainWindow()
        {
            InitializeComponent();
            CreateUI();
            LoadState();
            ApplyTheme();
        }

        private void CreateUI()
        {
            Title = "📖 EPUBReader — C#";
            Width = 900;
            Height = 700;
            var grid = new Grid();
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            // Toolbar
            var toolbar = new StackPanel { Orientation = Orientation.Horizontal };
            var openBtn = new Button { Content = "Открыть", Width = 80 };
            var bookmarkBtn = new Button { Content = "Закладка", Width = 80 };
            var gotoBtn = new Button { Content = "Перейти к закладке", Width = 100 };
            var nightBtn = new Button { Content = "Ночной режим", Width = 100 };
            var incBtn = new Button { Content = "A+", Width = 40 };
            var decBtn = new Button { Content = "A-", Width = 40 };
            toolbar.Children.Add(openBtn);
            toolbar.Children.Add(bookmarkBtn);
            toolbar.Children.Add(gotoBtn);
            toolbar.Children.Add(nightBtn);
            toolbar.Children.Add(incBtn);
            toolbar.Children.Add(decBtn);
            Grid.SetRow(toolbar, 0);
            grid.Children.Add(toolbar);

            // Info
            infoLabel = new Label { Content = "Книга не загружена" };
            Grid.SetRow(infoLabel, 1);
            grid.Children.Add(infoLabel);

            // Text
            scrollViewer = new ScrollViewer();
            textBlock = new TextBlock { TextWrapping = TextWrapping.Wrap, FontSize = fontSize, FontFamily = new FontFamily(fontFamily) };
            scrollViewer.Content = textBlock;
            Grid.SetRow(scrollViewer, 2);
            grid.Children.Add(scrollViewer);

            // Status
            statusLabel = new Label { Content = "Готов" };
            Grid.SetRow(statusLabel, 3);
            grid.Children.Add(statusLabel);

            Content = grid;

            openBtn.Click += (s, e) => OpenBook();
            bookmarkBtn.Click += (s, e) => AddBookmark();
            gotoBtn.Click += (s, e) => GotoBookmark();
            nightBtn.Click += (s, e) => ToggleNightMode();
            incBtn.Click += (s, e) => { fontSize += 2; textBlock.FontSize = fontSize; };
            decBtn.Click += (s, e) => { if (fontSize > 8) { fontSize -= 2; textBlock.FontSize = fontSize; } };

            // Hotkeys
            this.KeyDown += (s, e) => {
                if (e.Key == Key.Right) NextPage();
                if (e.Key == Key.Left) PrevPage();
                if (e.Key == Key.O && Keyboard.Modifiers == ModifierKeys.Control) OpenBook();
            };
        }

        private void OpenBook()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog { Filter = "EPUB (*.epub)|*.epub" };
            if (dialog.ShowDialog() == true)
            {
                try
                {
                    using (var zip = ZipFile.OpenRead(dialog.FileName))
                    {
                        var textBuilder = new StringBuilder();
                        foreach (var entry in zip.Entries)
                        {
                            if (Regex.IsMatch(entry.Name, @".*\.(html|xhtml|htm)$"))
                            {
                                using (var reader = new StreamReader(entry.Open()))
                                {
                                    string content = reader.ReadToEnd();
                                    // Удаляем HTML теги
                                    content = Regex.Replace(content, "<[^>]+>", " ");
                                    textBuilder.Append(content).Append(" ");
                                }
                            }
                        }
                        fullText = Regex.Replace(textBuilder.ToString(), @"\s+", " ").Trim();
                        pages.Clear();
                        int pageSize = 2000;
                        for (int i = 0; i < fullText.Length; i += pageSize)
                        {
                            pages.Add(fullText.Substring(i, Math.Min(pageSize, fullText.Length - i)));
                        }
                        currentPage = 0;
                        ShowPage();
                        infoLabel.Content = "Книга: " + System.IO.Path.GetFileName(dialog.FileName);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Ошибка открытия EPUB: " + ex.Message);
                }
            }
        }

        private void ShowPage()
        {
            if (pages.Count == 0) return;
            if (currentPage >= pages.Count) currentPage = pages.Count - 1;
            if (currentPage < 0) currentPage = 0;
            textBlock.Text = pages[currentPage];
            statusLabel.Content = $"Страница {currentPage+1}/{pages.Count}";
        }

        private void NextPage() { if (currentPage < pages.Count - 1) { currentPage++; ShowPage(); } }
        private void PrevPage() { if (currentPage > 0) { currentPage--; ShowPage(); } }

        private void AddBookmark()
        {
            string name = Microsoft.VisualBasic.Interaction.InputBox("Введите название закладки:", "Закладка", "", -1, -1);
            if (!string.IsNullOrEmpty(name))
            {
                bookmarks[name] = currentPage;
                statusLabel.Content = "Закладка добавлена: " + name;
            }
        }

        private void GotoBookmark()
        {
            if (bookmarks.Count == 0) { MessageBox.Show("Нет закладок"); return; }
            var names = bookmarks.Keys.ToArray();
            string selected = (string)Microsoft.VisualBasic.Interaction.InputBox("Выберите закладку:", "Перейти к закладке", names[0], -1, -1);
            if (!string.IsNullOrEmpty(selected) && bookmarks.ContainsKey(selected))
            {
                currentPage = bookmarks[selected];
                ShowPage();
                statusLabel.Content = "Переход к закладке: " + selected;
            }
        }

        private void ToggleNightMode()
        {
            nightMode = !nightMode;
            ApplyTheme();
        }

        private void ApplyTheme()
        {
            var bg = nightMode ? new SolidColorBrush(Color.FromRgb(30, 30, 30)) : new SolidColorBrush(Colors.White);
            var fg = nightMode ? new SolidColorBrush(Color.FromRgb(212, 212, 212)) : new SolidColorBrush(Colors.Black);
            textBlock.Background = bg;
            textBlock.Foreground = fg;
            scrollViewer.Background = bg;
            // Сохраняем состояние
            System.IO.File.WriteAllText(stateFile, $"{{\"nightMode\":{nightMode.ToString().ToLower()},\"fontSize\":{fontSize}}}");
        }

        private void LoadState()
        {
            if (System.IO.File.Exists(stateFile))
            {
                string json = System.IO.File.ReadAllText(stateFile);
                if (json.Contains("nightMode\":true")) nightMode = true;
                // Парсим fontSize
                int idx = json.IndexOf("fontSize\":");
                if (idx != -1)
                {
                    int start = idx + 10;
                    int end = json.IndexOf(",", start);
                    if (end == -1) end = json.IndexOf("}", start);
                    if (end != -1)
                    {
                        int.TryParse(json.Substring(start, end - start).Trim(), out fontSize);
                    }
                }
            }
        }

        [STAThread]
        static void Main()
        {
            var app = new Application();
            app.Run(new MainWindow());
        }
    }
}
