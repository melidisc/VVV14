/* Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
 * Authors: Ilaria Gori
 * email:   ilaria.gori@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found in the file LICENSE located in the
 * root directory.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

/**
\defgroup kinectClientExample kinectClientExample

Example module for the use of \ref kinectClientExample "Kinect 
Wrapper Client". 

\section intro_sec Description
This simple module retrieves and display depth and rgb images, players information and the skeleton
reading everything from a \ref kinectServer.

It requires the \ref kinectServer running.

\section lib_sec Libraries
- YARP libraries.
- \ref kinectWrapper library.

\section parameters_sec Parameters
--verbosity \e verbosity
- specify the verbosity level of the client print-outs.

--carrier \e carrier
- specify the protocol used to connect to the server ports.

--remote \e remote
- specify the kinectServer name to connect to.

--local \e name
- specify the kinectClient stem-name.

\section tested_os_sec Tested OS
Windows, Linux

\author Ilaria Gori
*/
#include <math.h>
#include <stdlib.h>
#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>

#include <kinectWrapper/kinectTags.h>
#include <kinectWrapper/kinectWrapper_client.h>

#include <yarp/sig/Vector.h>
#include <cmath>
#include <yarp/os/BufferedPort.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace kinectWrapper;
struct theStruct
{
    int u;
    int v;
    double x;
    double y;
    double z;
    
};
class KinectClient: public RFModule
{
protected:
    KinectWrapperClient client;
    ImageOf<PixelRgb> rgb;
    ImageOf<PixelMono16> depth;
    ImageOf<PixelFloat> depthToDisplay;
    ImageOf<PixelBgr> playersImage;
    ImageOf<PixelBgr> skeletonImage;
    IplImage* depthTmp;
    IplImage* rgbTmp;
    
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelFloat> > depthPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> > imagePort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelBgr> > playersPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelBgr> > skeletonPort;
    yarp::os::BufferedPort< yarp::os::Bottle > outPort;
    
    bool showImages;
    Player joint;
    deque<Player> joints;
    Matrix players;
    int similarity;
    kinectWrapper::Skeleton processBuf; //8 locations x 3 values each
    yarp::os::BufferedPort<yarp::os::Bottle> jointsPort;
    Bottle *jointBot;
    
public:

    bool configure(ResourceFinder &rf)
    {
        //prePro stuf
        similarity = 10;
        jointsPort.open("/kinectClientExample/skeleton:i");
        Network::connect("/kinectServer/joints:o","/kinectClientExample/skeleton:i");
        outPort.open("/kinectClient/out");
        
//        int verbosity=rf.check("verbosity",Value(0)).asInt();
//        string name=rf.check("name",Value("kinectClientExample")).asString().c_str();
//        string show=rf.check("showImages",Value("false")).asString().c_str();
//        showImages=(show=="true");
//        
//        depthPort.open("/"+name+"/depthPort:o");
//        imagePort.open("/"+name+"/imagePort:o");
//        playersPort.open("/"+name+"/playersPort:o");
//        skeletonPort.open("/"+name+"/skeletonPort:o");

//        Property options;
//        options.put("carrier","udp");
//        options.put("remote","kinectServer");
//        options.put("local",(name+"/client").c_str());
//        options.put("verbosity",verbosity);
//
//        if (!client.open(options))
//            return false;

//        Property opt;
//        client.getInfo(opt);
//
//        int img_width=opt.find("img_width").asInt();
//        int img_height=opt.find("img_height").asInt();
//        int depth_width=opt.find("depth_width").asInt();
//        int depth_height=opt.find("depth_height").asInt();
//        rgb.resize(img_width, img_height);
//        depth.resize(depth_width,depth_height);
//        depthToDisplay.resize(depth_width,depth_height);
//        playersImage.resize(depth_width,depth_height);
//        skeletonImage.resize(depth_width,depth_height);
//        
//        depthTmp=cvCreateImage(cvSize(depth_width,depth_height),IPL_DEPTH_32F,1);
//        rgbTmp=cvCreateImage(cvSize(img_width,img_height),IPL_DEPTH_8U,3);
//        
//        if (showImages)
//        {
//            cvNamedWindow("rgb",CV_WINDOW_AUTOSIZE);
//            cvMoveWindow("rgb", 160, 100);
//            cvNamedWindow("depth",CV_WINDOW_AUTOSIZE);
//            cvMoveWindow("depth", 510, 100);
//            cvNamedWindow("skeleton", CV_WINDOW_AUTOSIZE);
//            cvMoveWindow("skeleton", 860, 100);
//            cvNamedWindow("players", CV_WINDOW_AUTOSIZE);
//            cvMoveWindow("players", 320, 400);
//        }
         
        return true;
    }

    bool close()
    {
        outPort.interrupt();
        outPort.close();
        /*
        depthPort.interrupt();
        depthPort.close();
        imagePort.interrupt();
        imagePort.close();
        playersPort.interrupt();
        playersPort.close();
        skeletonPort.interrupt();
        skeletonPort.close();
        client.close();
        cvReleaseImage(&depthTmp);
        cvReleaseImage(&rgbTmp);
         */
        return true;
    }

    double getPeriod()
    {
        return 0.01;
    }

    bool updateModule()
    {
        /*
        client.getDepthAndPlayers(depth,players);
        client.getRgb(rgb);

        /*Alternatively you can ask for getJoints(joints), retrieving a deque of Players,
        having information on all the players instead of having only information on
        the closest one* /
        bool tracked=client.getJoints(joint,KINECT_TAGS_CLOSEST_PLAYER);

        if (tracked)
            client.getSkeletonImage(joint,skeletonImage);

        client.getPlayersImage(players,playersImage);
        client.getDepthImage(depth,depthToDisplay);
        
        if (depthPort.getOutputCount()>0)
        {
            depthPort.prepare()=depthToDisplay;
            depthPort.write();
        }
        
        if (imagePort.getOutputCount()>0)
        {
            imagePort.prepare()=rgb;
            imagePort.write();
        }
        
        if (playersPort.getOutputCount()>0)
        {
            playersPort.prepare()=playersImage;
            playersPort.write();
        }
        
        if (skeletonPort.getOutputCount()>0)
        {
            skeletonPort.prepare()=skeletonImage;
            skeletonPort.write();
        }
        
        int u=160;
        int v=120;
        yarp::sig::Vector point3D;
        client.get3DPoint(u,v,point3D);
        */
        //preprocess the images
        jointBot = (Bottle *) jointsPort.read();
        double *timestamp;
        kinectWrapper::Player player;
        
        joint = getJoints(jointBot,-1);
        fprintf(stdout, "We got a reply and found %d players\n", joint.ID);
        kinectWrapper::Skeleton jointMap;
        jointMap = joint.skeleton;
        kinectWrapper::Skeleton::iterator it;
        for(it = jointMap.begin(); it!= jointMap.end(); it++){
            fprintf(stdout, "Joint %s values %f %f %f\n", it->first.c_str(), it->second.x, it->second.y, it->second.z);
        }
        
        Bottle outGoingBottle;
        outGoingBottle.clear();
        outGoingBottle.addString("train");
        if (!prePro(jointMap, outGoingBottle)){
            fprintf(stdout,"Jancked values\t\n");
            return true;
        }
        
        Bottle &sender = outPort.prepare();
        sender.clear();
        sender.addList() = outGoingBottle;
        fprintf(stdout, "OUT %s\n", sender.toString().c_str());
        if(outPort.getOutputCount()>0){
            outPort.write();
        }
        
        /*
        fprintf(stdout, "%s\n", point3D.toString().c_str());

        if (showImages)
        {
            cvConvertScale((IplImage*)depthToDisplay.getIplImage(),depthTmp,1.0/255);
            cvShowImage("depth",depthTmp);
            cvShowImage("players",(IplImage*)playersImage.getIplImage());
            cvShowImage("skeleton",(IplImage*)skeletonImage.getIplImage());

            cvWaitKey(1);

            cvCvtColor((IplImage*)rgb.getIplImage(),rgbTmp,CV_BGR2RGB);
            cvShowImage("rgb",rgbTmp);
        }
         */
        return true;
    }
    bool prePro(kinectWrapper::Skeleton &captured, Bottle &out){
        fprintf(stdout,"Pre pro\n");
        double simx = 0.0, simy = 0.0, simz = 0.0;
        int count = 0;
        Bottle theList;
        
        kinectWrapper::Skeleton::iterator it;
        for(it = captured.begin(); it != captured.end(); it++){
            theList.clear();
            theList.addString(it->first);
            theList.addDouble(it->second.x);
            theList.addDouble(it->second.y);
            theList.addDouble(it->second.z);
            out.addList() = theList;
            
            simx =  ( std::abs(it->second.x - processBuf[it->first].x) / std::abs(it->second.x) )*100 ;
            simy =  ( std::abs(it->second.y - processBuf[it->first].y) / std::abs(it->second.y) )*100 ;
            simz =  ( std::abs(it->second.y - processBuf[it->first].y) / std::abs(it->second.y) )*100 ;
            
            if (simx < similarity && simy < similarity && simz < similarity)
               count++;       
           fprintf(stdout, "%f | %f= %f / %f   %f | %f = %f / %f %f | %f= %f / %f\n", 
                   simx,  std::abs(it->second.x - processBuf[it->first].x) / std::abs(it->second.x), std::abs(it->second.x - processBuf[it->first].x),  std::abs(it->second.x), 
                   simy, std::abs(it->second.y - processBuf[it->first].y) / std::abs(it->second.y), std::abs(it->second.y - processBuf[it->first].y), std::abs(it->second.y),
                   simz, std::abs(it->second.y - processBuf[it->first].y) / std::abs(it->second.y), std::abs(it->second.z - processBuf[it->first].z) , std::abs(it->second.z) );
        }
       fprintf(stdout,"count %d \t\n",count);
        //all parts captured now have a similarity value above threshold with the previous ones
        if (count == captured.size())
            return false;
        
            
        processBuf = captured;
        return true;
    }
   Player getJoints(Bottle* skeleton, int playerId)
    {
        Player p;
        bool found=false;
        if (playerId<0)
            p=managePlayerRequest(skeleton,playerId);
        else
        {
            for (int i=0; i<skeleton->size(); i++)
            {
                Bottle* player=skeleton->get(i).asList();
                if(player->get(0).asInt()==playerId)
                {
                    found=true;
                    p.ID=playerId;
                    for (int j=1; j<player->size(); j++)
                    {
                        Bottle* joints=player->get(j).asList();
                        Bottle* jointsPosition=joints->get(1).asList();
                        Joint joint;
                        joint.u=jointsPosition->get(0).asInt();
                        joint.v=jointsPosition->get(1).asInt();
                        joint.x=jointsPosition->get(2).asDouble();
                        joint.y=jointsPosition->get(3).asDouble();
                        joint.z=jointsPosition->get(4).asDouble();
                        p.skeleton[joints->get(0).asString().c_str()]=joint;
                    }
                }
            }
            if (!found)
                p.ID=-1;
        }
        return p;
    }
   Player managePlayerRequest(Bottle* skeleton, int playerId)
    {
        Player p;
        if (playerId==-1)
        {
            double distance=20000;
            deque<Player> players=getJoints(skeleton);
            if (players.size()==0)
            {
                p.ID=-1;
                return p;
            }
            for (unsigned int i=0; i<players.size(); i++)
            {
                if (players.at(i).skeleton["shoulderCenter"].z < distance)
                {
                    distance=players.at(i).skeleton["shoulderCenter"].z;
                    p=players.at(i);
                }
            }
        }
        return p;
    }
   std::deque<Player> getJoints(Bottle* skeleton)
    {
        deque<Player> players;
        for (int i=0; i<skeleton->size(); i++)
        {
            Skeleton limbs;
            Bottle* player=skeleton->get(i).asList();
            Player pi;
            pi.ID=player->get(0).asInt();
            for (int j=1; j<player->size(); j++)
            {
                Bottle* joints=player->get(j).asList();
                Bottle* jointsPosition=joints->get(1).asList();
                Joint joint;
                joint.u=jointsPosition->get(0).asInt();
                joint.v=jointsPosition->get(1).asInt();
                joint.x=jointsPosition->get(2).asDouble();
                joint.y=jointsPosition->get(3).asDouble();
                joint.z=jointsPosition->get(4).asDouble();
                limbs[joints->get(0).asString().c_str()]=joint;
            }
            pi.skeleton=limbs;
            players.push_back(pi);
        }
        return players;
    }
};



int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        fprintf(stdout, "Yarp network not available\n");
        return -1;
    }

    ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultContext("kinectClientExample");
    rf.configure(argc,argv);

    KinectClient mod;
    mod.runModule(rf);
    return 0;
}

