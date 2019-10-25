
//#cgo CFLAGS: -std=gnu99
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "shabal/sph_shabal.h"

#define sM    16


#define DECL_STATE   \
	sph_u32 A00, A01, A02, A03, A04, A05, A06, A07, \
	        A08, A09, A0A, A0B; \
	sph_u32 B0, B1, B2, B3, B4, B5, B6, B7, \
	        B8, B9, BA, BB, BC, BD, BE, BF; \
	sph_u32 C0, C1, C2, C3, C4, C5, C6, C7, \
	        C8, C9, CA, CB, CC, CD, CE, CF; \
	sph_u32 M0, M1, M2, M3, M4, M5, M6, M7, \
	        M8, M9, MA, MB, MC, MD, ME, MF; \
	sph_u32 Wlow, Whigh;

#define READ_STATE(state)   do { \
		A00 = (state)->A[0]; \
		A01 = (state)->A[1]; \
		A02 = (state)->A[2]; \
		A03 = (state)->A[3]; \
		A04 = (state)->A[4]; \
		A05 = (state)->A[5]; \
		A06 = (state)->A[6]; \
		A07 = (state)->A[7]; \
		A08 = (state)->A[8]; \
		A09 = (state)->A[9]; \
		A0A = (state)->A[10]; \
		A0B = (state)->A[11]; \
		B0 = (state)->B[0]; \
		B1 = (state)->B[1]; \
		B2 = (state)->B[2]; \
		B3 = (state)->B[3]; \
		B4 = (state)->B[4]; \
		B5 = (state)->B[5]; \
		B6 = (state)->B[6]; \
		B7 = (state)->B[7]; \
		B8 = (state)->B[8]; \
		B9 = (state)->B[9]; \
		BA = (state)->B[10]; \
		BB = (state)->B[11]; \
		BC = (state)->B[12]; \
		BD = (state)->B[13]; \
		BE = (state)->B[14]; \
		BF = (state)->B[15]; \
		C0 = (state)->C[0]; \
		C1 = (state)->C[1]; \
		C2 = (state)->C[2]; \
		C3 = (state)->C[3]; \
		C4 = (state)->C[4]; \
		C5 = (state)->C[5]; \
		C6 = (state)->C[6]; \
		C7 = (state)->C[7]; \
		C8 = (state)->C[8]; \
		C9 = (state)->C[9]; \
		CA = (state)->C[10]; \
		CB = (state)->C[11]; \
		CC = (state)->C[12]; \
		CD = (state)->C[13]; \
		CE = (state)->C[14]; \
		CF = (state)->C[15]; \
		Wlow = (state)->Wlow; \
		Whigh = (state)->Whigh; \
	} while (0)

#define WRITE_STATE(state)   do { \
		(state)->A[0] = A00; \
		(state)->A[1] = A01; \
		(state)->A[2] = A02; \
		(state)->A[3] = A03; \
		(state)->A[4] = A04; \
		(state)->A[5] = A05; \
		(state)->A[6] = A06; \
		(state)->A[7] = A07; \
		(state)->A[8] = A08; \
		(state)->A[9] = A09; \
		(state)->A[10] = A0A; \
		(state)->A[11] = A0B; \
		(state)->B[0] = B0; \
		(state)->B[1] = B1; \
		(state)->B[2] = B2; \
		(state)->B[3] = B3; \
		(state)->B[4] = B4; \
		(state)->B[5] = B5; \
		(state)->B[6] = B6; \
		(state)->B[7] = B7; \
		(state)->B[8] = B8; \
		(state)->B[9] = B9; \
		(state)->B[10] = BA; \
		(state)->B[11] = BB; \
		(state)->B[12] = BC; \
		(state)->B[13] = BD; \
		(state)->B[14] = BE; \
		(state)->B[15] = BF; \
		(state)->C[0] = C0; \
		(state)->C[1] = C1; \
		(state)->C[2] = C2; \
		(state)->C[3] = C3; \
		(state)->C[4] = C4; \
		(state)->C[5] = C5; \
		(state)->C[6] = C6; \
		(state)->C[7] = C7; \
		(state)->C[8] = C8; \
		(state)->C[9] = C9; \
		(state)->C[10] = CA; \
		(state)->C[11] = CB; \
		(state)->C[12] = CC; \
		(state)->C[13] = CD; \
		(state)->C[14] = CE; \
		(state)->C[15] = CF; \
		(state)->Wlow = Wlow; \
		(state)->Whigh = Whigh; \
	} while (0)

#define DECODE_BLOCK   do { \
		M0 = sph_dec32le_aligned(buf + 0); \
		M1 = sph_dec32le_aligned(buf + 4); \
		M2 = sph_dec32le_aligned(buf + 8); \
		M3 = sph_dec32le_aligned(buf + 12); \
		M4 = sph_dec32le_aligned(buf + 16); \
		M5 = sph_dec32le_aligned(buf + 20); \
		M6 = sph_dec32le_aligned(buf + 24); \
		M7 = sph_dec32le_aligned(buf + 28); \
		M8 = sph_dec32le_aligned(buf + 32); \
		M9 = sph_dec32le_aligned(buf + 36); \
		MA = sph_dec32le_aligned(buf + 40); \
		MB = sph_dec32le_aligned(buf + 44); \
		MC = sph_dec32le_aligned(buf + 48); \
		MD = sph_dec32le_aligned(buf + 52); \
		ME = sph_dec32le_aligned(buf + 56); \
		MF = sph_dec32le_aligned(buf + 60); \
	} while (0)

#define INPUT_BLOCK_ADD   do { \
		B0 = SPH_T32(B0 + M0); \
		B1 = SPH_T32(B1 + M1); \
		B2 = SPH_T32(B2 + M2); \
		B3 = SPH_T32(B3 + M3); \
		B4 = SPH_T32(B4 + M4); \
		B5 = SPH_T32(B5 + M5); \
		B6 = SPH_T32(B6 + M6); \
		B7 = SPH_T32(B7 + M7); \
		B8 = SPH_T32(B8 + M8); \
		B9 = SPH_T32(B9 + M9); \
		BA = SPH_T32(BA + MA); \
		BB = SPH_T32(BB + MB); \
		BC = SPH_T32(BC + MC); \
		BD = SPH_T32(BD + MD); \
		BE = SPH_T32(BE + ME); \
		BF = SPH_T32(BF + MF); \
	} while (0)

#define INPUT_BLOCK_SUB   do { \
		C0 = SPH_T32(C0 - M0); \
		C1 = SPH_T32(C1 - M1); \
		C2 = SPH_T32(C2 - M2); \
		C3 = SPH_T32(C3 - M3); \
		C4 = SPH_T32(C4 - M4); \
		C5 = SPH_T32(C5 - M5); \
		C6 = SPH_T32(C6 - M6); \
		C7 = SPH_T32(C7 - M7); \
		C8 = SPH_T32(C8 - M8); \
		C9 = SPH_T32(C9 - M9); \
		CA = SPH_T32(CA - MA); \
		CB = SPH_T32(CB - MB); \
		CC = SPH_T32(CC - MC); \
		CD = SPH_T32(CD - MD); \
		CE = SPH_T32(CE - ME); \
		CF = SPH_T32(CF - MF); \
	} while (0)

#define XOR_W   do { \
		A00 ^= Wlow; \
		A01 ^= Whigh; \
	} while (0)

#define SWAP(v1, v2)   do { \
		sph_u32 tmp = (v1); \
		(v1) = (v2); \
		(v2) = tmp; \
	} while (0)

#define SWAP_BC   do { \
		SWAP(B0, C0); \
		SWAP(B1, C1); \
		SWAP(B2, C2); \
		SWAP(B3, C3); \
		SWAP(B4, C4); \
		SWAP(B5, C5); \
		SWAP(B6, C6); \
		SWAP(B7, C7); \
		SWAP(B8, C8); \
		SWAP(B9, C9); \
		SWAP(BA, CA); \
		SWAP(BB, CB); \
		SWAP(BC, CC); \
		SWAP(BD, CD); \
		SWAP(BE, CE); \
		SWAP(BF, CF); \
	} while (0)

#define PERM_ELT(xa0, xa1, xb0, xb1, xb2, xb3, xc, xm)   do { \
		xa0 = SPH_T32((xa0 \
			^ (((xa1 << 15) | (xa1 >> 17)) * 5U) \
			^ xc) * 3U) \
			^ xb1 ^ (xb2 & ~xb3) ^ xm; \
		xb0 = SPH_T32(~(((xb0 << 1) | (xb0 >> 31)) ^ xa0)); \
	} while (0)

#define PERM_STEP_0   do { \
		PERM_ELT(A00, A0B, B0, BD, B9, B6, C8, M0); \
		PERM_ELT(A01, A00, B1, BE, BA, B7, C7, M1); \
		PERM_ELT(A02, A01, B2, BF, BB, B8, C6, M2); \
		PERM_ELT(A03, A02, B3, B0, BC, B9, C5, M3); \
		PERM_ELT(A04, A03, B4, B1, BD, BA, C4, M4); \
		PERM_ELT(A05, A04, B5, B2, BE, BB, C3, M5); \
		PERM_ELT(A06, A05, B6, B3, BF, BC, C2, M6); \
		PERM_ELT(A07, A06, B7, B4, B0, BD, C1, M7); \
		PERM_ELT(A08, A07, B8, B5, B1, BE, C0, M8); \
		PERM_ELT(A09, A08, B9, B6, B2, BF, CF, M9); \
		PERM_ELT(A0A, A09, BA, B7, B3, B0, CE, MA); \
		PERM_ELT(A0B, A0A, BB, B8, B4, B1, CD, MB); \
		PERM_ELT(A00, A0B, BC, B9, B5, B2, CC, MC); \
		PERM_ELT(A01, A00, BD, BA, B6, B3, CB, MD); \
		PERM_ELT(A02, A01, BE, BB, B7, B4, CA, ME); \
		PERM_ELT(A03, A02, BF, BC, B8, B5, C9, MF); \
	} while (0)

#define PERM_STEP_1   do { \
		PERM_ELT(A04, A03, B0, BD, B9, B6, C8, M0); \
		PERM_ELT(A05, A04, B1, BE, BA, B7, C7, M1); \
		PERM_ELT(A06, A05, B2, BF, BB, B8, C6, M2); \
		PERM_ELT(A07, A06, B3, B0, BC, B9, C5, M3); \
		PERM_ELT(A08, A07, B4, B1, BD, BA, C4, M4); \
		PERM_ELT(A09, A08, B5, B2, BE, BB, C3, M5); \
		PERM_ELT(A0A, A09, B6, B3, BF, BC, C2, M6); \
		PERM_ELT(A0B, A0A, B7, B4, B0, BD, C1, M7); \
		PERM_ELT(A00, A0B, B8, B5, B1, BE, C0, M8); \
		PERM_ELT(A01, A00, B9, B6, B2, BF, CF, M9); \
		PERM_ELT(A02, A01, BA, B7, B3, B0, CE, MA); \
		PERM_ELT(A03, A02, BB, B8, B4, B1, CD, MB); \
		PERM_ELT(A04, A03, BC, B9, B5, B2, CC, MC); \
		PERM_ELT(A05, A04, BD, BA, B6, B3, CB, MD); \
		PERM_ELT(A06, A05, BE, BB, B7, B4, CA, ME); \
		PERM_ELT(A07, A06, BF, BC, B8, B5, C9, MF); \
	} while (0)

#define PERM_STEP_2   do { \
		PERM_ELT(A08, A07, B0, BD, B9, B6, C8, M0); \
		PERM_ELT(A09, A08, B1, BE, BA, B7, C7, M1); \
		PERM_ELT(A0A, A09, B2, BF, BB, B8, C6, M2); \
		PERM_ELT(A0B, A0A, B3, B0, BC, B9, C5, M3); \
		PERM_ELT(A00, A0B, B4, B1, BD, BA, C4, M4); \
		PERM_ELT(A01, A00, B5, B2, BE, BB, C3, M5); \
		PERM_ELT(A02, A01, B6, B3, BF, BC, C2, M6); \
		PERM_ELT(A03, A02, B7, B4, B0, BD, C1, M7); \
		PERM_ELT(A04, A03, B8, B5, B1, BE, C0, M8); \
		PERM_ELT(A05, A04, B9, B6, B2, BF, CF, M9); \
		PERM_ELT(A06, A05, BA, B7, B3, B0, CE, MA); \
		PERM_ELT(A07, A06, BB, B8, B4, B1, CD, MB); \
		PERM_ELT(A08, A07, BC, B9, B5, B2, CC, MC); \
		PERM_ELT(A09, A08, BD, BA, B6, B3, CB, MD); \
		PERM_ELT(A0A, A09, BE, BB, B7, B4, CA, ME); \
		PERM_ELT(A0B, A0A, BF, BC, B8, B5, C9, MF); \
	} while (0)

#define APPLY_P   do { \
		B0 = SPH_T32(B0 << 17) | (B0 >> 15); \
		B1 = SPH_T32(B1 << 17) | (B1 >> 15); \
		B2 = SPH_T32(B2 << 17) | (B2 >> 15); \
		B3 = SPH_T32(B3 << 17) | (B3 >> 15); \
		B4 = SPH_T32(B4 << 17) | (B4 >> 15); \
		B5 = SPH_T32(B5 << 17) | (B5 >> 15); \
		B6 = SPH_T32(B6 << 17) | (B6 >> 15); \
		B7 = SPH_T32(B7 << 17) | (B7 >> 15); \
		B8 = SPH_T32(B8 << 17) | (B8 >> 15); \
		B9 = SPH_T32(B9 << 17) | (B9 >> 15); \
		BA = SPH_T32(BA << 17) | (BA >> 15); \
		BB = SPH_T32(BB << 17) | (BB >> 15); \
		BC = SPH_T32(BC << 17) | (BC >> 15); \
		BD = SPH_T32(BD << 17) | (BD >> 15); \
		BE = SPH_T32(BE << 17) | (BE >> 15); \
		BF = SPH_T32(BF << 17) | (BF >> 15); \
		PERM_STEP_0; \
		PERM_STEP_1; \
		PERM_STEP_2; \
		A0B = SPH_T32(A0B + C6); \
		A0A = SPH_T32(A0A + C5); \
		A09 = SPH_T32(A09 + C4); \
		A08 = SPH_T32(A08 + C3); \
		A07 = SPH_T32(A07 + C2); \
		A06 = SPH_T32(A06 + C1); \
		A05 = SPH_T32(A05 + C0); \
		A04 = SPH_T32(A04 + CF); \
		A03 = SPH_T32(A03 + CE); \
		A02 = SPH_T32(A02 + CD); \
		A01 = SPH_T32(A01 + CC); \
		A00 = SPH_T32(A00 + CB); \
		A0B = SPH_T32(A0B + CA); \
		A0A = SPH_T32(A0A + C9); \
		A09 = SPH_T32(A09 + C8); \
		A08 = SPH_T32(A08 + C7); \
		A07 = SPH_T32(A07 + C6); \
		A06 = SPH_T32(A06 + C5); \
		A05 = SPH_T32(A05 + C4); \
		A04 = SPH_T32(A04 + C3); \
		A03 = SPH_T32(A03 + C2); \
		A02 = SPH_T32(A02 + C1); \
		A01 = SPH_T32(A01 + C0); \
		A00 = SPH_T32(A00 + CF); \
		A0B = SPH_T32(A0B + CE); \
		A0A = SPH_T32(A0A + CD); \
		A09 = SPH_T32(A09 + CC); \
		A08 = SPH_T32(A08 + CB); \
		A07 = SPH_T32(A07 + CA); \
		A06 = SPH_T32(A06 + C9); \
		A05 = SPH_T32(A05 + C8); \
		A04 = SPH_T32(A04 + C7); \
		A03 = SPH_T32(A03 + C6); \
		A02 = SPH_T32(A02 + C5); \
		A01 = SPH_T32(A01 + C4); \
		A00 = SPH_T32(A00 + C3); \
	} while (0)

#define INCR_W   do { \
		if ((Wlow = SPH_T32(Wlow + 1)) == 0) \
			Whigh = SPH_T32(Whigh + 1); \
	} while (0)


static const sph_u32 A_init_256[] = {
	SPH_C32(0x52F84552), SPH_C32(0xE54B7999), SPH_C32(0x2D8EE3EC), SPH_C32(0xB9645191),
	SPH_C32(0xE0078B86), SPH_C32(0xBB7C44C9), SPH_C32(0xD2B5C1CA), SPH_C32(0xB0D2EB8C),
	SPH_C32(0x14CE5A45), SPH_C32(0x22AF50DC), SPH_C32(0xEFFDBC6B), SPH_C32(0xEB21B74A)
};

static const sph_u32 B_init_256[] = {
	SPH_C32(0xB555C6EE), SPH_C32(0x3E710596), SPH_C32(0xA72A652F), SPH_C32(0x9301515F),
	SPH_C32(0xDA28C1FA), SPH_C32(0x696FD868), SPH_C32(0x9CB6BF72), SPH_C32(0x0AFE4002),
	SPH_C32(0xA6E03615), SPH_C32(0x5138C1D4), SPH_C32(0xBE216306), SPH_C32(0xB38B8890),
	SPH_C32(0x3EA8B96B), SPH_C32(0x3299ACE4), SPH_C32(0x30924DD4), SPH_C32(0x55CB34A5)
};

static const sph_u32 C_init_256[] = {
	SPH_C32(0xB405F031), SPH_C32(0xC4233EBA), SPH_C32(0xB3733979), SPH_C32(0xC0DD9D55),
	SPH_C32(0xC51C28AE), SPH_C32(0xA327B8E1), SPH_C32(0x56C56167), SPH_C32(0xED614433),
	SPH_C32(0x88B59D60), SPH_C32(0x60E2CEBA), SPH_C32(0x758B4B8B), SPH_C32(0x83E82A7F),
	SPH_C32(0xBC968828), SPH_C32(0xE6E00BF7), SPH_C32(0xBA839E55), SPH_C32(0x9B491C60)
};

void sph_shabal256_init(sph_shabal_context *cc)
{
	//const sph_u32 *A_init, *B_init, *C_init;
	//sph_shabal_context *sc;

		//A_init = A_init_256;
		//B_init = B_init_256;
		//C_init = C_init_256;

	//sc = (sph_shabal_context *) cc;
	memcpy(cc->B, B_init_256, sizeof cc->B);
	memcpy(cc->A, A_init_256, sizeof cc->A);
	memcpy(cc->C, C_init_256, sizeof cc->C);
	cc->Wlow = 1;
	cc->Whigh = 0;
	cc->ptr = 0;
}

void sph_shabal256(void *cc, const unsigned char *data, size_t len)
{
	sph_shabal_context *sc;
	unsigned char *buf;
	size_t ptr;
	DECL_STATE

	sc = (sph_shabal_context *) cc;
	buf = sc->buf;
	ptr = sc->ptr;

	if (len < (sizeof sc->buf) - ptr)
	{
		memcpy(buf + ptr, data, len);
		ptr += len;
		sc->ptr = ptr;
		return;
	}

	READ_STATE(sc);
	while (len > 0) {
		size_t clen;

		clen = (sizeof sc->buf) - ptr;
		if (clen > len)	clen = len;
		memcpy(buf + ptr, data, clen);
		ptr += clen;
		data += clen;
		len -= clen;
		if (ptr == sizeof sc->buf) {
			DECODE_BLOCK;
			INPUT_BLOCK_ADD;
			XOR_W;
			APPLY_P;
			INPUT_BLOCK_SUB;
			SWAP_BC;
			INCR_W;
			ptr = 0;
		}
	}
	WRITE_STATE(sc);
	sc->ptr = ptr;
}

static size_t shabal_close(void *cc, unsigned ub, unsigned n, void *dst, unsigned size_words)
{
	sph_shabal_context *sc;
	unsigned char *buf;
	size_t ptr;
	unsigned z;
	union {
		unsigned char tmp_out[64];
		sph_u32 dummy;
	} u;
	size_t out_len;
	DECL_STATE

	sc = (sph_shabal_context *) cc;
	buf = sc->buf;
	ptr = sc->ptr;
	z = 0x80 >> n;
	buf[ptr] = ((ub & -z) | z) & 0xFF;
	memset(buf + ptr + 1, 0, (sizeof sc->buf) - (ptr + 1));
	READ_STATE(sc);
	DECODE_BLOCK;
	INPUT_BLOCK_ADD;
	XOR_W;
	APPLY_P;
//#pragma loop(hint_parallel(3))
	for (int i = 0; i < 3; i ++) {
		SWAP_BC;
		XOR_W;
		APPLY_P;
	}

		sph_enc32le_aligned(u.tmp_out + 32, B8);

		sph_enc32le_aligned(u.tmp_out + 36, B9);

		sph_enc32le_aligned(u.tmp_out + 40, BA);
		sph_enc32le_aligned(u.tmp_out + 44, BB);
		sph_enc32le_aligned(u.tmp_out + 48, BC);
		sph_enc32le_aligned(u.tmp_out + 52, BD);
		sph_enc32le_aligned(u.tmp_out + 56, BE);
		sph_enc32le_aligned(u.tmp_out + 60, BF);

	out_len = size_words << 2;
	memcpy(dst, u.tmp_out + (sizeof u.tmp_out) - out_len, out_len);
	//sph_shabal256_init(sc, size_words << 5);
	return out_len;
}
size_t sph_shabal256_close(void *cc, void *dst)
{
	return shabal_close(cc, 0, 0, dst, 8);
}

static int xdigit(char const digit) {
	int val;
	if ('0' <= digit && digit <= '9') val = digit - '0';
	else if ('a' <= digit && digit <= 'f') val = digit - 'a' + 10;
	else if ('A' <= digit && digit <= 'F') val = digit - 'A' + 10;
	else val = -1;
	return val;
}

static size_t xstr2strr(char *buf, size_t const bufsize, const char *const in) {
	if (!in) return 0; // missing input string

	size_t inlen = (size_t)strlen(in);
	if (inlen % 2 != 0) inlen--; // hex string must even sized

	size_t i, j;
	for (i = 0; i<inlen; i++)
		if (xdigit(in[i])<0) return 0; // bad character in hex string

	if (!buf || bufsize<inlen / 2 + 1) return 0; // no buffer or too small

	for (i = 0, j = 0; i<inlen; i += 2, j++)
		buf[j] = xdigit(in[i]) * 16 + xdigit(in[i + 1]);

	buf[inlen / 2] = '\0';
	return inlen / 2 + 1;
}

static unsigned int calc_scoop (const char *signature, unsigned long long height, sph_shabal_context *gcc) {
	char scoopgen[40];
	memmove(scoopgen, signature, 32);
	const char *mov = (char*)&height;
	sph_shabal_context local_32;
	memcpy(&local_32, gcc, sizeof(sph_shabal_context));
	scoopgen[32] = mov[7]; scoopgen[33] = mov[6]; scoopgen[34] = mov[5]; scoopgen[35] = mov[4]; scoopgen[36] = mov[3]; scoopgen[37] = mov[2]; scoopgen[38] = mov[1]; scoopgen[39] = mov[0];

	sph_shabal256(&local_32, (const unsigned char*)(const unsigned char*)scoopgen, 40);
	char xcache[32];
	sph_shabal256_close(&local_32, xcache);

	return (((unsigned char)xcache[31]) + 256 * (unsigned char)xcache[30]) % 4096;
}

#define SCOOP_SIZE 64
#define NUM_SCOOPS 4096
#define NONCE_SIZE (NUM_SCOOPS * SCOOP_SIZE)

#define HASH_SIZE 32
#define HASH_CAP 4096

#define SSE4_PARALLEL 4
#define AVX2_PARALLEL 8

#define SET_NONCE(gendata, nonce, offset)                                      \
  xv = (char *)&nonce;                                                         \
  gendata[NONCE_SIZE + offset] = xv[7];                                        \
  gendata[NONCE_SIZE + offset + 1] = xv[6];                                    \
  gendata[NONCE_SIZE + offset + 2] = xv[5];                                    \
  gendata[NONCE_SIZE + offset + 3] = xv[4];                                    \
  gendata[NONCE_SIZE + offset + 4] = xv[3];                                    \
  gendata[NONCE_SIZE + offset + 5] = xv[2];                                    \
  gendata[NONCE_SIZE + offset + 6] = xv[1];                                    \
  gendata[NONCE_SIZE + offset + 7] = xv[0]

unsigned long long calc_dl(unsigned long long height, const unsigned char *sig, unsigned long long accid, unsigned long long nonce) {
  	unsigned int scoop_nr = 0;
  	// char sig[32 + 128];
  	// xstr2strr(sig, 33, signature);
  	sph_shabal_context save_32;
  	sph_shabal256_init(&save_32);
  	scoop_nr = calc_scoop ((const char*)sig, height, &save_32);

  	char final[32];
  	char gendata[16 + NONCE_SIZE];
  	char *xv;

  	SET_NONCE(gendata, accid, 0);
  	SET_NONCE(gendata, nonce, 8);

  	sph_shabal_context x;
  	int len;

  	for (int i = NONCE_SIZE; i > 0; i -= HASH_SIZE) {
    // sph_shabal256_init(&x);
  		memcpy(&x, &save_32, sizeof(save_32));

  		len = NONCE_SIZE + 16 - i;
  		if (len > HASH_CAP)
  			len = HASH_CAP;

  		sph_shabal256(&x, (unsigned char *)&gendata[i], len);
  		sph_shabal256_close(&x, &gendata[i - HASH_SIZE]);
  	}

  //sph_shabal256_init(&x);
  	memcpy(&x, &save_32, sizeof(save_32));
  	sph_shabal256(&x, (unsigned char *)gendata, 16 + NONCE_SIZE);
  	sph_shabal256_close(&x, final);

  // XOR with final
  	for (int i = 0; i < NONCE_SIZE; i++)
  		gendata[i] ^= (final[i % 32]);

  	sph_shabal_context deadline_sc;
  // sph_shabal256_init(&deadline_sc);
  	memcpy(&deadline_sc, &save_32, sizeof(save_32));
  	sph_shabal256(&deadline_sc, (const unsigned char *)sig, HASH_SIZE);

  	uint8_t scoop[SCOOP_SIZE];
  	memcpy(scoop, gendata + (scoop_nr * SCOOP_SIZE), 32);
  	memcpy(scoop + 32, gendata + ((4095 - scoop_nr) * SCOOP_SIZE) + 32, 32);

  	uint8_t finals2[HASH_SIZE];
  	sph_shabal256(&deadline_sc, scoop, SCOOP_SIZE);
  	sph_shabal256_close(&deadline_sc, (uint32_t *)finals2);
  	unsigned long long *finals3 = (unsigned long long*)finals2;
  // req->deadline = *(uint64_t *)finals2 / req->base_target;
  	return *finals3;
}

unsigned long long calc_dl_ex(unsigned long long height, const char *signature_hex, unsigned long long accid, unsigned long long nonce) {
	unsigned char sig[33] = {};
	xstr2strr((char*)sig, 33, signature_hex);

	return calc_dl(height, sig, accid, nonce);
}

