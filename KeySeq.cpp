//-----------------------------------------------------------------------------
// name: KeySeq.cpp
// desc: step sequencer for hw3
//
// author: Matt Horton (mattah@ccrma.stanford.edu)
//   date: fall 2014
//   uses: RtAudio by Gary Scavone
//-----------------------------------------------------------------------------
#include "RtAudio.h"
#include "color.h"
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <sys/time.h>
#include <time.h>
#include "y-entity.h"
#include "globals.h"
#include "bk-sim.h"
using namespace std;

#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#ifdef __UNIX_JACK__
#include <GL/glut.h>
#endif




//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void initGfx();
void idleFunc();
void displayFunc();
void reshapeFunc( GLsizei width, GLsizei height );
void keyboardFunc( unsigned char, int, int );
void specialFunc( int, int, int );
void mouseFunc( int button, int state, int x, int y );
void help();
void initSeq();
void selectStep(int prev, int idx);


// our datetype
#define SAMPLE float
// corresponding format for RtAudio
#define MY_FORMAT RTAUDIO_FLOAT32
// sample rate
#define MY_SRATE 44100
// number of channels
#define MY_CHANNELS 1
// for convenience
#define MY_PIE 3.14159265358979

// width and height
long g_width = 1024;
long g_height = 720;
long g_last_width = g_width;
long g_last_height = g_height;

// refresh rate settings
long time_pre = 0;
int refresh_rate = 15000; //us
struct timeval timer;
// global buffer
long g_bufferSize;

#ifdef __UNIX_JACK__
GLboolean g_fullscreen = gl_FALSE;
#else
GLboolean g_fullscreen = FALSE;
#endif


//-----------------------------------------------------------------------------
// name: callme()
// desc: audio callback
//-----------------------------------------------------------------------------
int callme( void * outputBuffer, void * inputBuffer, unsigned int numFrames,
            double streamTime, RtAudioStreamStatus status, void * data )
{
    // cast!
    SAMPLE * input = (SAMPLE *)inputBuffer;
    SAMPLE * output = (SAMPLE *)outputBuffer;
    
    // fill
    for( int i = 0; i < numFrames; i++ )
    {
        // zero output
        output[i] = 0;
    }
    
    return 0;
}

//-----------------------------------------------------------------------------
// Name: help( )
// Desc: print usage
//-----------------------------------------------------------------------------
void help()
{
//    cerr << "----------------------------------------------------" << endl;
//    cerr << "sound-sphere (v1.0)" << endl;
//    cerr << "Matt Horton" << endl;
//    cerr << "http://ccrma.stanford.edu/~mattah/256a/sound-sphere/" << endl;
//    cerr << "----------------------------------------------------" << endl;
//    cerr << " All modifier keys can be used in their capital form" << endl;
//    cerr << endl;
//    cerr << "'h' - print this help message" << endl;
//    cerr << "'m' - toggle fullscreen" << endl;
//    cerr << "'q' - quit visualization" << endl;
//    cerr << "'c' - show/hide circular signal spectrum (will hide sphere if shown)" << endl;
//    cerr << "'s' - show/hide spherical signal spectrum" << endl;
//    cerr << "'f' - toggle drawing of historical spectra" << endl;
//    cerr << "'w' - show/hide time-domain window visualization" << endl;
//    cerr << "'p' - toggle party mode" << endl;
//    cerr << "'a' - toggle max averaging in party mode. Makes color change more smoothly." << endl;
//    cerr << "'b' - toggle buggy...er...awesome mode" << endl;
//    cerr << "'r' - toggle rotation" << endl;
//    cerr << endl;
//    cerr << "radius controls:" << endl;
//    cerr << "Press or hold the up and down keys to increase or " << endl;
//    cerr << "decrease (respectively) the radius of the sphere or circle." << endl;
//    cerr << endl;
//    cerr << "rotation controls:" << endl;
//    cerr << "Press or hold the left and right keys to rotate about the y axis." << endl;
//    cerr << "----------------------------------------------------" << endl;
}



//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main( int argc, char ** argv )
{
    // instantiate RtAudio object
    RtAudio audio;
    // variables
    unsigned int bufferBytes = 0;
    // frame size
    unsigned int bufferFrames = 512;
    
    // check for audio devices
    if( audio.getDeviceCount() < 1 )
    {
        // nopes
        cout << "no audio devices found!" << endl;
        exit( 1 );
    }

    // initialize GLUT
    glutInit( &argc, argv );
    // init gfx
    initGfx();
    
    // instantiate simulation
    Globals::sim = new BKSim();
    
    // Draw sequencer at start
    initSeq();

    // let RtAudio print messages to stderr.
    audio.showWarnings( true );

    // set input and output parameters
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = audio.getDefaultInputDevice();
    iParams.nChannels = MY_CHANNELS;
    iParams.firstChannel = 0;
    oParams.deviceId = audio.getDefaultOutputDevice();
    oParams.nChannels = MY_CHANNELS;
    oParams.firstChannel = 0;
    
    // create stream options
    RtAudio::StreamOptions options;

    // go for it
    try {
        // open a stream
        audio.openStream( &oParams, &iParams, MY_FORMAT, MY_SRATE, &bufferFrames, &callme, (void *)&bufferBytes, &options );
    }
    catch( RtError& e )
    {
        // error!
        cout << e.getMessage() << endl;
        exit( 1 );
    }

    // compute
    bufferBytes = bufferFrames * MY_CHANNELS * sizeof(SAMPLE);
    // allocate global buffer
    g_bufferSize = bufferFrames;
    
    // print help
    help();
    
    // go for it
    try {
        // start stream
        audio.startStream();

        // let GLUT handle the current thread from here
        glutMainLoop();
        
        // stop the stream.
        audio.stopStream();
    }
    catch( RtError& e )
    {
        // print error message
        cout << e.getMessage() << endl;
        goto cleanup;
    }
    
cleanup:
    // close if open
    if( audio.isStreamOpen() )
        audio.closeStream();
    
    // done
    return 0;
}




//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void initGfx()
{
    // double buffer, use rgb color, enable depth buffer
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    // initialize the window size
    glutInitWindowSize( g_width, g_height );
    // set the window postion
    glutInitWindowPosition( 100, 100 );
    // create the window
    glutCreateWindow( "KeySeq" );
    
    // set the idle function - called when idle
    glutIdleFunc( idleFunc );
    // set the display function - called when redrawing
    glutDisplayFunc( displayFunc );
    // set the reshape function - called when client area changes
    glutReshapeFunc( reshapeFunc );
    // set the keyboard function - called on keyboard events
    glutKeyboardFunc( keyboardFunc );
    // set the special keyboard function - called on special keyboard events
    glutSpecialFunc( specialFunc );
    // set the mouse function - called on mouse stuff
    glutMouseFunc( mouseFunc );
    
    // set clear color
    // set the GL clear color - use when the color buffer is cleared
    glClearColor( Globals::bgColor.actual().x, Globals::bgColor.actual().y, Globals::bgColor.actual().z, 1.0f );
    // enable color material
    glEnable( GL_COLOR_MATERIAL );
    // enable depth test
    glEnable( GL_DEPTH_TEST );
}




//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void reshapeFunc( GLsizei w, GLsizei h )
{
    // save the new window size
    g_width = w; g_height = h;
    // map the view port to the client area
    glViewport( 0, 0, w, h );
    // set the matrix mode to project
    glMatrixMode( GL_PROJECTION );
    // load the identity matrix
    glLoadIdentity( );
    // create the viewing frustum
    gluPerspective( 45.0, (GLfloat) w / (GLfloat) h, 1.0, 300.0 );
    // set the matrix mode to modelview
    glMatrixMode( GL_MODELVIEW );
    // load the identity matrix
    glLoadIdentity( );
    // position the view point
    gluLookAt( 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}




//-----------------------------------------------------------------------------
// Name: keyboardFunc( )
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y )
{
    int prev;
    
    switch( key )
    {
        case 'Q':
        case 'q':
            exit(1);
            break;
        case 'G':
        case 'g': // toggle fullscreen
        {
            // check fullscreen
            if( !g_fullscreen )
            {
                g_last_width = g_width;
                g_last_height = g_height;
                glutFullScreen();
            }
            else
                glutReshapeWindow( g_last_width, g_last_height );
            
            // toggle variable value
            g_fullscreen = !g_fullscreen;
            break;
        }
        case 'A':
        case 'a':
            prev = Globals::selectedStep;
            Globals::selectedStep = 0;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'S':
        case 's':
            prev = Globals::selectedStep;
            Globals::selectedStep = 1;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'D':
        case 'd':
            prev = Globals::selectedStep;
            Globals::selectedStep = 2;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'F':
        case 'f':
            prev = Globals::selectedStep;
            Globals::selectedStep = 3;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'J':
        case 'j':
            prev = Globals::selectedStep;
            Globals::selectedStep = 4;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'K':
        case 'k':
            prev = Globals::selectedStep;
            Globals::selectedStep = 5;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'L':
        case 'l':
            prev = Globals::selectedStep;
            Globals::selectedStep = 6;
            selectStep(prev, Globals::selectedStep);
            break;
        case ':':
        case ';':
            prev = Globals::selectedStep;
            Globals::selectedStep = 7;
            selectStep(prev, Globals::selectedStep);
            break;
    }

    
    glutPostRedisplay( );
}

//-----------------------------------------------------------------------------
// Name: specialFunc( )
// Desc: special key event
//-----------------------------------------------------------------------------
void specialFunc(int key, int x, int y) {
    if (key == GLUT_KEY_UP) {
    } else if (key == GLUT_KEY_DOWN) {
    } else if (key == GLUT_KEY_RIGHT) {
        //Globals::cube->size = Globals::cube->size + Vector3D::Vector3D(2,5,6);
    } else if (key == GLUT_KEY_LEFT) {
        //Globals::cube->size = Globals::cube->size - Vector3D::Vector3D(1,1,1);
    }
}


//-----------------------------------------------------------------------------
// Name: mouseFunc( )
// Desc: handles mouse stuff
//-----------------------------------------------------------------------------
void mouseFunc( int button, int state, int x, int y )
{
    if( button == GLUT_LEFT_BUTTON )
    {
        // when left mouse button is down
        if( state == GLUT_DOWN )
        {
        }
        else
        {
        }
    }
    else if ( button == GLUT_RIGHT_BUTTON )
    {
        // when right mouse button down
        if( state == GLUT_DOWN )
        {
        }
        else
        {
        }
    }
    else
    {
    }
    
    glutPostRedisplay( );
}




//-----------------------------------------------------------------------------
// Name: idleFunc( )
// Desc: callback from GLUT
//-----------------------------------------------------------------------------
void idleFunc( )
{
    // render the scene
    glutPostRedisplay( );
}



//-----------------------------------------------------------------------------
// Name: displayFunc( )
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc( )
{
    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    // cascade simulation
    Globals::sim->systemCascade();
    
    // flush!
    glFlush( );
    // swap the double buffer
    glutSwapBuffers( );
}


//-----------------------------------------------------------------------------
// Name: initSeq( )
// Desc: Draw and init the sequencer
//-----------------------------------------------------------------------------
void initSeq() {
    
    for (int i = 0; i < 8; i++) {
        YCubeOutline * cube = new YCubeOutline();
        cube->loc = Vector3D::Vector3D((i-3.7)+(i+.7)*.1,0,0);
        cube->sca = Vector3D::Vector3D(1,1,1);
        cube->outlineColor = Vector3D::Vector3D(0.655, 0.859, 0.859);
        cube->col = Vector3D::Vector3D(0.412, 0.824, 0.906);
        //cube->ori = Vector3D::Vector3D(0,11,0);
        cube->ori = Vector3D::Vector3D(0,0,0);
        Globals::sim->root().addChild(cube);
        Globals::steps.push_back(cube);
    }
    
    for(int i = 0; i != Globals::steps.size(); i++){
        cerr << i << endl;
    }
}

//-----------------------------------------------------------------------------
// Name: selectStep( )
// Desc: Select the step we're interested in modifying
//-----------------------------------------------------------------------------
void selectStep(int prev, int idx){
    YCubeOutline * step = NULL;
    YCubeOutline * nextStep = NULL;
    
    cerr << prev << idx << endl;
    if (!(prev == -1)) {
        step = Globals::steps.at(prev);
        step->col = Vector3D::Vector3D(0.412, 0.824, 0.906);
    }
    nextStep = Globals::steps.at(idx);
    cerr << "got here" << endl;
    nextStep->col.set(0.953f, 0.525f, 0.188f);
    cerr << "got here" << endl;
}
