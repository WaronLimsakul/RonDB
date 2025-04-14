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
      "insert 1 ron1 ron@gmail.com",
      "select",
      ".exit",
    ])
    expect(result).to match_array([
      "insert 1",
      "id: 1 | name: ron1 | email: ron@gmail.com",
      "exiting! Bye bye",
    ])
  end
end
