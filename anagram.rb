#!/usr/bin/env ruby
# anagram.rb
# encoding: UTF-8

require 'json'
require 'fileutils'
require 'timeout'

COLORS = {
  reset: "\e[0m",
  green: "\e[92m",
  red: "\e[91m",
  yellow: "\e[93m",
  blue: "\e[94m",
  cyan: "\e[96m",
  bold: "\e[1m"
}

def colorize(text, color)
  "#{COLORS[color]}#{text}#{COLORS[:reset]}"
end

WORDS = {
  'easy' => %w[кот нос рот сом мир ток сон топ вес год дом дым жара зима луна море нога рука вода],
  'medium' => %w[камень молния небо облако окно песня река солнце трава утро вечер город дерево земля книга ложка ночь остров парта роза стена тетрадь],
  'hard' => %w[аэропорт библиотека велосипед государство департамент инструмент компьютер лексикография математика насекомое оборудование программирование руководство свидетельство телевидение университет философия хирургия цивилизация электричество]
}

class AnagramGame
  attr_reader :level, :words, :max_words, :score, :attempts, :correct, :skipped,
              :hint_used, :time_limit, :stats, :stats_file

  def initialize(level = 'easy')
    @level = level
    @words = WORDS[level]
    @max_words = 10
    @score = 0
    @attempts = 0
    @correct = 0
    @skipped = 0
    @hint_used = 0
    @stats_file = File.join(Dir.home, '.anagram_stats.json')
    load_stats
    @time_limit = { 'easy' => 60, 'medium' => 45, 'hard' => 30 }[level]
  end

  def load_stats
    if File.exist?(@stats_file)
      @stats = JSON.parse(File.read(@stats_file))
    else
      @stats = { 'games' => 0, 'best_score' => 0, 'total_words' => 0 }
    end
  end

  def save_stats
    File.write(@stats_file, JSON.pretty_generate(@stats))
  end

  def shuffle_word(word)
    letters = word.chars
    shuffled = word
    while shuffled == word
      shuffled = letters.shuffle.join
    end
    shuffled
  end

  def timer_input(prompt, timeout)
    print colorize(prompt, :bold)
    answer = nil
    begin
      Timeout.timeout(timeout) do
        answer = STDIN.gets.chomp
      end
    rescue Timeout::Error
      answer = nil
    end
    answer
  end

  def show_answer(word)
    puts colorize("Загаданное слово: #{word}", :yellow)
  end

  def play_round(word)
    shuffled = shuffle_word(word)
    puts colorize("\nАнаграмма: #{shuffled}", :cyan)
    puts colorize("Длина слова: #{word.length}", :blue)
    puts 'Введите слово, ? для подсказки, pass для пропуска, quit для выхода.'

    start = Time.now
    answer = timer_input('Ваш ответ: ', @time_limit)
    elapsed = Time.now - start

    if answer.nil?
      puts colorize('⏰ Время вышло!', :red)
      @skipped += 1
      show_answer(word)
      return
    end
    if answer == 'quit'
      puts 'Выход.'
      save_stats
      exit
    end
    if answer == '?'
      if @hint_used < 1
        puts colorize("Подсказка: первая буква '#{word[0]}'", :green)
        @hint_used += 1
        @score = [0, @score - 5].max
        play_round(word)
        return
      else
        puts colorize('Подсказка уже использована.', :yellow)
        play_round(word)
        return
      end
    end
    if answer == 'pass'
      @skipped += 1
      show_answer(word)
      return
    end
    @attempts += 1
    if answer == word
      @correct += 1
      points = word.length
      @score += points
      puts colorize("✅ Верно! +#{points} очков. Время: #{elapsed.round(1)} сек", :green)
    else
      @skipped += 1
      show_answer(word)
      puts colorize("❌ Неверно. Загадано: #{word}", :red)
    end
  end

  def play
    puts colorize('🎯 Добро пожаловать в игру "Анаграммы"!', :bold)
    puts "Уровень: #{@level}, лимит времени: #{@time_limit} сек на слово."
    puts 'Цель: угадать слово из перемешанных букв.'
    puts 'Вводите слова, используйте ? для подсказки, pass для пропуска, quit для выхода.'

    @words.shuffle!
    total = [@max_words, @words.size].min
    (0...total).each do |i|
      puts colorize("\nСлово #{i+1}/#{total}", :blue)
      play_round(@words[i])
    end
    puts colorize("\n🏁 Игра завершена!", :bold)
    puts "  Угадано слов: #{@correct}"
    puts "  Пропущено: #{@skipped}"
    puts "  Использовано подсказок: #{@hint_used}"
    puts "  Счёт: #{@score}"
    @stats['games'] += 1
    if @score > @stats['best_score']
      @stats['best_score'] = @score
      puts colorize('🏆 Новый рекорд!', :green)
    end
    @stats['total_words'] += @correct
    save_stats
    puts colorize("Лучший результат: #{@stats['best_score']}", :yellow)
  end
end

def main
  level = 'easy'
  show_stats = false
  reset_stats = false
  ARGV.each do |arg|
    case arg
    when 'easy', 'medium', 'hard'
      level = arg
    when '-s', '--stats'
      show_stats = true
    when '-r', '--reset'
      reset_stats = true
    when '-h', '--help'
      puts 'Usage: ruby anagram.rb [easy|medium|hard] [-s] [-r]'
      return
    end
  end
  if reset_stats
    f = File.join(Dir.home, '.anagram_stats.json')
    File.delete(f) if File.exist?(f)
    puts 'Статистика сброшена.'
    return
  end
  if show_stats
    f = File.join(Dir.home, '.anagram_stats.json')
    if File.exist?(f)
      stats = JSON.parse(File.read(f))
      puts colorize('📊 Статистика:', :bold)
      puts "  Сыграно игр: #{stats['games']}"
      puts "  Лучший счёт: #{stats['best_score']}"
      puts "  Всего угадано слов: #{stats['total_words']}"
    else
      puts 'Статистика пуста.'
    end
    return
  end
  game = AnagramGame.new(level)
  game.play
end

main if __FILE__ == $0
