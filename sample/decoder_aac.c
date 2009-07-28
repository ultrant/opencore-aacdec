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
//  File: decoder_aac.c                                                         //
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
#include "config.h"


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

        for (i=count; i<32; i++) {
            fprintf(stderr, "   ");
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


int RetrieveDecodedStreamType(tPVMP4AudioDecoderExternal *pExt)
{
    if ((pExt->extendedAudioObjectType == MP4AUDIO_AAC_LC) ||
            (pExt->extendedAudioObjectType == MP4AUDIO_LTP))
    {
        return AAC;   /*  AAC */
    }
    else if (pExt->extendedAudioObjectType == MP4AUDIO_SBR)
    {
        return AACPLUS;   /*  AAC+ */
    }
    else if (pExt->extendedAudioObjectType == MP4AUDIO_PS)
    {
        return ENH_AACPLUS;   /*  AAC++ */
    }

    return -1;   /*  Error evaluating the stream type */
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
	
	
	
	
	
	
#ifdef AAC_PLUS
    pExt->pOutputBuffer_plus = &iOutputBuf[2048];
#else
    pExt->pOutputBuffer_plus = NULL;
#endif
	
    pExt->pInputBuffer			   = iInputBuf;
    pExt->pOutputBuffer			   = iOutputBuf;
	pExt->inputBufferMaxLength 	   = KAAC_MAX_STREAMING_BUFFER_SIZE;
    pExt->desiredChannels          = 2;
    pExt->inputBufferCurrentLength = 0;
    pExt->outputFormat             = OUTPUTFORMAT_16PCM_INTERLEAVED;
    pExt->repositionFlag           = FALSE;
    pExt->aacPlusEnabled           = TRUE;  /* Dynamically enable AAC+ decoding */
    pExt->inputBufferUsedLength    = 0;
    pExt->remainderBits            = 0;

    

    if (PVMP4AudioDecoderInitLibrary(pExt, pMem) != 0) {
    	fprintf(stderr, "init library failed\n");
        exit(-1);
    }
    
        

	uint8_t iAacInitFlag = 0;
	int32_t Status;
	uint32_t aSamplesPerFrame = 0;
	int32_t aOutputLength = 0;
	int8_t  aIsFirstBuffer = 1;
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
			if(!fread(iInputBuf+2, 1, KAAC_MAX_STREAMING_BUFFER_SIZE-2, ifile)) {
				fprintf(stderr, "eof reached\n");
				exit(-1);
			}
			break;
		}
	}

   	pExt->inputBufferUsedLength=0;
   	pExt->inputBufferCurrentLength = KAAC_MAX_STREAMING_BUFFER_SIZE;
	print_bytes(iInputBuf, pExt->inputBufferCurrentLength);
	
	audio_file *aufile = NULL;
	
	while (1) {
		
	   	/* initialization of decoder (look into input stream)*/
	   	if (0 == iAacInitFlag) {
	   		Status = PVMP4AudioDecoderConfig(pExt, pMem);
    		if (Status != MP4AUDEC_SUCCESS) {
    			fprintf(stderr, "[INIT] can be adif?\n");
        		Status = PVMP4AudioDecodeFrame(pExt, pMem);
    		}
    		
        	fprintf(stderr, "[INIT] Status: %d\n", Status);
        	
        	if (MP4AUDEC_SUCCESS == Status) {
	        	fprintf(stderr, "[INIT] SUCCESS\n");
        		iAacInitFlag = 1;
        		pExt->inputBufferUsedLength=0;
        	}
        	
        	
        	
        	fprintf(stderr, "[INIT] inputBufferUsedLength: %d, "
      						"inputBufferCurrentLength: %d, "
      						"aacPlusUpsamplingFactor: %d, "
      						"Status: %d\n",
      						pExt->inputBufferUsedLength,
      						pExt->inputBufferCurrentLength,
      						pExt->aacPlusUpsamplingFactor,
      						Status);
			if(KAAC_MAX_STREAMING_BUFFER_SIZE <= pExt->inputBufferUsedLength) {
		        fread(	iInputBuf,
						KAAC_MAX_STREAMING_BUFFER_SIZE,
						1,
						ifile);
				pExt->inputBufferUsedLength=0;
		        pExt->inputBufferCurrentLength = KAAC_MAX_STREAMING_BUFFER_SIZE;
			}
        	continue;
        }
		
		
		/* after successful initialisation */
		if (2 == pExt->aacPlusUpsamplingFactor) {
        	aSamplesPerFrame = 2 * KAAC_NUM_SAMPLES_PER_FRAME;
        } else {
        	aSamplesPerFrame = KAAC_NUM_SAMPLES_PER_FRAME;
        }
		
        Status = PVMP4AudioDecodeFrame(pExt, pMem);
       	fprintf(stderr, "[MAIN] Status: %d\n", Status);
       	
       	if (MP4AUDEC_SUCCESS == Status || SUCCESS == Status) {
       		fprintf(stderr, "[SUCCESS] Status: SUCCESS "
       						"inputBufferUsedLength: %u, "
       						"inputBufferCurrentLength: %u, "
       						"remainderBits: %u, "
       						"frameLength: %u\n",
       						pExt->inputBufferUsedLength,
       						pExt->inputBufferCurrentLength,
       						pExt->remainderBits,
       						pExt->frameLength);
       		/*aOutputLength = pExt->frameLength * pExt->desiredChannels;*/
       		aOutputLength = aSamplesPerFrame * pExt->desiredChannels;
       		fprintf(stderr, "[SUCCESS] aOutputLength (samples*channels): %d\n", aOutputLength);

#ifdef AAC_PLUS
			fprintf(stderr, "[SUCCESS] AAC_PLUS ENABLED\n");
        	if (2 == pExt->aacPlusUpsamplingFactor) {
        		fprintf(stderr, "[SUCCESS] AAC_PLUS aOutputLength=%d\n", aOutputLength);
            	if (1 == pExt->desiredChannels) {
            		fprintf(stderr, "[SUCCESS] AAC_PLUS DOWNSAMPLE desiredChannels=%d\n",
            					pExt->desiredChannels);
                	memcpy(&iOutputBuf[1024], &iOutputBuf[2048], (aOutputLength * 2));
            	}
            	aOutputLength *= 2;
        	}
#endif
        	//After decoding the first frame, modify all the input & output port settings
        	if (1 == aIsFirstBuffer) {
        		int StreamType = (int32_t) RetrieveDecodedStreamType(pExt);
        		
        		if ((0 == StreamType) && (2 == pExt->aacPlusUpsamplingFactor)) {
        			fprintf(stderr, "[SUCCESS] DisableAacPlus StreamType=%d, "
        									  "aacPlusUpsamplingFactor=%d\n",
        									  StreamType,
        									  pExt->aacPlusUpsamplingFactor);
        			PVMP4AudioDecoderDisableAacPlus(pExt, pMem);
        			aSamplesPerFrame = KAAC_NUM_SAMPLES_PER_FRAME;
            	}
            	fprintf(stderr, "[SUCCESS] CreateWavHeader: desiredChannels=%d, samplingRate=%d\n",
            					pExt->desiredChannels,
            					pExt->samplingRate);
				/*CreateWavHeader(ofile,  pExt->desiredChannels, pExt->samplingRate, 16);*/
                aufile = open_audio_file(ofile, 48000, 2, FMT_16BIT, OUTPUT_WAV, 0);
				fprintf(stderr, "[SUCCESS] aSamplesPerFrame=%d\n", aSamplesPerFrame);
				aIsFirstBuffer=0;
			}
			
			write_audio_file(aufile, iOutputBuf, aOutputLength*2, 0);
			pExt->inputBufferCurrentLength = KAAC_MAX_STREAMING_BUFFER_SIZE - pExt->inputBufferUsedLength;
       		memmove(	iInputBuf,
       					iInputBuf + pExt->inputBufferUsedLength,
        				pExt->inputBufferCurrentLength);
        	if(!fread(	iInputBuf+pExt->inputBufferCurrentLength,
        				1, pExt->inputBufferUsedLength, ifile)) {
				fprintf(stderr, "eof reached\n");
				exit(-1);
			}
			pExt->inputBufferCurrentLength = KAAC_MAX_STREAMING_BUFFER_SIZE;
			pExt->inputBufferUsedLength = 0;
        	//print_bytes(iInputBuf, pExt->inputBufferCurrentLength);


    	} else if (MP4AUDEC_INCOMPLETE_FRAME == Status) {
        	fprintf(stderr, "[INCOMPLETE] Status: MP4AUDEC_INCOMPLETE_FRAME\n");
        	if (KAAC_MAX_STREAMING_BUFFER_SIZE - pExt->inputBufferUsedLength != 0) {
        		fprintf(stderr, "[SUCCESS] inputBufferUsedLength: %d\n", pExt->inputBufferUsedLength);
        		memmove(	iInputBuf,
        					iInputBuf + pExt->inputBufferUsedLength,
        					KAAC_MAX_STREAMING_BUFFER_SIZE - pExt->inputBufferUsedLength);
        		
        		pExt->inputBufferUsedLength=0;
        	}
        } else {
        	fprintf(stderr, "[BAD] Status: %s, inputBufferUsedLength: %u\n", Status==1?"UNKNOWN":"BAD", pExt->inputBufferUsedLength);
        	
        	if (KAAC_MAX_STREAMING_BUFFER_SIZE - pExt->inputBufferUsedLength != 0) {
        		fprintf(stderr, "[SUCCESS] inputBufferUsedLength: %d\n", pExt->inputBufferUsedLength);
        		memmove(	iInputBuf,
        					iInputBuf + pExt->inputBufferUsedLength,
        					KAAC_MAX_STREAMING_BUFFER_SIZE - pExt->inputBufferUsedLength);
        		
        		pExt->inputBufferUsedLength=0;
        	}
    	}
    	fprintf(stderr,"---------------------------------------------------------\n");
	}
	
	close_audio_file(aufile);
	return 0;
}
