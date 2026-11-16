require 'csv'
require 'bigdecimal'
require 'fileutils'

layout_dir = __dir__
source_dir = File.expand_path('../../ct_follower_png/images', layout_dir)
output_dir = File.join(layout_dir, 'data')
FileUtils.mkdir_p(output_dir)
signals = %w[vx vy vz d_yaw_ ex ey detect_w]

[3, 4, 5].each do |case_number|
  source = File.join(source_dir, "case#{case_number}_staionary_target_vpng_intercept__kz_2_stats.csv")
  destination = File.join(output_dir, "case#{case_number}_vpng_intercept_stats.csv")
  rows = CSV.read(source, headers: true, skip_blanks: true, skip_lines: /^\s*#/)
  required = ['time_s'] + signals
  missing = required - rows.headers
  raise "#{source}: missing columns #{missing.join(', ')}" unless missing.empty?

  selected = rows.reject { |row| Float(row['detect_w']) == -1 }
  raise "#{source}: no valid detections" if selected.empty?
  origin = BigDecimal(selected.first['time_s'])
  previous_time = nil
  CSV.open(destination, 'w') do |output|
    output << ['time_s', 'elapsed_s'] + signals.map { |signal| "case#{case_number}/#{signal}" }
    selected.each do |row|
      timestamp = BigDecimal(row['time_s'])
      raise "#{source}: non-finite timestamp" unless timestamp.finite?
      raise "#{source}: timestamps not increasing" if previous_time && timestamp <= previous_time
      signals.each do |signal|
        raise "#{source}: non-finite #{signal}" unless Float(row[signal]).finite?
      end
      output << [row['time_s'], (timestamp - origin).to_s('F')] + signals.map { |signal| row[signal] }
      previous_time = timestamp
    end
  end
  duration = (previous_time - origin).to_s('F')
  puts "Case #{case_number}: #{selected.size}/#{rows.size} rows, elapsed 0..#{duration} s -> #{destination}"
  signals.first(6).each do |signal|
    values = selected.map { |row| Float(row[signal]) }
    puts "  #{signal}: #{values.min}..#{values.max}"
  end
end