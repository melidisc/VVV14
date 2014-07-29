#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <yarp/os/all.h>

using namespace yarp::os;

int main() {
    yarp::os::Network yarp;

    yarp::os::Bottle reply;
    Port port;
    port.open("/move_ball");
    yarp.connect("/move_ball", "/icubSim/world");

    Bottle del_all;
    del_all.addString("world");
    del_all.addString("del");
    del_all.addString("all");

    port.write(del_all, reply);
    Bottle create_obj("world mk ssph");
    create_obj.addDouble(0.1); //radius
    create_obj.addInt(1); //x
    create_obj.addInt(1); //y
    create_obj.addInt(1); //z
    create_obj.addInt(0); //r
    create_obj.addInt(0); //g
    create_obj.addInt(1); //b
    
    srand (time(NULL));
    
    port.write(create_obj, reply);

    if (reply.get(0).asVocab() != Vocab::encode("ok"))
        fprintf(stderr, "Error setting sphere\n");

    double start = Time::now();
    while (true) {
        double t = Time::now() - 0.5;//start;
        double dx = cos(t);
        double dy = sin(t);
        Bottle move_obj;
        move_obj.addString("world");
        move_obj.addString("set");
        move_obj.addString("ssph");
        double x = 0.0;//((double) rand() / (RAND_MAX)-0.5) /4;
        move_obj.addInt(1); //radius
        move_obj.addDouble(dx / 2.5 - x); //x
        move_obj.addDouble(dy / 2.5 + 1 -x); //y
        move_obj.addDouble(0.8); //z
        Time::delay(0.5);
        port.write(move_obj, reply);

        if (reply.get(0).asVocab() != Vocab::encode("ok"))
            fprintf(stderr, "Error moving sphere\n");
    }
    return 0;
}
