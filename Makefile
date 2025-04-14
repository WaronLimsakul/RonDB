.PHONY: run, test

run:
	gcc -Wall -g main.c -o RonDB && ./RonDB

test:
	gcc -Wall -g main.c -o RonDB && bundle exec rspec
