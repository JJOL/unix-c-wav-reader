CC := gcc
build:
	$(CC) wav_reader.c -o reader
clean:
	rm reader