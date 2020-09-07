CC := gcc
CFLAGS := -O0
.PHONY : build clean test
# Modules
reader:
	$(CC) $(CFLAGS) wav_reader.c -o $@
# Help Phase Commands
build: clean reader
clean:
	rm reader
test: reader
	./reader audio2.wav