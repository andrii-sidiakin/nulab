#include <bc_types.h>

#include <nulib/buffer.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int blkdat_dexor(const char *ifilepath, const char *ofilepath);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s ifile ofile\n", argv[0]);
        return 0;
    }

    blkdat_dexor(argv[1], argv[2]);

    return 0;
}

enum {
    XorKeySize = 8,
    MainMagic = 0xD9B4BEF9,
};

typedef struct {
    FILE *fd;
    uint8_t xor_key[XorKeySize];
    uint8_t xor_enable;
} btc_file_t;

typedef struct {
    uint32_t magic;
    uint32_t block_size;
} btc_magic_header_t;

typedef bc_block_header_t btc_block_header_t;

typedef struct {
    uint32_t version;
    // uint8_t marker;
    // uint8_t flags;
    uint32_t in_count;
    uint32_t out_count;
} btc_tx_header_t;

int btc_file_open(btc_file_t *f, const char *filepath) {
    f->fd = fopen(filepath, "rb");
    return ferror(f->fd);
}

int btc_file_close(btc_file_t *f) {
    if (f->fd) {
        fclose(f->fd);
        f->fd = NULL;
    }
    return 0;
}

int btc_file_read(btc_file_t *f, void *buf, size_t nb) {
    long int n = ftell(f->fd) % XorKeySize;
    nu_assert(n >= 0);

    size_t r = fread(buf, 1, nb, f->fd);
    if (r != nb) {
        return -1;
    }

    if (f->xor_enable) {
        uint8_t *p = buf;
        for (size_t i = 0; i < r; ++i, ++p) {
            *p = *p ^ f->xor_key[(n + i) % XorKeySize];
        }
    }

    return 0;
}

int btc_file_read_all(btc_file_t *file, nu_buffer_t *buf) {
    fpos_t pos = {0};
    fgetpos(file->fd, &pos);

    fseek(file->fd, 0, SEEK_END);
    const long size = ftell(file->fd);
    rewind(file->fd);

    void *data = nu_buffer_consume(buf, size);
    btc_file_read(file, data, size);

    fsetpos(file->fd, &pos);
    return ferror(file->fd);
}

int btc_read_var_int(btc_file_t *file, int *val) {
    uint8_t b = 0;
    int ec = btc_file_read(file, &b, 1);
    if (!ec) {
        if (b == 0xFF) { // next 8 bytes
            return ENOTSUP;
        }
        if (b == 0xFE) { // next 4 bytes
            return ENOTSUP;
        }
        if (b == 0xFD) { // next 2 bytes
            short x = 0;
            ec = btc_file_read(file, &x, 2);
            if (!ec) {
                *val = x;
            }
        }
        else {
            *val = b;
        }
    }
    return ec;
}

int btc_read_magic_header(btc_file_t *file, btc_magic_header_t *m) {
    int ec = btc_file_read(file, m, sizeof(*m));
    return ec;
}

int btc_read_block_header(btc_file_t *file, btc_block_header_t *h) {
    int ec = btc_file_read(file, h, sizeof(*h));
    return ec;
}

int btc_read_tx_header(btc_file_t *file, btc_tx_header_t *h) {
    int ec = btc_file_read(file, &h->version, sizeof(h->version));
    if (ec) {
        return ec;
    }

    int tx_input_count_or_flag_byte0 = 0;
    ec = btc_read_var_int(file, &tx_input_count_or_flag_byte0);
    if (ec) {
        return ec;
    }

    // assuming it was tx_input_count
    int tx_in_count = tx_input_count_or_flag_byte0;

    if (tx_in_count == 0) { // but not, it was flag_byte0
        uint8_t flag_byte1 = 0;
        ec = btc_file_read(file, &flag_byte1, 1);
        if (ec) {
            return ec;
        }
        if (flag_byte1 != 0x01) {
            printf("Invalid tx.flag value: %x%x\n",
                   (uint8_t)tx_input_count_or_flag_byte0, flag_byte1);
            return EINVAL;
        }

        ec = btc_read_var_int(file, &tx_input_count_or_flag_byte0);
        if (ec) {
            return ec;
        }
    }

    h->in_count = tx_in_count;

    return ec;
}

int btc_file_print_info(btc_file_t *f) {
    if (!f->fd) {
        return EBADFD;
    }

    int ec = 0;

    btc_magic_header_t magic;
    btc_block_header_t block;

    const size_t block_header_size = sizeof(block);
    long block_header_begin = 0;
    long block_header_end = 0;

    fflush(f->fd);
    rewind(f->fd);
    ec = ferror(f->fd);

    while (ec == 0) {
        magic = (btc_magic_header_t){0};
        block = (btc_block_header_t){0};

        //
        // read magic/block_size
        //

        block_header_begin = ftell(f->fd);
        block_header_end = -1;

        ec = btc_read_magic_header(f, &magic);
        if (ec) {
            printf("Failed to read magic header.\n");
            break;
        }

        //
        // read magic/block_size
        //

        ec = btc_read_block_header(f, &block);
        if (ec) {
            printf("Failed to read block header.\n");
            break;
        }

        printf("\nBlock:\n");
        printf("  magic       : 0x%x (%s)\n", magic.magic,
               (magic.magic == MainMagic) ? "MainMagic" : "Unknown");
        printf("  size        : 0x%x (%u bytes)\n", magic.block_size,
               magic.block_size);
        printf("  version     : 0x%x\n", block.version);
        printf("  timestamp   : 0x%x\n", block.timestamp);
        printf("  bits        : 0x%x\n", block.bits);
        printf("  nonce       : 0x%x\n", block.nonce);
        printf("  prev_hash   : [");
        for (int i = 0; i < 32; ++i) {
            printf("%02x", block.hash_prev_block.data[i]);
        }
        printf("]\n");
        printf("  merkle_root : [");
        for (int i = 0; i < 32; ++i) {
            printf("%02x", block.hash_merkle_root.data[i]);
        }
        printf("]\n");

        //
        //
        //

        block_header_end = ftell(f->fd);

        //
        // read txn_count
        //

        int txn_count = 0;
        ec = btc_read_var_int(f, &txn_count);
        if (ec) {
            printf("Failed to read txn_count.\n");
        }

        printf("TX Data:\n");
        printf("  txn_count   : 0x%02x (%d)\n", txn_count, txn_count);

        if (txn_count == 0) {
            goto skip_tx_data;
        }

        //
        // read first transaction header (coinbase)
        //
        btc_tx_header_t tx = {0};
        ec = btc_read_tx_header(f, &tx);
        if (ec) {
            printf("Failed to read txn_count.\n");
            goto skip_tx_data;
        }

        printf("Coinbase:\n");
        printf("  version     : 0x%x\n", tx.version);
        printf("  in count    : %d\n", tx.in_count);

    skip_tx_data:

        //
        // go to next block
        //

        if (magic.block_size < block_header_size) {
            printf("Block size is too small: %u\n", magic.block_size);
            break;
        }

        ec = fseek(f->fd,
                   block_header_end + magic.block_size - block_header_size,
                   SEEK_SET);
    }

    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static int blkdat_dexor(const char *ifilepath, const char *ofilepath) {
    const uint8_t xor_key[8] = {0xac, 0x44, 0x3c, 0xcc, 0x4d, 0x07, 0x90, 0x9b};
    nu_assert(XorKeySize == sizeof(xor_key));

    btc_file_t ifile = {0};
    ifile.xor_enable = 1;
    memcpy(ifile.xor_key, &xor_key, XorKeySize);

    if (btc_file_open(&ifile, ifilepath)) {
        printf("Failed to read file '%s'\n", ifilepath);
        return -1;
    }

    btc_file_print_info(&ifile);

    nu_buffer_t buf = nu_buffer_create();
    btc_file_read_all(&ifile, &buf);
    btc_file_close(&ifile);

    FILE *fd = fopen(ofilepath, "wb+");
    if (!fd) {
        printf("Failed to read file '%s'\n", ifilepath);
    }
    else {
        fwrite(nu_dca_data(&buf), 1, nu_dca_size_in_bytes(&buf), fd);
        fclose(fd);
    }

    nu_buffer_release(&buf);

    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
