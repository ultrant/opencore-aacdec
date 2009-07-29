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



/* Receive data from source */
int bufferUpdate(FILE *fhandle, tPVMP4AudioDecoderExternal *pExt){
		/* rest of buffer */
		int BytesNeeded = pExt->inputBufferUsedLength;
		int BytesExists = pExt->inputBufferCurrentLength - pExt->inputBufferUsedLength;
		
		if(BytesExists >= 0) {
			/* have bytes in buffer */
			fprintf(stderr, "[GET] have %d bytes, ", BytesExists);
			if(BytesExists > 0) {
				memmove(	pExt->pInputBuffer,
							pExt->pInputBuffer+BytesNeeded,
							BytesExists);
			}
   			fprintf(stderr, "receiving %d bytes\n", BytesNeeded);
   			if(!fread(	pExt->pInputBuffer+BytesExists,
   						1, BytesNeeded, fhandle)) {
   				fprintf(stderr, "[GET] eof reached\n");
   				pExt->inputBufferCurrentLength = BytesExists;
   				pExt->inputBufferUsedLength = 0;
   				return 2;
			} else {
				pExt->inputBufferCurrentLength = KAAC_MAX_STREAMING_BUFFER_SIZE;
				pExt->inputBufferUsedLength = 0;
				return 1;
   			}
		} else if (BytesExists < 0) {
			fprintf(stderr, "[GET] no buffer\n");
		}
		return 0;
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
	fprintf(stderr, "[MAIN] AAC+ ENABLED\n");
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
    pExt->repositionFlag           = TRUE;
    pExt->aacPlusEnabled           = TRUE;  /* Dynamically enable AAC+ decoding */
    pExt->inputBufferUsedLength    = 0;
    pExt->remainderBits            = 0;
    pExt->frameLength              = 0;
    

    

    if (PVMP4AudioDecoderInitLibrary(pExt, pMem) != 0) {
    	fprintf(stderr, "init library failed\n");
        exit(-1);
    }
    
	int32_t Status;
	int32_t aOutputLength = 0;
	int8_t  aIsFirstBuffer = 1;

	/* seek to first adts sync */
	fread (iInputBuf, 1, 2, ifile);
	fprintf(stderr, "sync search:");
	while(1) {	
		if (!((iInputBuf[0] == 0xFF)&&((iInputBuf[1] & 0xF6) == 0xF0))) {
			fprintf(stderr, ".");
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
	fprintf(stderr, "found!\n");
   	pExt->inputBufferUsedLength=0;
   	pExt->inputBufferCurrentLength = KAAC_MAX_STREAMING_BUFFER_SIZE;

	audio_file *aufile = NULL;
	
	/* pre-init search adts sync */
	while (pExt->frameLength == 0) {
   		Status = PVMP4AudioDecoderConfig(pExt, pMem);
       	fprintf(stderr, "[INIT] Status[0]: %d\n", Status);
   		if (Status != MP4AUDEC_SUCCESS) {
       		Status = PVMP4AudioDecodeFrame(pExt, pMem);
       		fprintf(stderr, "[INIT] Status[1]: %d\n", Status);
       		if (MP4AUDEC_SUCCESS == Status) {
   				fprintf(stderr, "[INIT] frameLength: %d\n", pExt->frameLength);
    			continue;
        	}
   		}
   		
   		if(!bufferUpdate(ifile, pExt)){
   			exit(-1);
   		}
   		pExt->inputBufferUsedLength = 0;
	}	

	fprintf(stderr, "[INIT] Synced\n");
	
	/* main loop */
	while (1) {
		if(!bufferUpdate(ifile, pExt)){
   			exit(0);
   		}
   				
        Status = PVMP4AudioDecodeFrame(pExt, pMem);

       	if (MP4AUDEC_SUCCESS == Status || SUCCESS == Status) {
			/*fprintf(stderr, "[SUCCESS] Status: SUCCESS "
       						"inputBufferUsedLength: %u, "
       						"inputBufferCurrentLength: %u, "
       						"remainderBits: %u, "
       						"frameLength: %u\n",
       						pExt->inputBufferUsedLength,
       						pExt->inputBufferCurrentLength,
       						pExt->remainderBits,
       						pExt->frameLength);*/

       		aOutputLength = pExt->frameLength;
       		/*fprintf(stderr, "[SUCCESS] aOutputLength (samples*channels): %d\n", aOutputLength);*/

#ifdef AAC_PLUS
        	if (2 == pExt->aacPlusUpsamplingFactor) {
        		/* we have 2x samples */
        		aOutputLength *= 2;
        		if (1 == aIsFirstBuffer) {
        			fprintf(stderr, "[SUCCESS] AAC+ detected\n");
        		}
        		
            	if (1 == pExt->desiredChannels) {
	        		if (1 == aIsFirstBuffer) {
    	    			fprintf(stderr, "[SUCCESS] downsampling stereo to mono\n");
        			}
                	memcpy(&iOutputBuf[1024], &iOutputBuf[2048], (aOutputLength * 2));
            	}
        	}
#endif
        	//After decoding the first frame, modify all the input & output port settings
        	if (1 == aIsFirstBuffer) {
        		/* first loop */
        		int StreamType = (int32_t) RetrieveDecodedStreamType(pExt);
        		
        		if ((0 == StreamType) && (2 == pExt->aacPlusUpsamplingFactor)) {
        			fprintf(stderr, "[SUCCESS] DisableAacPlus StreamType=%d, "
        									  "aacPlusUpsamplingFactor=%d\n",
        									  StreamType,
        									  pExt->aacPlusUpsamplingFactor);
        			PVMP4AudioDecoderDisableAacPlus(pExt, pMem);
        			aOutputLength = pExt->frameLength;
            	}
            	fprintf(stderr, "[WAV] desiredChannels=%d, samplingRate=%d\n",
            					pExt->desiredChannels,
            					pExt->samplingRate);
				/* create wav header */
                aufile = open_audio_file(	ofile, 
                							pExt->samplingRate,
                							pExt->desiredChannels,
                							FMT_16BIT,
                							OUTPUT_WAV,
                							0 /* write big header */);
				aIsFirstBuffer=0;
			}
			/* save decoded frames */
			write_audio_file(aufile, iOutputBuf, aOutputLength*pExt->desiredChannels, 0);

    	} else if (MP4AUDEC_INCOMPLETE_FRAME == Status) {
        	fprintf(stderr, "[STATUS] Status: MP4AUDEC_INCOMPLETE_FRAME\n");
        	break;
        } else {
        	fprintf(stderr, "[STATUS] Status: %s, eof?\n", Status==1?"UNKNOWN":"BAD");
       		break;
    	}
	}
	
	close_audio_file(aufile);
	return 0;
}
