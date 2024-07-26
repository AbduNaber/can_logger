#include "can_logger.hpp"

using VICL = vehicle_interface::CanLogger;

VICL::CanLogger(){
    
    readConfig();
    connect();

    if(!isCanInterfaceUp()){
        std::cerr << "CAN interface is down" << std::endl;
        return;
    }
    std:: cout << "CAN interface is up" << std::endl;
    thread_ = std::thread(&VICL::read, this);
    
    run();
}


void VICL::connect() {

    socketFileDescriptor_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFileDescriptor_ == -1){
        perror("Error while opening socket");
        return;
    }

    struct sockaddr_can can_address; 
    struct ifreq interface_request; 

    strncpy(interface_request.ifr_name, can_interface_.c_str(), IFNAMSIZ);
    if (ioctl(socketFileDescriptor_, SIOCGIFINDEX, &interface_request) == -1){

        perror("Error while getting interface index");
        ::close(socketFileDescriptor_);  
        return;
    }

    can_address.can_family = AF_CAN;
    can_address.can_ifindex = interface_request.ifr_ifindex;   

    int bind_result = bind(socketFileDescriptor_, (struct sockaddr *)&can_address, sizeof(can_address));

    if (bind_result == -1){
        perror("Error while binding socket");
        ::close(socketFileDescriptor_);
        socketFileDescriptor_ = -1;
        return;
    }
    std::cout << "Connected to CAN interface" << std::endl;

}

void VICL::read() {

    

    int nbytes = ::read(socketFileDescriptor_, &frame_, sizeof(Can_frame));

    if (nbytes < 0) {
        std::cerr << "Couldn't Read from can" << std::endl;
        return;
    }

    is_recieved_ = true;    
   //  std::cout << "Frame received" << std::endl;
}

double VICL::getUnixTimestamp() {

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration) - std::chrono::duration_cast<std::chrono::nanoseconds>(seconds);
    
    return seconds.count() + nanoseconds.count() * 1e-9;
}

void VICL::add(const Can_frame frame,int pre_post) {

   if(pre_frames_.empty() ){
       pre_frames_.push(frame);
       
       return;
   }
   else if (post_frames_.empty()){
       post_frames_.push(frame);
       
       return;
   }

   if(pre_post == 0){
       if(pre_frames_.size() > max_frames_ || pre_frames_.back().timestamp - pre_frames_.front().timestamp > duration_){
            
            pre_frames_.pop();
       }

        pre_frames_.push(frame);
        
    }
    else{
        if(post_frames_.size() > max_frames_ ){
            post_frames_.pop();
        }
        post_frames_.push(frame);
        
    }
    
}
void VICL::log (){

    time_t currtime;
    struct tm now;
    localtime_r(&currtime, &now);
    char fname[128];

    snprintf(fname, sizeof(fname), "candump-%04d-%02d-%02d_%02d%02d%02d.log",
				now.tm_year + 1900,
				now.tm_mon + 1,
				now.tm_mday,
				now.tm_hour,
				now.tm_min,
				now.tm_sec);

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
    clear();
    std::cout << "Log file created" << std::endl;

}
void VICL::log(const Can_frame & frame, FILE *fp ) {

    char timestring[25];
    char afrbuf[AFRSZ];
    int numchars = 0;

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_time_t);
    auto now_ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

    // Format the timestamp
    snprintf(timestring, sizeof(timestring), "%ld.%06ld",
             static_cast<long>(now_time_t),
             now_ms.count());

    // Format the CAN frame information
    numchars = snprintf(afrbuf, sizeof(afrbuf), "(%s) %s %03X#",
                        timestring,
                        can_interface_.c_str(),
                        frame.frame.can_id);

    // Append CAN data
    for (int i = 0; i < frame.frame.can_dlc; ++i) {
        numchars += snprintf(afrbuf + numchars, sizeof(afrbuf) - numchars, "%02X",
                             frame.frame.data[i]);
    }

    // Null-terminate the buffer and print
    afrbuf[sizeof(afrbuf) - 1] = '\0';

    fprintf(fp, "%s\n", afrbuf);

}

void VICL::run() {
    
    if(!is_recieved_){
        return;
    }
    add(frame_, 0);
    std::cout << "Running" << std::endl;
    while (!isEventOccur(pre_frames_.back())) {
        add(frame_, 0);
    }

    add(frame_, 1);
    while (post_frames_.back().timestamp - post_frames_.front().timestamp < duration_) {
        add(frame_, 1);
    }
    log();

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

    if (frame.frame.can_id != event_msg_.can_id) {
        return false;
    }
    uint64_t raw_value = 0; // Initialize the raw value to 0

    for (int i = 0; i < event_msg_.signal.length; ++i) {
        int byte_index = (event_msg_.signal.start_bit + i) / 8;  // Determine which byte the bit is in
        int bit_index = (event_msg_.signal.start_bit + i) % 8;   // Determine the bit's position within the byte

        
        raw_value |= ((frame.frame.data[byte_index] >> bit_index) & 0x01) << i;
    }
    raw_value = (raw_value * event_msg_.signal.scale + event_msg_.signal.offset);
    return raw_value  == event_msg_.signal.value;
    
}

void VICL::readConfig() {
    std::cout << "Reading config file" << std::endl;
    YAML::Node config = YAML::LoadFile("/home/abdu/adastec_ws/src/can_logger/config/config.yaml");
   
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
    std::cout << "Config file read successfully" << std::endl;
}

void VICL::clear() {

    while (!pre_frames_.empty()) {
        pre_frames_.pop();
        
    }
    while (!post_frames_.empty()) {
        post_frames_.pop();
    }
}

VICL::~CanLogger() {
   
    //log();
    clear();
    ::close(socketFileDescriptor_);

}


int main() {

    VICL can_logger;
    return 0;
}