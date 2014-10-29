//-----------------------------------------------------------------------------
// name: audio.h
// desc: audio stuff
//
// author: Matt Horton (mattah@ccrma.stanford.edu), after Ge Wang
//   date: 2014
//-----------------------------------------------------------------------------
#ifndef __AUDIO_H__
#define __AUDIO_H__




// init audio
bool audio_init( unsigned int srate, unsigned int frameSize, unsigned channels );
// start audio
bool audio_start();

// play some notes
void play( float pitch, float velocity );



#endif
