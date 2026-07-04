// anagram.go
package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"math/rand"
	"os"
	"path/filepath"
	"strings"
	"time"
)

const (
	reset  = "\033[0m"
	green  = "\033[92m"
	red    = "\033[91m"
	yellow = "\033[93m"
	blue   = "\033[94m"
	cyan   = "\033[96m"
	bold   = "\033[1m"
)

func colorize(text, color string) string {
	return color + text + reset
}

var wordsMap = map[string][]string{
	"easy":   {"кот", "нос", "рот", "сом", "мир", "ток", "сон", "топ", "вес", "год", "дом", "дым", "жара", "зима", "луна", "море", "нога", "рука", "вода"},
	"medium": {"камень", "молния", "небо", "облако", "окно", "песня", "река", "солнце", "трава", "утро", "вечер", "город", "дерево", "земля", "книга", "ложка", "ночь", "остров", "парта", "роза", "стена", "тетрадь"},
	"hard":   {"аэропорт", "библиотека", "велосипед", "государство", "департамент", "инструмент", "компьютер", "лексикография", "математика", "насекомое", "оборудование", "программирование", "руководство", "свидетельство", "телевидение", "университет", "философия", "хирургия", "цивилизация", "электричество"},
}

type Stats struct {
	Games      int `json:"games"`
	BestScore  int `json:"best_score"`
	TotalWords int `json:"total_words"`
}

type AnagramGame struct {
	level      string
	words      []string
	maxWords   int
	score      int
	attempts   int
	correct    int
	skipped    int
	hintUsed   int
	timeLimit  int
	stats      Stats
	statsFile  string
}

func NewAnagramGame(level string) *AnagramGame {
	g := &AnagramGame{
		level:    level,
		maxWords: 10,
		statsFile: filepath.Join(os.Getenv("HOME"), ".anagram_stats.json"),
	}
	g.words = wordsMap[level]
	g.loadStats()
	switch level {
	case "easy":
		g.timeLimit = 60
	case "medium":
		g.timeLimit = 45
	default:
		g.timeLimit = 30
	}
	return g
}

func (g *AnagramGame) loadStats() {
	data, err := os.ReadFile(g.statsFile)
	if err != nil {
		g.stats = Stats{}
		return
	}
	json.Unmarshal(data, &g.stats)
}

func (g *AnagramGame) saveStats() {
	data, _ := json.MarshalIndent(g.stats, "", "  ")
	os.WriteFile(g.statsFile, data, 0644)
}

func (g *AnagramGame) shuffleWord(word string) string {
	letters := []rune(word)
	shuffled := word
	for shuffled == word {
		rand.Shuffle(len(letters), func(i, j int) {
			letters[i], letters[j] = letters[j], letters[i]
		})
		shuffled = string(letters)
	}
	return shuffled
}

func (g *AnagramGame) timerInput(prompt string, timeout int) string {
	fmt.Print(colorize(prompt, bold))
	ch := make(chan string)
	go func() {
		scanner := bufio.NewScanner(os.Stdin)
		if scanner.Scan() {
			ch <- scanner.Text()
		}
	}()
	select {
	case answer := <-ch:
		return answer
	case <-time.After(time.Duration(timeout) * time.Second):
		return ""
	}
}

func (g *AnagramGame) showAnswer(word string) {
	fmt.Printf("%s\n", colorize("Загаданное слово: "+word, yellow))
}

func (g *AnagramGame) playRound(word string) {
	shuffled := g.shuffleWord(word)
	fmt.Printf("%s\n", colorize("\nАнаграмма: "+shuffled, cyan))
	fmt.Printf("%s\n", colorize("Длина слова: "+string(rune(len(word))), blue))
	fmt.Println("Введите слово, ? для подсказки, pass для пропуска, quit для выхода.")

	start := time.Now()
	answer := g.timerInput("Ваш ответ: ", g.timeLimit)
	elapsed := int(time.Since(start).Seconds())

	if answer == "" {
		fmt.Println(colorize("⏰ Время вышло!", red))
		g.skipped++
		g.showAnswer(word)
		return
	}
	if answer == "quit" {
		fmt.Println("Выход.")
		g.saveStats()
		os.Exit(0)
	}
	if answer == "?" {
		if g.hintUsed < 1 {
			fmt.Printf("%s\n", colorize("Подсказка: первая буква '"+string(word[0])+"'", green))
			g.hintUsed++
			g.score = max(0, g.score-5)
			g.playRound(word)
			return
		} else {
			fmt.Println(colorize("Подсказка уже использована.", yellow))
			g.playRound(word)
			return
		}
	}
	if answer == "pass" {
		g.skipped++
		g.showAnswer(word)
		return
	}
	g.attempts++
	if answer == word {
		g.correct++
		points := len(word)
		g.score += points
		fmt.Printf("%s\n", colorize(fmt.Sprintf("✅ Верно! +%d очков. Время: %d сек", points, elapsed), green))
	} else {
		g.skipped++
		g.showAnswer(word)
		fmt.Printf("%s\n", colorize("❌ Неверно. Загадано: "+word, red))
	}
}

func (g *AnagramGame) play() {
	fmt.Println(colorize("🎯 Добро пожаловать в игру 'Анаграммы'!", bold))
	fmt.Printf("Уровень: %s, лимит времени: %d сек на слово.\n", g.level, g.timeLimit)
	fmt.Println("Цель: угадать слово из перемешанных букв.")
	fmt.Println("Вводите слова, используйте ? для подсказки, pass для пропуска, quit для выхода.\n")

	rand.Seed(time.Now().UnixNano())
	rand.Shuffle(len(g.words), func(i, j int) {
		g.words[i], g.words[j] = g.words[j], g.words[i]
	})
	total := min(g.maxWords, len(g.words))
	for i := 0; i < total; i++ {
		fmt.Printf("%s\n", colorize(fmt.Sprintf("\nСлово %d/%d", i+1, total), blue))
		g.playRound(g.words[i])
	}
	fmt.Println(colorize("\n🏁 Игра завершена!", bold))
	fmt.Printf("  Угадано слов: %d\n", g.correct)
	fmt.Printf("  Пропущено: %d\n", g.skipped)
	fmt.Printf("  Использовано подсказок: %d\n", g.hintUsed)
	fmt.Printf("  Счёт: %d\n", g.score)
	g.stats.Games++
	if g.score > g.stats.BestScore {
		g.stats.BestScore = g.score
		fmt.Println(colorize("🏆 Новый рекорд!", green))
	}
	g.stats.TotalWords += g.correct
	g.saveStats()
	fmt.Printf("%s\n", colorize(fmt.Sprintf("Лучший результат: %d", g.stats.BestScore), yellow))
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}

func main() {
	level := "easy"
	showStats := false
	resetStats := false
	args := os.Args[1:]
	for i := 0; i < len(args); i++ {
		arg := args[i]
		switch arg {
		case "easy", "medium", "hard":
			level = arg
		case "-s", "--stats":
			showStats = true
		case "-r", "--reset":
			resetStats = true
		case "-h", "--help":
			fmt.Println("Usage: anagram [easy|medium|hard] [-s] [-r]")
			return
		}
	}
	if resetStats {
		f := filepath.Join(os.Getenv("HOME"), ".anagram_stats.json")
		os.Remove(f)
		fmt.Println("Статистика сброшена.")
		return
	}
	if showStats {
		f := filepath.Join(os.Getenv("HOME"), ".anagram_stats.json")
		data, err := os.ReadFile(f)
		if err != nil {
			fmt.Println("Статистика пуста.")
			return
		}
		var stats Stats
		json.Unmarshal(data, &stats)
		fmt.Println(colorize("📊 Статистика:", bold))
		fmt.Printf("  Сыграно игр: %d\n", stats.Games)
		fmt.Printf("  Лучший счёт: %d\n", stats.BestScore)
		fmt.Printf("  Всего угадано слов: %d\n", stats.TotalWords)
		return
	}
	game := NewAnagramGame(level)
	game.play()
}
