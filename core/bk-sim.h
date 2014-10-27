//-----------------------------------------------------------------------------
// name: bk-sim.h
// desc: visualization simulation
//
// author: Ge Wang (ge@ccrma.stanford.edu)
//   date: 2013
//-----------------------------------------------------------------------------
#ifndef __BK_SIM_H__
#define __BK_SIM_H__

#include "y-entity.h"

//-----------------------------------------------------------------------------
// name: class BKSim
// desc: simulation class
//-----------------------------------------------------------------------------
class BKSim
{
public:
    BKSim();
    virtual ~BKSim();
    
public:
    // cascade timestep simulation through system (as connected to this)
    void systemCascade();
    
public:
    // pause the simulation
    void pause();
    // resume the simulation
    void resume();
    // get is paused
    bool isPaused() const;
    
public:
    // set desired frame rate
    void setDesiredFrameRate( double frate );
    // get it
    double getDesiredFrameRate() const;
    // get the timestep in effect (fixed or dynamic)
    YTimeInterval delta() const;
    
public:
    // get the root
    YEntity & root() { return m_gfxRoot; }

protected:
    YEntity m_gfxRoot;

public:
    double m_desiredFrameRate;
    bool m_useFixedTimeStep;
    
public:
    YTimeInterval m_timeLeftOver;
    YTimeInterval m_simTime;
    YTimeInterval m_lastDelta;
    bool m_first;
    bool m_isPaused;
};




#endif
