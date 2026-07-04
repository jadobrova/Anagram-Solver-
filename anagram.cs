// anagram.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading;
using System.Diagnostics;

class AnagramGame
{
    static string Colorize(string text, string color)
    {
        string col = color switch
        {
            "green" => "\x1b[92m",
            "red" => "\x1b[91m",
            "yellow" => "\x1b[93m",
            "blue" => "\x1b[94m",
            "cyan" => "\x1b[96m",
            "bold" => "\x1b[1m",
            _ => "\x1b[0m"
        };
        return col + text + "\x1b[0m";
    }

    static Dictionary<string, List<string>> WORDS = new Dictionary<string, List<string>>
    {
        {"easy", new List<string>{"кот","нос","рот","сом","мир","ток","сон","топ","вес","год","дом","дым","жара","зима","луна","море","нога","рука","вода"}},
        {"medium", new List<string>{"камень","молния","небо","облако","окно","песня","река","солнце","трава","утро","вечер","город","дерево","земля","книга","ложка","ночь","остров","парта","роза","стена","тетрадь"}},
        {"hard", new List<string>{"аэропорт","библиотека","велосипед","государство","департамент","инструмент","компьютер","лексикография","математика","насекомое","оборудование","программирование","руководство","свидетельство","телевидение","университет","философия","хирургия","цивилизация","электричество"}}
    };

    class Stats
    {
        public int games { get; set; }
        public int best_score { get; set; }
        public int total_words { get; set; }
    }

    private string level;
    private List<string> words;
    private int maxWords;
    private int score;
    private int attempts;
    private int correct;
    private int skipped;
    private int hintUsed;
    private int timeLimit;
    private Stats stats;
    private string statsFile;

    public AnagramGame(string lvl)
    {
        level = lvl;
        words = WORDS[level];
        maxWords = 10;
        score = 0;
        attempts = 0;
        correct = 0;
        skipped = 0;
        hintUsed = 0;
        statsFile = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".anagram_stats.json");
        LoadStats();
        timeLimit = level == "easy" ? 60 : level == "medium" ? 45 : 30;
    }

    void LoadStats()
    {
        if (File.Exists(statsFile))
        {
            try
            {
                string json = File.ReadAllText(statsFile);
                stats = JsonSerializer.Deserialize<Stats>(json);
            }
            catch { stats = new Stats(); }
        }
        else stats = new Stats();
    }

    void SaveStats()
    {
        string json = JsonSerializer.Serialize(stats);
        File.WriteAllText(statsFile, json);
    }

    string ShuffleWord(string word)
    {
        Random rnd = new Random();
        char[] letters = word.ToCharArray();
        string shuffled = word;
        while (shuffled == word)
        {
            for (int i = letters.Length - 1; i > 0; i--)
            {
                int j = rnd.Next(i + 1);
                char tmp = letters[i];
                letters[i] = letters[j];
                letters[j] = tmp;
            }
            shuffled = new string(letters);
        }
        return shuffled;
    }

    string TimerInput(string prompt, int timeout)
    {
        Console.Write(Colorize(prompt, "bold"));
        var sw = Stopwatch.StartNew();
        string input = "";
        while (sw.Elapsed.TotalSeconds < timeout)
        {
            if (Console.KeyAvailable)
            {
                input = Console.ReadLine();
                return input;
            }
            int remaining = timeout - (int)sw.Elapsed.TotalSeconds;
            Console.Write($"\r{Colorize($"Осталось времени: {remaining} сек", "yellow")}");
            Thread.Sleep(1000);
        }
        return "";
    }

    void ShowAnswer(string word)
    {
        Console.WriteLine(Colorize($"Загаданное слово: {word}", "yellow"));
    }

    void PlayRound(string word)
    {
        string shuffled = ShuffleWord(word);
        Console.WriteLine(Colorize($"\nАнаграмма: {shuffled}", "cyan"));
        Console.WriteLine(Colorize($"Длина слова: {word.Length}", "blue"));
        Console.WriteLine("Введите слово, ? для подсказки, pass для пропуска, quit для выхода.");

        var sw = Stopwatch.StartNew();
        string answer = TimerInput("Ваш ответ: ", timeLimit);
        sw.Stop();
        double elapsed = sw.Elapsed.TotalSeconds;

        if (string.IsNullOrEmpty(answer))
        {
            Console.WriteLine(Colorize("⏰ Время вышло!", "red"));
            skipped++;
            ShowAnswer(word);
            return;
        }
        if (answer == "quit")
        {
            Console.WriteLine("Выход.");
            SaveStats();
            Environment.Exit(0);
        }
        if (answer == "?")
        {
            if (hintUsed < 1)
            {
                Console.WriteLine(Colorize($"Подсказка: первая буква '{word[0]}'", "green"));
                hintUsed++;
                score = Math.Max(0, score - 5);
                PlayRound(word);
                return;
            }
            else
            {
                Console.WriteLine(Colorize("Подсказка уже использована.", "yellow"));
                PlayRound(word);
                return;
            }
        }
        if (answer == "pass")
        {
            skipped++;
            ShowAnswer(word);
            return;
        }
        attempts++;
        if (answer == word)
        {
            correct++;
            int points = word.Length;
            score += points;
            Console.WriteLine(Colorize($"✅ Верно! +{points} очков. Время: {elapsed:F1} сек", "green"));
        }
        else
        {
            skipped++;
            ShowAnswer(word);
            Console.WriteLine(Colorize($"❌ Неверно. Загадано: {word}", "red"));
        }
    }

    public void Play()
    {
        Console.WriteLine(Colorize("🎯 Добро пожаловать в игру 'Анаграммы'!", "bold"));
        Console.WriteLine($"Уровень: {level}, лимит времени: {timeLimit} сек на слово.");
        Console.WriteLine("Цель: угадать слово из перемешанных букв.");
        Console.WriteLine("Вводите слова, используйте ? для подсказки, pass для пропуска, quit для выхода.\n");

        Random rnd = new Random();
        words = words.OrderBy(x => rnd.Next()).ToList();
        int total = Math.Min(maxWords, words.Count);
        for (int i = 0; i < total; i++)
        {
            Console.WriteLine(Colorize($"\nСлово {i+1}/{total}", "blue"));
            PlayRound(words[i]);
        }
        Console.WriteLine(Colorize("\n🏁 Игра завершена!", "bold"));
        Console.WriteLine($"  Угадано слов: {correct}");
        Console.WriteLine($"  Пропущено: {skipped}");
        Console.WriteLine($"  Использовано подсказок: {hintUsed}");
        Console.WriteLine($"  Счёт: {score}");
        stats.games++;
        if (score > stats.best_score)
        {
            stats.best_score = score;
            Console.WriteLine(Colorize("🏆 Новый рекорд!", "green"));
        }
        stats.total_words += correct;
        SaveStats();
        Console.WriteLine(Colorize($"Лучший результат: {stats.best_score}", "yellow"));
    }

    static void Main(string[] args)
    {
        string level = "easy";
        bool showStats = false, resetStats = false;
        foreach (var arg in args)
        {
            if (arg == "easy" || arg == "medium" || arg == "hard") level = arg;
            else if (arg == "-s" || arg == "--stats") showStats = true;
            else if (arg == "-r" || arg == "--reset") resetStats = true;
            else if (arg == "-h" || arg == "--help")
            {
                Console.WriteLine("Usage: anagram [easy|medium|hard] [-s] [-r]");
                return;
            }
        }
        if (resetStats)
        {
            string f = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".anagram_stats.json");
            if (File.Exists(f)) File.Delete(f);
            Console.WriteLine("Статистика сброшена.");
            return;
        }
        if (showStats)
        {
            string f = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".anagram_stats.json");
            if (File.Exists(f))
            {
                try
                {
                    string json = File.ReadAllText(f);
                    var stats = JsonSerializer.Deserialize<Stats>(json);
                    Console.WriteLine(Colorize("📊 Статистика:", "bold"));
                    Console.WriteLine($"  Сыграно игр: {stats.games}");
                    Console.WriteLine($"  Лучший счёт: {stats.best_score}");
                    Console.WriteLine($"  Всего угадано слов: {stats.total_words}");
                }
                catch { Console.WriteLine("Статистика пуста."); }
            }
            else Console.WriteLine("Статистика пуста.");
            return;
        }
        AnagramGame game = new AnagramGame(level);
        game.Play();
    }
}
