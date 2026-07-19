// epubreader_java.java — Читалка EPUB с ночным режимом на Java (Swing)

import javax.swing.*;
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.List;
import java.util.zip.*;
import javax.xml.parsers.*;
import org.w3c.dom.*;
import org.xml.sax.InputSource;

public class EpubReaderJava extends JFrame {
    private static final String DATA_FILE = "reader_state.json";
    private JTextPane textPane;
    private JLabel statusLabel, infoLabel;
    private String fullText = "";
    private List<String> pages = new ArrayList<>();
    private int currentPage = 0;
    private boolean nightMode = false;
    private Map<String, Integer> bookmarks = new HashMap<>();
    private int fontSize = 14;
    private String fontFamily = "Serif";

    public EpubReaderJava() {
        setTitle("📖 EPUBReader — Java");
        setSize(900, 700);
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setLayout(new BorderLayout());
        loadState();
        createUI();
        applyTheme();
    }

    private void createUI() {
        // Toolbar
        JPanel toolbar = new JPanel();
        JButton openBtn = new JButton("Открыть");
        JButton bookmarkBtn = new JButton("Закладка");
        JButton gotoBtn = new JButton("Перейти к закладке");
        JButton nightBtn = new JButton("Ночной режим");
        JButton incFontBtn = new JButton("A+");
        JButton decFontBtn = new JButton("A-");
        toolbar.add(openBtn);
        toolbar.add(bookmarkBtn);
        toolbar.add(gotoBtn);
        toolbar.add(nightBtn);
        toolbar.add(incFontBtn);
        toolbar.add(decFontBtn);
        add(toolbar, BorderLayout.NORTH);

        // Info
        infoLabel = new JLabel("Книга не загружена");
        add(infoLabel, BorderLayout.SOUTH);

        // Text
        textPane = new JTextPane();
        textPane.setEditable(false);
        textPane.setFont(new Font(fontFamily, Font.PLAIN, fontSize));
        JScrollPane scroll = new JScrollPane(textPane);
        add(scroll, BorderLayout.CENTER);

        // Status
        statusLabel = new JLabel("Готов");
        add(statusLabel, BorderLayout.SOUTH);

        // Handlers
        openBtn.addActionListener(e -> openBook());
        bookmarkBtn.addActionListener(e -> addBookmark());
        gotoBtn.addActionListener(e -> gotoBookmark());
        nightBtn.addActionListener(e -> toggleNightMode());
        incFontBtn.addActionListener(e -> { fontSize += 2; textPane.setFont(new Font(fontFamily, Font.PLAIN, fontSize)); });
        decFontBtn.addActionListener(e -> { if (fontSize > 8) { fontSize -= 2; textPane.setFont(new Font(fontFamily, Font.PLAIN, fontSize)); } });

        // Hotkeys
        getRootPane().registerKeyboardAction(e -> nextPage(),
                KeyStroke.getKeyStroke(KeyEvent.VK_RIGHT, 0), JComponent.WHEN_IN_FOCUSED_WINDOW);
        getRootPane().registerKeyboardAction(e -> prevPage(),
                KeyStroke.getKeyStroke(KeyEvent.VK_LEFT, 0), JComponent.WHEN_IN_FOCUSED_WINDOW);
        getRootPane().registerKeyboardAction(e -> openBook(),
                KeyStroke.getKeyStroke(KeyEvent.VK_O, InputEvent.CTRL_MASK), JComponent.WHEN_IN_FOCUSED_WINDOW);
    }

    private void openBook() {
        JFileChooser chooser = new JFileChooser();
        chooser.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("EPUB", "epub"));
        if (chooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            File file = chooser.getSelectedFile();
            try {
                // Распаковываем EPUB как ZIP
                ZipFile zip = new ZipFile(file);
                Enumeration<? extends ZipEntry> entries = zip.entries();
                // Ищем OPF и файлы содержимого
                // Упрощённо: читаем все .html/.xhtml и извлекаем текст
                StringBuilder textBuilder = new StringBuilder();
                while (entries.hasMoreElements()) {
                    ZipEntry entry = entries.nextElement();
                    String name = entry.getName();
                    if (name.matches(".*\\.(html|xhtml|htm)$")) {
                        InputStream is = zip.getInputStream(entry);
                        BufferedReader reader = new BufferedReader(new InputStreamReader(is, "UTF-8"));
                        String line;
                        while ((line = reader.readLine()) != null) {
                            // Удаляем HTML теги
                            line = line.replaceAll("<[^>]+>", " ");
                            textBuilder.append(line).append(" ");
                        }
                        reader.close();
                    }
                }
                zip.close();
                fullText = textBuilder.toString().replaceAll("\\s+", " ").trim();
                // Разбиваем на страницы
                pages.clear();
                int pageSize = 2000;
                for (int i = 0; i < fullText.length(); i += pageSize) {
                    pages.add(fullText.substring(i, Math.min(i+pageSize, fullText.length())));
                }
                currentPage = 0;
                showPage();
                infoLabel.setText("Книга: " + file.getName());
            } catch (Exception e) {
                JOptionPane.showMessageDialog(this, "Ошибка открытия EPUB: " + e.getMessage());
            }
        }
    }

    private void showPage() {
        if (pages.isEmpty()) return;
        if (currentPage >= pages.size()) currentPage = pages.size() - 1;
        if (currentPage < 0) currentPage = 0;
        textPane.setText(pages.get(currentPage));
        statusLabel.setText("Страница " + (currentPage+1) + "/" + pages.size());
    }

    private void nextPage() {
        if (currentPage < pages.size() - 1) {
            currentPage++;
            showPage();
        }
    }

    private void prevPage() {
        if (currentPage > 0) {
            currentPage--;
            showPage();
        }
    }

    private void addBookmark() {
        String name = JOptionPane.showInputDialog(this, "Введите название закладки:");
        if (name != null && !name.isEmpty()) {
            bookmarks.put(name, currentPage);
            statusLabel.setText("Закладка добавлена: " + name);
        }
    }

    private void gotoBookmark() {
        if (bookmarks.isEmpty()) {
            JOptionPane.showMessageDialog(this, "Нет закладок");
            return;
        }
        Object[] names = bookmarks.keySet().toArray();
        Object selected = JOptionPane.showInputDialog(this, "Выберите закладку:", "Перейти к закладке",
                JOptionPane.QUESTION_MESSAGE, null, names, names[0]);
        if (selected != null) {
            currentPage = bookmarks.get(selected);
            showPage();
            statusLabel.setText("Переход к закладке: " + selected);
        }
    }

    private void toggleNightMode() {
        nightMode = !nightMode;
        applyTheme();
    }

    private void applyTheme() {
        Color bg, fg;
        if (nightMode) {
            bg = new Color(30, 30, 30);
            fg = new Color(212, 212, 212);
        } else {
            bg = Color.WHITE;
            fg = Color.BLACK;
        }
        textPane.setBackground(bg);
        textPane.setForeground(fg);
        // Сохраняем состояние
        try (PrintWriter pw = new PrintWriter(new File(DATA_FILE))) {
            pw.println("{\"nightMode\":" + nightMode + ",\"fontSize\":" + fontSize + "}");
        } catch (IOException e) {}
    }

    private void loadState() {
        File file = new File(DATA_FILE);
        if (file.exists()) {
            try (BufferedReader br = new BufferedReader(new FileReader(file))) {
                String line = br.readLine();
                if (line != null && line.contains("nightMode")) {
                    // Простой парсинг JSON (не используем библиотеку для упрощения)
                    if (line.contains("\"nightMode\":true")) nightMode = true;
                    // Извлечение fontSize
                    int idx = line.indexOf("\"fontSize\":");
                    if (idx != -1) {
                        int start = idx + 11;
                        int end = line.indexOf(",", start);
                        if (end == -1) end = line.indexOf("}", start);
                        if (end != -1) {
                            try {
                                fontSize = Integer.parseInt(line.substring(start, end).trim());
                            } catch (NumberFormatException e) {}
                        }
                    }
                }
            } catch (IOException e) {}
        }
    }

    public static void main(String[] args) throws Exception {
        UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        SwingUtilities.invokeLater(() -> new EpubReaderJava().setVisible(true));
    }
}
