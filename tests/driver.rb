#!/usr/bin/env ruby

# TODO:
# use optparse for standard input/output/error tests

arguments = ARGV.dup
executable = File.join(File.dirname(__FILE__), arguments.shift)
unless File.executable?(executable)
  puts "Not executable #{executable}"
  exit 1
end

safeexec = File.join(File.dirname(__FILE__), '../safeexec')
unless File.executable?(safeexec)
  puts "Not executable #{safeexec}"
  exit 2
end

out = File.join(File.dirname(__FILE__), 'usage.out')
File.unlink(out) if File.exist?(out)
params = arguments.shift
system "#{safeexec} #{params} --usage #{out} --exec #{executable}> /dev/null"
usage = File.read(out).split("\n")

arguments.each do |expr|
  line = usage.shift
  op, value, regex = nil, nil, nil

  if match = expr.match(/(<|>|=)([\.0-9]+)/)
    regex = Regexp.new(expr.gsub(/(<|>|=)([\.0-9]+)/, '([\.0-9]+)'))
    op, value = match[1], Float(match[2])
  elsif expr.index('?')
    regex = Regexp.new(expr.gsub('?', '([\.0-9]+)'))
  else
    regex = Regexp.new(expr)
  end

  if match = line.match(regex)
    satisfy = case op
              when '<'
                Float(match[1]) <= value
              when '>'
                Float(match[1]) >= value
              when '='
                Float(match[1]) == value
              else
                true
              end
    unless satisfy
      puts "  expected: #{expr}"
      puts "       got: #{line}"
      exit 3
    end
  else
    puts "  expected: #{expr}"
    puts "       got: #{line}"
    exit 3
  end
end

exit 0
