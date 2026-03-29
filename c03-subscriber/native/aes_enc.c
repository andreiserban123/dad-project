/**
 * aes_enc.c  —  OpenMPI + OpenMP parallel AES-256-CBC encryption/decryption
 *
 * Usage:
 *   mpirun -np 2 --host c03-subscriber,c04-mpi-worker \
 *          ./aes_enc <encrypt|decrypt> <mode> <key_hex> <input.bmp> <output.bmp>
 *
 * Build:
 *   mpicc -fopenmp -o aes_enc aes_enc.c -lssl -lcrypto -O2
 *
 * Architecture:
 *   - MPI rank 0: reads BMP, scatters scanline chunks to all ranks,
 *                 gathers results, writes output BMP.
 *   - All ranks (including 0): use OpenMP to parallelize AES over their chunk.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define BMP_HEADER_SIZE 54
#define AES_KEY_BYTES   32   /* AES-256 */
#define AES_BLOCK_SIZE  16
#define IV_SIZE         16

/* ── Helpers ──────────────────────────────────────────────── */

static void hex_to_bytes(const char *hex, unsigned char *out, int len) {
    for (int i = 0; i < len; i++)
        sscanf(hex + i*2, "%2hhx", &out[i]);
}

static void die(const char *msg) {
    fprintf(stderr, "[aes_enc] ERROR: %s\n", msg);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

/* AES-256-CBC encrypt/decrypt one block of data.
 * out_len must be >= in_len + AES_BLOCK_SIZE.
 * Returns number of bytes written. */
static int aes_cbc(int encrypt,
                   const unsigned char *key, const unsigned char *iv,
                   const unsigned char *in, int in_len,
                   unsigned char *out) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    const EVP_CIPHER *cipher = EVP_aes_256_cbc();
    EVP_CipherInit_ex(ctx, cipher, NULL, key, iv, encrypt);
    EVP_CIPHER_CTX_set_padding(ctx, 1);  /* PKCS7 */

    int out_len = 0, final_len = 0;
    EVP_CipherUpdate(ctx, out, &out_len, in, in_len);
    EVP_CipherFinal_ex(ctx, out + out_len, &final_len);
    EVP_CIPHER_CTX_free(ctx);
    return out_len + final_len;
}

/* AES-256-ECB encrypt/decrypt one 16-byte block (no padding, no IV).
 * Used for ECB mode over scanlines. */
static void aes_ecb_block(int encrypt,
                           const unsigned char *key,
                           const unsigned char *in,
                           unsigned char *out) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CipherInit_ex(ctx, EVP_aes_256_ecb(), NULL, key, NULL, encrypt);
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    int len = 0, flen = 0;
    EVP_CipherUpdate(ctx, out, &len, in, AES_BLOCK_SIZE);
    EVP_CipherFinal_ex(ctx, out + len, &flen);
    EVP_CIPHER_CTX_free(ctx);
}

/* ── Main ─────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc < 6) {
        if (rank == 0)
            fprintf(stderr, "Usage: aes_enc <encrypt|decrypt> <CBC|ECB> <key_hex64> <in.bmp> <out.bmp>\n");
        MPI_Finalize();
        return 1;
    }

    int  encrypt   = (strcmp(argv[1], "encrypt") == 0) ? 1 : 0;
    int  use_ecb   = (strcmp(argv[2], "ECB")     == 0) ? 1 : 0;
    char *key_hex  = argv[3];
    char *in_path  = argv[4];
    char *out_path = argv[5];

    unsigned char key[AES_KEY_BYTES];
    hex_to_bytes(key_hex, key, AES_KEY_BYTES);

    /* ── Rank 0: read BMP ── */
    unsigned char *full_data  = NULL;
    unsigned char  header[BMP_HEADER_SIZE];
    int            pixel_size = 0;   /* bytes in pixel data only */
    int            width = 0, height = 0, bpp = 0, row_size = 0;

    if (rank == 0) {
        FILE *f = fopen(in_path, "rb");
        if (!f) die("Cannot open input BMP");

        if (fread(header, 1, BMP_HEADER_SIZE, f) != BMP_HEADER_SIZE)
            die("Failed to read BMP header");

        /* Parse BMP header fields (little-endian) */
        int data_offset = *(int *)(header + 10);
        width    = *(int *)(header + 18);
        height   = abs(*(int *)(header + 22));
        bpp      = *(short *)(header + 28);
        row_size = ((width * (bpp / 8) + 3) / 4) * 4;  /* 4-byte aligned */
        pixel_size = row_size * height;

        full_data = (unsigned char *)malloc(pixel_size);
        if (!full_data) die("malloc failed");

        fseek(f, data_offset, SEEK_SET);
        if ((int)fread(full_data, 1, pixel_size, f) != pixel_size)
            die("Failed to read pixel data");
        fclose(f);

        printf("[rank 0] BMP %dx%d bpp=%d pixel_bytes=%d\n",
               width, height, bpp, pixel_size);
    }

    /* ── Broadcast dimensions ── */
    int dims[4] = { pixel_size, width, height, row_size };
    MPI_Bcast(dims, 4, MPI_INT, 0, MPI_COMM_WORLD);
    pixel_size = dims[0]; width = dims[1]; height = dims[2]; row_size = dims[3];

    /* ── Scatter pixel data in row chunks ── */
    int base_rows  = height / nprocs;
    int extra_rows = height % nprocs;

    /* Build sendcounts and displs for MPI_Scatterv */
    int *sendcounts = malloc(nprocs * sizeof(int));
    int *displs     = malloc(nprocs * sizeof(int));
    int offset = 0;
    for (int i = 0; i < nprocs; i++) {
        int rows = base_rows + (i < extra_rows ? 1 : 0);
        sendcounts[i] = rows * row_size;
        displs[i]     = offset;
        offset       += sendcounts[i];
    }

    int my_rows = base_rows + (rank < extra_rows ? 1 : 0);
    int my_size = my_rows * row_size;

    unsigned char *my_chunk = malloc(my_size + AES_BLOCK_SIZE); /* +16 for padding */
    if (!my_chunk) die("malloc chunk");

    MPI_Scatterv(full_data, sendcounts, displs, MPI_BYTE,
                 my_chunk, my_size, MPI_BYTE,
                 0, MPI_COMM_WORLD);

    /* ── OpenMP parallel AES over my chunk ── */
    unsigned char *out_chunk = malloc(my_size + AES_BLOCK_SIZE);
    if (!out_chunk) die("malloc out_chunk");

    /* Generate / embed IV per row for CBC; ECB needs no IV */
    unsigned char iv[IV_SIZE];
    if (encrypt)
        RAND_bytes(iv, IV_SIZE);  /* random IV on encrypt */
    else
        memcpy(iv, my_chunk, IV_SIZE); /* first 16 bytes = IV on decrypt */

    if (use_ecb) {
        /* ECB: process each 16-byte block independently — parallelisable */
        int nblocks = my_size / AES_BLOCK_SIZE;
        #pragma omp parallel for schedule(static)
        for (int b = 0; b < nblocks; b++) {
            aes_ecb_block(encrypt, key,
                          my_chunk  + b * AES_BLOCK_SIZE,
                          out_chunk + b * AES_BLOCK_SIZE);
        }
    } else {
        /* CBC: parallelise over scanlines (each row uses its own IV = last block of previous) */
        #pragma omp parallel for schedule(static)
        for (int r = 0; r < my_rows; r++) {
            unsigned char row_iv[IV_SIZE];
            /* For simplicity use shared IV per-rank; production code chains IVs */
            memcpy(row_iv, iv, IV_SIZE);
            aes_cbc(encrypt, key, row_iv,
                    my_chunk  + r * row_size, row_size,
                    out_chunk + r * row_size);
        }
    }

    /* ── Gather results at rank 0 ── */
    unsigned char *result_data = NULL;
    if (rank == 0) {
        result_data = malloc(pixel_size + nprocs * AES_BLOCK_SIZE);
        if (!result_data) die("malloc result");
    }

    MPI_Gatherv(out_chunk, my_size, MPI_BYTE,
                result_data, sendcounts, displs, MPI_BYTE,
                0, MPI_COMM_WORLD);

    /* ── Rank 0: write output BMP ── */
    if (rank == 0) {
        FILE *out = fopen(out_path, "wb");
        if (!out) die("Cannot open output BMP");
        fwrite(header, 1, BMP_HEADER_SIZE, out);
        fwrite(result_data, 1, pixel_size, out);
        fclose(out);
        printf("[rank 0] Written to %s\n", out_path);
        free(full_data);
        free(result_data);
    }

    free(my_chunk);
    free(out_chunk);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}
