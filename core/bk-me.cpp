//-----------------------------------------------------------------------------
// name: bk-me.cpp
// desc: audio stuff
//
// author: Ge Wang (ge@ccrma.stanford.edu)
//   date: 2013
//-----------------------------------------------------------------------------
#include "bk-me.h"
#include <iostream>
using namespace std;




//-----------------------------------------------------------------------------
// init audio
//-----------------------------------------------------------------------------
bool bk_audio_init()
{
    return true;
}




//-----------------------------------------------------------------------------
// name: BKSynth()
// desc: ...
//-----------------------------------------------------------------------------
BKSynth::BKSynth()
{
    // zero out
    m_buffer = NULL;
    m_srate = 0;
    m_now = 0;
    // set pause ramp
    m_pauseRamp.set( 1, 1, 2 );
    // set envelope
    m_envelope.set( 1, 1, 1 );
}




//-----------------------------------------------------------------------------
// name: ~BKSynth()
// desc: ...
//-----------------------------------------------------------------------------
BKSynth::~BKSynth()
{
    // delete buffer
    SAFE_DELETE_ARRAY( m_buffer );
    // delete previous
    for( int i = 0; i < m_previous.size(); i++ )
        SAFE_DELETE( m_previous[i] );
    // zero out
    m_srate = 0;
}




//-----------------------------------------------------------------------------
// name: init()
// desc: init
//-----------------------------------------------------------------------------
void BKSynth::init( int srate, int frameSize, int polyphony, int numChannels )
{
    // set
    m_srate = srate;
    // initialize
    m_synth.init( srate, polyphony );
    
    // delete previous
    for( int i = 0; i < m_previous.size(); i++ ) SAFE_DELETE( m_previous[i] );
    // allocate previous
    m_previous.resize( numChannels );
    // zero out
    for( int i = 0; i < m_previous.size(); i++ ) m_previous[i] = NULL;
    
    // check
    if( m_buffer != NULL ) SAFE_DELETE_ARRAY( m_buffer );
    // allocate (stereo)
    m_buffer = new float[frameSize * 2];
}




//-----------------------------------------------------------------------------
// name: loadFont()
// desc: load font
//-----------------------------------------------------------------------------
void BKSynth::loadFont( const std::string & name, const std::string & ext )
{
    // get the path
    string path = name;
    
    // log
    cerr << "[bk-synth]: loading soundfont: " <<  name << "."  << ext << "..." << endl;
    
    // load soundfont
    if( !m_synth.load( path.c_str(), ext.c_str() ) )
    {
        // log
        cerr << "[bk-synth]: *** ERROR *** loading soundfont!" << endl;
        // done
        return;
    }
}




//-----------------------------------------------------------------------------
// name: programChange()
// desc: change program
//-----------------------------------------------------------------------------
void BKSynth::programChange( int data1, int data2 )
{
    // change sound
    m_synth.programChange( data1, data2 );
}




//-----------------------------------------------------------------------------
// reset (clear everything)
//-----------------------------------------------------------------------------
void BKSynth::reset()
{
    // clear chord
    clearAllChords();
    // all note oof
    m_synth.allNotesOff( 0 );
}




//-----------------------------------------------------------------------------
// name: clearAllChords()
// desc: clear all chanell chords
//-----------------------------------------------------------------------------
void BKSynth::clearAllChords()
{
    // iterate
    for( int i = 0; i < m_previous.size(); i++ )
    {
        // clear channel
        clearChord( i );
    }
}




//-----------------------------------------------------------------------------
// name: playNotes()
// desc: play one or more notes (simultaneous)
//-----------------------------------------------------------------------------
void BKSynth::playNotes( BKNoteEvent * e )
{
    // sanity check
    if( e->channel < 0 || e->channel >= m_previous.size() )
    {
        // message
        cerr << "[bk-synth]: WARNING: invalid note channel: " << e->channel << endl;
        return;
    }
    
    // clear
    clearChord( e->channel );
    
    // save it
    if( e ) m_previous[e->channel] = new BKNoteEvent( e );
    
    // pointer
    BKNoteEvent * curr = e ? m_previous[e->channel] : NULL;
    
    // iterate
    while( curr )
    {
        // set start time
        curr->synthStartTime = m_now;
        // if duration not zero set end time
        if( curr->duration > 0 ) curr->synthStopTime = m_now + curr->duration*m_srate;
        // note on
        m_synth.noteOn( curr->channel, curr->pitch, curr->velocity * 127 );
        // next simultaneous
        curr = curr->simultaneous;
    }
    
    // reset envelope
    // m_envelope.value = 0.05;
    // m_envelope.update( 1, .09 );
}




//-----------------------------------------------------------------------------
// ramp down chord
//-----------------------------------------------------------------------------
void BKSynth::rampDownChord()
{
    // reset envelope
    m_envelope.update( 0.12, 1.5 );
}




//-----------------------------------------------------------------------------
// clear a chord
//-----------------------------------------------------------------------------
void BKSynth::clearChord( int channel )
{
    // pointer
    BKNoteEvent * curr = m_previous[channel];
    
    // iterate
    while( curr )
    {
        // note off
        // m_synth->noteOff( 0, m_prevChord[i] );
        // note off
        m_synth.noteOn( curr->channel, curr->pitch, 0 );
        // next
        curr = curr->simultaneous;
    }
    
    // delete it
    SAFE_DELETE( m_previous[channel] );
}




//-----------------------------------------------------------------------------
// synthesize
//-----------------------------------------------------------------------------
void BKSynth::synthesize2( float * buffer, unsigned int numFrames )
{
    // sanity check
    assert( m_srate > 0 );
    // TODO: why is this here?
    assert( numFrames <= 2048 );
    
    // increment now
    m_now += numFrames;
    
    // iterate
    for( int i = 0; i < m_previous.size(); i++ )
    {
        // check end time
        if( m_previous[i] && m_previous[i]->synthStopTime > 0
           && m_previous[i]->synthStopTime < m_now )
        {
            // stop that channel
            clearChord( i );
        }
    }
    
    // synthesize stereo
    m_synth.synthesize2( m_buffer, numFrames );
    
    // interp
    m_envelope.interp( (float)numFrames / m_srate );
    m_pauseRamp.interp( (float)numFrames / m_srate );
    
    // HACK: upward
    if( m_envelope.goal > .9 )
    {
        if( m_envelope.value > .3 ) m_envelope.slew = .6;
        else if( m_envelope.value > .2 ) m_envelope.slew = .3;
        else if( m_envelope.value > .15 ) m_envelope.slew = .2;
    }
    
    // apply gain factor and copy to outbound buffer
    for( int i = 0; i < numFrames; i++ )
    {
        buffer[i*2] = m_buffer[i*2] * m_envelope.value * m_pauseRamp.value;
        buffer[i*2+1] = m_buffer[i*2+1] * m_envelope.value * m_pauseRamp.value;
    }
}
