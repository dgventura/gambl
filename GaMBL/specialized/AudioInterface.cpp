
//
//  AudioInterface.cpp
//  GaMBL
//
//  Created by David Ventura on 4/11/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//
//  Note: Thanks to Tim Bolstad and James McCartney for their CoreAudio tutorials that
//  helped get this lightweight device wrapper running.
//
//  Tim: http://timbolstad.com/2010/03/14/core-audio-getting-started/
//  James: http://www.audiosynth.com
//

#include "AudioInterface.h"
#include "Music_Player.h"

using namespace std;

#define USE_AUGRAPH         0
#define DUMP_SAMPLES        0
#define DUMP_STATUS         0

#define WAVE_HEADER_SIZE    48

#if !USE_AUGRAPH
typedef struct {
    double freq, pan, amp;
    double phase;
	Boolean		soundPlaying;
	AudioDeviceID	device;			// the default device
	UInt32		deviceBufferSize;	// bufferSize returned by kAudioDevicePropertyBufferSize
    AudioStreamBasicDescription	deviceFormat;	// info about the default device
    AudioInterface* m_pAudioItfc;
    AudioDeviceIOProcID theIOProcID;
} appGlobals, *appGlobalsPtr, **appGlobalsHandle;

appGlobals gAppGlobals;
OSStatus  SetupHAL (appGlobalsPtr  globals);
OSStatus  StartPlayingThruHAL(appGlobalsPtr globals);
OSStatus  StopPlayingThruHAL(appGlobalsPtr globals);

float clamp( const float f1, const float f2, const float f3 )
{
    if ( f3 < f1 )
        return f1;
    else if ( f3 > f2 )
        return f2;
    return f3;
}

#include <iostream>
#include <fstream>

ifstream MyFile;

OSStatus appIOProc (AudioDeviceID  inDevice, const AudioTimeStamp*  inNow, const AudioBufferList*   inInputData,
                    const AudioTimeStamp*  inInputTime, AudioBufferList*  outOutputData, const AudioTimeStamp* inOutputTime,
                    void* appGlobals)
{
    appGlobalsPtr	globals = (appGlobalsPtr)appGlobals;
    
    // read more samples from the internal emulator, move the read pointer along
    gAppGlobals.m_pAudioItfc->m_pMusicPlayer->sound_callback();
 
    int i;
    double phase = gAppGlobals.phase;
    double amp = gAppGlobals.amp;
    double pan = gAppGlobals.pan;
    
    int numSamples = globals->deviceBufferSize / globals->deviceFormat.mBytesPerFrame;

    float *out = (float*)outOutputData->mBuffers[0].mData;
    const AudioInterface::sample_t* pReadBuffer = gAppGlobals.m_pAudioItfc->m_pSampleHead;
    for ( i = 0; i < numSamples; ++i )
    {
        if ( i < gAppGlobals.m_pAudioItfc->m_nSampleCount )
        {            
            *out++ = clamp( -1.0f, 1.0f, (*pReadBuffer / (float)SHRT_MAX) * (1.0-pan) * amp );
            ++pReadBuffer;
            *out++ = clamp( -1.0f, 1.0f, (*pReadBuffer / (float)SHRT_MAX) * pan * amp );
            ++pReadBuffer; //TODO: confirm why we need to increment read twice... I guess internally the samples are already stereo?
        }
    }
    gAppGlobals.phase = phase;

#if DUMP_STATUS
    printf( "AUD ITFC: smpl: %d, i:%.02f o:%.02f n:%.02f\n", numSamples, inInputTime->mSampleTime, inOutputTime->mSampleTime, inNow );
#endif

    //TODO: figure out why not calling this makes the music stop
    gAppGlobals.m_pAudioItfc->m_pMusicPlayer->is_done();
    
    return (kAudioHardwareNoError);
}

#else 

// Native iphone sample rate of 44.1kHz, same as a CD.
const float kGraphSampleRate = 44100.0f;

// audio render procedure, don't allocate memory, don't take any locks, don't waste time
static OSStatus renderInput(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
	// Get a reference to the object that was passed with the callback
	// In this case, the AudioController passed itself so
	// that you can access its data.
	AudioInterface *THIS = (AudioInterface*)inRefCon;
    
	// Get a pointer to the dataBuffer of the AudioBufferList
	AudioSampleType *outA = (AudioSampleType *)ioData->mBuffers[0].mData;
    
	// Calculations to produce a 600 Hz sinewave
	// A constant frequency value, you can pass in a reference vary this.
	float freq = 440;
	// The amount the phase changes in  single sample
	double phaseIncrement = M_PI * freq / 44100.0;
	// Pass in a reference to the phase value, you have to keep track of this
	// so that the sin resumes right where the last call left off
	float phase = THIS->sinPhase;
    
	float sinSignal;
	// Loop through the callback buffer, generating samples
	for (UInt32 i = 0; i < inNumberFrames; ++i) {
        
        // calculate the next sample
        sinSignal = sin(phase);
        // Put the sample into the buffer
        // Scale the -1 to 1 values float to
        // -32767 to 32767 and then cast to an integer
        //outA[i] = (SInt16)(sinSignal * 32767.0f);
        outA[i] = sinSignal;
        // calculate the phase for the next sample
        phase = phase + phaseIncrement;
    }
    // Reset the phase value to prevent the float from overflowing
    if (phase >=  M_PI * freq) {
		phase = phase - M_PI * freq;
	}
	// Store the phase for the next callback.
	THIS->sinPhase = phase;
    
	return noErr;
}
#endif

AudioInterface::AudioInterface()
{
    gAppGlobals.amp = 0.5f;
    gAppGlobals.pan = 0.5f;
}

// Clean up memory
AudioInterface::~AudioInterface()
{
    DisposeAUGraph(mGraph);
    
    MyFile.close();
}

void AudioInterface::set_gain( double dGain )
{
    gAppGlobals.amp = dGain;
}

void AudioInterface::play_buffer(const sample_t *pSampleBuffer, int nCount)
{
    if ( !m_bPlaying )
    {
    #if USE_AUGRAPH
        startAUGraph();
    #else
        gAppGlobals.phase = 0;
        gAppGlobals.freq = 440.0f;
        gAppGlobals.m_pAudioItfc = this;
        StartPlayingThruHAL( &gAppGlobals );
    #endif
    }
    else
    {
        // debug dump samples to stdout
#if DUMP_SAMPLES
        const int nStride = 128;
        printf( "S: ");
        char szLilBuf[24];
        char szBigBuf[nCount * sizeof(szLilBuf)];
        strcpy( szBigBuf, "" );
        
        const sample_t* pSmpl = pSampleBuffer;
        for ( int n = 0; n < nCount; n += nStride, pSmpl += nStride )
        {
            strcpy( szLilBuf, "" );
            sprintf( szLilBuf, "%4d ", *pSmpl );
            strcat( szBigBuf, szLilBuf );
        }
        printf( "%s\n", szBigBuf );
#endif
        
        m_pSampleHead = pSampleBuffer;
        m_nSampleCount = nCount;
    }
    
    m_bPlaying = true;
}

void AudioInterface::setup( double nSampleRate, bool bStereo, callback_t func, void* pData )
{
    m_pMusicPlayer = (Music_Player*)pData;
    m_MusicPlayerCallback = func;

#if USE_AUGRAPH
    initializeAUGraph( nSampleRate, bStereo );
#else
    SetupHAL( &gAppGlobals );
#endif
}

void AudioInterface::stop()
{
#if USE_AUGRAPH
    stopAUGraph();
#else
    StopPlayingThruHAL( &gAppGlobals );
#endif
    
    m_bPlaying = false;
}

// starts render
void AudioInterface::startAUGraph()
{
	// Start the AUGraph
	OSStatus result = AUGraphStart(mGraph);
    printf("AUGraphStart result %d %08X %4.4s\n", (int)result, (int)result, (char*)&result);
    while ( result != noErr )
    {
        printf("AUGraphStart result %d %08X %4.4s\n", (int)result, (int)result, (char*)&result);
        
        usleep(500);
        
        result = AUGraphStart(mGraph);
    }
}

// stops render
void AudioInterface::stopAUGraph()
{
    Boolean isRunning = false;
    
    // Check to see if the graph is running.
    OSStatus result = AUGraphIsRunning(mGraph, &isRunning);
    printf("AUGraphStop was running %d\n", (int)isRunning);
    // If the graph is running, stop it.
    if (isRunning) {
        result = AUGraphStop(mGraph);
    }
}

#if USE_AUGRAPH
//TODO: use sample rate?
void AudioInterface::initializeAUGraph( double nSampleRate, bool bStereo )
{
	//************************************************************
	//*** Setup the AUGraph, add AUNodes, and make connections ***
	//************************************************************
	// Error checking result
	OSStatus result = noErr;
    
	// create a new AUGraph
	result = NewAUGraph(&mGraph);
    assert( result == noErr );
    
    // AUNodes represent AudioUnits on the AUGraph and provide an
	// easy means for connecting audioUnits together.
    AUNode outputNode;
    AUNode mixerNode;
    
    // Create AudioComponentDescriptions for the AUs we want in the graph
    // mixer component
	AudioComponentDescription mixer_desc;
	mixer_desc.componentType = kAudioUnitType_Mixer;
	mixer_desc.componentSubType = kAudioUnitSubType_MultiChannelMixer;
	mixer_desc.componentFlags = 0;
	mixer_desc.componentFlagsMask = 0;
	mixer_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
	//  output component
	AudioComponentDescription output_desc;
	output_desc.componentType = kAudioUnitType_Output;
	output_desc.componentSubType = kAudioUnitSubType_HALOutput;//kAudioUnitSubType_DefaultOutput;
	output_desc.componentFlags = 0;
	output_desc.componentFlagsMask = 0;
	output_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    // Add nodes to the graph to hold our AudioUnits,
	// You pass in a reference to the  AudioComponentDescription
	// and get back an  AudioUnit
	result = AUGraphAddNode(mGraph, &output_desc, &outputNode);
    assert( result == noErr );
    	result = AUGraphAddNode(mGraph, &mixer_desc, &mixerNode );
    assert( result == noErr );
    
	// Now we can manage connections using nodes in the graph.
    // Connect the mixer node's output to the output node's input
	result = AUGraphConnectNodeInput(mGraph, mixerNode, 0, outputNode, 0);
    assert( result == noErr );
    
    // open the graph AudioUnits are open but not initialized (no resource allocation occurs here)
	result = AUGraphOpen(mGraph);
    assert( result == noErr );
    
	// Get a link to the mixer AU so we can talk to it later
	result = AUGraphNodeInfo(mGraph, mixerNode, NULL, &mMixer);
    assert( result == noErr );
    
	//************************************************************
	//*** Make connections to the mixer unit's inputs ***
	//************************************************************
    // Set the number of input busses on the Mixer Unit
	// Right now we are only doing a single bus.
	UInt32 numbuses = 1;
	UInt32 size = sizeof(numbuses);
    result = AudioUnitSetProperty(mMixer, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &numbuses, size);

	CAStreamBasicDescription desc;
    
	// Loop through and setup a callback for each source you want to send to the mixer.
	// Right now we are only doing a single bus so we could do without the loop.
	for (int i = 0; i < numbuses; ++i) {
        
		// Setup render callback struct
		// This struct describes the function that will be called
		// to provide a buffer of audio samples for the mixer unit.
		AURenderCallbackStruct renderCallbackStruct;
		renderCallbackStruct.inputProc = &renderInput;
		renderCallbackStruct.inputProcRefCon = (void*)this;
        
        // Set a callback for the specified node's specified input
        result = AUGraphSetNodeInputCallback(mGraph, mixerNode, i, &renderCallbackStruct);
        assert( result == noErr );
        
		// Get a CAStreamBasicDescription from the mixer bus.
        size = sizeof(desc);
        result = AudioUnitGetProperty(  mMixer,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input,
                                      i,
                                      &desc,
                                      &size);
        printf("Old Mixer file format: "); desc.Print();
		
        // Initializes the structure to 0 to ensure there are no spurious values.
        memset (&desc, 0, sizeof (desc));
        
		// Make modifications to the CAStreamBasicDescription
		// We're going to use 16 bit Signed Ints because they're easier to deal with
		// The Mixer unit will accept either 16 bit signed integers or
		// 32 bit 8.24 fixed point integers.
		desc.mSampleRate        = (Float64)kGraphSampleRate; // set sample rate
		desc.mFormatID          = kAudioFormatLinearPCM;
		desc.mFormatFlags       = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
		desc.mBitsPerChannel    = 8 * 2; // bytes per sample = 2
		desc.mChannelsPerFrame  = /*bStereo ? 2 :8*/ 1;
		desc.mFramesPerPacket   = 1;
		desc.mBytesPerFrame     = ( desc.mBitsPerChannel / 8 ) * desc.mChannelsPerFrame;
		desc.mBytesPerPacket    = desc.mBytesPerFrame * desc.mFramesPerPacket;
        
        CAStreamBasicDescription junk( (double)44100, 2, CAStreamBasicDescription::kPCMFormatFloat32, false );
        desc = junk;
        
		printf("New Mixer file format: ");
        desc.Print();
		// Apply the modified CAStreamBasicDescription to the mixer input bus
		result = AudioUnitSetProperty( mMixer, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, i, &desc, sizeof(desc) );
        assert( result == noErr );
         /**/
	}
    
	// Apply the CAStreamBasicDescription to the mixer output bus
	result = AudioUnitSetProperty( mMixer, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &desc, sizeof(desc) );
    assert( result == noErr );
    
	//************************************************************
	//*** Setup the audio output stream ***
	//************************************************************
    
	// Get a CAStreamBasicDescription from the output Audio Unit
    /*
     result = AudioUnitGetProperty(  mMixer,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output,
                                  0,
                                  &desc,
                                  &size);
    assert( result == noErr );
    
	// Initializes the structure to 0 to ensure there are no spurious values.
	memset (&desc, 0, sizeof (desc));
    
	// Make modifications to the CAStreamBasicDescription
	// AUCanonical on the iPhone is the 8.24 integer format that is native to the iPhone.
	// The Mixer unit does the format shifting for you.
	desc.SetAUCanonical(1, true);
	desc.mSampleRate = kGraphSampleRate;
    
    // Apply the modified CAStreamBasicDescription to the output Audio Unit
	result = AudioUnitSetProperty(  mMixer,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output,
                                  0,
                                  &desc,
                                  sizeof(desc));
    assert( result == noErr );
    */
    
  //  result = AudioUnitInitialize( mMixer );
  //  assert( result == noErr );
    
    // Once everything is set up call initialize to validate connections
	result = AUGraphInitialize(mGraph);
    assert( result == noErr );
}

#else

OSStatus	SetupHAL (appGlobalsPtr	globals)
{
    OSStatus				err = noErr;
    UInt32				count,
    bufferSize;
    AudioDeviceID			device = kAudioDeviceUnknown;
    AudioStreamBasicDescription		format;
    
    // get the default output device for the HAL
    count = sizeof(globals->device);					// it is required to pass the size of the data to be returned
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,  &count, (void *) &device);
    fprintf(stderr, "kAudioHardwarePropertyDefaultOutputDevice %d\n", err);
    if (err != noErr) goto Bail;
    
    // get the buffersize that the default device uses for IO
    count = sizeof(globals->deviceBufferSize);				// it is required to pass the size of the data to be returned
    err = AudioDeviceGetProperty(device, 0, false, kAudioDevicePropertyBufferSize, &count, &bufferSize);
    fprintf(stderr, "kAudioDevicePropertyBufferSize %d %d\n", err, bufferSize);
    if (err != noErr) goto Bail;
    
    // get a description of the data format used by the default device
    count = sizeof(globals->deviceFormat);				// it is required to pass the size of the data to be returned
    err = AudioDeviceGetProperty(device, 0, false, kAudioDevicePropertyStreamFormat, &count, &format);
    fprintf(stderr, "kAudioDevicePropertyStreamFormat %d\n", err);
    fprintf(stderr, "sampleRate %g\n", format.mSampleRate);
    fprintf(stderr, "mFormatFlags %08X\n", format.mFormatFlags);
    fprintf(stderr, "mBytesPerPacket %d\n", format.mBytesPerPacket);
    fprintf(stderr, "mFramesPerPacket %d\n", format.mFramesPerPacket);
    fprintf(stderr, "mChannelsPerFrame %d\n", format.mChannelsPerFrame);
    fprintf(stderr, "mBytesPerFrame %d\n", format.mBytesPerFrame);
    fprintf(stderr, "mBitsPerChannel %d\n", format.mBitsPerChannel);
    if (err != kAudioHardwareNoError) goto Bail;
    assert(format.mFormatID == kAudioFormatLinearPCM);	// bail if the format is not linear pcm
    
//    err = AudioDeviceSetProperty(device, 0, 0, true, AudioDevicePropertyID inPropertyID, <#UInt32 inPropertyDataSize#>, <#const void *inPropertyData#>)
    
    // everything is ok so fill in these globals
    globals->device = device;
    globals->deviceBufferSize = bufferSize;
    globals->deviceFormat = format;
    
Bail:
    return (err);
}

OSStatus  StartPlayingThruHAL(appGlobalsPtr	globals)
{
    OSStatus		err = kAudioHardwareNoError;
    
    if (globals->soundPlaying) return 0;
    globals->phase = 0.0;
    
    err = AudioDeviceCreateIOProcID(globals->device, appIOProc, (void *) globals, &globals->theIOProcID);	// setup our device with an IO proc
    if (err != kAudioHardwareNoError) goto Bail;
    
    err = AudioDeviceStart(globals->device, globals->theIOProcID);				// start playing sound through the device
    if (err != kAudioHardwareNoError) goto Bail;
    
    globals->soundPlaying = true;						// set the playing status global to true
    
Bail:
    return (err);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus  StopPlayingThruHAL(appGlobalsPtr	globals)
{
    OSStatus 	err = kAudioHardwareNoError;
    
    if (!globals->soundPlaying) return 0;
    err = AudioDeviceStop(globals->device, globals->theIOProcID);				// stop playing sound through the device
    if (err != kAudioHardwareNoError) goto Bail;
    
    err = AudioDeviceDestroyIOProcID(globals->device, globals->theIOProcID);			// remove the IO proc from the device
    if (err != kAudioHardwareNoError) goto Bail;
    
    globals->soundPlaying = false;						// set the playing status global to false
    
Bail:
    return (err);
}

#endif