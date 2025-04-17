.PHONY: run, test

run:
	gcc -Wall -g main.c -o RonDB && ./RonDB mydb.db

test:
	gcc -Wall -g main.c -o RonDB && bundle exec rspec
