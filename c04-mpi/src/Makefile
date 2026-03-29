CC      = mpicc
CFLAGS  = -fopenmp -O2 -Wall
LDFLAGS = -lssl -lcrypto

all: aes_enc

aes_enc: aes_enc.c
	$(CC) $(CFLAGS) -o aes_enc aes_enc.c $(LDFLAGS)

clean:
	rm -f aes_enc
