


#include <queue>
#include <yaml-cpp/yaml.h>
#include <string>
#include <chrono>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <cstring> 
#include <thread>
#include <mutex>

#define CONFIG_FILE_PATH "/home/abdu/adastec_ws/src/can_logger/config/config.yaml"
#define AFRSZ 6300 // The buffer size for ASCII CAN frame string representations


namespace vehicle_interface {



struct Can_signal {
    int start_bit;
    int length;
    float scale;
    float offset;
    float value;

};

struct Can_message {
    int can_id;
    Can_signal signal;
};

struct Can_frame {
    struct can_frame frame;
    double timestamp;
};

class CanLogger {

    public:
        CanLogger();
        bool isCanInterfaceUp();
        void connect();
        void add (const Can_frame frame, int pre_post);
        void read();
        void log();
        void log(const Can_frame &frame, FILE *fp = stdout) ;
        void run();
        bool isEventOccur(const Can_frame& frame) ;
        void clear() ; 
        void readConfig();
        ~CanLogger();

    private:

        std::string can_interface_;
        int socketFileDescriptor_;

        std::queue<Can_frame> pre_frames_; // vector of frames to be logged for size please refer to config.yaml
        
    
        std::queue<Can_frame> post_frames_; // vector of frames to be logged for size please refer to config.yaml
        long int max_frames_; // maximum number of frames to be logged. It uses duration*hz to calculate the size of the queue. duration and hz must be defined in config.yaml
        int duration_;
        std::string config_file_;
        Can_message event_msg_;

        std::mutex mtx_;
        std::thread thread_;
        Can_frame frame_;
        bool is_recieved_ ;
        double getUnixTimestamp();

};
}