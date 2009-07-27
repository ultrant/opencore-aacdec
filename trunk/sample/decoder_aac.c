/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
//  File: decoder_aac.cpp                                                       //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "pvmp4audiodecoder_api.h"
#include "e_tmp4audioobjecttype.h"
#include "getactualaacconfig.h"
#include "wav.h"

#define KAAC_NUM_SAMPLES_PER_FRAME      1024
#define KAAC_MAX_STREAMING_BUFFER_SIZE  (PVMP4AUDIODECODER_INBUFSIZE * 1)

#define KCAI_CODEC_NO_MEMORY -1
#define KCAI_CODEC_INIT_FAILURE -2

FILE *ifile;
FILE *ofile;

/*
 * Debugging
 * */
void print_bytes(uint8_t *bytes, int len) {
    int i;
    int count;
    int done = 0;

    while (len > done) {
        if (len-done > 32){
            count = 32;
        } else {
            count = len-done;
        }

        for (i=0; i<count; i++) {
            fprintf(stderr, "%02x ", (int)((unsigned char)bytes[done+i]));
        }

        fprintf(stderr, "\t\"");

        for (i=0; i<count; i++) {
            fprintf(stderr, "%c", isprint(bytes[done+i]) ? bytes[done+i] : '.');
        }

        fprintf(stderr, "\"\n");
        done += count;
    }
}



/*
 * Handle arguments
 * */
static void parse_args(int argc, char **argv)
{
    if (argc>2) {
        ifile = fopen (argv[1],"r");
        if (ifile==NULL) {
            fprintf (stderr, "input fopen error: %s\n", argv[1]);
            exit(-1);
        }

        ofile = fopen (argv[2],"w");
        if (ofile==NULL) {
            fprintf (stderr, "output fopen error: %s\n", argv[2]);
            exit(-1);
        }
            } else {
        fprintf(stderr,"Usage: %s <in-file> <out-file>\n\n", argv[0]);
        exit(-1);
    }
}


/*
 * Decoder main l00p
 * */
int main(int argc, char **argv)
{
	tPVMP4AudioDecoderExternal *pExt = malloc(sizeof(tPVMP4AudioDecoderExternal));
	uint8_t *iInputBuf = calloc(KAAC_MAX_STREAMING_BUFFER_SIZE, sizeof(uint8_t));
#ifdef AAC_PLUS
    int16_t *iOutputBuf = calloc(4096, sizeof(int16_t));
#else
    int16_t *iOutputBuf = calloc(2048, sizeof(int16_t));
#endif
	int32_t memreq =  PVMP4AudioDecoderGetMemRequirements();
	void *pMem = malloc(memreq);

    if (pExt == NULL || iInputBuf == NULL || iOutputBuf == NULL || pMem == NULL) {
    	fprintf(stderr, "not enough memory\n");
        exit(-1);
    }
	
	
	
	
	parse_args(argc, argv);
	
    pExt->pInputBuffer = iInputBuf;
    pExt->inputBufferMaxLength = KAAC_MAX_STREAMING_BUFFER_SIZE;
    pExt->pOutputBuffer = iOutputBuf;

#ifdef AAC_PLUS
    pExt->pOutputBuffer_plus = &iOutputBuf[2048];
#else
    pExt->pOutputBuffer_plus = NULL;
#endif

    pExt->desiredChannels          = 2;
    pExt->inputBufferCurrentLength = 0;
    pExt->outputFormat             = OUTPUTFORMAT_16PCM_INTERLEAVED;
    pExt->repositionFlag           = TRUE;
    pExt->aacPlusEnabled           = 1;  /* Dynamically enable AAC+ decoding */
    pExt->inputBufferUsedLength    = 0;
    pExt->remainderBits            = 0;

    

    if (PVMP4AudioDecoderInitLibrary(pExt, pMem) != 0) {
    	fprintf(stderr, "init library failed\n");
        exit(-1);
    }


    if (PVMP4SetAudioConfig(pExt, pMem,
                            1, /* upsamplingFactor */
                            48000, /* samp_rate */
                            2, /* num_ch */
                            MP4AUDIO_SBR) != SUCCESS) {
    	fprintf(stderr, "set audio config failed\n");
        exit(-1);
    }
    
    CreateWavHeader(ofile, 2, 48000, 16);
    
    uint8_t first = 1;
    int32_t nResult = -1;
    uint16_t frame_length = 0;
	
	/* seek to first adts sync */
	fread (iInputBuf, 1, 2, ifile);
	while(1) {	
		if (!((iInputBuf[0] == 0xFF)&&((iInputBuf[1] & 0xF6) == 0xF0))) {
			fprintf(stderr, "sync search\n");
			iInputBuf[0] = iInputBuf[1];
			if(!fread(iInputBuf+1, 1, 1, ifile)) {
				fprintf(stderr, "eof reached\n");
				exit(-1);
			}
		} else {
			if(!fread(iInputBuf+2, 1, 5, ifile)) {
				fprintf(stderr, "eof reached\n");
				exit(-1);
			}
			break;
		}
	}
	
	while (1) {

		if (first == 0) {
			if(!fread(iInputBuf, 1, 7, ifile)) {
				fprintf(stderr, "eof reached\n");
				exit(-1);
			}
		}

	    frame_length = ((((unsigned int)iInputBuf[3] & 0x3)) << 11)
                | (((unsigned int)iInputBuf[4]) << 3) | (iInputBuf[5] >> 5);
		
		fprintf(stderr, "frame_length=%u\n", frame_length);
		if(frame_length > KAAC_MAX_STREAMING_BUFFER_SIZE || frame_length < 7) {
				fprintf(stderr, "Too big frame %u bytes\n", frame_length);
				exit(-1);
		}

		if(!fread (iInputBuf+7, frame_length-7, 1, ifile)){
			fprintf(stderr, "eof reached\n");
			exit(-1);
		}
		pExt->inputBufferCurrentLength = frame_length;
		
		print_bytes(iInputBuf, frame_length);
		if (first == 0) {
			nResult = PVMP4AudioDecodeFrame(pExt, pMem);
		} else {
			
			if (PVMP4AudioDecoderConfig(pExt, pMem) !=  SUCCESS) {
				fprintf(stderr, "could be ADIF type\n");
				nResult = PVMP4AudioDecodeFrame(pExt, pMem);    /* could be ADIF type  */
			}
			
			//pExt->desiredChannels = pExt->encodedChannels;
			first = 0;
		}
		fprintf(stderr, "result=%d, "
						"inputBufferUsedLength=%d, "
						"remainderBits=%d, "
						"samplingRate=%d, "
						"bitRate=%d, "
						"frameLength=%d, "
						"audioObjectType=%d\n",
						nResult,
						pExt->inputBufferUsedLength,
						pExt->remainderBits,
						pExt->samplingRate,
						pExt->bitRate,
						pExt->frameLength,
						pExt->audioObjectType);
		if (nResult) {
			fwrite (iOutputBuf, 1, pExt->desiredChannels * 2 * 1024, ofile);
		}
	}
	
	UpdateWavHeader(ofile);
	return 0;
}
