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
#include <stdarg.h>
#include <sys/param.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <pthread.h>




#include "pvmp4audiodecoder_api.h"
#include "e_tmp4audioobjecttype.h"
#include "getactualaacconfig.h"
#include "audio.h"
#include "ringbuffer.h"

#include "config.h"


#define KAAC_NUM_SAMPLES_PER_FRAME      1024
#define KAAC_MAX_STREAMING_BUFFER_SIZE  (PVMP4AUDIODECODER_INBUFSIZE * 32)

#define KCAI_CODEC_NO_MEMORY -1
#define KCAI_CODEC_INIT_FAILURE -2

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



typedef struct {
	/* decoder handler */
	tPVMP4AudioDecoderExternal *pExt;
	
	/* decoder internal buffers */
	void *pMem;

	/* curl internal struct */
	CURL *curl;
	/* socket */
	int sockfd;
	
	/* ringbuffer struct */
	ringbuffer_t *rbuf;
	uint32_t 	 bufSize;
		
	/* external buffers */
	uint8_t *iInputBuf;
	int16_t *iOutputBuf;
} netplayer_t;


void freeall(netplayer_t *handler){
	if(handler->pMem != NULL)
		PVMP4AudioDecoderResetBuffer(handler->pMem);
	if(handler->pExt != NULL)
		free(handler->pExt);
	if(handler->iInputBuf != NULL)
		free(handler->iInputBuf);
	if(handler->iOutputBuf != NULL)
		free(handler->iOutputBuf);
	if(handler->curl != NULL)
	    curl_easy_cleanup(handler->curl);
	
	free(handler);
	handler=NULL;
}

void fail(netplayer_t *handler, const char *format, ...)
{
	va_list         pvar;
	
	va_start (pvar, format);
	fprintf(stderr, format, pvar);
	va_end (pvar);
	fprintf(stderr, "\n");
	freeall(handler);
	exit(-1);
}

void fill_ringBuffer(netplayer_t *h, size_t bytes)
{
	size_t avaliable = ringbuffer_write_space(h->rbuf);
	size_t iolen;
	size_t total = 0;
#define TCP_MTU		1600
	char buf[TCP_MTU];
	
	if (bytes > avaliable)
		fail(h, "no avaliable space in buffer");
	
	fprintf(stderr, "filling buffer: %u/%u %d percent", (int)bytes, (int)h->bufSize, (int)(bytes*100)/h->bufSize);
	while (avaliable >= TCP_MTU && bytes <= ringbuffer_read_space(h->rbuf)) {
		iolen = read(h->sockfd, buf, TCP_MTU);
        if(0 == iolen)
	         break;
	    
	    if( ringbuffer_write(h->rbuf, buf, iolen) != iolen)
	    	fail(h, "not all data written to ring buffer");
	    total += iolen;
		fprintf(stderr, ". (%d)\n", (int)iolen);
	}
	fprintf(stderr, "\nfilled: (%d)\n", (int)total);
}

static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t readsize = size * nmemb;
	ringbuffer_t *rbuf = (ringbuffer_t *)data;
	size_t avaliable = ringbuffer_write_space(rbuf);
	
	if (avaliable >= readsize) {
		if( ringbuffer_write(rbuf, ptr, readsize) != readsize)
			fprintf(stderr, "not all data written to ring buffer");
		
		fprintf(stderr, "read (%d)\n", (int)readsize);
	}
	
	return readsize;
}



/*
 * Decoder main l00p
 * */
int main(int argc, char **argv)
{
	netplayer_t *player = calloc(1, sizeof(netplayer_t));
	player->pExt = malloc(sizeof(tPVMP4AudioDecoderExternal));
	tPVMP4AudioDecoderExternal *pExt = player->pExt;
	
	player->iInputBuf = calloc(KAAC_MAX_STREAMING_BUFFER_SIZE, sizeof(uint8_t));
#ifdef AAC_PLUS
    player->iOutputBuf = calloc(4096, sizeof(int16_t));
#else
    player->iOutputBuf = calloc(2048, sizeof(int16_t));
#endif

	int32_t memreq =  PVMP4AudioDecoderGetMemRequirements();
	player->pMem = malloc(memreq);
	void *pMem = player->pMem;

	player->curl = curl_easy_init();
	CURL *curl = player->curl;

#define BUFFER_SIZE		131072
#define PREBUFFER_SIZE	32768
	player->bufSize = BUFFER_SIZE;
	player->rbuf = ringbuffer_create(player->bufSize);
	ringbuffer_t *rbuf = player->rbuf;

    CURLcode res;


	
    if (pExt == NULL || player->iInputBuf == NULL || player->iOutputBuf == NULL
    				 || pMem == NULL || curl == NULL || rbuf == NULL ) {
    	fail(player, "not enough memory\n");
    }
	
	/* Handle arguments */
	if (argc == 3) {
		/* argv[1] - inurl
		 * argv[2] - output alsa device 
		 * */
    } else {
        fail(player, "Usage: %s <URL> <out-file>\n\n", argv[0]);
    }
	
	
	/* open network connection */
    curl_easy_setopt(curl, CURLOPT_URL, "http://217.20.164.164:8000/bbc_radio1.aacp");
    /* Do not do the transfer - only connect to host */
    //curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
    
    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    
    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)rbuf);
    
    /* some servers don't like requests that are made without a user-agent
     * field, so we provide one */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");



	res = curl_easy_perform(curl);
    if(CURLE_OK != res)
    	fail(player, "CURL: %s\n", strerror(res));
	
	/* Extract the socket from the curl handle - we'll need it
     * for waiting */
        
    fprintf(stderr, "[MAIN] sockfd %d\n", player->sockfd);
    
    
    ringbuffer_reset(rbuf);

	
	
#ifdef AAC_PLUS
	fprintf(stderr, "[MAIN] AAC+ ENABLED\n");
    pExt->pOutputBuffer_plus = &iOutputBuf[2048];
#else
    pExt->pOutputBuffer_plus = NULL;
#endif
	
    pExt->pInputBuffer			   = player->iInputBuf;
    pExt->pOutputBuffer			   = player->iOutputBuf;
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
    	fail(player, "init library failed\n");
    }
    
	int32_t Status;
	int32_t aOutputLength = 0;
	int8_t  aIsFirstBuffer = 1;

	/* seek to first adts sync */
	exit(1);
//	read (iInputBuf, 1, 2, ifile);
	fprintf(stderr, "sync search:");
//	while(1) {	
//		if (!((player->iInputBuf[0] == 0xFF)&&((player->iInputBuf[1] & 0xF6) == 0xF0))) {
//			fprintf(stderr, ".");
//			iInputBuf[0] = iInputBuf[1];
//			if(!fread(player->iInputBuf+1, 1, 1, ifile)) {
//				fail(player, "eof reached\n");
//			}
//		} else {
//			if(!fread(player->iInputBuf+2, 1, KAAC_MAX_STREAMING_BUFFER_SIZE-2, ifile)) {
//				fail(player, "eof reached\n");
//			}
//			break;
//		}
//	}
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
   		
//   		if(!bufferUpdate(ifile, pExt)){
//   			exit(-1);
//   		}
   		pExt->inputBufferUsedLength = 0;
	}	

	fprintf(stderr, "[INIT] Synced\n");
	
	/* main loop */
	while (1) {
//		if(!bufferUpdate(ifile, pExt)){
//   			exit(0);
//   		}
   				
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
                aufile = open_audio_file(	argv[2], 
                							pExt->samplingRate,
                							pExt->desiredChannels,
                							FMT_16BIT,
                							OUTPUT_WAV,
                							0 /* write big header */);
				aIsFirstBuffer=0;
			}
			/* save decoded frames */
			write_audio_file(aufile, player->iOutputBuf, aOutputLength*pExt->desiredChannels, 0);

    	} else if (MP4AUDEC_INCOMPLETE_FRAME == Status) {
        	fprintf(stderr, "[STATUS] Status: MP4AUDEC_INCOMPLETE_FRAME\n");
        	break;
        } else {
        	fprintf(stderr, "[STATUS] Status: %s, eof?\n", Status==1?"UNKNOWN":"BAD");
       		break;
    	}
	}
	
	close_audio_file(aufile);
	freeall(player); 
	return 0;
}
