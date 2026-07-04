// anagram.cpp
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <cctype>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

const string RESET = "\033[0m";
const string GREEN = "\033[92m";
const string RED = "\033[91m";
const string YELLOW = "\033[93m";
const string BLUE = "\033[94m";
const string CYAN = "\033[96m";
const string BOLD = "\033[1m";

string colorize(const string& text, const string& color) {
    return color + text + RESET;
}

string getHomeDir() {
    const char* home = getenv("HOME");
    if (!home) home = getenv("USERPROFILE");
    return string(home);
}

map<string, vector<string>> WORDS = {
    {"easy", {"кот","нос","рот","сом","мир","ток","сон","топ","вес","год","дом","дым","жара","зима","луна","море","нога","рука","вода"}},
    {"medium", {"камень","молния","небо","облако","окно","песня","река","солнце","трава","утро","вечер","город","дерево","земля","книга","ложка","ночь","остров","парта","роза","стена","тетрадь"}},
    {"hard", {"аэропорт","библиотека","велосипед","государство","департамент","инструмент","компьютер","лексикография","математика","насекомое","оборудование","программирование","руководство","свидетельство","телевидение","университет","философия","хирургия","цивилизация","электричество"}}
};

class AnagramGame {
public:
    string level;
    vector<string> words;
    int maxWords;
    int score;
    int attempts;
    int correct;
    int skipped;
    int hintUsed;
    int totalTime;
    string statsFile;
    map<string, int> stats;
    int timeLimit;

    AnagramGame(string lvl) : level(lvl), maxWords(10), score(0), attempts(0), correct(0), skipped(0), hintUsed(0), totalTime(0) {
        words = WORDS[level];
        statsFile = getHomeDir() + "/.anagram_stats.json";
        loadStats();
        if (level == "easy") timeLimit = 60;
        else if (level == "medium") timeLimit = 45;
        else timeLimit = 30;
    }

    void loadStats() {
        ifstream f(statsFile);
        if (!f) {
            stats["games"] = 0; stats["best_score"] = 0; stats["total_words"] = 0;
            return;
        }
        string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        auto extract = [&](const string& key) -> int {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == string::npos) return 0;
            pos = content.find(":", pos) + 1;
            size_t end = content.find(",", pos);
            if (end == string::npos) end = content.find("}", pos);
            try { return stoi(content.substr(pos, end-pos)); } catch (...) { return 0; }
        };
        stats["games"] = extract("games");
        stats["best_score"] = extract("best_score");
        stats["total_words"] = extract("total_words");
    }

    void saveStats() {
        ofstream f(statsFile);
        if (f) {
            f << "{\"games\":" << stats["games"] << ",\"best_score\":" << stats["best_score"]
              << ",\"total_words\":" << stats["total_words"] << "}";
        }
    }

    string shuffleWord(const string& word) {
        string shuffled = word;
        random_device rd;
        mt19937 g(rd());
        while (shuffled == word) {
            shuffle(shuffled.begin(), shuffled.end(), g);
        }
        return shuffled;
    }

    string timerInput(const string& prompt, int timeout) {
        cout << colorize(prompt, BOLD) << flush;
        string input;
        auto start = chrono::steady_clock::now();
        while (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count() < timeout) {
            if (cin.rdbuf()->in_avail() > 0) {
                getline(cin, input);
                return input;
            }
            int remaining = timeout - chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count();
            cout << "\r" << colorize("Осталось времени: " + to_string(remaining) + " сек", YELLOW) << flush;
            this_thread::sleep_for(chrono::seconds(1));
        }
        return "";
    }

    void showAnswer(const string& word) {
        cout << colorize("Загаданное слово: " + word, YELLOW) << endl;
    }

    void playRound(const string& word) {
        string shuffled = shuffleWord(word);
        cout << colorize("\nАнаграмма: " + shuffled, CYAN) << endl;
        cout << colorize("Длина слова: " + to_string(word.size()), BLUE) << endl;
        cout << "Введите слово, ? для подсказки, pass для пропуска, quit для выхода." << endl;

        auto start = chrono::steady_clock::now();
        string answer = timerInput("Ваш ответ: ", timeLimit);
        auto elapsed = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count();

        if (answer.empty()) {
            cout << colorize("⏰ Время вышло!", RED) << endl;
            skipped++;
            showAnswer(word);
            return;
        }
        if (answer == "quit") {
            cout << "Выход." << endl;
            saveStats();
            exit(0);
        }
        if (answer == "?") {
            if (hintUsed < 1) {
                cout << colorize("Подсказка: первая буква '" + string(1, word[0]) + "'", GREEN) << endl;
                hintUsed++;
                score = max(0, score - 5);
                playRound(word);
                return;
            } else {
                cout << colorize("Подсказка уже использована.", YELLOW) << endl;
                playRound(word);
                return;
            }
        }
        if (answer == "pass") {
            skipped++;
            showAnswer(word);
            return;
        }
        attempts++;
        if (answer == word) {
            correct++;
            int points = word.size();
            score += points;
            cout << colorize("✅ Верно! +" + to_string(points) + " очков. Время: " + to_string(elapsed) + " сек", GREEN) << endl;
        } else {
            skipped++;
            showAnswer(word);
            cout << colorize("❌ Неверно. Загадано: " + word, RED) << endl;
        }
    }

    void play() {
        cout << colorize("🎯 Добро пожаловать в игру 'Анаграммы'!", BOLD) << endl;
        cout << "Уровень: " << level << ", лимит времени: " << timeLimit << " сек на слово." << endl;
        cout << "Цель: угадать слово из перемешанных букв." << endl;
        cout << "Вводите слова, используйте ? для подсказки, pass для пропуска, quit для выхода.\n" << endl;

        random_device rd;
        mt19937 g(rd());
        shuffle(words.begin(), words.end(), g);
        int total = min(maxWords, (int)words.size());
        for (int i=0; i<total; ++i) {
            cout << colorize("\nСлово " + to_string(i+1) + "/" + to_string(total), BLUE) << endl;
            playRound(words[i]);
        }
        cout << colorize("\n🏁 Игра завершена!", BOLD) << endl;
        cout << "  Угадано слов: " << correct << endl;
        cout << "  Пропущено: " << skipped << endl;
        cout << "  Использовано подсказок: " << hintUsed << endl;
        cout << "  Счёт: " << score << endl;
        stats["games"]++;
        if (score > stats["best_score"]) {
            stats["best_score"] = score;
            cout << colorize("🏆 Новый рекорд!", GREEN) << endl;
        }
        stats["total_words"] += correct;
        saveStats();
        cout << colorize("Лучший результат: " + to_string(stats["best_score"]), YELLOW) << endl;
    }
};

int main(int argc, char* argv[]) {
    string level = "easy";
    bool showStats = false, resetStats = false;
    for (int i=1; i<argc; ++i) {
        string arg = argv[i];
        if (arg == "easy" || arg == "medium" || arg == "hard") level = arg;
        else if (arg == "-s" || arg == "--stats") showStats = true;
        else if (arg == "-r" || arg == "--reset") resetStats = true;
        else if (arg == "-h" || arg == "--help") {
            cout << "Usage: anagram [easy|medium|hard] [-s] [-r]" << endl;
            return 0;
        }
    }
    if (resetStats) {
        string f = getHomeDir() + "/.anagram_stats.json";
        if (fs::exists(f)) fs::remove(f);
        cout << "Статистика сброшена." << endl;
        return 0;
    }
    if (showStats) {
        string f = getHomeDir() + "/.anagram_stats.json";
        ifstream file(f);
        if (file) {
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            auto extract = [&](const string& key) -> int {
                size_t pos = content.find("\"" + key + "\"");
                if (pos == string::npos) return 0;
                pos = content.find(":", pos) + 1;
                size_t end = content.find(",", pos);
                if (end == string::npos) end = content.find("}", pos);
                try { return stoi(content.substr(pos, end-pos)); } catch (...) { return 0; }
            };
            int games = extract("games"), best = extract("best_score"), words = extract("total_words");
            cout << colorize("📊 Статистика:", BOLD) << endl;
            cout << "  Сыграно игр: " << games << endl;
            cout << "  Лучший счёт: " << best << endl;
            cout << "  Всего угадано слов: " << words << endl;
        } else {
            cout << "Статистика пуста." << endl;
        }
        return 0;
    }
    AnagramGame game(level);
    game.play();
    return 0;
}
