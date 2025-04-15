describe persist_db do
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

  it 'keeps data after closing connection' do
    result1 = run_script([
      "insert 1 ron ron@email.com",
      ".exit",
    ])

    expect(result1).to match_array([
      "RonDB >insert 1",
      "RonDB >exiting! Bye bye",
    ])

    result2 = run_script([
      "select",
      ".exit",
    ])

    expect(result2).to match_array([
      "RonDB >id: 1 | name: ron | email: ron@email.com",
      "RonDB >exiting! Bye bye",
    ])

  end
end
