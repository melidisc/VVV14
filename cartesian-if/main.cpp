// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
//
// A tutorial on how to use the Cartesian Interface to control a limb
// in the operational space.
//
// Author: Ugo Pattacini - <ugo.pattacini@iit.it>

#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Vector.h>
#include <yarp/math/Math.h>

#include <yarp/dev/Drivers.h>
#include <yarp/dev/CartesianControl.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/GazeControl.h>

#include <gsl/gsl_math.h>

#include <stdio.h>

#define CTRL_THREAD_PER     0.02    // [s]
#define PRINT_STATUS_PER    1.0     // [s]
#define MAX_TORSO_PITCH     20.0    // [deg]

YARP_DECLARE_DEVICES(icubmod)

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;


class CtrlThread: public RateThread
{
protected:
    PolyDriver         client,clientGazeCtrl;
    ICartesianControl *armLeft, *armRight;
    IGazeControl *igaze;
    Network yarp;
    Vector xd;
    Vector od;

    int startup_context_id;

    double t;
    double t0;
    double t1;
    
    BufferedPort<Bottle> incomingPort;
    
    
    
public:
    CtrlThread(const double period) : RateThread(int(period*1000.0)) { }

    virtual bool threadInit()
    {
      
        if (!yarp.checkNetwork())
            return -1;
        incomingPort.open("/cartesian/target/in");
        Network::connect("/objectDetector/target","/cartesian/target/in");
        
        // open a client interface to connect to the cartesian server of the simulator
        // we suppose that:
        //
        // 1 - the iCub simulator is running
        //     (launch iCub_SIM)
        //
        // 2 - the cartesian server is running
        //     (launch simCartesianControl)
        //     
        // 3 - the cartesian solver for the left arm is running too
        //     (launch iKinCartesianSolver --context simCartesianControl --part left_arm)
        //
        Property option("(device cartesiancontrollerclient)");
        option.put("remote","/icubSim/cartesianController/right_arm");
        option.put("local","/cartesian_client/right_arm");
        
        if (!client.open(option))
            return false;
        
        // open the view
        client.view(armLeft);
        
//        Property option2;
        option.clear();
        option.put("device","gazecontrollerclient");
        option.put("remote","/iKinGazeCtrl");
        option.put("local","/client/gaze");
        
        clientGazeCtrl.open(option);
        
        if (clientGazeCtrl.isValid()) {
            clientGazeCtrl.view(igaze);
        }
        

        // latch the controller context in order to preserve
        // it after closing the module
        // the context contains the dofs status, the tracking mode,
        // the resting positions, the limits and so on.
        armLeft->storeContext(&startup_context_id);

        // set trajectory time
        armLeft->setTrajTime(1.0);

        // get the torso dofs
        Vector newDof, curDof;
        armLeft->getDOF(curDof);
        newDof=curDof;

        // enable the torso yaw and pitch
        // disable the torso roll
        newDof[0]=1;
        newDof[1]=0;
        newDof[2]=1;

        // impose some restriction on the torso pitch
        limitTorsoPitch();

        // send the request for dofs reconfiguration
        armLeft->setDOF(newDof,curDof);

        xd.resize(3);
        od.resize(4);

        return true;
    }

    virtual void afterStart(bool s)
    {
        if (s)
            fprintf(stdout,"Thread started successfully\n");
        else
            fprintf(stdout,"Thread did not start\n");

        t=t0=t1=Time::now();
    }

    virtual void run()
    {
        t=Time::now();

        generateTarget();
        
        // go to the target :)
        // (in streaming)
        armLeft->goToPose(xd,od);
        
        // some verbosity
        printStatus();
    }

    virtual void threadRelease()
    {    
        // we require an immediate stop
        // before closing the client for safety reason
        armLeft->stopControl();

        // it's a good rule to restore the controller
        // context as it was before opening the module
        armLeft->restoreContext(startup_context_id);

        client.close();
        clientGazeCtrl.close();
    }

    void generateTarget()
    {   
        // translational target part: a circular trajectory
        // in the yz plane centered in [-0.3,-0.1,0.1] with radius=0.1 m
        // and frequency 0.1 Hz
//        xd[0]=-0.3;
//        xd[1]=-0.1+0.2*cos(2.0*M_PI*0.1*(t-t0));
//        xd[2]=+0.1+0.2*sin(2.0*M_PI*0.1*(t-t0));            
   
        // we keep the orientation of the left arm constant:
        // we want the middle finger to point forward (end-effector x-axis)
        // with the palm turned down (end-effector y-axis points leftward);
        // to achieve that it is enough to rotate the root frame of pi around z-axis
        od[0]=0.0; od[1]=0.0; od[2]=1.0; od[3]=M_PI;
        
        //adding code for taget location through vision
        //incoming image from both eyes
        Bottle *target = incomingPort.read();
        Vector r(2), l(2);
        r[0] = target->get(0).asDouble();
        r[1] = target->get(1).asDouble();
        l[0] = target->get(3).asDouble();
        l[1] = target->get(4).asDouble();
        
        if(r[0]==0.0 && r[1]==0.0 && l[0]==0.0 && l[1]==0.0){
            xd[0]=-0.3;
            xd[1]=-.1;
            xd[2]=.1;
            
        }
        else{
            igaze->lookAtStereoPixels(l,r);
            igaze->triangulate3DPoint(l,r,xd);
        }

        
        
        
    }

    double norm(const Vector &v)
    {
        return sqrt(dot(v,v));
    }

    void limitTorsoPitch()
    {
        int axis=0; // pitch joint
        double min, max;

        // sometimes it may be helpful to reduce
        // the range of variability of the joints;
        // for example here we don't want the torso
        // to lean out more than 30 degrees forward

        // we keep the lower limit
        armLeft->getLimits(axis,&min,&max);
        armLeft->setLimits(axis,min,MAX_TORSO_PITCH);
    }

    void printStatus()
    {        
        if (t-t1>=PRINT_STATUS_PER)
        {
            Vector x,o,xdhat,odhat,qdhat;

            // we get the current arm pose in the
            // operational space
            armLeft->getPose(x,o);

            // we get the final destination of the arm
            // as found by the solver: it differs a bit
            // from the desired pose according to the tolerances
            armLeft->getDesired(xdhat,odhat,qdhat);

            double e_x=norm(xdhat-x);
            double e_o=norm(odhat-o);

            fprintf(stdout,"+++++++++\n");
            fprintf(stdout,"xd          [m] = %s\n",xd.toString().c_str());
            fprintf(stdout,"xdhat       [m] = %s\n",xdhat.toString().c_str());
            fprintf(stdout,"x           [m] = %s\n",x.toString().c_str());
            fprintf(stdout,"od        [rad] = %s\n",od.toString().c_str());
            fprintf(stdout,"odhat     [rad] = %s\n",odhat.toString().c_str());
            fprintf(stdout,"o         [rad] = %s\n",o.toString().c_str());
            fprintf(stdout,"norm(e_x)   [m] = %g\n",e_x);
            fprintf(stdout,"norm(e_o) [rad] = %g\n",e_o);
            fprintf(stdout,"---------\n\n");

            t1=t;
        }
    }
};



class CtrlModule: public RFModule
{
protected:
    CtrlThread *thr;

public:
    virtual bool configure(ResourceFinder &rf)
    {
        Time::turboBoost();

        thr=new CtrlThread(CTRL_THREAD_PER);
        if (!thr->start())
        {
            delete thr;
            return false;
        }

        return true;
    }

    virtual bool close()
    {
        thr->stop();
        delete thr;

        return true;
    }

    virtual double getPeriod()    { return 1.0;  }
    virtual bool   updateModule() { return true; }
};



int main()
{   
    // we need to initialize the drivers list 
    YARP_REGISTER_DEVICES(icubmod)

    Network yarp;
    if (!yarp.checkNetwork())
        return -1;
//    Network::connect("/objectDetector/target","/cartesian/target/in");
    
    
    CtrlModule mod;

    ResourceFinder rf;
    return mod.runModule(rf);
}



