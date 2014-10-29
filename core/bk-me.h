//-----------------------------------------------------------------------------
// name: bk-me.h
// desc: audio/music engine
//
// author: Ge Wang (ge@ccrma.stanford.edu)
//   date: 2013
//-----------------------------------------------------------------------------
#ifndef __BK_MUSIC_ENGINE_H__
#define __BK_MUSIC_ENGINE_H__

#include "x-def.h"
#include "x-audio.h"
#include "x-vector3d.h"
#include "y-fluidsynth.h"
#include "y-echo.h"
#include <string>
#include <vector>
#include <map>




//-----------------------------------------------------------------------------
// name: struct BKNoteEvent
// desc: a MIDI note event, parsed for start and end time, with simultaneous
//       notes combined into the 'simultaneous' linked list
//-----------------------------------------------------------------------------
struct BKNoteEvent
{
    // data
    long type;
    unsigned short channel;
    unsigned short pitch;
    float velocity;
    float energy;
    
    // more data
    double duration; // in seconds
    double startTime; // in seconds
    double endTime; // in second
    long transition;
    
    // internal use (e.g., by synth)
    double synthStartTime;
    double synthStopTime;
    
    // link to simultaneous events
    BKNoteEvent * simultaneous;
    // link to parent (could be self)
    BKNoteEvent * next;
    
    // constructor
    BKNoteEvent() : type(0), channel(0), pitch(0), velocity(0), energy(1),
    simultaneous(NULL), next(NULL), duration(0), startTime(0),
    endTime(0), synthStartTime(0), synthStopTime(0), transition(0) { }
    
    // copy constructor (does not copy next)
    BKNoteEvent( const BKNoteEvent * rhs )
    {
        // shallow copy
        *this = *rhs;
        // set next to NULL (does not copy at all)
        this->next = NULL;
        // new simultaneous
        if( this->simultaneous != NULL )
        {
            // allocate and copy
            this->simultaneous = new BKNoteEvent( rhs->simultaneous );
        }
    }
    
    // destructor
    ~BKNoteEvent()
    {
        // if simultaneous
        if( this->simultaneous != NULL )
        {
            // delete child
            SAFE_DELETE( this->simultaneous );
        }
        
        // if next
        // TODO: does this iteratively instead of recursively
        if( this->next != NULL )
        {
            // delete next
            SAFE_DELETE( this->next );
        }
    }
};




//-----------------------------------------------------------------------------
// name: class BKSynth
// desc: synth wrapper
//-----------------------------------------------------------------------------
class BKSynth
{
public:
    BKSynth();
    ~BKSynth();
    
public:
    // init
    void init( int srate, int frameSize, int polyphony, int numChannels );
    // load font
    void loadFont( const std::string & name, const std::string & ext );
    // change program
    void programChange( int data1, int data2 );
    
public:
    // reset (clear everything)
    void reset();
    // play one or more notes (simultaneous)
    void playNotes( BKNoteEvent * e );
    // ramp down chord
    void rampDownChord();
    // clear a chord
    void clearChord( int channel );
    // clear all chores
    void clearAllChords();
    
public:
    // pause
    void pause() { m_pauseRamp.update( 0 ); }
    // unpause
    void unpause() { m_pauseRamp.update( 1 ); }
    // get the envelope
    Vector3D & normalEnvelope() { return m_envelope; }
    
public:
    // get the music engine now
    double now() { return m_now; };
    // get sample rate
    int srate() { return m_srate; }
    // get the underlying synth
    YFluidSynth & synth() { return m_synth; }
    // synthesize
    void synthesize2( float * buffer, unsigned int numFrames );
    
protected:
    // the synth
    YFluidSynth m_synth;
    // the envelope
    Vector3D m_envelope;
    // pause ramp
    Vector3D m_pauseRamp;
    // prevous note event
    std::vector<BKNoteEvent *> m_previous;
    
protected:
    // the buffer
    float * m_buffer;
    // sample rate
    int m_srate;
    
protected:
    // the system now;
    double m_now;
};




#endif
