//-----------------------------------------------------------------------------
// name: audio.cpp
// desc: audio stuff for KeySeq
//
// author: Matt Horton (mattah@ccrma.stanford.edu), after Ge Wang
//   date: 2014
//-----------------------------------------------------------------------------
#include "audio.h"
#include "globals.h"
#include "Mediator.h"
#include "bk-sim.h"
#include "y-fft.h"
#include "y-fluidsynth.h"
#include "y-echo.h"
#include <iostream>
using namespace std;



// globals
YFluidSynth * g_synth;
YEcho * g_echo;
double g_now;
double g_nextTime;
int g_prog = 0;



struct Note
{
    int channel;
    float pitch;
    float velocity;
    float duration; // in seconds

    Note( int c, float p, float v, float d )
    {
        channel = c;
        pitch = p;
        velocity = v;
        duration = d;
    }
};




vector<Note> g_notes;
int g_noteIndex = 0;
XMutex g_mutex;

// play some notes
void play(float pitch, float velocity )
{
    //cerr << "pitch：" << pitch << " " << velocity << endl;
    // g_synth->noteOn( 0, pitch, velocity * 128 );

    // lock
    g_mutex.acquire();
    // clear notes
    g_notes.clear();

    g_notes.push_back( Note(0, pitch, velocity, .15/**(1 - i/24.0) */) );

    // unlock
    g_mutex.release();

    // reset the index
    g_noteIndex = 0;

    // play now!
    g_nextTime = g_now;
}



//-----------------------------------------------------------------------------
// name: audio_callback
// desc: audio callback
//-----------------------------------------------------------------------------
static void audio_callback( SAMPLE * buffer, unsigned int numFrames, void * userData )
{
    // zero out for output
    memset( buffer, 0, sizeof(SAMPLE)*numFrames*XAudioIO::numChannels() );

    g_now += numFrames;

    if( g_now > g_nextTime )
    {
        g_mutex.acquire();
        if( g_noteIndex < g_notes.size() )
        {
            g_synth->noteOn( 0, g_notes[g_noteIndex].pitch, g_notes[g_noteIndex].velocity * 127 );
            g_synth->noteOn( 0, g_notes[g_noteIndex].pitch + 4, g_notes[g_noteIndex].velocity * 127 );
            g_nextTime += g_notes[g_noteIndex].duration * THE_SRATE;
            g_noteIndex++;
        }
        g_mutex.release();
    }

    int sampCount = Globals::mediator->getCurrentSamp();

    // fill
    for( int i = 0; i < numFrames; i++ )
    {
        sampCount++;

        Globals::mediator->updateCount(sampCount);
    }

    // synthesize it
    g_synth->synthesize2( buffer, numFrames );

}




//-----------------------------------------------------------------------------
// name: audio_init()
// desc: initialize audio system
//-----------------------------------------------------------------------------
bool audio_init( unsigned int srate, unsigned int frameSize, unsigned channels )
{
    // initialize
    if( !XAudioIO::init( 0, 0, srate, frameSize, channels, audio_callback, NULL ) )
    {
        // done
        return false;
    }

    g_synth = new YFluidSynth();
    g_synth->init( srate, 32 );
    g_synth->load( "data/sfonts/jRhodes3c-stereo.sf2", "" );
    g_synth->programChange( 0, 0 );

    // allocate echo
    g_echo = new YEcho( srate );
    g_echo->setDelay( 0, .25 );
    g_echo->setDelay( 1, .5 );

    // allocate
    Globals::lastAudioBuffer = new SAMPLE[frameSize*channels];
    // allocate mono buffer
    Globals::lastAudioBufferMono = new SAMPLE[frameSize];
    // allocate window buffer
    Globals::audioBufferWindow = new SAMPLE[frameSize];
    // set frame size (could have changed in XAudioIO::init())
    Globals::lastAudioBufferFrames = frameSize;
    // set num channels
    Globals::lastAudioBufferChannels = channels;

    // compute the window
    hanning( Globals::audioBufferWindow, frameSize );

    return true;
}




//-----------------------------------------------------------------------------
// name: vq_audio_start()
// desc: start audio system
//-----------------------------------------------------------------------------
bool audio_start()
{
    // start the audio
    if( !XAudioIO::start() )
    {
        // done
        return false;
    }

    return true;
}
