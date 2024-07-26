#include "can_logger.hpp"

using VICL = vehicle_interface::CanLogger;

VICL::CanLogger(){

    createLogFile();
    

    logger_->info("Started CAN Logger");
    readConfig();
    
    if (!isCanInterfaceUp()) {
        logger_->error("CAN interface is down");
        return;
    }
    else {
        logger_->info("CAN interface is up");
    }

    connect(std::make_shared<can::ThreadedSocketCANInterface>());
    run();
}

void VICL::createLogFile(){
    /**
     * todo: 
     *  1) delete old log files
     *  
     */
    auto now = std::chrono::system_clock::now();
        std::time_t currtime = std::chrono::system_clock::to_time_t(now);
        struct tm now_tm;
        localtime_r(&currtime, &now_tm);

        // Define the directory path
        const char* dir_path = LOG_FILE_PATH;

        // Prepare the file name buffer
        char fname[128];

        // Format the file name
        snprintf(fname, sizeof(fname), "%slog-%04d-%02d-%02d_%02d%02d%02d.log",
                dir_path,
                now_tm.tm_year + 1900,
                now_tm.tm_mon + 1,
                now_tm.tm_mday,
                now_tm.tm_hour,
                now_tm.tm_min,
                now_tm.tm_sec);
                
        logger_ = spdlog::basic_logger_mt("can_logger", std::string(fname));

        logger_->set_level(spdlog::level::debug);
        logger_->flush_on(spdlog::level::debug);
}

void VICL::connect(can::DriverInterfaceSharedPtr can_driver) {
    can_driver_ = can_driver;

    while (! can_driver_->init(can_interface_,0)){
        logger_->error("Failed to initialize CAN interface");
        sleep(1);
    }

    frame_listener_ = can_driver_->createMsgListenerM(this, &VICL::canFrameCallback);
    logger_->info("Connected to CAN interface at {} ", can_interface_);
}

void VICL::canFrameCallback( const can::Frame& f) {
    
    frame_.frame = f;
    frame_.timestamp = getUnixTimestamp();
    frame_received_ = true;
    
}

void VICL::run() {
    
    while(true){
        
        if (!frame_received_ ) {
            
            continue;
        }

        logger_->info("started 60 sc part");
        pre_frames_.push(frame_);
        
        frame_received_ = false;

        while (!isEventOccur(pre_frames_.back())) {
            add(frame_, 0);
        }
        logger_->info("Event occured at {}",frame_.timestamp);
        // Event has occured get 30 seconds of post frames
        
        post_frames_.push(frame_);
        frame_received_ = false;
        
        
        while (post_frames_.back().timestamp - post_frames_.front().timestamp < duration_) {
            
            add(frame_, 1);
            
        }

        log();
        
        while(isEventOccur(pre_frames_.back() ) || pre_event_occured_){
            add(frame_, 0);
        }
        logger_->info("Logged frames");
        frame_received_ = false;
        logger_->info("ended 60 sc part");
    }
    
}

double VICL::getUnixTimestamp() {

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration) - std::chrono::duration_cast<std::chrono::nanoseconds>(seconds);
    
    return seconds.count() + nanoseconds.count() * 1e-9;
}

void VICL::add(const Can_frame frame,int pre_post) {
     
    if (!frame_received_) {
           
            return;
        }

    if(pre_frames_.empty() && post_frames_.empty()){
       pre_frames_.push(frame);
       return;
   }

    if(pre_post == 0){

        
        if(pre_frames_.size() > max_frames_ || pre_frames_.back().timestamp - pre_frames_.front().timestamp > duration_){
           
            pre_frames_.pop();
        }
        
        pre_frames_.push(frame);

        
    }
    else if (pre_post == 1){
        if(post_frames_.size() > max_frames_ ){
            post_frames_.pop();
        }
        post_frames_.push(frame);
        
    }
    frame_received_ = false;
    
}

void VICL::log (){
    
    auto now = std::chrono::system_clock::now();
    std::time_t currtime = std::chrono::system_clock::to_time_t(now);
    struct tm now_tm;
    localtime_r(&currtime, &now_tm);

    // Define the directory path
    const char* dir_path = LOG_FILE_PATH;

    // Prepare the file name buffer
    char fname[128];

    // Format the file name
    snprintf(fname, sizeof(fname), "%scandump-%04d-%02d-%02d_%02d%02d%02d.log",
             dir_path,
             now_tm.tm_year + 1900,
             now_tm.tm_mon + 1,
             now_tm.tm_mday,
             now_tm.tm_hour,
             now_tm.tm_min,
             now_tm.tm_sec);
    
    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        perror("Error while opening file");
        return;
    }

    while (!pre_frames_.empty()) {
        log(pre_frames_.front(), fp);
        
        pre_frames_.pop();
       
    }

    while (!post_frames_.empty()) {
        log(post_frames_.front(), fp);
        post_frames_.pop();
    }

    fclose(fp);
    logger_->info("Logged frames to {}", fname);
    clear();

}

void VICL::log(const Can_frame & frame, FILE *fp) {

    char timestring[25];
    char afrbuf[AFRSZ];
    int numchars = 0;

    // Format the timestamp
    snprintf(timestring, sizeof(timestring), "%f", frame.timestamp);

    // Format the CAN frame information
    numchars = snprintf(afrbuf, sizeof(afrbuf), "(%s) %s %03X#",
                        timestring,
                        can_interface_.c_str(),
                        frame.frame.id);

    // Append CAN data
    for (int i = 0; i < frame.frame.dlc; ++i) {
        numchars += snprintf(afrbuf + numchars, sizeof(afrbuf) - numchars, "%02X",
                             frame.frame.data[i]);
    }

    // Null-terminate the buffer and print
    afrbuf[sizeof(afrbuf) - 1] = '\0';

    fprintf(fp, "%s\n", afrbuf);

}

bool VICL::isCanInterfaceUp() {

    std::array<char, 128> buffer;
    std::string result;
    std::string command = "ip link show " + can_interface_; // "ip link show ---" message will use in console

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
    {
    throw std::runtime_error("popen() failed");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
    result += buffer.data();
    }
  
    return result.find("UP") != std::string::npos;
}

bool VICL::isEventOccur(const Can_frame& frame) {

    if (frame.frame.id != event_msg_.can_id) {
        return false;
    }
    uint64_t raw_value = 0; // Initialize the raw value to 0

    for (int i = 0; i < event_msg_.signal.length; ++i) {
        int byte_index = (event_msg_.signal.start_bit + i) / 8;  // Determine which byte the bit is in
        int bit_index = (event_msg_.signal.start_bit + i) % 8;   // Determine the bit's position within the byte

        
        raw_value |= ((frame.frame.data[byte_index] >> bit_index) & 0x01) << i;
    }
    raw_value = (raw_value * event_msg_.signal.scale + event_msg_.signal.offset);

    pre_event_occured_ = (raw_value  == event_msg_.signal.value);
    return pre_event_occured_ ;
    
}

void VICL::readConfig() {

    YAML::Node config = YAML::LoadFile(CONFIG_FILE_PATH);
   
    can_interface_ = config["CAN_INTERFACE"].as<std::string>();

    event_msg_.can_id = config["EVENT_FRAME"]["CAN_ID"].as<int>();
    event_msg_.signal.start_bit = config["EVENT_FRAME"]["SIGNAL"]["START_BIT"].as<int>();
    event_msg_.signal.length = config["EVENT_FRAME"]["SIGNAL"]["LENGTH"].as<int>();
    event_msg_.signal.scale = config["EVENT_FRAME"]["SIGNAL"]["SCALE"].as<float>();
    event_msg_.signal.offset = config["EVENT_FRAME"]["SIGNAL"]["OFFSET"].as<float>();
    event_msg_.signal.value = config["EVENT_FRAME"]["SIGNAL"]["VALUE"].as<float>();

    duration_ = config["DURATION"].as<int>();

    long int hz = config["CAN_HZ"].as<long int>();

    max_frames_ = duration_ * hz;
    logger_->info("Config file is read successfully");
}

void VICL::clear() {
    pre_frames_ = std::move(post_frames_); 
   
    logger_->info("Cleared all frames");
}

VICL::~CanLogger() {
    // Destructor
    logger_->info("Exiting CAN Logger");
}

int main() {

    VICL can_logger;
    return 0;

}