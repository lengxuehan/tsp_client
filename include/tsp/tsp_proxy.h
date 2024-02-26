//
// Created by wuting.xu on 23-11-15.
//

#pragma once

#include <condition_variable>
#include <unordered_map>
#include "client/tsp_client.h"
#include "common/timer.h"
#include "packages/messages.h"


using namespace tsp_client;

class TspProxy: public std::enable_shared_from_this<TspProxy>{
public:
    typedef std::function<void(const std::string&, const std::vector<uint8_t>&)> reply_callback_t;
    using connect_state_t = tsp_client::connect_state_t;
    TspProxy();
    virtual ~TspProxy() = default;
    /*
     * Ensure that objects of this type are not copyable and not movable.
    */
    TspProxy(TspProxy&& other) = delete;
    TspProxy(const TspProxy& other) = delete;
    TspProxy& operator=(TspProxy&& other) = delete;
    TspProxy& operator=(const TspProxy& other) = delete;

    void initialize();

    void publish_message(const std::string &str_topic, const std::vector<uint8_t> &msg);

    void start_to_connect();

    /**
     * @brief 通过短信或者加电或者发动机启动唤醒
     */
    void on_wake_up();

    void register_event_callback(const reply_callback_t &call_back);

    connect_state_t get_connect_state();

    /**
     * stops service handling, join threads and exit threads. Method returns
     * as soon as services have been stopped.
     */
    void shutdown();

    void send_conn_status_to_android();

    void send_pass_check_conn_status_to_android();

    /**
    * @brief 从云端获取远控的响应或者指令是否发给远控
    * @param can_be_published  true,发给远控; false, 丢掉,不发给远控
    */
    void can_be_published_to_rvcs(bool can_be_published);

    void notify_from_muc_power_status(const std::vector<uint8_t> &msg);

    void on_rtc_timer_call_back();
private:
    /**
     * @brief 根据云端登录响应得到的token，解密tcp协议message body得到明文.
     * @param[in] vec_token : 用于解密的随机数种子，长度128bit
     * @param[in] random : 用于解密的盐，长度2字节
     * @param[in] whisper_message_body : 云端下发的一段密文。
     * @param[out] plain_message_body : 解密后得到的明文
     * @retval bool 是否解密成功.
     */
    bool decrypt(const std::vector<uint8_t> &vec_token, uint16_t random, const std::vector<uint8_t> &whisper_message_body, std::vector<uint8_t> &plain_message_body);
     /**
     * @brief 根据云端登录响应得到的token，加密tcp协议message body得到密文.
     * @param[in] vec_token : 用于加密的随机数种子，长度128bit
     * @param[in] random : 用于加密的盐，长度2字节
     * @param[in] plain_message_body: 待上传云端的一段明文。
     * @param[out] whisper_message_body : 待上传云端的明文加密后得到的密文
     * @retval bool 是否加密成功.
     */
    bool encrypt(const std::vector<uint8_t> &vec_token, uint16_t random, const std::vector<uint8_t> &plain_message_body, std::vector<uint8_t> &whisper_message_body);

    /**
     * @brief 为避免消息出现黏包时，查找消息头出错而导致消息解析失败，对message body及部分header转义后再传输
     * @param[in] message_body: 转义前的一段明文或者密文
     * @param[out] transferred_message: 转义后的一段明文或者密文
     */
    void transfer_message(const std::vector<uint8_t> &message, std::vector<uint8_t> &transferred_message);
    /**
     * @brief 收到云端传输过来的明文或者密文并转义后的message body
     * @param[in] message: 收到云端传输过来的message body
     * @param[out] detransferred_message: 去除转义后的一段明文或者密文
     */
    void de_transfer_message(const std::vector<uint8_t> &message, std::vector<uint8_t> &detransferred_message);

    void transfer_message_data(const std::vector<uint8_t> &data, std::vector<uint8_t> &transferred_data);
    void de_transfer_message_data(const std::vector<uint8_t> &data, std::vector<uint8_t> &de_transferred_data);

private:
    void on_connection_changed(const std::string& host, std::size_t port, connect_state_t conn_state);
    void on_connection_state_changed(connect_state_t conn_state);
    void on_message_arrive(const std::vector<uint8_t>& msg);
    std::string get_conn_state_info(connect_state_t conn_state);

    void parse_header_and_msg_body(const std::vector<uint8_t>& msg, MessageHeader& header, std::vector<uint8_t>& msg_body);
    /**
     * @brief 根据云端下行消息获取消息分发对应的topic
     * @return 是否本地handler处理
     */
    virtual bool get_event_topic(const std::vector<uint8_t>& msg_body, std::string &str_topic);

    void register_local_event_topic(uint8_t sid, uint8_t mid, const std::string &topic);

    /**
     * @brief 发送登录消息给TSP
     */
    void start_to_login();
    void handle_login_response(const MessageHeader& header, const std::vector<uint8_t> &msg_body);

    /**
     * @brief 发送登出消息给TSP
     */
    void start_to_logout();
    void handle_logout_response(const MessageHeader& header, const std::vector<uint8_t> &msg_body);

    /**
     * @brief 发送休眠心跳给TSP
     */
    void heartbeat_sleep();
    void handle_heartbeat_sleep_response(const MessageHeader& header, const std::vector<uint8_t> &msg_body);

    void publish_msg_to_cloud(const std::string &str_topic, MessageHeader& header, std::vector<uint8_t> &msg_body);

    void on_message_published(const std::vector<uint8_t> &msg, bool published);
private:
    common::Timer heartbeat_sleep_timer_;       // 休眠心跳维持timer
    reply_callback_t reply_callback_{nullptr};
    std::atomic<tsp_client::connect_state_t> conn_status_{tsp_client::connect_state_t::stopped};
    std::atomic<tsp_client::connect_state_t> check_pass_conn_status_{tsp_client::connect_state_t::stopped};
    std::unique_ptr<tsp_client::TspClient> tsp_client_{nullptr};
    uint16_t u_heartBeatInterval_{60U}; //以s为单位,终端与服务器间隔时间heartBeatInterval内没有数据交互时发送休眠心跳{ID=5, MID=203}
    uint8_t u_seed_{0U}; // seed of request id
    std::unordered_map<uint32_t, std::string> local_event_topics_map_{}; // SID,MID与topic对应的关系
    typedef std::function<void(const MessageHeader&, const std::vector<uint8_t>&)> local_reply_callback_t;
    std::unordered_map<std::string, local_reply_callback_t> local_event_topics_handler_map_{}; // topic对应的处理函数
    std::vector<uint8_t> vec_aes_key_{};
    uint64_t last_publish_tm_{0}; // 上次发布消息时间
    uint64_t last_reply_tm_{0};   // 上次收到消息时间
};