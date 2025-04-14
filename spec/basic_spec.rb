describe 'database' do
  def run_script(commands)
    raw_output = nil
    IO.popen("./RonDB", "r+") do |pipe|
      commands.each do |command|
        pipe.puts command
      end

      pipe.close_write

      raw_output = pipe.gets(nil)

    end
    raw_output.split("\n")
  end

  it 'inserts and retrieves a row' do
    result = run_script([
      "insert 1 ron1 ron@test.com",
      "select",
      ".exit",
    ])
    expect(result).to match_array([
      "RonDB >insert 1",
      "RonDB >id: 1 | name: ron1 | email: ron@test.com",
      "RonDB >exiting! Bye bye",
    ])
  end

  it 'prints error message when table is full' do
    script = (1..1487).map do |i|
      "insert #{i} user#{i} person#{i}@test.com"
    end
    script << ".exit"
    result = run_script(script)
    expect(result[-2]).to eq('RonDB >error: table is full')
  end

  it 'allows inserting maximum length strings' do
    long_name = "a"*32
    long_email = "b"*255
    script = [
      "insert 1 #{long_name} #{long_email}",
      "select",
      ".exit",
    ]

    result = run_script(script)
    expect(result).to match_array([
      "RonDB >insert 1",
      "RonDB >id: 1 | name: #{long_name} | email: #{long_email}",
      "RonDB >exiting! Bye bye",
    ])
  end

end
