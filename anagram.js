// anagram.js
#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');
const os = require('os');
const readline = require('readline');

const COLORS = {
    reset: '\x1b[0m',
    green: '\x1b[92m',
    red: '\x1b[91m',
    yellow: '\x1b[93m',
    blue: '\x1b[94m',
    cyan: '\x1b[96m',
    bold: '\x1b[1m'
};

function colorize(text, color) {
    return COLORS[color] + text + COLORS.reset;
}

const WORDS = {
    easy: ['кот', 'нос', 'рот', 'сом', 'мир', 'ток', 'сон', 'топ', 'вес', 'год', 'дом', 'дым', 'жара', 'зима', 'луна', 'море', 'нога', 'рука', 'вода'],
    medium: ['камень', 'молния', 'небо', 'облако', 'окно', 'песня', 'река', 'солнце', 'трава', 'утро', 'вечер', 'город', 'дерево', 'земля', 'книга', 'ложка', 'ночь', 'остров', 'парта', 'роза', 'стена', 'тетрадь'],
    hard: ['аэропорт', 'библиотека', 'велосипед', 'государство', 'департамент', 'инструмент', 'компьютер', 'лексикография', 'математика', 'насекомое', 'оборудование', 'программирование', 'руководство', 'свидетельство', 'телевидение', 'университет', 'философия', 'хирургия', 'цивилизация', 'электричество']
};

class AnagramGame {
    constructor(level = 'easy') {
        this.level = level;
        this.words = WORDS[level];
        this.maxWords = 10;
        this.score = 0;
        this.attempts = 0;
        this.correct = 0;
        this.skipped = 0;
        this.hintUsed = 0;
        this.statsFile = path.join(os.homedir(), '.anagram_stats.json');
        this.loadStats();
        this.timeLimit = { easy: 60, medium: 45, hard: 30 }[level];
    }

    loadStats() {
        try {
            this.stats = JSON.parse(fs.readFileSync(this.statsFile, 'utf8'));
        } catch {
            this.stats = { games: 0, best_score: 0, total_words: 0 };
        }
    }

    saveStats() {
        fs.writeFileSync(this.statsFile, JSON.stringify(this.stats, null, 2));
    }

    shuffleWord(word) {
        let letters = word.split('');
        let shuffled = word;
        while (shuffled === word) {
            for (let i = letters.length - 1; i > 0; i--) {
                const j = Math.floor(Math.random() * (i + 1));
                [letters[i], letters[j]] = [letters[j], letters[i]];
            }
            shuffled = letters.join('');
        }
        return shuffled;
    }

    timerInput(prompt, timeout) {
        return new Promise((resolve) => {
            const rl = readline.createInterface({
                input: process.stdin,
                output: process.stdout
            });
            let answered = false;
            const timer = setTimeout(() => {
                if (!answered) {
                    rl.close();
                    resolve('');
                }
            }, timeout * 1000);
            rl.question(colorize(prompt, 'bold'), (answer) => {
                answered = true;
                clearTimeout(timer);
                rl.close();
                resolve(answer.trim());
            });
        });
    }

    showAnswer(word) {
        console.log(colorize(`Загаданное слово: ${word}`, 'yellow'));
    }

    async playRound(word) {
        const shuffled = this.shuffleWord(word);
        console.log(colorize(`\nАнаграмма: ${shuffled}`, 'cyan'));
        console.log(colorize(`Длина слова: ${word.length}`, 'blue'));
        console.log('Введите слово, ? для подсказки, pass для пропуска, quit для выхода.');

        const start = Date.now();
        const answer = await this.timerInput('Ваш ответ: ', this.timeLimit);
        const elapsed = Math.floor((Date.now() - start) / 1000);

        if (answer === '') {
            console.log(colorize('⏰ Время вышло!', 'red'));
            this.skipped++;
            this.showAnswer(word);
            return;
        }
        if (answer === 'quit') {
            console.log('Выход.');
            this.saveStats();
            process.exit(0);
        }
        if (answer === '?') {
            if (this.hintUsed < 1) {
                console.log(colorize(`Подсказка: первая буква '${word[0]}'`, 'green'));
                this.hintUsed++;
                this.score = Math.max(0, this.score - 5);
                await this.playRound(word);
                return;
            } else {
                console.log(colorize('Подсказка уже использована.', 'yellow'));
                await this.playRound(word);
                return;
            }
        }
        if (answer === 'pass') {
            this.skipped++;
            this.showAnswer(word);
            return;
        }
        this.attempts++;
        if (answer === word) {
            this.correct++;
            const points = word.length;
            this.score += points;
            console.log(colorize(`✅ Верно! +${points} очков. Время: ${elapsed} сек`, 'green'));
        } else {
            this.skipped++;
            this.showAnswer(word);
            console.log(colorize(`❌ Неверно. Загадано: ${word}`, 'red'));
        }
    }

    async play() {
        console.log(colorize('🎯 Добро пожаловать в игру "Анаграммы"!', 'bold'));
        console.log(`Уровень: ${this.level}, лимит времени: ${this.timeLimit} сек на слово.`);
        console.log('Цель: угадать слово из перемешанных букв.');
        console.log('Вводите слова, используйте ? для подсказки, pass для пропуска, quit для выхода.\n');

        // Перемешиваем слова
        for (let i = this.words.length - 1; i > 0; i--) {
            const j = Math.floor(Math.random() * (i + 1));
            [this.words[i], this.words[j]] = [this.words[j], this.words[i]];
        }
        const total = Math.min(this.maxWords, this.words.length);
        for (let i = 0; i < total; i++) {
            console.log(colorize(`\nСлово ${i+1}/${total}`, 'blue'));
            await this.playRound(this.words[i]);
        }
        console.log(colorize('\n🏁 Игра завершена!', 'bold'));
        console.log(`  Угадано слов: ${this.correct}`);
        console.log(`  Пропущено: ${this.skipped}`);
        console.log(`  Использовано подсказок: ${this.hintUsed}`);
        console.log(`  Счёт: ${this.score}`);
        this.stats.games++;
        if (this.score > this.stats.best_score) {
            this.stats.best_score = this.score;
            console.log(colorize('🏆 Новый рекорд!', 'green'));
        }
        this.stats.total_words += this.correct;
        this.saveStats();
        console.log(colorize(`Лучший результат: ${this.stats.best_score}`, 'yellow'));
    }
}

async function main() {
    let level = 'easy';
    let showStats = false;
    let resetStats = false;
    const args = process.argv.slice(2);
    for (const arg of args) {
        if (arg === 'easy' || arg === 'medium' || arg === 'hard') level = arg;
        else if (arg === '-s' || arg === '--stats') showStats = true;
        else if (arg === '-r' || arg === '--reset') resetStats = true;
        else if (arg === '-h' || arg === '--help') {
            console.log('Usage: node anagram.js [easy|medium|hard] [-s] [-r]');
            process.exit(0);
        }
    }
    if (resetStats) {
        const f = path.join(os.homedir(), '.anagram_stats.json');
        if (fs.existsSync(f)) fs.unlinkSync(f);
        console.log('Статистика сброшена.');
        return;
    }
    if (showStats) {
        const f = path.join(os.homedir(), '.anagram_stats.json');
        try {
            const stats = JSON.parse(fs.readFileSync(f, 'utf8'));
            console.log(colorize('📊 Статистика:', 'bold'));
            console.log(`  Сыграно игр: ${stats.games}`);
            console.log(`  Лучший счёт: ${stats.best_score}`);
            console.log(`  Всего угадано слов: ${stats.total_words}`);
        } catch {
            console.log('Статистика пуста.');
        }
        return;
    }
    const game = new AnagramGame(level);
    await game.play();
}

main().catch(console.error);
