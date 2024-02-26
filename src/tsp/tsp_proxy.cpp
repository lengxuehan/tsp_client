//
// Created by xuwuting on 23-11-15.
//

#include <algorithm>
#include "tsp/tsp_proxy.h"
#include "client/client_tcp_iface.h"
#include "client/tsp_client.h"
#include "tb_log.h"
#include "common/common.h"
#include "packages/packet.h"

constexpr auto str_login_res = "/from/tsp/login_res";
constexpr auto str_logout_res = "/from/tsp/logout_res";
constexpr auto str_heart_beat_sleep_res = "/from/tsp/heartbeat_sleep_res";
constexpr auto str_login_pass_check_platform_res = "/to/tsp/login_pass_check_platform_res";
constexpr auto str_logout_pass_check_platform_res = "/to/tsp/logout_pass_check_platform_res";

TspProxy::TspProxy():heartbeat_sleep_timer_{[this](const boost::any& ) noexcept { heartbeat_sleep();}}{
    tsp_client::tls_tcp_config tcp_cfg;
    tcp_cfg.port = 8888;
    tcp_cfg.server_ip = "10.58.1.17";
    tcp_cfg.support_tls = false;
    tsp_client_ = std::make_unique<tsp_client::TspClient>(tcp_cfg);
    register_local_event_topic(5, 200, str_login_res);     // 终端登录消息
    register_local_event_topic(5, 201, str_logout_res);    // 终端登出消息
    register_local_event_topic(5, 203, str_heart_beat_sleep_res);  // 终端休眠心跳消息
    register_local_event_topic(5, 204, str_login_pass_check_platform_res);   // 终端上报登录过检平台消息
    register_local_event_topic(5, 205, str_logout_pass_check_platform_res);  // 终端上报登出过检平台消息
}

void TspProxy::initialize(){
    /*
    std::weak_ptr<TspProxy> self = shared_from_this();
    local_event_topics_handler_map_.emplace(str_heart_beat_sleep_res, [this, self](const MessageHeader& header, const std::vector<uint8_t> &msg_body){
        if(self.lock()){
            handle_heartbeat_sleep_response(header, msg_body);
        }
    });
    local_event_topics_handler_map_.emplace(str_login_res, [this, self](const MessageHeader& header, const std::vector<uint8_t> &msg_body){
        if(self.lock()){
            handle_login_response(header, msg_body);
        }
    });
    local_event_topics_handler_map_.emplace(str_logout_res, [this, self](const MessageHeader& header, const std::vector<uint8_t> &msg_body){
        if(self.lock()){
            handle_logout_response(header, msg_body);
        }
    });
    local_event_topics_handler_map_.emplace(str_login_pass_check_platform_res, [this, self](const MessageHeader& header, const std::vector<uint8_t> &msg_body){
        if(self.lock()){
            handle_heartbeat_sleep_response(header, msg_body);
        }
    });
    local_event_topics_handler_map_.emplace(str_logout_pass_check_platform_res, [this, self](const MessageHeader& header, const std::vector<uint8_t> &msg_body){
        if(self.lock()){
            handle_heartbeat_sleep_response(header, msg_body);
        }
    });

    tsp_client_->set_message_published_handle([this, self](const std::vector<uint8_t> &msg, bool published){
        if(self.lock()){
            on_message_published(msg, published);
        }
    });*/
}

void TspProxy::shutdown() {
    TB_LOG_INFO("TspProxy shutting down");
    tsp_client_.reset(nullptr);
    heartbeat_sleep_timer_.stop_and_join_run_thread();
}

void TspProxy::register_event_callback(const reply_callback_t &call_back) {
    reply_callback_ = call_back;
}

 void TspProxy::on_wake_up(){
    if(conn_status_ == tsp_client::connect_state_t::login){ // 登录状态
        // TODO 接收白名单的短信或者来电时立即进行一次心跳
    }else{ // 非登录状态尝试登录
        if(!heartbeat_sleep_timer_.is_running()) {
            auto cyclic = std::chrono::seconds(u_heartBeatInterval_);
            common::Timer::Clock::duration timer_period{cyclic};
            TB_LOG_INFO("TspProxy::on_wake_up start timer to send heartbeat_sleep");
            heartbeat_sleep_timer_.start_periodic_immediate(timer_period);
        }
    }
 }

void TspProxy::publish_message(const std::string &str_topic, const std::vector<uint8_t> &msg) {
    //std::string str = convert_to_hex_string(msg);
    //TB_LOG_INFO("TspProxy::publish_message topic:%s size:%d, content:%s", str_topic.c_str(), msg.size(), str.c_str());
    //TB_LOG_INFO("TspProxy::publish_message topic:%s size:%d", str_topic.c_str(), msg.size());
    MessageHeader header;
    if(msg.size() < header.get_ipc_header_size()) {
        TB_LOG_ERROR("TspProxy::publish_message size is too small");
        return;
    }
    std::vector<uint8_t> message_body;
    parse_header_and_msg_body(msg, header, message_body);
    //header.dump();
    /*
    uint8_t u_sid = message_body[2U];
    uint8_t u_mid = message_body[3U];
    if(check_pass_conn_status_ != tsp_client::connect_state_t::login && u_sid == 5) {
        if(u_mid == 100 || u_mid == 101) {
            header.body_length = message_body.size(); // 如果消息加密, 则为加密后的size
            std::vector<uint8_t> new_msg;
            header.serialize(new_msg); // 消息头
            std::copy(message_body.begin(), message_body.end(), std::back_inserter(new_msg)); // 消息体
            on_message_published(new_msg,false);
            return;
        }
    }*/
    publish_msg_to_cloud(str_topic, header, message_body);
}

void TspProxy::publish_msg_to_cloud(const std::string &str_topic, MessageHeader& header, std::vector<uint8_t> &msg_body) {
    if(header.encrypt_flag == 1) { // 加密消息
        std::vector<uint8_t> encrypt_message_body;
        uint16_t random = (msg_body[0] << 8) | msg_body[1];
        if(!encrypt(vec_aes_key_, random, msg_body, encrypt_message_body)){
            TB_LOG_ERROR("TspProxy::publish_msg_to_cloud encrypt failed");
            return;
        }
        msg_body.swap(encrypt_message_body);
    }
    std::vector<uint8_t> vec_transfer_message;
    last_publish_tm_ = get_timestamp();
    std::vector<uint8_t> new_msg;
    header.body_length = msg_body.size(); // 如果消息加密, 则为加密后的size
    header.serialize(new_msg); // 消息头
    std::copy(msg_body.begin(), msg_body.end(), std::back_inserter(new_msg)); // 消息体
    //transfer_message(new_msg, vec_transfer_message);
    //vec_transfer_message.emplace_back(255); // 消息尾
    if (tsp_client_ != nullptr){
        tsp_client_->publish(str_topic, new_msg);
    }
    //TB_LOG_INFO("TspProxy::publish_msg_to_cloud finish %s", convert_to_hex_string(vec_transfer_message).c_str());
}

void TspProxy::start_to_connect(){
    TB_LOG_INFO("TspProxy::start_to_connect");
    tsp_client_->set_connection_changed_handle([this](const std::string &host, std::size_t port, tsp_client::connect_state_t status) {
        on_connection_changed(host, port, status);
        if (status == tsp_client::connect_state_t::ok) {
            TB_LOG_INFO("TspProxy connected to %s:%d", host.c_str(), port);
            //start_to_login();
            start_to_logout();
            last_reply_tm_ = get_timestamp();
        }
        if (status == tsp_client::connect_state_t::failed) {
            TB_LOG_INFO("TspProxy failed to connect from %s:%d", host.c_str(), port);
        }
    });

    tsp_client_->set_message_received_handle([this](const std::vector<uint8_t> &message){
        on_message_arrive(message);
    });
    bool connected{false};
    int n_max_try{1}; // 最多重试1次
    while (!connected && n_max_try > 0) {
        connected = tsp_client_->connect(); // retry to connect to platform
        std::this_thread::sleep_for(std::chrono::seconds(2));
        --n_max_try;
    }
}

std::string TspProxy::get_conn_state_info(connect_state_t conn_state) {
    std::string str_info{"drop"};
    if(conn_state == connect_state_t::start){
         str_info = "start";
    }else if(conn_state == connect_state_t::failed){
        str_info = "failed";
    }else if(conn_state == connect_state_t::ok){
        str_info = "ok";
    }else if(conn_state == connect_state_t::sleeping){
        str_info = "sleeping";
    }else if(conn_state == connect_state_t::login){
        str_info = "login";
    }else if(conn_state == connect_state_t::login_failed){
        str_info = "login_failed";
    }else if(conn_state == connect_state_t::logout){
        str_info = "logout";
    }else if(conn_state == connect_state_t::stopped){
        str_info = "stopped";
    }
    return str_info;
}

void TspProxy::on_connection_changed(const std::string& host, std::size_t port, connect_state_t conn_state) {
    TB_LOG_INFO("TspProxy::on_connection_changed host:%s, port:%d", host.c_str(), port);
    on_connection_state_changed(conn_state);
}

void TspProxy::send_conn_status_to_android() {
    TB_LOG_INFO("TspProxy::send_conn_status_to_android");
    static const std::string str_conn_status_topic = "/from/tsp/conn_status";
    uint8_t u_state{0};
    if(conn_status_ == connect_state_t::ok){
        u_state = 1;
    }else if(conn_status_ == connect_state_t::failed){
        u_state = 2;
    }else if(conn_status_ == connect_state_t::login){
        u_state = 4;
    }else if(conn_status_ == connect_state_t::logout){
        u_state = 5;
    }
    std::vector<uint8_t> data;
    data.emplace_back(u_state);
    //heart beat interval
    data.emplace_back(u_heartBeatInterval_ >> 8);
    data.emplace_back(u_heartBeatInterval_ & 0xFF);

    if(reply_callback_ != nullptr) {
        reply_callback_(str_conn_status_topic, data);
    }else{
        TB_LOG_ERROR("TspProxy::on_connection_changed got empty reply_callback_");
    }
}

void TspProxy::send_pass_check_conn_status_to_android() {
    TB_LOG_INFO("TspProxy::send_pass_check_conn_status_to_android");
    static const std::string str_conn_status_topic = "/to/android/32960/conn_status";
    uint8_t u_state{0}; // 未登入
    if(check_pass_conn_status_ == connect_state_t::login){
        u_state = 1;   // 已登入
    }
    std::vector<uint8_t> data;
    //data.emplace_back(0x02);
    //data.emplace_back(0x01);
    //data.emplace_back(0); // 直联32960未登录
    data.emplace_back(u_state); // 长安TSP过检平台登录状态

    if(reply_callback_ != nullptr) {
        reply_callback_(str_conn_status_topic, data);
    }else{
        TB_LOG_ERROR("TspProxy::on_connection_changed got empty reply_callback_");
    }
}

void TspProxy::notify_from_muc_power_status(const std::vector<uint8_t> &msg) {
    if(msg.size() < 1) {
        TB_LOG_ERROR("TspProxy::notify_from_muc_power_status msg size:%d is small than 4", msg.size());
        return;
    }
    uint8_t index{0U}; // start index of data
    uint8_t u_power_status = msg[index];
    TB_LOG_INFO("TspProxy::notify_from_muc_power_status status:%d, cur conn_status:%s pass_check_conn_status:%s ", u_power_status,
                get_conn_state_info(conn_status_).c_str(), get_conn_state_info(check_pass_conn_status_).c_str());

    if(conn_status_ != tsp_client::connect_state_t::login) {
        return;
    }
    if(u_power_status == 0x0) { // 0x0=OFF, IGN-OFF, logout
        if(check_pass_conn_status_ == tsp_client::connect_state_t::login) {
            //send_logout_pass_check_platform();
        }
    }else{
        if(check_pass_conn_status_ != tsp_client::connect_state_t::login) {
            //send_login_pass_check_platform();
        }
    }
}

void TspProxy::on_connection_state_changed(connect_state_t conn_state){
    TB_LOG_INFO("TspProxy::on_connection_state_changed state:%s", get_conn_state_info(conn_state).c_str());

    /*if(conn_state == connect_state_t::login && !heartbeat_sleep_timer_.is_running()) {
        auto cyclic = std::chrono::seconds(u_heartBeatInterval_);
        common::Timer::Clock::duration timer_period{cyclic};
        TB_LOG_INFO("TspProxy::on_connection_changed start timer to send heartbeat_sleep");
        heartbeat_sleep_timer_.start_periodic_immediate(timer_period);
    }
    if(conn_state == connect_state_t::logout) {
        if(heartbeat_sleep_timer_.is_running()){
            heartbeat_sleep_timer_.stop();
            TB_LOG_INFO("TspProxy::on_connection_changed stop timer to send heartbeat_sleep");
        }
    }*/
    conn_status_ = conn_state;
    send_conn_status_to_android();

    if(conn_status_ == connect_state_t::login){
        // 断线重连并登录后,如果上次登录了过检平台,则发送登录过检平台消息
        if(check_pass_conn_status_ != tsp_client::connect_state_t::login){
            //send_login_pass_check_platform();
        }else{
            //send_pass_check_conn_status_to_android();
        }
    }
}

void TspProxy::on_message_arrive(const std::vector<uint8_t>& msg) {
    last_reply_tm_ = get_timestamp();
    std::vector<uint8_t> de_transfer_msg_data;
    de_transfer_message(msg, de_transfer_msg_data);

    std::string str = convert_to_hex_string(de_transfer_msg_data);
    TB_LOG_INFO("TspProxy::on_message_arrive msg size:%d content:%s", de_transfer_msg_data.size(), str.c_str());
    MessageHeader header;
    header.parse(de_transfer_msg_data);

    uint16_t head_size = header.get_header_size();
    uint16_t body_length = de_transfer_msg_data.size() - head_size;
    std::vector<uint8_t> message_body(de_transfer_msg_data.data() + head_size,
                                      de_transfer_msg_data.data() + head_size + body_length);
    //header.dump();

    if(header.encrypt_flag == 1) { // 解密消息
        std::vector<uint8_t> de_crypt_message_body;
        uint16_t random {0U}; // not used; (message_body[0] << 8) | message_body[1];
        if(!decrypt(vec_aes_key_, random, message_body, de_crypt_message_body)){
            TB_LOG_ERROR("TspProxy::on_message_arrive decrypt failed");
            return;
        }
        message_body.swap(de_crypt_message_body);
    }

    //MessageBody msg_body;
    //msg_body.parse(message_body);
    //msg_body.dump();

    std::string str_topic{};
    bool is_local = get_event_topic(message_body, str_topic);
    if(is_local){
        const auto iter = local_event_topics_handler_map_.find(str_topic);
        if(iter != local_event_topics_handler_map_.end()){
            TB_LOG_INFO("TspProxy::on_message_arrive got local message topic:%s", str_topic.c_str());
            iter->second(header, message_body);
        }else {
            TB_LOG_ERROR("TspProxy::on_message_arrive got local empty reply_callback_");
        }
        return;
    }

    std::vector<uint8_t> raw_msg; // 消息头
    header.body_length = message_body.size();
    header.serialize_to_ipc_msg(raw_msg);

    std::copy(message_body.begin(), message_body.end(), std::back_inserter(raw_msg));

    if(reply_callback_ != nullptr) {
        TB_LOG_INFO("TspProxy::on_message_arrive got message topic:%s size:%d", str_topic.c_str(), raw_msg.size());
        reply_callback_(str_topic, raw_msg);
    }else {
        TB_LOG_ERROR("TspProxy::on_message_arrive got empty reply_callback_");
    }
}

tsp_client::connect_state_t TspProxy::get_connect_state(){
    return conn_status_;
}

 bool TspProxy::decrypt(const std::vector<uint8_t> &vec_token, uint16_t random, const std::vector<uint8_t> &whisper_message_body, std::vector<uint8_t> &plain_message_body) {
    if(vec_token.size() < 16){
        TB_LOG_ERROR("TspProxy::decrypt vec_token size is small than 16");
        return false;
    }
    try {
        EVPKey aes_key;
        EVPIv aes_iv;
        std::copy_n(vec_token.begin(), 16, aes_key.begin());
        //aes_iv[0] = random >> 8;
        //aes_iv[1] = random & 0xFF;
        EVPCipher cipher(EVP_aes_128_ecb(), aes_key, aes_iv, false);
        cipher.update(whisper_message_body);
        plain_message_body = std::vector<uint8_t>(cipher.cbegin(), cipher.cend());
    } catch (EVPCipherException& e){
        TB_LOG_ERROR("TspProxy::decrypt update error:%s", e.what());
        return false;
    }
    return true;
 }

 bool TspProxy::encrypt(const std::vector<uint8_t> &vec_token, uint16_t random, const std::vector<uint8_t> &plain_message_body, std::vector<uint8_t> &whisper_message_body) {
    if(vec_token.size() < 16){
        TB_LOG_ERROR("TspProxy::encrypt vec_token size is small than 16");
        return false;
    }
    try {
        EVPKey aes_key;
        EVPIv aes_iv;
        std::copy_n(vec_token.begin(), 16, aes_key.begin());
        //aes_iv[0] = random >> 8;
        //aes_iv[1] = random & 0xFF;
        EVPCipher cipher(EVP_aes_128_ecb(), aes_key, aes_iv, true);
        cipher.update(plain_message_body);
        whisper_message_body = std::vector<uint8_t>(cipher.cbegin(), cipher.cend());
    } catch (EVPCipherException& e){
        TB_LOG_ERROR("TspProxy::encrypt update error:%s", e.what());
        return false;
    }
    return true;
 }

void TspProxy::transfer_message(const std::vector<uint8_t> &message, std::vector<uint8_t> &transferred_message) {
    transferred_message.push_back(message[0]);
    for(size_t i = 1; i < message.size(); ++i) {
        uint8_t uchar = message[i];
        if(uchar == 202U || uchar == 87U || uchar == 255U || uchar == 0x3D) {
            transferred_message.push_back(0x3D);
            uint8_t data = uchar^0x3D;
            transferred_message.push_back(data);
        }else {
            transferred_message.push_back(uchar);
        }
    }
}

void TspProxy::de_transfer_message(const std::vector<uint8_t> &message, std::vector<uint8_t> &de_transferred_message) {
    //需要对消息除了 LinkHeader 和 LinkTail 外的部分进行转义后再进行传输
    de_transferred_message.push_back(message[0]);
    for(size_t i = 1; i < message.size() - 1; ++i) {
        uint8_t uchar = message[i];
        if(uchar != 0x3D){
            de_transferred_message.push_back(uchar);
        }else {
            ++i;
            uint8_t data = message[i];
            data = data ^ 0x3D;
            de_transferred_message.push_back(data);
        }
    }
}

void TspProxy::transfer_message_data(const std::vector<uint8_t> &data, std::vector<uint8_t> &transferred_data) {
    for(auto uchar: data) {
        if(uchar == 202U || uchar == 87U || uchar == 255U || uchar == 0x3D) {
            transferred_data.push_back(0x3D);
            uint8_t data = uchar^0x3D;
            transferred_data.push_back(data);
        }else {
            transferred_data.push_back(uchar);
        }
    }
}

void TspProxy::de_transfer_message_data(const std::vector<uint8_t> &data, std::vector<uint8_t> &de_transferred_data) {
    for(auto iter = data.begin(); iter != data.end(); ++iter) {
        if(*iter != 0x3D){
            de_transferred_data.push_back(*iter);
        }else {
            ++iter;
            uint8_t data = *iter;
            data = data ^ 0x3D;
            de_transferred_data.push_back(data);
        }
    }
}

void TspProxy::parse_header_and_msg_body(const std::vector<uint8_t> &msg,
                                         MessageHeader &header,
                                         std::vector<uint8_t> &msg_body) {
    // 远控发的消息头只包含: StatusCode AckFlag RequestId EncryptFlag BodyLength
    static uint8_t rcvs_header_size = sizeof(header.status_code) + sizeof(header.ack_flag)
                                      + sizeof(header.request_id) + sizeof(header.encrypt_flag) + sizeof(header.body_length);

    header.status_code = msg[0];    // StatusCode 状态（错误）码
    header.ack_flag = msg[1];       // AckFlag 应答标识
    // Request Id
    for(uint32_t i = 0; i < 6U; ++i) {
        header.request_id[i] = msg[2U+i];
    }

    header.encrypt_flag = msg[8U];   // EncryptFlag 加密标识
    header.body_length = static_cast<std::uint16_t>(((msg[9U] & 0xFF) << 8) |
                                                        (msg[10U] & 0xFF));    // BodyLength Message Body体的长度
    header.link_header = 202;
    header.port_version = 200;
    // 终端ID号
    std::vector<uint8_t> vec_tuid;
    //CloudInfo::get_instance().get_tuid(header.tuid, 16U);
    std::copy(msg.data()+ rcvs_header_size, msg.data()+ rcvs_header_size + header.body_length, std::back_inserter(msg_body));
}

bool TspProxy::get_event_topic(const std::vector<uint8_t>& msg_body, std::string &str_topic) {
    bool is_local{true};
    if(msg_body.size() < 4) {
        TB_LOG_ERROR("TspProxy::get_event_topic msg body is too short");
        return is_local;
    }
    uint8_t u_sid = msg_body[2U];
    uint8_t u_mid = msg_body[3U];
    uint32_t event_id = u_sid << 8 | u_mid;
    const auto iter = local_event_topics_map_.find(event_id);
    if(iter != local_event_topics_map_.end()){
        str_topic = iter->second;
        TB_LOG_INFO("TspProxy::get_event_topic sid:%d, mid:%d, topic:%s", u_sid, u_mid, str_topic.c_str());
        return is_local;
    }
    is_local = false;
    //str_topic = CloudManager::get_instance().get_event_topic(u_sid, u_mid);
    TB_LOG_INFO("TspProxy::get_event_topic sid:%d, mid:%d, topic:%s", u_sid, u_mid, str_topic.c_str());
     return is_local;
}

void TspProxy::start_to_login() {
    TB_LOG_INFO("TspProxy::start_to_login");
    MessageHeader header;
    header.link_header = 202;
    header.port_version = 200;
    header.status_code = 0;
    header.ack_flag = 1; // 1代表请求消息, 0代表响应消息

    std::array<uint8_t, 6> array_request_id {1,2,3,4,5,6};//common::random_utility::generate_request_id(u_seed_);
    for(uint8_t i = 0; i < 6; i++){
         header.request_id[i] = array_request_id[i];
    }
    // 终端ID号
    //CloudInfo::get_instance().get_tuid(header.tuid, 16U);
    header.encrypt_flag = 0; // 登录不加密

    MessageBody body;
    body.random = 1;//common::random_utility::GenerateRandom();
    body.sid = 5;
    body.mid = 200;
    TLV tlv(true);
    tlv.type = 4007; // 设备时间戳
    std::vector<uint8_t> vec_dec_tm;
    uint64_t u_device_tm{}; //
    //CloudManager::get_instance().get_device_time(vec_dec_tm, u_device_tm); // 毫秒
    tlv.value =  vec_dec_tm;
    tlv.short_length = tlv.value.size();
    body.content.push_back(tlv);

    tlv.type = 4008; // 登录签名 sha256(md5(终端证书公钥文件)+ deviceTime)
    std::string str_md5{};// CloudManager::get_instance().md5_public_key_file();
    transform(str_md5.begin(),str_md5.end(), str_md5.begin(), toupper);
    std::string str_sign{}; //CloudManager::get_instance().sha256_sign(str_md5, u_device_tm);
    transform(str_sign.begin(),str_sign.end(), str_sign.begin(), toupper);
    tlv.value.clear();
    convert_hex_string_to_vector(str_sign, tlv.value);
    tlv.short_length = tlv.value.size();
    body.content.push_back(tlv);

    tlv.value.clear();
    tlv.type = 4011; // 整车vin
    std::string str_vin {}; // CloudInfo::get_instance().get_vin();
    std::copy(str_vin.begin(), str_vin.end(), std::back_inserter(tlv.value));
    tlv.short_length = tlv.value.size();
    body.content.push_back(tlv);

    tlv.value.clear();
    tlv.type = 4014; // 终端当前软件版本
    //CloudManager::get_instance().get_software_version(tlv.value);
    tlv.short_length = tlv.value.size();
    body.content.push_back(tlv);

    tlv.value.clear();
    tlv.type = 4015; // 终端当前硬件版本
    //CloudManager::get_instance().get_hardware_version(tlv.value);
    tlv.short_length = tlv.value.size();
    body.content.push_back(tlv);

    tlv.value.clear();
    tlv.type = 20200; // 是否过检转发
    tlv.value.push_back(0); // 0不转发, 1转发
    tlv.short_length = tlv.value.size();
    body.content.push_back(tlv);

    std::vector<uint8_t> vec_msg_body;
    body.serialize(vec_msg_body);
    header.body_length = vec_msg_body.size(); // 如果消息加密, 则为加密后的size

    publish_msg_to_cloud("/to/tsp/login", header, vec_msg_body);
}

void TspProxy::handle_login_response(const MessageHeader& header, const std::vector<uint8_t> &msg_body){
    TB_LOG_INFO("TspProxy::handle_login_response ");
    if(header.status_code != static_cast<uint8_t>(StatusCode::kNormal)){
        TB_LOG_ERROR("TspProxy::handle_login_response header status code:%d,failed", header.status_code);
        return;
    }
    MessageBody body;
    bool noerr = body.parse(msg_body);
    if(!noerr){
        TB_LOG_ERROR("TspProxy::handle_login_response parse msg_body failed");
        return;
    }
    for(const auto& tlv : body.content) {
        if(tlv.type == 4000U){
            if(tlv.value.size() > 0){
                if(tlv.value[0] == 1){
                    TB_LOG_INFO("TspProxy::handle_login_response login success, pass_check_conn_status:%s",
                                get_conn_state_info(check_pass_conn_status_).c_str());
                    conn_status_ = tsp_client::connect_state_t::login;
                }else{
                    TB_LOG_INFO("TspProxy::handle_login_response login failed 4000=%d.", tlv.value[0]);
                    conn_status_ = tsp_client::connect_state_t::login_failed;
                }
            }else{
                TB_LOG_ERROR("type 4000 value length should large than 0");
                return;
            }
        }else if(tlv.type == 4009U){
            if(tlv.value.size() != 16){
                TB_LOG_ERROR("type 4009 value length should be 16, got:%d", tlv.value.size());
            }
            TB_LOG_INFO("got token:%s", convert_to_hex_string(tlv.value).c_str());
            vec_aes_key_ = tlv.value; // 128位AES密钥, 16bytes
        }
    }

    on_connection_state_changed(conn_status_);
}

void TspProxy::start_to_logout() {
    TB_LOG_INFO("TspProxy::start_to_logout");
    MessageHeader header;
    header.link_header = 202;
    header.port_version = 200;
    header.status_code = 0;
    header.ack_flag = 1; // 1代表请求消息, 0代表响应消息
    std::array<uint8_t, 6> array_request_id{1,2,3,4,5,6};// common::random_utility::generate_request_id(u_seed_);
    for(uint8_t i = 0; i < 6; ++i){
         header.request_id[i] = array_request_id[i];
    }
    // 终端ID号
    //CloudInfo::get_instance().get_tuid(header.tuid, 16U);

    header.encrypt_flag = 0; // 登出不加密

    MessageBody body;
    body.random = 1; // common::random_utility::GenerateRandom();
    body.sid = 5;
    body.mid = 201;

    std::vector<uint8_t> vec_msg_body;
    //body.serialize(vec_msg_body);
    header.body_length = vec_msg_body.size();

    publish_msg_to_cloud("/to/tsp/logout", header, vec_msg_body);
}

void TspProxy::handle_logout_response(const MessageHeader& header, const std::vector<uint8_t> &msg_body){
    TB_LOG_INFO("TspProxy::handle_logout_response ");
    if(header.status_code != static_cast<uint8_t>(StatusCode::kNormal)){
        TB_LOG_ERROR("TspProxy::handle_logout_response header status code:%d,failed", header.status_code);
        return;
    }
    MessageBody body;
    bool noerr = body.parse(msg_body);
    if(!noerr){
        TB_LOG_ERROR("TspProxy::handle_logout_response parse msg_body failed");
        return;
    }
    conn_status_ = tsp_client::connect_state_t::logout;
    on_connection_state_changed(conn_status_);
}

void TspProxy::register_local_event_topic(uint8_t sid, uint8_t mid, const std::string &topic) {
    uint32_t event_id = sid << 8 | mid;
    local_event_topics_map_[event_id] = topic;
}

void TspProxy::heartbeat_sleep() {
    TB_LOG_INFO("TspProxy::heartbeat_sleep conn_status:%s pass_check_conn_status:%s",
                get_conn_state_info(conn_status_).c_str(), get_conn_state_info(check_pass_conn_status_).c_str());
    // reconnect stopped, so try to connect to server
    if(conn_status_ == tsp_client::connect_state_t::stopped) {
        TB_LOG_INFO("TspProxy::heartbeat_sleep start to reconnect to remote server");
        start_to_connect();
        return;
    }

    // in case of no login response received, send login again
    if(conn_status_ == tsp_client::connect_state_t::ok) {
        TB_LOG_INFO("TspProxy::heartbeat_sleep start to login to remote server");
        start_to_logout();
        return;
    }

    if(conn_status_ != tsp_client::connect_state_t::login &&
       conn_status_ != tsp_client::connect_state_t::failed) { // connect failed
        return;
    }

    if(last_publish_tm_ == 0 && conn_status_ == tsp_client::connect_state_t::failed) { // never connect successful to server
        start_to_connect();
        return;
    }

    if((last_publish_tm_ > last_reply_tm_) && (last_publish_tm_ - last_reply_tm_)  > u_heartBeatInterval_) {
        TB_LOG_INFO("TspProxy::heartbeat_sleep start to reconnect in case of longtime no response last publish tm:%lld "
                    "last reply tm:%lld", last_publish_tm_, last_reply_tm_);
        tsp_client_->disconnect();
        start_to_connect();
        return;
    }

    uint64_t heart_sleep_tm_interval = get_timestamp() - last_publish_tm_;
    if(heart_sleep_tm_interval < 120U) {
        TB_LOG_INFO("TspProxy::heartbeat_sleep skip to send, since last time publish interval:%lld", heart_sleep_tm_interval);
        return;
    }

    MessageHeader header;
    header.link_header = 202;
    header.port_version = 200;
    header.status_code = 0;
    header.ack_flag = 1; // 1代表请求消息, 0代表响应消息
    std::array<uint8_t, 6> array_request_id{1,2,3,4,5,6}; //common::random_utility::generate_request_id(u_seed_);
    for(uint8_t i = 0; i < 6; i++){
        header.request_id[i] = array_request_id[i];
    }
    // 终端ID号
    //CloudInfo::get_instance().get_tuid(header.tuid, 16U);
    header.encrypt_flag = 0; // 休眠心跳不加密

    MessageBody body;
    body.random = 1;// common::random_utility::GenerateRandom();
    body.sid = 5;
    body.mid = 203;

    TLV tlv(true);
    tlv.type = 4007; // 设备时间戳
    std::vector<uint8_t> vec_dec_tm;
    uint64_t u_device_tm = 0; // CloudManager::get_instance().get_device_time();
    //CloudManager::get_instance().get_device_time(vec_dec_tm, u_device_tm); // 毫秒
    tlv.value =  vec_dec_tm;
    tlv.short_length = tlv.value.size();
    body.content.push_back(tlv);

    std::vector<uint8_t> vec_msg_body;
    body.serialize(vec_msg_body);
    header.body_length = vec_msg_body.size();

    publish_msg_to_cloud("/to/tsp/heartbeat_sleep", header, vec_msg_body);
}

void TspProxy::handle_heartbeat_sleep_response(const MessageHeader& header, const std::vector<uint8_t> &msg_body) {
    TB_LOG_INFO("TspProxy::handle_heartbeat_sleep_response ");
    if(header.status_code != static_cast<uint8_t>(StatusCode::kNormal)){
        TB_LOG_ERROR("TspProxy::handle_heartbeat_sleep_response header status code:%d,failed", header.status_code);
        return;
    }
    MessageBody body;
    bool noerr = body.parse(msg_body);
    if(!noerr){
        TB_LOG_ERROR("TspProxy::handle_heartbeat_sleep_response parse msg_body failed");
        return;
    }
    for(const auto& tlv : body.content) {
        if(tlv.type == 4000U){
            if(tlv.value.size() > 0){
                if(tlv.value[0] == 0){ // 表示TSP收到结果
                    TB_LOG_INFO("TspProxy::handle_heartbeat_sleep_response send sleep msg success.");
                }else{
                    TB_LOG_INFO("TspProxy::handle_heartbeat_sleep_response send sleep msg failed.");
                }
            }else{
                TB_LOG_ERROR("type 4000 value length should large than 0");
                return;
            }
        }
    }
}

void TspProxy::on_message_published(const std::vector<uint8_t> &msg, bool published) {
    MessageHeader header;
    if(msg.size() < header.get_header_size() + 3U) {
        TB_LOG_ERROR("TspProxy::on_message_published invalid msg size:%d: content:%s", msg.size(), convert_to_hex_string(msg).c_str());
        return;
    }
    header.parse(msg);
    uint8_t u_sid = msg[header.get_header_size() + 2U];
    uint8_t u_mid = msg[header.get_header_size() + 3U];

    TB_LOG_INFO("TspProxy::on_message_published sid:%d mid:%d published:%d", u_sid, u_mid, published);

    uint32_t event_id = u_sid << 8 | u_mid;
    const auto iter = local_event_topics_map_.find(event_id);
    if(iter != local_event_topics_map_.end()){
        TB_LOG_INFO("TspProxy::on_message_published local event");
        return;
    }

    std::vector<uint8_t> raw_msg; // 消息头
    Packet packet(raw_msg);
    packet << u_sid << u_mid;
    packet.serialize(header.request_id, 6U);

    uint8_t  status{0}; // 0,成功; 1失败
    if(!published){
        status = 1;
    }
    packet << status;
    if(reply_callback_ != nullptr) {
        reply_callback_("/to/tsp/send_result", raw_msg);
    }else {
        TB_LOG_ERROR("TspProxy::on_message_arrive got empty reply_callback_");
    }
}