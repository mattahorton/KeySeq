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
#include "Mediator.h"
#include "audio.h"
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
void playStep(int prev, int idx);
int sampsPerBeat(float bpm);
RtAudio initAudio();
void startAudio(RtAudio audio);
void decrementOctave(int num);
void incrementOctave(int num);
void decrementPitch(int num);
void incrementPitch(int num);
void incrementVel(int num);
void decrementVel(int num);
void snapStepLocs();
void decTrack();
void incTrack();
void setLineColors();

// our datetype
#define SAMPLE float
// corresponding format for RtAudio
#define MY_FORMAT RTAUDIO_FLOAT32
// for convenience
#define MY_PIE 3.14159265358979

// refresh rate settings
long time_pre = 0;
int refresh_rate = 15000; //us
struct timeval timer;
// global buffer
long g_bufferSize;



//-----------------------------------------------------------------------------
// Name: help( )
// Desc: print usage
//-----------------------------------------------------------------------------
void help()
{
   cerr << "----------------------------------------------------" << endl;
   cerr << "keyseq (v1.0)" << endl;
   cerr << "Matt Horton" << endl;
   cerr << "http://ccrma.stanford.edu/~mattah/256a/keyseq/" << endl;
   cerr << "----------------------------------------------------" << endl;
   cerr << "Select a step by pressing one of the home keys (a, s, d, f, etc.)" << endl;
   cerr << endl;
   cerr << "Press Enter to turn a step on or off." << endl;
   cerr << endl;
   cerr << "The keys above and below the selection keys modify the value " << endl;
   cerr << "of the current paramater for the step to which they correspond. " << endl;
   cerr << endl;
   cerr << "So, Q and Z will raise and lower pitch, octave, or velocity " << endl;
   cerr << "for the step selected by A." << endl;
   cerr << endl;
   cerr << "Default paramater is pitch. Cycle through parameters using " << endl;
   cerr << "right and left arrows. Up and down arrows move through tracks." << endl;
   cerr << "----------------------------------------------------" << endl;
}



//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main( int argc, char ** argv )
{
    // init audio
    audio_init(THE_SRATE, FRAMESIZE, NUMCHANNELS);

    // initialize GLUT
    glutInit( &argc, argv );
    // init gfx
    initGfx();

    // instantiate simulation
    Globals::sim = new BKSim();

    // instantiate global Mediator
    Globals::mediator = new Mediator();
    Globals::mediator->registerCallback(10000,&playStep);

    // Draw sequencer at start
    initSeq();

    // Midi setup
    for (int j = 0; j < 4; j++) {
      for (int i = 0; i < Globals::steps.size(); i++) {
        Globals::octaveOffsets[j].push_back(0);
        Globals::pitchOffsets[j].push_back(0);
        Globals::velOffsets[j].push_back(10);
        Globals::stepBools[j].push_back(false);
      }
    }
    snapStepLocs();

    // Start Audio
    audio_start();

    // print help
    help();

    // let GLUT handle the current thread from here
    glutMainLoop();
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
    glutInitWindowSize( Globals::windowWidth, Globals::windowHeight );
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
    Globals::windowWidth = w; Globals::windowHeight = h;
    // map the view port to the client area
    glViewport( 0, 0, w, h );
    // set the matrix mode to project
    glMatrixMode( GL_PROJECTION );
    // load the identity matrix
    glLoadIdentity( );
    // create the viewing frustum
    gluPerspective( 50.0, (GLfloat) w / (GLfloat) h, 0.0005, 500.0 );
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
    YEntity * step;
    bool currentVal;

    switch( key )
    {
        case 27: // escape
            exit(1);
            break;
        case 13: // enter
            if(Globals::selectedStep != -1) {
              currentVal = Globals::stepBools[Globals::currentTrack].at(Globals::selectedStep);

              Globals::stepBools[Globals::currentTrack].at(Globals::selectedStep) = !currentVal;

              setLineColors();

            }
            break;
        case 'G':
        case 'g': // toggle fullscreen
        {
            // check fullscreen
            if( !Globals::fullscreen )
            {
                Globals::lastWindowWidth = Globals::windowWidth;
                Globals::lastWindowHeight = Globals::windowHeight;
                glutFullScreen();
            }
            else
                glutReshapeWindow( Globals::lastWindowWidth, Globals::lastWindowHeight );

            // toggle variable value
            Globals::fullscreen = !Globals::fullscreen;
            break;
        }
        case 'A':
        case 'a':
            prev = Globals::selectedStep;
            Globals::selectedStep = 0;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'Q':
        case 'q':
            switch(Globals::mode) {
              case PITCH:
                incrementPitch(0);
                break;
              case OCTAVE:
                incrementOctave(0);
                break;
              case VELOCITY:
                incrementVel(0);
                break;
            }

            break;
        case 'Z':
        case 'z':
            switch(Globals::mode) {
              case PITCH:
                decrementPitch(0);
                break;
              case OCTAVE:
                decrementOctave(0);
                break;
              case VELOCITY:
                decrementVel(0);
                break;
            }

            break;
        case 'S':
        case 's':
            prev = Globals::selectedStep;
            Globals::selectedStep = 1;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'W':
        case 'w':
            switch(Globals::mode) {
              case PITCH:
                incrementPitch(1);
                break;
              case OCTAVE:
                incrementOctave(1);
                break;
              case VELOCITY:
                incrementVel(1);
                break;
            }

            break;
        case 'X':
        case 'x':
            switch(Globals::mode) {
              case PITCH:
                decrementPitch(1);
                break;
              case OCTAVE:
                decrementOctave(1);
                break;
              case VELOCITY:
                decrementVel(1);
                break;
            }

            break;
        case 'D':
        case 'd':
            prev = Globals::selectedStep;
            Globals::selectedStep = 2;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'E':
        case 'e':
            switch(Globals::mode) {
              case PITCH:
                incrementPitch(2);
                break;
              case OCTAVE:
                incrementOctave(2);
                break;
              case VELOCITY:
                incrementVel(2);
                break;
            }

            break;
        case 'C':
        case 'c':
            switch(Globals::mode) {
              case PITCH:
                decrementPitch(2);
                break;
              case OCTAVE:
                decrementOctave(2);
                break;
              case VELOCITY:
                decrementVel(2);
                break;
            }

            break;
        case 'F':
        case 'f':
            prev = Globals::selectedStep;
            Globals::selectedStep = 3;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'R':
        case 'r':
            switch(Globals::mode) {
              case PITCH:
                incrementPitch(3);
                break;
              case OCTAVE:
                incrementOctave(3);
                break;
              case VELOCITY:
                incrementVel(3);
                break;
            }

            break;
        case 'V':
        case 'v':
            switch(Globals::mode) {
              case PITCH:
                decrementPitch(3);
                break;
              case OCTAVE:
                decrementOctave(3);
                break;
              case VELOCITY:
                decrementVel(3);
                break;
            }

            break;
        case 'J':
        case 'j':
            prev = Globals::selectedStep;
            Globals::selectedStep = 4;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'U':
        case 'u':
            switch(Globals::mode) {
              case PITCH:
                incrementPitch(4);
                break;
              case OCTAVE:
                incrementOctave(4);
                break;
              case VELOCITY:
                incrementVel(4);
                break;
            }

            break;
        case 'M':
        case 'm':
            switch(Globals::mode) {
              case PITCH:
                decrementPitch(4);
                break;
              case OCTAVE:
                decrementOctave(4);
                break;
              case VELOCITY:
                decrementVel(4);
                break;
            }

            break;
        case 'K':
        case 'k':
            prev = Globals::selectedStep;
            Globals::selectedStep = 5;
            selectStep(prev, Globals::selectedStep);
            break;

        case 'I':
        case 'i':
            switch(Globals::mode) {
              case PITCH:
                incrementPitch(5);
                break;
              case OCTAVE:
                incrementOctave(5);
                break;
              case VELOCITY:
                incrementVel(5);
                break;
            }

            break;
        case '<':
        case ',':
            switch(Globals::mode) {
              case PITCH:
                decrementPitch(5);
                break;
              case OCTAVE:
                decrementOctave(5);
                break;
              case VELOCITY:
                decrementVel(5);
                break;
            }

            break;
        case 'L':
        case 'l':
            prev = Globals::selectedStep;
            Globals::selectedStep = 6;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'O':
        case 'o':
            switch(Globals::mode) {
              case PITCH:
                incrementPitch(6);
                break;
              case OCTAVE:
                incrementOctave(6);
                break;
              case VELOCITY:
                incrementVel(6);
                break;
            }

            break;
        case '>':
        case '.':
            switch(Globals::mode) {
              case PITCH:
                decrementPitch(6);
                break;
              case OCTAVE:
                decrementOctave(6);
                break;
              case VELOCITY:
                decrementVel(6);
                break;
            }

            break;
        case ':':
        case ';':
            prev = Globals::selectedStep;
            Globals::selectedStep = 7;
            selectStep(prev, Globals::selectedStep);
            break;
        case 'P':
        case 'p':
            switch(Globals::mode) {
              case PITCH:
                incrementPitch(7);
                break;
              case OCTAVE:
                incrementOctave(7);
                break;
              case VELOCITY:
                incrementVel(7);
                break;
            }

            break;
        case '?':
        case '/':
            switch(Globals::mode) {
              case PITCH:
                decrementPitch(7);
                break;
              case OCTAVE:
                decrementOctave(7);
                break;
              case VELOCITY:
                decrementVel(7);
                break;
            }

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
      incTrack();
    } else if (key == GLUT_KEY_DOWN) {
      decTrack();
    } else if (key == GLUT_KEY_RIGHT) {
        switch(Globals::mode) {
          case PITCH:
            Globals::mode = OCTAVE;
            Globals::modeText->set("Octave");
            break;
          case OCTAVE:
            Globals::mode = VELOCITY;
            Globals::modeText->set("Velocity");
            break;
          case VELOCITY:
            Globals::mode = PITCH;
            Globals::modeText->set("Pitch");
            break;
        }

    } else if (key == GLUT_KEY_LEFT) {
        switch(Globals::mode) {
          case PITCH:
            Globals::mode = VELOCITY;
            Globals::modeText->set("Velocity");
            break;
          case OCTAVE:
            Globals::mode = PITCH;
            Globals::modeText->set("Pitch");
            break;
          case VELOCITY:
            Globals::mode = OCTAVE;
            Globals::modeText->set("Octave");
            break;
        }
    }

    snapStepLocs();
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

    YEntity * seq = new YEntity();
    Globals::sim->root().addChild(seq);

    Globals::modeText = new YText(1);
    Globals::modeText->loc = Vector3D::Vector3D(6,3.65,0);
    Globals::modeText->sca = Vector3D::Vector3D(3,3,3);
    Globals::modeText->col = Vector3D::Vector3D(0.953f, 0.525f, 0.188f);
    Globals::modeText->set("Pitch");
    seq->addChild(Globals::modeText);

    for (int i = 0; i < 8; i++) {
        YCubeOutline * cube = new YCubeOutline();
        cube->loc = Vector3D::Vector3D((i-6.0)+(i*.7),0,0);
        cube->sca = Vector3D::Vector3D(.7,.7,.7);
        cube->outlineColor = Vector3D::Vector3D(0.655, 0.859, 0.859);
        cube->col = Vector3D::Vector3D(0.412, 0.824, 0.906);
        cube->ori = Vector3D::Vector3D(0,0,0);

        YLine * line = new YLine();
        line->loc = Vector3D::Vector3D((i-6.0)+(i*.7),0,0);
        line->sca = Vector3D::Vector3D(0,2.6,0);
        line->col = Vector3D::Vector3D(0.655, 0.859, 0.859);
        line->ori = Vector3D::Vector3D(0,0,0);


        seq->addChild(cube);
        seq->addChild(line);
        Globals::steps.push_back(cube);
        Globals::lines.push_back(line);
    }
}

//-----------------------------------------------------------------------------
// Name: selectStep( )
// Desc: Select the step we're interested in modifying
//-----------------------------------------------------------------------------
void selectStep(int prev, int idx){
    YEntity * step = NULL;
    YEntity * nextStep = NULL;
    YEntity * line = NULL;

    if (!(prev == -1)) {
        step = Globals::steps.at(prev);
        step->col.set(0.412, 0.824, 0.906);
    }
    nextStep = Globals::steps.at(idx);
    nextStep->col.set(0.953f, 0.525f, 0.188f);
}

//-----------------------------------------------------------------------------
// Name: playStep( )
// Desc: play a step
//-----------------------------------------------------------------------------
void playStep(int prev, int idx){
    YEntity * step = NULL;
    YEntity * nextStep = NULL;
    int track, pitchOff, octOff, velOff;
    vector<int> chans;
    vector<float> pitches, vels;

    if (!(prev == -1) && (prev != Globals::selectedStep)) {
      step = Globals::steps.at(prev);
      step->col = Vector3D::Vector3D(0.412, 0.824, 0.906);
    } else if (prev == Globals::selectedStep) {
      step = Globals::steps.at(prev);
      step->col.set(0.953f, 0.525f, 0.188f);
    }
    nextStep = Globals::steps.at(idx);
    nextStep->col.set(0.655, 0.859, 0.859);

    for (int i = 0; i < 4; i++) {

      if(Globals::stepBools[i].at(idx)) {

        pitchOff = Globals::pitchOffsets[i].at(idx);
        octOff = Globals::octaveOffsets[i].at(idx);
        velOff = Globals::velOffsets[i].at(idx);

        chans.push_back(i);
        pitches.push_back(60+pitchOff+12*octOff);
        vels.push_back(.05*velOff);
      }

      play(chans,pitches,vels);
    }
}

//-----------------------------------------------------------------------------
// Name: sampsPerBeat( )
// Desc: calculate the samples per beat for the desired bpm
//-----------------------------------------------------------------------------
int sampsPerBeat(float bpm) {
    int out = 0;
    out = ceil(THE_SRATE*60/bpm);
    return out;
}

//-----------------------------------------------------------------------------
// Name: decrementOctave( )
// Desc: decrement the octave of a particular step
//-----------------------------------------------------------------------------
void decrementOctave(int num) {
  YEntity * step = Globals::steps.at(num);

  if (Globals::octaveOffsets[Globals::currentTrack].at(num) > -5) {
    Globals::octaveOffsets[Globals::currentTrack].at(num) = Globals::octaveOffsets[Globals::currentTrack].at(num)-1;
    step->loc.y -= 0.5;
  } else {
    Globals::octaveOffsets[Globals::currentTrack].at(num) = -5;
  }

}


//-----------------------------------------------------------------------------
// Name: incrementOctave( )
// Desc: increment the octave of a particular step
//-----------------------------------------------------------------------------
void incrementOctave(int num) {
  YEntity * step = Globals::steps.at(num);

  if (Globals::octaveOffsets[Globals::currentTrack].at(num) < 5) {
    Globals::octaveOffsets[Globals::currentTrack].at(num) = Globals::octaveOffsets[Globals::currentTrack].at(num)+1;
    step->loc.y += 0.5;
  } else {
    Globals::octaveOffsets[Globals::currentTrack].at(num) = 5;
  }

}


//-----------------------------------------------------------------------------
// Name: decrementPitch( )
// Desc: decrement the pitch of a particular step
//-----------------------------------------------------------------------------
void decrementPitch(int num) {
  YEntity * step = Globals::steps.at(num);

  if (Globals::pitchOffsets[Globals::currentTrack].at(num) > 0) {
    Globals::pitchOffsets[Globals::currentTrack].at(num) = Globals::pitchOffsets[Globals::currentTrack].at(num)-1;
    step->loc.y -= 0.5;
  } else {
    Globals::pitchOffsets[Globals::currentTrack].at(num) = 0;
  }

}


//-----------------------------------------------------------------------------
// Name: incrementPitch( )
// Desc: increment the pitch of a particular step
//-----------------------------------------------------------------------------
void incrementPitch(int num) {
  YEntity * step = Globals::steps.at(num);

  if (Globals::pitchOffsets[Globals::currentTrack].at(num) < 11) {
    Globals::pitchOffsets[Globals::currentTrack].at(num) = Globals::pitchOffsets[Globals::currentTrack].at(num)+1;
    step->loc.y += 0.5;
  } else {
    Globals::pitchOffsets[Globals::currentTrack].at(num) = 11;
  }

}
//-----------------------------------------------------------------------------
// Name: decrementVel( )
// Desc: decrement the velocity of a particular step
//-----------------------------------------------------------------------------
void decrementVel(int num) {
  YEntity * step = Globals::steps.at(num);

  if (Globals::velOffsets[Globals::currentTrack].at(num) > 5) {
    Globals::velOffsets[Globals::currentTrack].at(num) = Globals::velOffsets[Globals::currentTrack].at(num)-1;
    step->loc.y -= 0.3667;
  } else {
    Globals::velOffsets[Globals::currentTrack].at(num) = 5;
  }

}


//-----------------------------------------------------------------------------
// Name: incrementVel( )
// Desc: increment the velocity of a particular step
//-----------------------------------------------------------------------------
void incrementVel(int num) {
  YEntity * step = Globals::steps.at(num);

  if (Globals::velOffsets[Globals::currentTrack].at(num) < 20) {
    Globals::velOffsets[Globals::currentTrack].at(num) = Globals::velOffsets[Globals::currentTrack].at(num)+1;
    step->loc.y += 0.3667;
  } else {
    Globals::velOffsets[Globals::currentTrack].at(num) = 20;
  }

}


//-----------------------------------------------------------------------------
// Name: snapStepLocs( )
// Desc: snap step locations to current offsets for mode
//-----------------------------------------------------------------------------
void snapStepLocs() {
  float base;
  float locStep;
  array<vector<int>,4> offsets;

  switch(Globals::mode) {
    case PITCH:
      offsets = Globals::pitchOffsets;
      base = -2.5f;
      locStep = 0.5f;
      break;
    case OCTAVE:
      offsets = Globals::octaveOffsets;
      base = 0.0f;
      locStep = 0.5f;
      break;
    case VELOCITY:
      offsets = Globals::velOffsets;
      base = -2.5f;
      locStep = 0.3667f;
      break;
  }

  for (int i = 0; i < Globals::steps.size(); i++) {
    if (Globals::mode == VELOCITY) {
      Globals::steps.at(i)->loc.y = base+locStep*(offsets[Globals::currentTrack].at(i)-5);
    } else {
      Globals::steps.at(i)->loc.y = base+locStep*offsets[Globals::currentTrack].at(i);
    }
  }


}


//-----------------------------------------------------------------------------
// Name: incTrack( )
// Desc: increment the track number
//-----------------------------------------------------------------------------
void incTrack() {
  Globals::currentTrack = (Globals::currentTrack + 1)%4;

  switch (Globals::currentTrack) {
    case 0:
      Globals::bgColor.set(Vector3D::Vector3D(TAN));
      break;
    case 1:
      Globals::bgColor.set(Vector3D::Vector3D(BLUE));
      break;
    case 2:
      Globals::bgColor.set(Vector3D::Vector3D(GREEN));
      break;
    case 3:
      Globals::bgColor.set(Vector3D::Vector3D(ORANGE));
      break;
  }

  Globals::bgColor.setSlew(1);
  glClearColor( Globals::bgColor.actual().x, Globals::bgColor.actual().y, Globals::bgColor.actual().z, 1.0f );

  setLineColors();
}

//-----------------------------------------------------------------------------
// Name: decTrack( )
// Desc: decrement the track number
//-----------------------------------------------------------------------------
void decTrack() {
  if (Globals::currentTrack == 0) {
    Globals::currentTrack = 3;
  } else {
    Globals::currentTrack -= 1;
  }

  switch (Globals::currentTrack) {
    case 0:
      Globals::bgColor.set(Vector3D::Vector3D(TAN));
      break;
    case 1:
      Globals::bgColor.set(Vector3D::Vector3D(BLUE));
      break;
    case 2:
      Globals::bgColor.set(Vector3D::Vector3D(GREEN));
      break;
    case 3:
      Globals::bgColor.set(Vector3D::Vector3D(ORANGE));
      break;
  }

  Globals::bgColor.setSlew(1);
  glClearColor( Globals::bgColor.actual().x, Globals::bgColor.actual().y, Globals::bgColor.actual().z, 1.0f );

  setLineColors();

}


//-----------------------------------------------------------------------------
// Name: setLineColors( )
// Desc: set the line colors for the current track
//-----------------------------------------------------------------------------
void setLineColors() {
  for (int step = 0; step < 8; step++) {
    if (Globals::stepBools[Globals::currentTrack].at(step)) {
      Globals::lines.at(step)->col.set(LINEON);
    } else {
      Globals::lines.at(step)->col.set(LINEOFF);
    }
  }
}
