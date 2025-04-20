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
    script = [
      "insert 18 user18 person18@example.com",
      "insert 7 user7 person7@example.com",
      "insert 10 user10 person10@example.com",
      "insert 29 user29 person29@example.com",
      "insert 23 user23 person23@example.com",
      "insert 4 user4 person4@example.com",
      "insert 14 user14 person14@example.com",
      "insert 30 user30 person30@example.com",
      "insert 15 user15 person15@example.com",
      "insert 26 user26 person26@example.com",
      "insert 22 user22 person22@example.com",
      "insert 19 user19 person19@example.com",
      "insert 2 user2 person2@example.com",
      "insert 1 user1 person1@example.com",
      "insert 21 user21 person21@example.com",
      "insert 11 user11 person11@example.com",
      "insert 6 user6 person6@example.com",
      "insert 20 user20 person20@example.com",
      "insert 5 user5 person5@example.com",
      "insert 8 user8 person8@example.com",
      "insert 9 user9 person9@example.com",
      "insert 3 user3 person3@example.com",
      "insert 12 user12 person12@example.com",
      "insert 27 user27 person27@example.com",
      "insert 17 user17 person17@example.com",
      "insert 16 user16 person16@example.com",
      "insert 13 user13 person13@example.com",
      "insert 24 user24 person24@example.com",
      "insert 25 user25 person25@example.com",
      "insert 28 user28 person28@example.com",
      ".btree",
      ".exit",
    ]

    result = run_script(script)
    expect_result = [
      "RonDB >Tree:",
      "- internal (size 3)",
      "  - leaf (size 7)",
      "    - 1", "    - 2", "    - 3", "    - 4",
      "    - 5", "    - 6", "    - 7",
      "  - key 7",
      "  - leaf (size 8)",
      "    - 8", "    - 9", "    - 10", "    - 11",
      "    - 12", "    - 13", "    - 14", "    - 15",
      "  - key 15",
      "  - leaf (size 7)",
      "    - 16", "    - 17", "    - 18", "    - 19",
      "    - 20", "    - 21", "    - 22",
      "  - key 22",
      "  - leaf (size 8)",
      "    - 23", "    - 24", "    - 25", "    - 26",
      "    - 27", "    - 28", "    - 29", "    - 30",
      "RonDB >exiting! Bye bye",
    ]

    expect(result.last(40)).to match_array(expect_result)
  end
end
