// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "http/action/stream_load.h"

#include <gtest/gtest.h>

#include <event2/http.h>
#include <event2/http_struct.h>
#include <rapidjson/document.h>

#include "exec/schema_scanner/frontend_helper.h"
#include "http/http_channel.h"
#include "http/http_request.h"
#include "runtime/exec_env.h"
#include "util/doris_metrics.h"

class mg_connection;

namespace doris {

std::string k_response_str;

// Send Unauthorized status with basic challenge
void HttpChannel::send_basic_challenge(HttpRequest* req, const std::string& realm) {
}

void HttpChannel::send_error(HttpRequest* request, HttpStatus status) {
}

void HttpChannel::send_reply(HttpRequest* request, HttpStatus status) {
}

void HttpChannel::send_reply(
        HttpRequest* request, HttpStatus status, const std::string& content) {
    k_response_str = content;
}

void HttpChannel::send_file(HttpRequest* request, int fd, size_t off, size_t size) {
}

extern TLoadTxnBeginResult k_stream_load_begin_result;
extern TLoadTxnCommitResult k_stream_load_commit_result;
extern TLoadTxnRollbackResult k_stream_load_rollback_result;
extern TStreamLoadPutResult k_stream_load_put_result;
extern Status k_stream_load_plan_status;

class StreamLoadActionTest : public testing::Test {
public:
    StreamLoadActionTest() { }
    virtual ~StreamLoadActionTest() { }
    void SetUp() override {
        k_stream_load_begin_result = TLoadTxnBeginResult();
        k_stream_load_commit_result = TLoadTxnCommitResult();
        k_stream_load_rollback_result = TLoadTxnRollbackResult();
        k_stream_load_put_result = TStreamLoadPutResult();
        k_stream_load_plan_status = Status::OK;
        k_response_str = "";
        config::streaming_load_max_mb = 1;
    }
private:
};

TEST_F(StreamLoadActionTest, no_auth) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Fail", doc["Status"].GetString());
}

#if 0
TEST_F(StreamLoadActionTest, no_content_length) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;
    request._headers.emplace(HttpHeaders::AUTHORIZATION, "Basic cm9vdDo=");
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Fail", doc["Status"].GetString());
}

TEST_F(StreamLoadActionTest, unknown_encoding) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;
    request._headers.emplace(HttpHeaders::AUTHORIZATION, "Basic cm9vdDo=");
    request._headers.emplace(HttpHeaders::TRANSFER_ENCODING, "chunked111");
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Fail", doc["Status"].GetString());
}
#endif

TEST_F(StreamLoadActionTest, normal) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;

    struct evhttp_request ev_req;
    ev_req.remote_host = nullptr;
    request._ev_req = &ev_req;

    request._headers.emplace(HttpHeaders::AUTHORIZATION, "Basic cm9vdDo=");
    request._headers.emplace(HttpHeaders::CONTENT_LENGTH, "0");
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Success", doc["Status"].GetString());
}

TEST_F(StreamLoadActionTest, put_fail) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;

    struct evhttp_request ev_req;
    ev_req.remote_host = nullptr;
    request._ev_req = &ev_req;

    request._headers.emplace(HttpHeaders::AUTHORIZATION, "Basic cm9vdDo=");
    request._headers.emplace(HttpHeaders::CONTENT_LENGTH, "16");
    Status status("TestFail");
    status.to_thrift(&k_stream_load_put_result.status);
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Fail", doc["Status"].GetString());
}

TEST_F(StreamLoadActionTest, commit_fail) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;
    struct evhttp_request ev_req;
    ev_req.remote_host = nullptr;
    request._ev_req = &ev_req;
    request._headers.emplace(HttpHeaders::AUTHORIZATION, "Basic cm9vdDo=");
    request._headers.emplace(HttpHeaders::CONTENT_LENGTH, "16");
    Status status("TestFail");
    status.to_thrift(&k_stream_load_commit_result.status);
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Fail", doc["Status"].GetString());
}

TEST_F(StreamLoadActionTest, begin_fail) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;
    struct evhttp_request ev_req;
    ev_req.remote_host = nullptr;
    request._ev_req = &ev_req;
    request._headers.emplace(HttpHeaders::AUTHORIZATION, "Basic cm9vdDo=");
    request._headers.emplace(HttpHeaders::CONTENT_LENGTH, "16");
    Status status("TestFail");
    status.to_thrift(&k_stream_load_begin_result.status);
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Fail", doc["Status"].GetString());
}

#if 0
TEST_F(StreamLoadActionTest, receive_failed) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;
    request._headers.emplace(HttpHeaders::AUTHORIZATION, "Basic cm9vdDo=");
    request._headers.emplace(HttpHeaders::TRANSFER_ENCODING, "chunked");
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Fail", doc["Status"].GetString());
}
#endif

TEST_F(StreamLoadActionTest, plan_fail) {
    DorisMetrics::instance()->initialize("StreamLoadActionTest");
    ExecEnv env;
    StreamLoadAction action(&env);

    HttpRequest request;
    struct evhttp_request ev_req;
    ev_req.remote_host = nullptr;
    request._ev_req = &ev_req;
    request._headers.emplace(HttpHeaders::AUTHORIZATION, "Basic cm9vdDo=");
    request._headers.emplace(HttpHeaders::CONTENT_LENGTH, "16");
    k_stream_load_plan_status = Status("TestFail");
    action.on_header(&request);
    action.handle(&request);

    rapidjson::Document doc;
    doc.Parse(k_response_str.c_str());
    ASSERT_STREQ("Fail", doc["Status"].GetString());
}

}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    doris::CpuInfo::init();
    return RUN_ALL_TESTS();
}
