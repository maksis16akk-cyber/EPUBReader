// epubreader_go.go — Читалка EPUB с ночным режимом на Go (консоль)

package main

import (
	"archive/zip"
	"bufio"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"regexp"
	"strconv"
	"strings"
)

type Reader struct {
	fullText  string
	pages     []string
	current   int
	nightMode bool
}

func NewReader() *Reader {
	return &Reader{}
}

func (r *Reader) loadEPUB(filename string) error {
	reader, err := zip.OpenReader(filename)
	if err != nil {
		return err
	}
	defer reader.Close()

	var textBuilder strings.Builder
	for _, file := range reader.File {
		if matched, _ := regexp.MatchString(`.*\.(html|xhtml|htm)`, file.Name); matched {
			rc, err := file.Open()
			if err != nil {
				continue
			}
			content, err := ioutil.ReadAll(rc)
			rc.Close()
			if err != nil {
				continue
			}
			// Удаляем HTML теги
			text := string(content)
			re := regexp.MustCompile(`<[^>]+>`)
			text = re.ReplaceAllString(text, " ")
			textBuilder.WriteString(text)
			textBuilder.WriteString(" ")
		}
	}
	r.fullText = regexp.MustCompile(`\s+`).ReplaceAllString(textBuilder.String(), " ")
	r.fullText = strings.TrimSpace(r.fullText)
	// Разбиваем на страницы
	r.pages = nil
	pageSize := 2000
	for i := 0; i < len(r.fullText); i += pageSize {
		end := i + pageSize
		if end > len(r.fullText) {
			end = len(r.fullText)
		}
		r.pages = append(r.pages, r.fullText[i:end])
	}
	r.current = 0
	return nil
}

func (r *Reader) showPage() {
	if len(r.pages) == 0 {
		fmt.Println("Книга не загружена")
		return
	}
	if r.current < 0 {
		r.current = 0
	}
	if r.current >= len(r.pages) {
		r.current = len(r.pages) - 1
	}
	fmt.Println("\n--- Страница", r.current+1, "/", len(r.pages), "---")
	if r.nightMode {
		fmt.Print("\033[30;47m") // чёрный текст на белом фоне (инвертировано)
	}
	fmt.Println(r.pages[r.current])
	if r.nightMode {
		fmt.Print("\033[0m") // сброс
	}
	fmt.Println("---")
}

func (r *Reader) next() {
	if r.current < len(r.pages)-1 {
		r.current++
		r.showPage()
	} else {
		fmt.Println("Это последняя страница")
	}
}

func (r *Reader) prev() {
	if r.current > 0 {
		r.current--
		r.showPage()
	} else {
		fmt.Println("Это первая страница")
	}
}

func (r *Reader) toggleNight() {
	r.nightMode = !r.nightMode
	fmt.Println("Ночной режим:", map[bool]string{true: "включён", false: "выключен"}[r.nightMode])
	r.showPage()
}

func main() {
	reader := NewReader()
	scanner := bufio.NewScanner(os.Stdin)
	fmt.Println("📖 EPUBReader — Go Edition")
	fmt.Println("Команды: open <filename>, next, prev, night, exit")
	for {
		fmt.Print("> ")
		if !scanner.Scan() {
			break
		}
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}
		parts := strings.SplitN(line, " ", 2)
		cmd := parts[0]
		arg := ""
		if len(parts) > 1 {
			arg = parts[1]
		}
		switch cmd {
		case "open":
			if arg == "" {
				fmt.Println("Укажите имя файла")
				continue
			}
			err := reader.loadEPUB(arg)
			if err != nil {
				fmt.Println("Ошибка:", err)
			} else {
				fmt.Println("Книга загружена, страниц:", len(reader.pages))
				reader.showPage()
			}
		case "next":
			reader.next()
		case "prev":
			reader.prev()
		case "night":
			reader.toggleNight()
		case "exit":
			fmt.Println("До свидания!")
			return
		default:
			fmt.Println("Неизвестная команда")
		}
	}
}
