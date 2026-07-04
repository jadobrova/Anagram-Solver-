// anagram.java
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;

public class anagram {
    private static final String RESET = "\u001B[0m";
    private static final String GREEN = "\u001B[92m";
    private static final String RED = "\u001B[91m";
    private static final String YELLOW = "\u001B[93m";
    private static final String BLUE = "\u001B[94m";
    private static final String CYAN = "\u001B[96m";
    private static final String BOLD = "\u001B[1m";

    private static String colorize(String text, String color) {
        return color + text + RESET;
    }

    private static final Map<String, List<String>> WORDS = new HashMap<>();
    static {
        WORDS.put("easy", Arrays.asList("кот","нос","рот","сом","мир","ток","сон","топ","вес","год","дом","дым","жара","зима","луна","море","нога","рука","вода"));
        WORDS.put("medium", Arrays.asList("камень","молния","небо","облако","окно","песня","река","солнце","трава","утро","вечер","город","дерево","земля","книга","ложка","ночь","остров","парта","роза","стена","тетрадь"));
        WORDS.put("hard", Arrays.asList("аэропорт","библиотека","велосипед","государство","департамент","инструмент","компьютер","лексикография","математика","насекомое","оборудование","программирование","руководство","свидетельство","телевидение","университет","философия","хирургия","цивилизация","электричество"));
    }

    private static class Stats {
        int games, best_score, total_words;
    }

    private String level;
    private List<String> words;
    private int maxWords;
    private int score;
    private int attempts;
    private int correct;
    private int skipped;
    private int hintUsed;
    private int timeLimit;
    private Stats stats;
    private String statsFile;
    private Scanner scanner;

    public anagram(String lvl) {
        level = lvl;
        words = WORDS.get(level);
        maxWords = 10;
        score = 0;
        attempts = 0;
        correct = 0;
        skipped = 0;
        hintUsed = 0;
        statsFile = System.getProperty("user.home") + "/.anagram_stats.json";
        loadStats();
        timeLimit = level.equals("easy") ? 60 : level.equals("medium") ? 45 : 30;
        scanner = new Scanner(System.in);
    }

    private void loadStats() {
        stats = new Stats();
        try {
            String json = new String(Files.readAllBytes(Paths.get(statsFile)));
            stats.games = extractInt(json, "games");
            stats.best_score = extractInt(json, "best_score");
            stats.total_words = extractInt(json, "total_words");
        } catch (Exception e) {
            // file not exists
        }
    }

    private int extractInt(String json, String key) {
        int idx = json.indexOf("\"" + key + "\"");
        if (idx == -1) return 0;
        int start = json.indexOf(":", idx) + 1;
        int end = json.indexOf(",", start);
        if (end == -1) end = json.indexOf("}", start);
        try { return Integer.parseInt(json.substring(start, end).trim()); } catch (Exception e) { return 0; }
    }

    private void saveStats() {
        try {
            String json = "{\"games\":" + stats.games + ",\"best_score\":" + stats.best_score +
                          ",\"total_words\":" + stats.total_words + "}";
            Files.write(Paths.get(statsFile), json.getBytes());
        } catch (IOException e) {}
    }

    private String shuffleWord(String word) {
        char[] letters = word.toCharArray();
        String shuffled = word;
        Random rnd = new Random();
        while (shuffled.equals(word)) {
            for (int i = letters.length - 1; i > 0; i--) {
                int j = rnd.nextInt(i + 1);
                char tmp = letters[i];
                letters[i] = letters[j];
                letters[j] = tmp;
            }
            shuffled = new String(letters);
        }
        return shuffled;
    }

    private String timerInput(String prompt, int timeout) {
        System.out.print(colorize(prompt, BOLD));
        ExecutorService executor = Executors.newSingleThreadExecutor();
        Future<String> future = executor.submit(() -> scanner.nextLine());
        try {
            return future.get(timeout, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
            future.cancel(true);
            return "";
        } catch (Exception e) {
            return "";
        } finally {
            executor.shutdownNow();
        }
    }

    private void showAnswer(String word) {
        System.out.println(colorize("Загаданное слово: " + word, YELLOW));
    }

    private void playRound(String word) {
        String shuffled = shuffleWord(word);
        System.out.println(colorize("\nАнаграмма: " + shuffled, CYAN));
        System.out.println(colorize("Длина слова: " + word.length(), BLUE));
        System.out.println("Введите слово, ? для подсказки, pass для пропуска, quit для выхода.");

        long start = System.currentTimeMillis();
        String answer = timerInput("Ваш ответ: ", timeLimit);
        double elapsed = (System.currentTimeMillis() - start) / 1000.0;

        if (answer.isEmpty()) {
            System.out.println(colorize("⏰ Время вышло!", RED));
            skipped++;
            showAnswer(word);
            return;
        }
        if (answer.equals("quit")) {
            System.out.println("Выход.");
            saveStats();
            System.exit(0);
        }
        if (answer.equals("?")) {
            if (hintUsed < 1) {
                System.out.println(colorize("Подсказка: первая буква '" + word.charAt(0) + "'", GREEN));
                hintUsed++;
                score = Math.max(0, score - 5);
                playRound(word);
                return;
            } else {
                System.out.println(colorize("Подсказка уже использована.", YELLOW));
                playRound(word);
                return;
            }
        }
        if (answer.equals("pass")) {
            skipped++;
            showAnswer(word);
            return;
        }
        attempts++;
        if (answer.equals(word)) {
            correct++;
            int points = word.length();
            score += points;
            System.out.println(colorize("✅ Верно! +" + points + " очков. Время: " + elapsed + " сек", GREEN));
        } else {
            skipped++;
            showAnswer(word);
            System.out.println(colorize("❌ Неверно. Загадано: " + word, RED));
        }
    }

    public void play() {
        System.out.println(colorize("🎯 Добро пожаловать в игру 'Анаграммы'!", BOLD));
        System.out.println("Уровень: " + level + ", лимит времени: " + timeLimit + " сек на слово.");
        System.out.println("Цель: угадать слово из перемешанных букв.");
        System.out.println("Вводите слова, используйте ? для подсказки, pass для пропуска, quit для выхода.\n");

        Collections.shuffle(words);
        int total = Math.min(maxWords, words.size());
        for (int i = 0; i < total; i++) {
            System.out.println(colorize("\nСлово " + (i+1) + "/" + total, BLUE));
            playRound(words.get(i));
        }
        System.out.println(colorize("\n🏁 Игра завершена!", BOLD));
        System.out.println("  Угадано слов: " + correct);
        System.out.println("  Пропущено: " + skipped);
        System.out.println("  Использовано подсказок: " + hintUsed);
        System.out.println("  Счёт: " + score);
        stats.games++;
        if (score > stats.best_score) {
            stats.best_score = score;
            System.out.println(colorize("🏆 Новый рекорд!", GREEN));
        }
        stats.total_words += correct;
        saveStats();
        System.out.println(colorize("Лучший результат: " + stats.best_score, YELLOW));
        scanner.close();
    }

    public static void main(String[] args) {
        String level = "easy";
        boolean showStats = false, resetStats = false;
        for (String arg : args) {
            if (arg.equals("easy") || arg.equals("medium") || arg.equals("hard")) level = arg;
            else if (arg.equals("-s") || arg.equals("--stats")) showStats = true;
            else if (arg.equals("-r") || arg.equals("--reset")) resetStats = true;
            else if (arg.equals("-h") || arg.equals("--help")) {
                System.out.println("Usage: java anagram [easy|medium|hard] [-s] [-r]");
                return;
            }
        }
        if (resetStats) {
            String f = System.getProperty("user.home") + "/.anagram_stats.json";
            try { Files.deleteIfExists(Paths.get(f)); } catch (Exception e) {}
            System.out.println("Статистика сброшена.");
            return;
        }
        if (showStats) {
            String f = System.getProperty("user.home") + "/.anagram_stats.json";
            try {
                String json = new String(Files.readAllBytes(Paths.get(f)));
                int games = extractInt(json, "games");
                int best = extractInt(json, "best_score");
                int words = extractInt(json, "total_words");
                System.out.println(colorize("📊 Статистика:", BOLD));
                System.out.println("  Сыграно игр: " + games);
                System.out.println("  Лучший счёт: " + best);
                System.out.println("  Всего угадано слов: " + words);
            } catch (Exception e) {
                System.out.println("Статистика пуста.");
            }
            return;
        }
        anagram game = new anagram(level);
        game.play();
    }

    private static int extractInt(String json, String key) {
        int idx = json.indexOf("\"" + key + "\"");
        if (idx == -1) return 0;
        int start = json.indexOf(":", idx) + 1;
        int end = json.indexOf(",", start);
        if (end == -1) end = json.indexOf("}", start);
        try { return Integer.parseInt(json.substring(start, end).trim()); } catch (Exception e) { return 0; }
    }
}
