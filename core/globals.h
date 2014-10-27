//
//  globals.h
//  
//
//  Created by Matthew Horton on 10/24/14.
//  Based on bk-globals.h by Ge Wang
//


#include "x-def.h"
#include "x-audio.h"
#include "x-gfx.h"
#include "x-vector3d.h"
#include "y-waveform.h"
#include "y-entity.h"

// c++
#include <string>
#include <map>
#include <vector>
#include <utility>

// defines
#define BK_SRATE        44100
#define BK_FRAMESIZE    512
#define BK_NUMCHANNELS  2
#define BK_MAX_TEXTURES 32





// forward reference
class BKSim;

//-----------------------------------------------------------------------------
// name: class Globals
// desc: the global class
//-----------------------------------------------------------------------------
class Globals
{
public:
    // top level root simulation
    static BKSim * sim;
    
    static std::vector<YCubeOutline *> steps;
    static int numSteps;
    static int selectedStep;
    
    // path
    static std::string path;
    // path to datapath
    static std::string relpath;
    // datapath
    static std::string datapath;
    // version
    static std::string version;
    
    // last audio buffer
    static SAMPLE * lastAudioBuffer;
    static SAMPLE * lastAudioBufferMono;
    static SAMPLE * audioBufferWindow;
    static unsigned int lastAudioBufferFrames;
    static unsigned int lastAudioBufferChannels;
    
    // width and height of the window
    static GLsizei windowWidth;
    static GLsizei windowHeight;
    static GLsizei lastWindowWidth;
    static GLsizei lastWindowHeight;
    
    // graphics fullscreen
    static GLboolean fullscreen;
    // blend pane instead of clearing screen
    static GLboolean blendScreen;
    // blend screen parameters
    static Vector3D blendAlpha;
    static GLfloat blendRed;
    // fill mode
    static GLenum fillmode;
    // background color
    static iSlew3D bgColor;

};



