target/debug/find-dups: src/main.rs
	cargo build

run: src/main.rs
	cargo run

dups: dups.c
	gcc -g -o dups dups.c -lssl -lcrypto

install: dups target/debug/find-dups
	cp dups ~/.local/bin/dups
	cp target/debug/find-dups ~/.local/bin/find-dups

uninstall:
	rm ~/.local/bin/dups
	rm ~/.local/bin/find-dups
