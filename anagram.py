# anagram.py
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import random
import json
import time
import threading
from pathlib import Path

# ANSI-цвета
COLORS = {
    'reset': '\033[0m',
    'green': '\033[92m',
    'red': '\033[91m',
    'yellow': '\033[93m',
    'blue': '\033[94m',
    'cyan': '\033[96m',
    'bold': '\033[1m'
}

def colorize(text, color):
    return f"{COLORS.get(color, '')}{text}{COLORS['reset']}"

# Словарь (встроенный)
WORDS = {
    'easy': ['кот', 'нос', 'рот', 'сом', 'мир', 'ток', 'кот', 'сон', 'топ', 'вес', 'год', 'дом', 'дым', 'жара', 'зима', 'луна', 'море', 'нога', 'рука', 'вода'],
    'medium': ['камень', 'молния', 'небо', 'облако', 'окно', 'песня', 'река', 'солнце', 'трава', 'утро', 'вечер', 'город', 'дерево', 'земля', 'книга', 'ложка', 'ночь', 'остров', 'парта', 'роза', 'стена', 'тетрадь'],
    'hard': ['аэропорт', 'библиотека', 'велосипед', 'государство', 'департамент', 'инструмент', 'компьютер', 'лексикография', 'математика', 'насекомое', 'оборудование', 'программирование', 'руководство', 'свидетельство', 'телевидение', 'университет', 'философия', 'хирургия', 'цивилизация', 'электричество']
}

class AnagramGame:
    def __init__(self, level='easy'):
        self.level = level
        self.words = WORDS[level]
        self.max_words = 10
        self.score = 0
        self.attempts = 0
        self.correct = 0
        self.skipped = 0
        self.hint_used = 0
        self.total_time = 0
        self.stats_file = Path.home() / '.anagram_stats.json'
        self.load_stats()
        self.time_limit = {'easy': 60, 'medium': 45, 'hard': 30}[level]

    def load_stats(self):
        if self.stats_file.exists():
            with open(self.stats_file, 'r') as f:
                self.stats = json.load(f)
        else:
            self.stats = {'games': 0, 'best_score': 0, 'total_words': 0}

    def save_stats(self):
        with open(self.stats_file, 'w') as f:
            json.dump(self.stats, f, indent=2)

    def shuffle_word(self, word):
        letters = list(word)
        shuffled = word
        while shuffled == word:
            random.shuffle(letters)
            shuffled = ''.join(letters)
        return shuffled

    def timer_input(self, prompt, timeout):
        print(colorize(prompt, 'bold'), end='', flush=True)
        user_input = ['']
        def get_input():
            try:
                user_input[0] = sys.stdin.readline().strip()
            except:
                pass
        thread = threading.Thread(target=get_input)
        thread.daemon = True
        thread.start()
        start = time.time()
        while time.time() - start < timeout:
            if user_input[0] != '':
                return user_input[0]
            remaining = int(timeout - (time.time() - start))
            sys.stdout.write(f"\r{colorize(f'Осталось времени: {remaining} сек', 'yellow')}")
            sys.stdout.flush()
            time.sleep(1)
        return None

    def play_round(self, word):
        shuffled = self.shuffle_word(word)
        print(colorize(f"\nАнаграмма: {shuffled}", 'cyan'))
        print(colorize(f"Длина слова: {len(word)}", 'blue'))
        print("Введите слово, ? для подсказки, pass для пропуска, quit для выхода.")

        start_time = time.time()
        answer = self.timer_input("Ваш ответ: ", self.time_limit)
        elapsed = time.time() - start_time

        if answer is None:
            print(colorize("⏰ Время вышло!", 'red'))
            self.skipped += 1
            self.show_answer(word)
            return

        if answer == 'quit':
            print("Выход.")
            self.save_stats()
            sys.exit(0)
        if answer == '?':
            if self.hint_used < 1:
                print(colorize(f"Подсказка: первая буква '{word[0]}'", 'green'))
                self.hint_used += 1
                self.score = max(0, self.score - 5)
                # Повторяем ход без потери попытки
                self.play_round(word)
                return
            else:
                print(colorize("Подсказка уже использована.", 'yellow'))
                self.play_round(word)
                return
        if answer == 'pass':
            self.skipped += 1
            self.show_answer(word)
            return

        self.attempts += 1
        if answer.lower() == word:
            self.correct += 1
            points = len(word)
            self.score += points
            print(colorize(f"✅ Верно! +{points} очков. Время: {elapsed:.1f} сек", 'green'))
        else:
            self.skipped += 1
            self.show_answer(word)
            print(colorize(f"❌ Неверно. Загадано: {word}", 'red'))

    def show_answer(self, word):
        print(colorize(f"Загаданное слово: {word}", 'yellow'))

    def play(self):
        print(colorize("🎯 Добро пожаловать в игру 'Анаграммы'!", 'bold'))
        print(f"Уровень: {self.level}, лимит времени: {self.time_limit} сек на слово.")
        print("Цель: угадать слово из перемешанных букв.")
        print("Вводите слова, используйте ? для подсказки, pass для пропуска, quit для выхода.\n")

        random.shuffle(self.words)
        total_words = min(self.max_words, len(self.words))
        for i in range(total_words):
            print(colorize(f"\nСлово {i+1}/{total_words}", 'blue'))
            self.play_round(self.words[i])

        # Итоги
        print(colorize("\n🏁 Игра завершена!", 'bold'))
        print(f"  Угадано слов: {self.correct}")
        print(f"  Пропущено: {self.skipped}")
        print(f"  Использовано подсказок: {self.hint_used}")
        print(f"  Счёт: {self.score}")
        self.stats['games'] += 1
        if self.score > self.stats['best_score']:
            self.stats['best_score'] = self.score
            print(colorize("🏆 Новый рекорд!", 'green'))
        self.stats['total_words'] += self.correct
        self.save_stats()
        print(colorize(f"Лучший результат: {self.stats['best_score']}", 'yellow'))

def main():
    level = 'easy'
    show_stats = False
    reset_stats = False
    args = sys.argv[1:]
    for arg in args:
        if arg in ['easy', 'medium', 'hard']:
            level = arg
        elif arg == '-s' or arg == '--stats':
            show_stats = True
        elif arg == '-r' or arg == '--reset':
            reset_stats = True
        elif arg == '-h' or arg == '--help':
            print("Usage: anagram.py [easy|medium|hard] [-s] [-r]")
            return
    if reset_stats:
        stats_file = Path.home() / '.anagram_stats.json'
        if stats_file.exists():
            stats_file.unlink()
        print("Статистика сброшена.")
        return
    if show_stats:
        stats_file = Path.home() / '.anagram_stats.json'
        if stats_file.exists():
            with open(stats_file, 'r') as f:
                stats = json.load(f)
                print(colorize("📊 Статистика:", 'bold'))
                print(f"  Сыграно игр: {stats['games']}")
                print(f"  Лучший счёт: {stats['best_score']}")
                print(f"  Всего угадано слов: {stats['total_words']}")
        else:
            print("Статистика пуста.")
        return
    game = AnagramGame(level)
    game.play()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print(colorize("\nИгра прервана.", 'yellow'))
        sys.exit(0)
