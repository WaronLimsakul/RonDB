describe 'b-tree db' do
  before do
    `rm test.db`
  end

  def run_script(commands)
    raw_output = nil
    IO.popen("./RonDB test.db", "r+") do |pipe|
      commands.each do |command|
        begin
          pipe.puts command
        rescue Errno::EPIPE
          break
        end
      end

      pipe.close_write

      raw_output = pipe.gets(nil)

    end
    raw_output.split("\n")
  end

  it 'can print constants' do
    scripts=  [
      ".constants",
      ".exit",
    ]
    result = run_script(scripts)

    expect(result).to match_array([
      "RonDB >Constants:",
      "ROW_SIZE: 293",
      "COMMON_NODE_HEADER_SIZE: 6",
      "LEAF_NODE_HEADER_SIZE: 14",
      "LEAF_NODE_CELL_SIZE: 297",
      "LEAF_NODE_SPACE_FOR_CELLS: 4082",
      "LEAF_NODE_MAX_CELLS: 13",
      "RonDB >exiting! Bye bye",
    ])
  end

  it 'can print out the structure of one-node b-tree' do
    script = [3, 1, 2].map do |i|
    "insert #{i} ron#{i} ron#{i}@test.com"
    end

    script << ".btree"
    script << ".exit"

    result = run_script(script)
    expect(result).to match_array([
      "RonDB >insert 1",
      "RonDB >insert 1",
      "RonDB >insert 1",
      "RonDB >Tree:",
      "- leaf (size 3)",
      "  - 1",
      "  - 2",
      "  - 3",
      "RonDB >exiting! Bye bye"
    ])
  end

  it 'does not allow inserting duplicate key' do
    script = [
      "insert 1 ron ron@test.com",
      "insert 1 ron ron@test.com",
      "select",
      ".exit"
    ]

    result = run_script(script)
    expect(result).to match_array([
      "RonDB >insert 1",
      "RonDB >error: duplicate key",
      "RonDB >id: 1 | name: ron | email: ron@test.com",
      "RonDB >exiting! Bye bye"
    ])
  end

  it 'allows printing out the structure of 3-nodes btree' do
    script = (1..14).map do |i|
      "insert #{i} ron#{i} ron#{i}@test.com"
    end
    script << ".btree"
    script << "insert 15 ron15 ron15@test.com"
    script << ".exit"
    result = run_script(script)

    expect(result[14...(result.length)]).to match_array([
      "RonDB >Tree:",
      "- internal (size 1)",
      "  - leaf (size 7)",
      "    - 1",
      "    - 2",
      "    - 3",
      "    - 4",
      "    - 5",
      "    - 6",
      "    - 7",
      "  - key 7",
      "  - leaf (size 7)",
      "    - 8",
      "    - 9",
      "    - 10",
      "    - 11",
      "    - 12",
      "    - 13",
      "    - 14",
      "RonDB >insert 1",
      "RonDB >exiting! Bye bye",
    ])
  end

  it 'prints all rows in multi-level tree' do
    script = (1..15).map do |i|
      "insert #{i} ron#{i} ron#{i}@test.com"
    end
    script << "select"
    script << ".exit"

    result = run_script(script)

    expect_result = ["RonDB >id: 1 | name: ron1 | email: ron1@test.com"]
    (2..15).each do |i|
      expect_result << "id: #{i} | name: ron#{i} | email: ron#{i}@test.com"
    end
    expect_result << "RonDB >exiting! Bye bye"

    expect(result[15..result.length]).to match_array(expect_result)
  end

  it 'prints structure of 4-node (1 root + 3 leafs) tree' do
    script = (1..30).map do |i|
      "insert #{i} ron#{i} ron#{i}@tes.com"
    end
    script.shuffle!()
    script << ".btree"
    script << ".exit"

    result = run_script(script)
    expect_result = [
      "RonDB >- internal (size 3)",
      "  - leaf (size 7)",
      "    - 1", "    - 2", "    - 3", "    - 4",
      "    - 5", "    - 6", "    - 7",
      "  - key 1",
      "  - leaf (size 8)",
      "    - 8", "    - 9", "    - 10", "    - 11",
      "    - 12", "    - 13", "    - 14", "    - 15",
      "  - key 15",
      "  - leaf (size 7)",
      "    - 16", "    - 17", "    - 18", "    - 19",
      "    - 20", "    - 21", "    - 22", "  - key 22",
      "  - leaf (size 8)",
      "    - 23", "    - 24", "    - 25", "    - 26",
      "    - 27", "    - 28", "    - 29", "    - 30",
      "RonDB >exiting! Bye bye",
    ]

    expect(result.last(2)).to match_array(expect_result)
  end
end
