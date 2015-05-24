/* GIFFLEN.CPP
 * Native code used by the Giffle app
 * Performs color quantization and GIF encoding
 *
 * Mic, 2009
 */

/* NeuQuant Neural-Net Quantization Algorithm
 * ------------------------------------------
 *
 * Copyright (c) 1994 Anthony Dekker
 *
 * NEUQUANT Neural-Net quantization algorithm by Anthony Dekker, 1994.
 * See "Kohonen neural networks for optimal colour quantization"
 * in "Network: Computation in Neural Systems" Vol. 5 (1994) pp 351-367.
 * for a discussion of the algorithm.
 * See also  http://members.ozemail.com.au/~dekker/NEUQUANT.HTML
 *
 * Any party obtaining a copy of these files from the author, directly or
 * indirectly, is granted, free of charge, a full and unrestricted irrevocable,
 * world-wide, paid up, royalty-free, nonexclusive right and license to deal
 * in this software and documentation files (the "Software"), including without
 * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 */

/* The GIF encoder is based on the SAVE_PIC library written by:
 *  Christopher Street
 *  chris_street@usa.net
 */


#include <jni.h>
#include "neuquant.h"
#include <android/log.h>
#include <stdio.h>

int optCol=256, optQuality=100, optDelay=4;
unsigned char *data32bpp = NULL;
NeuQuant *neuQuant = NULL;
DIB inDIB, *outDIB;
JavaVM *gJavaVM;
static char s[128];

unsigned char palette[768];
FILE *pGif = NULL;


extern "C" {
JNIEXPORT jint JNICALL Java_com_cardinalblue_library_gifencoder_Giffle_Init(JNIEnv *ioEnv, jobject ioThis, jstring gifName,
                                                             jint w, jint h, jint numColors, jint quality, jint frameDelay);
JNIEXPORT void JNICALL Java_com_cardinalblue_library_gifencoder_Giffle_GenPalette(JNIEnv *ioEnv, jobject ioThis, jint len, jintArray inArray);
JNIEXPORT void JNICALL Java_com_cardinalblue_library_gifencoder_Giffle_Close(JNIEnv *ioEnv, jobject ioThis);
JNIEXPORT jint JNICALL Java_com_cardinalblue_library_gifencoder_Giffle_AddFrame(JNIEnv *ioEnv, jobject ioThis, jintArray inArray);
};

int max_bits(int);
int GIF_LZW_compressor(DIB *,unsigned int,FILE *,int);

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv *env;
	gJavaVM = vm;
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		return -1;
	}
	return JNI_VERSION_1_4;
}


JNIEXPORT jint JNICALL Java_com_cardinalblue_library_gifencoder_Giffle_Init(JNIEnv *ioEnv, jobject ioThis, jstring gifName,
                                                             jint w, jint h, jint numColors, jint quality, jint frameDelay) {
	const char *str;
    str = ioEnv->GetStringUTFChars(gifName, NULL);
    if (str == NULL) {
    	return -1; /* OutOfMemoryError already thrown */
    }

    if ((pGif = fopen(str, "wb")) == NULL) {
	    ioEnv->ReleaseStringUTFChars(gifName, str);
		return -2;
	}

    ioEnv->ReleaseStringUTFChars(gifName, str);

	optDelay = frameDelay;
	optCol = numColors;
	optQuality = quality;

	data32bpp = new unsigned char[w * h * PIXEL_SIZE];

	inDIB.bits = data32bpp;
	inDIB.width = w;
	inDIB.height = h;
	inDIB.bitCount = 32;
	inDIB.pitch = w * PIXEL_SIZE;
	inDIB.palette = NULL;

	outDIB = new DIB(w, h, 8);
	outDIB->palette = new unsigned char[768];

	neuQuant = new NeuQuant();
	neuQuant->imgw = w;
	neuQuant->imgh = h;

	// Output the GIF header and Netscape extension
	fwrite("GIF89a", 1, 6, pGif);
	s[0] = w & 0xFF; s[1] = w / 0x100;
	s[2] = h & 0xFF; s[3] = h / 0x100;
	s[4] = 0x50 + max_bits(numColors) - 1;
	s[5] = s[6] = 0;
	s[7] = 0x21; s[8] = 0xFF; s[9] = 0x0B;
	fwrite(s, 1, 10, pGif);
	fwrite("NETSCAPE2.0", 1, 11, pGif);
	s[0] = 3; s[1] = 1; s[2] = s[3] = s[4] = 0;
	fwrite(s, 1, 5, pGif);

	return 0;
}

JNIEXPORT void JNICALL Java_com_cardinalblue_library_gifencoder_Giffle_GenPalette(JNIEnv *ioEnv, jobject ioThis, jint len, jintArray inArray) {
    uint8_t* bits = (uint8_t*) malloc(len * PIXEL_SIZE * sizeof(uint8_t));
    ioEnv->GetIntArrayRegion(inArray, (jint)0, len, (jint*)(bits));
    int quality = optQuality;
    quality /= 3;
    if (quality > 30) {
        quality = 30;
    } else if (quality < 1) {
        quality = 1;
    }

    int numColors = optCol;
    neuQuant->netsize = numColors;
    neuQuant->initnet(bits, len * PIXEL_SIZE, 31-quality);
    neuQuant->learn();
    neuQuant->unbiasnet();

    int i;
    for (i = 0; i < numColors; i++) {
        palette[i*3 + 2] = neuQuant->network[i][0];
        palette[i*3 + 1] = neuQuant->network[i][1];
        palette[i*3 + 0] = neuQuant->network[i][2];
    }
    neuQuant->inxbuild();
    free(bits);
}


JNIEXPORT void JNICALL Java_com_cardinalblue_library_gifencoder_Giffle_Close(JNIEnv *ioEnv, jobject ioThis) {
	if (data32bpp) {
		delete [] data32bpp;
		data32bpp = NULL;
	}
	if (outDIB) {
		if (outDIB->palette) delete [] outDIB->palette;
		delete outDIB;
		outDIB = NULL;
	}
	if (pGif) {
		fputc(';',pGif);
		fclose(pGif);
		pGif = NULL;
	}
	if (neuQuant) {
		delete neuQuant;
		neuQuant = NULL;
	}
}

JNIEXPORT jint JNICALL Java_com_cardinalblue_library_gifencoder_Giffle_AddFrame(JNIEnv *ioEnv, jobject ioThis, jintArray inArray) {
	ioEnv->GetIntArrayRegion(inArray, (jint)0, (jint)(inDIB.width * inDIB.height), (jint*)(inDIB.bits));

	s[0] = '!'; s[1] = 0xF9; s[2] = 4;
	s[3] = 0; s[4] = optDelay & 0xFF; s[5] = optDelay / 0x100; s[6] = s[7] = 0;
	s[8] = ','; s[9] = s[10] = s[11] = s[12] = 0;
	s[13] = neuQuant->imgw & 0xFF; s[14] = neuQuant->imgw / 0x100;
	s[15] = neuQuant->imgh & 0xFF; s[16] = neuQuant->imgh / 0x100;
	s[17] = 0x80 + max_bits(optCol) - 1;

	fwrite(s, 1, 18, pGif);
    neuQuant->quantise(outDIB, &inDIB, optCol, 0);
	fwrite(palette, 1, optCol * 3, pGif);
	GIF_LZW_compressor(outDIB, optCol, pGif, 0);
	return 0;
}


/*************************************************************************************************************/

#define hash 11003

unsigned int stat_bits;
unsigned int code_in_progress;
unsigned int LZWpos;
char LZW[256];
short int hashtree[hash][3];

int find_hash(int pre, int suf) {
	int i,o;
    i = ((pre*256) ^ suf) % hash;
    o = (i == 0) ? 1 : hash - i;
    while (1) {
        if (hashtree[i][0]==-1)
            return i;
        else if ((hashtree[i][1]==pre) && (hashtree[i][2]==suf))
            return i;
        else {
            i = i-o;
            if (i < 0) i += hash;
        }
	}
	return 0;
}

int max_bits(int num) {
	for (int b=0; b<14; b++) {
        if ((1<<b)>=num)
            return b;
	}
	return 0;
}

void append_code(FILE *handle,int code) {
    LZW[LZWpos++] = code;
    if (LZWpos==256) {
		LZW[0] = 255;
		fwrite(LZW, 1, 256, handle);
		LZWpos = 1;
	}
}

void write_code(FILE *handle, int no_bits, int code) {
    code_in_progress = code_in_progress + (code << stat_bits); // * powers2[stat_bits+1]
    stat_bits=stat_bits+no_bits;
    while (stat_bits > 7) {
        append_code(handle,code_in_progress&255);
        code_in_progress >>= 8;
        stat_bits -= 8;
	}
}

int GIF_LZW_compressor(DIB *srcimg,unsigned int numColors,FILE *handle,int interlace) {
    int xdim,ydim,clear,EOI,code,bits,pre,suf,x,y,i,max,bits_color,done,rasterlen;
    static short int rasters[768];

    stat_bits = 0;
    code_in_progress = 0;
    LZWpos = 1;

    for (i = 0; i < hash; i++)
    {
		hashtree[i][0] = hashtree[i][1] = hashtree[i][2] = -1;
	}
    if (handle==NULL)
        return 0;
    xdim = srcimg->width;
    ydim = srcimg->height;
    bits_color = max_bits(numColors)-1;
    clear = (1 << (bits_color+1)); //powers2[bits_color+2]
    EOI = clear+1;
    code = EOI+1;
    bits = bits_color+2;
    max = (1 << bits); //powers2[bits+1]
    if (code==max)
    {
        clear = 4;
        EOI = 5;
        code = 6;
        bits++;
        max *= 2;
	}
    fputc(bits-1, handle);
    write_code(handle, bits, clear);
    rasterlen = 0;
    if (interlace)
    {
        for (int e=1; e<=5; e+=4)
        {
            for (int f=e; f<=ydim; f+=8)
            {
                rasters[rasterlen++] = f;
			}
		}
        for (int e=3; e<=ydim; e+=4)
        {
            rasters[rasterlen++] = e;
		}
        for (int e=2; e<=ydim; e+=2)
        {
            rasters[rasterlen++] = e;
	   }
   	}
    else
    {
        for (int e=1; e<=ydim; e++)
            rasters[rasterlen++] = e - 1;
	}
    pre = srcimg->bits[rasters[0] * xdim];
    x=1;     y=0;     done=0;
    if (x>=xdim)
    {
		y++;   x=0;
	}
    while (1)
    {
        while (1)
        {
            if (!done)
            {
                suf = srcimg->bits[rasters[y] * xdim + x];
                x++;
                if (x>=xdim)
                {
                    y++;
                    x=0;
                    if (y>=ydim)
                        done = 1;
				}
                i = find_hash(pre,suf);
                if (hashtree[i][0]==-1)
                    break;
                else
                    pre = hashtree[i][0];
			}
            else
            {
                write_code(handle,bits,pre);
                write_code(handle,bits,EOI);
                if (stat_bits)
                    write_code(handle,bits,0);
                LZW[0] = LZWpos - 1;
                fwrite(LZW, 1, LZWpos, handle);
                fputc(0, handle);
                return 1;
			}
		}
        write_code(handle,bits,pre);
        hashtree[i][0] = code; hashtree[i][1] = pre; hashtree[i][2] = suf;
        pre = suf;
        code++;
        if (code == max+1)
        {
            max *= 2;
            if (bits == 12)
            {
                write_code(handle,bits,clear);
                for (i = 0; i < hash; i++)
                {
					hashtree[i][0] = hashtree[i][1] = hashtree[i][2] = -1;
				}
                code = EOI+1;
                bits = bits_color+2;
                max = 1 << bits;
                if (bits == 2)
                {
                    clear = 4;
                    EOI = 5;
                    code = 6;
                    bits = 3;
                    max *= 2;
				}
            }
            else
                bits++;
		}
	}

	return 0;
}