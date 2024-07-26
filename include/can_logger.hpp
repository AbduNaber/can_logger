


#include <queue>
#include <yaml-cpp/yaml.h>
#include <string>
#include <chrono>
#include <socketcan_interface/socketcan.h>
#include <socketcan_interface/threading.h>
#include <can_msgs/Frame.h>
#include <iostream>
#include <ctime>
#include <cstring> 
#include <spdlog/spdlog.h>  
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>

#define CONFIG_FILE_PATH "WRITE THE PATH TO config.yaml"
#define LOG_FILE_PATH "WRITE THE PATH TO LOG FILE"

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
    can::Frame frame;
    double timestamp = 0;
};

class CanLogger {

    public:
        CanLogger();
        void createLogFile();
        bool isCanInterfaceUp();
        void connect(can::DriverInterfaceSharedPtr can_driver ) ;
        void add (const Can_frame frame, int pre_post);
        void canFrameCallback( const can::Frame& f) ;
        
        void log();
        void log(const Can_frame & frame, FILE *fp);
        void run();
        bool isEventOccur(const Can_frame& frame) ;
        void clear() ; 
        void readConfig();
        ~CanLogger();

    private:

        std::string can_interface_;
        can::DriverInterfaceSharedPtr can_driver_;
        can::FrameListenerConstSharedPtr frame_listener_;
        Can_frame frame_ = {};

        std::queue<Can_frame> pre_frames_; // vector of frames to be logged for size please refer to config.yaml
       
    
        std::queue<Can_frame> post_frames_; // vector of frames to be logged for size please refer to config.yaml
      

        long int max_frames_; // maximum number of frames to be logged. It uses duration*hz to calculate the size of the queue. duration and hz must be defined in config.yaml
        int duration_;
        std::string config_file_;
        Can_message event_msg_;
        bool frame_received_ = false;
        bool pre_event_occured_ = false;
        std::shared_ptr<spdlog::logger> logger_;

        double getUnixTimestamp();

};
}