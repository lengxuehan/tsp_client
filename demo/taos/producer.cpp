#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <condition_variable>
#include <csignal>
#include <inttypes.h>
#include "taos.h"

std::condition_variable cv;
bool running = true;

void signal_init_handler(int) {
    cv.notify_all();
}

static void queryDB(TAOS *taos, char *command) {
    int i;
    TAOS_RES *pSql = NULL;
    int32_t   code = -1;

    for (i = 0; i < 5; i++) {
        if (NULL != pSql) {
            taos_free_result(pSql);
            pSql = NULL;
        }

        pSql = taos_query(taos, command);
        code = taos_errno(pSql);
        if (0 == code) {
            break;
        }
    }

    if (code != 0) {
        fprintf(stderr, "Failed to run %s, reason: %s\n", command, taos_errstr(pSql));
        taos_free_result(pSql);
        taos_close(taos);
        exit(EXIT_FAILURE);
    }

    taos_free_result(pSql);
}

void Test(TAOS *taos, char *qstr, int i);

int main() {
    std::cout << "Hello, World!" << std::endl;

    // connect to server
    char      qstr[1024];
    TAOS *taos = taos_connect("xuwuting", "root", "taosdata", NULL, 0);
    if (taos == NULL) {
        printf("failed to connect to server, reason:%s\n", taos_errstr(NULL));
        exit(1);
    }
    for (int i = 0; i < 1; i++) {
        Test(taos, qstr, i);
    }
    taos_close(taos);
    taos_cleanup();
    signal(SIGINT, &signal_init_handler);

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);
    return 0;
}

uint64_t get_device_time(){
    std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();
    uint64_t u_device_time = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            start_time.time_since_epoch()).count());
    return u_device_time;
}

void Test(TAOS *taos, char *qstr, int index)  {
    printf("==================test at %d\n================================", index);
    //queryDB(taos, "drop database if exists demo");
    queryDB(taos, "create database if not exists demo");
    TAOS_RES *result;
    queryDB(taos, "use demo");

    queryDB(taos, "create table if not exists m1 (ts timestamp, accelerator_status tinyint, brake_status tinyint, "
                  "speed float, b binary(10))");
    printf("success to create table\n");

    int i = 0;
    for (i = 0; i < 30; ++i) {
        uint8_t accelerator_status{0U};
        uint8_t brake_status{1U};
        float speed = ((i+ 3)%10) * 1.0;
        if(speed < 1) {
            printf("ispeed row is 0\n");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        sprintf(qstr, "insert into m1 values (%" PRId64 ", %d, %d, %f, '%s')", get_device_time(),
                accelerator_status, brake_status, speed, "hello");
        printf("qstr: %s\n", qstr);

        // note: how do you wanna do if taos_query returns non-NULL
        // if (taos_query(taos, qstr)) {
        //   printf("insert row: %i, reason:%s\n", i, taos_errstr(taos));
        // }
        TAOS_RES *result1 = taos_query(taos, qstr);
        if (result1 == NULL || taos_errno(result1) != 0) {
            printf("failed to insert row, reason:%s\n", taos_errstr(result1));
            taos_free_result(result1);
            exit(1);
        } else {
            printf("insert row: %i\n", i);
        }
        taos_free_result(result1);
    }
    printf("success to insert rows, total %d rows\n", i);

    // query the records
    sprintf(qstr, "SELECT * FROM m1");
    result = taos_query(taos, qstr);
    if (result == NULL || taos_errno(result) != 0) {
        printf("failed to select, reason:%s\n", taos_errstr(result));
        taos_free_result(result);
        exit(1);
    }

    TAOS_ROW    row;
    int         rows = 0;
    int         num_fields = taos_field_count(result);
    TAOS_FIELD *fields = taos_fetch_fields(result);

    printf("num_fields = %d\n", num_fields);
    printf("select * from table, result:\n");
    // fetch the records row by row
    while ((row = taos_fetch_row(result))) {
        char temp[1024] = {0};
        rows++;
        taos_print_row(temp, row, fields, num_fields);
        printf("%s\n", temp);
    }

    taos_free_result(result);
    printf("====demo end====\n\n");
}

