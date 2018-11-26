/*
 * Project: curve
 * Created Date: 2018-12-26
 * Author: hzchenwei7
 * Copyright (c) 2018 netease
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/mds/nameserver2/session.h"
#include "test/mds/nameserver2/mock_repo.h"
#include "src/common/timeutility.h"

using ::testing::AtLeast;
using ::testing::StrEq;
using ::testing::_;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::DoAll;
using ::testing::SetArgPointee;

namespace curve {
namespace mds {

class SessionTest: public ::testing::Test {
 protected:
    void SetUp() override {
        mockRepo_ = std::make_shared<repo::MockRepo>();
        sessionOptions_.sessionDbName = "curve_mds_repo_test";
        sessionOptions_.sessionUser = "root";
        sessionOptions_.sessionUrl = "localhost";
        sessionOptions_.sessionPassword = "qwer";
        sessionOptions_.leaseTime = 100000;
        sessionOptions_.toleranceTime = 0;
        sessionOptions_.intevalTime = 50000;
    }

    void TearDown() override {
        mockRepo_ = nullptr;
    }

    std::shared_ptr<repo::MockRepo> mockRepo_;
    struct SessionOptions sessionOptions_;
};

TEST_F(SessionTest, testRepoInit) {
    // connectDB失败
    {
        SessionManager sessionManager_(mockRepo_);

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::SqlException));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), false);
    }

    // createDatabase失败
    {
        SessionManager sessionManager_(mockRepo_);

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::SqlException));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), false);
    }

    // useDataBase失败
    {
        SessionManager sessionManager_(mockRepo_);

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::SqlException));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), false);
    }

    // createAllTables失败
    {
        SessionManager sessionManager_(mockRepo_);

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createAllTables())
        .Times(1)
        .WillOnce(Return(repo::SqlException));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), false);
    }
}


TEST_F(SessionTest, testLoadSession) {
    // LoadSession失败
    {
        SessionManager sessionManager_(mockRepo_);

        EXPECT_CALL(*mockRepo_, LoadSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::SqlException));

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createAllTables())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), false);
        sessionManager_.Start();
        sessionManager_.Stop();
    }

    // 数据库中相同file的不同session，如果创建时间相同，出现错误打印
    {
        SessionManager sessionManager_(mockRepo_);

        std::vector<repo::SessionRepo> sessionList;

        sessionList.push_back(repo::SessionRepo("/file1", "sessionID1",
                                "token1", 12345, SessionStatus::kSessionOK ,
                                123456, "127.0.0.1"));

        sessionList.push_back(repo::SessionRepo("/file1", "sessionID2",
                                "token2", 12345, SessionStatus::kSessionOK ,
                                123456, "127.0.0.1"));

        EXPECT_CALL(*mockRepo_, LoadSessionRepo(_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<0>(sessionList),
                            Return(repo::OperationOK)));

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createAllTables())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), false);
        sessionManager_.Start();
        sessionManager_.Stop();
    }

    // 数据库中相同file的不同session，以创建时间晚的为准
    {
        SessionManager sessionManager_(mockRepo_);

        std::vector<repo::SessionRepo> sessionList;

        sessionList.push_back(repo::SessionRepo("/file1", "sessionID1",
                                "token1", 12345, SessionStatus::kSessionOK ,
                                123456, "127.0.0.1"));

        sessionList.push_back(repo::SessionRepo("/file1", "sessionID2",
                                "token2", 12345, SessionStatus::kSessionOK ,
                                1234567, "127.0.0.1"));

        sessionList.push_back(repo::SessionRepo("/file1", "sessionID3",
                                "token3", 12345, SessionStatus::kSessionOK ,
                                12345, "127.0.0.1"));

        EXPECT_CALL(*mockRepo_, LoadSessionRepo(_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<0>(sessionList),
                            Return(repo::OperationOK)));

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createAllTables())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), true);

        // manager start之后，后台线程把无效的session从数据库删除
        EXPECT_CALL(*mockRepo_, DeleteSessionRepo(_))
        .Times(2)
        .WillRepeatedly(Return(repo::OperationOK));

        sessionManager_.Start();

        repo::SessionRepo sessionRepo("/file1", "sessionID2", "token3",
                        sessionOptions_.leaseTime, SessionStatus::kSessionOK,
                                    111, "127.0.0.1");
        EXPECT_CALL(*mockRepo_, QuerySessionRepo(_, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<1>(sessionRepo),
                            Return(repo::OperationOK)));
        sessionManager_.Stop();
    }
}

// Load session之后，从数据库load出session，然后查看是否可以open file
TEST_F(SessionTest, testLoadAndInsertSession) {
    // init时候，从数据库加载session,session未过期,open session失败
    // 等session过期，再次open，open成功
    {
        SessionManager sessionManager_(mockRepo_);

        std::vector<repo::SessionRepo> sessionList;

        sessionList.push_back(repo::SessionRepo("/file1", "sessionID1",
                                "token1", 12345, SessionStatus::kSessionOK,
                                ::curve::common::TimeUtility::GetTimeofDayUs(),
                                "127.0.0.1"));

        EXPECT_CALL(*mockRepo_, LoadSessionRepo(_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<0>(sessionList),
                            Return(repo::OperationOK)));

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createAllTables())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        // init时候，从数据库加载session,session未过期
        ASSERT_EQ(sessionManager_.Init(sessionOptions_), true);

        sessionManager_.Start();

        ProtoSession protoSession;
        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                    &protoSession),
                    StatusCode::kFileOccupied);

        EXPECT_CALL(*mockRepo_, InsertSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, DeleteSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        // 等session过期，再次open，open成功
        usleep(sessionOptions_.leaseTime);
        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                    &protoSession),
                    StatusCode::kOK);

        repo::SessionRepo sessionRepo("/file1", "sessionID1", "token1",
                        sessionOptions_.leaseTime, SessionStatus::kSessionOK,
                                    111, "127.0.0.1");
        EXPECT_CALL(*mockRepo_, QuerySessionRepo(_, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<1>(sessionRepo),
                            Return(repo::OperationOK)));
        sessionManager_.Stop();
    }

    // init时候，从数据库加载session,session已过期，open session成功
    {
        SessionManager sessionManager_(mockRepo_);

        std::vector<repo::SessionRepo> sessionList;

        sessionList.push_back(repo::SessionRepo("/file1", "sessionID1",
                                "token1", 12345, SessionStatus::kSessionStaled,
                                ::curve::common::TimeUtility::GetTimeofDayUs(),
                                "127.0.0.1"));

        EXPECT_CALL(*mockRepo_, LoadSessionRepo(_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<0>(sessionList),
                            Return(repo::OperationOK)));

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createAllTables())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), true);

        sessionManager_.Start();

        EXPECT_CALL(*mockRepo_, InsertSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, DeleteSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        // open成功
        ProtoSession protoSession;
        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                    &protoSession),
                    StatusCode::kOK);

        repo::SessionRepo sessionRepo("/file1", "sessionID1", "token1",
                        sessionOptions_.leaseTime, SessionStatus::kSessionOK,
                                    111, "127.0.0.1");
        EXPECT_CALL(*mockRepo_, QuerySessionRepo(_, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<1>(sessionRepo),
                            Return(repo::OperationOK)));
        sessionManager_.Stop();
    }
}

// 测试insert session流程中的session过期流程
TEST_F(SessionTest, insert_session_test) {
    {
        SessionManager sessionManager_(mockRepo_);

        EXPECT_CALL(*mockRepo_, LoadSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createAllTables())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), true);
        sessionManager_.Start();

        // 先open file
        ProtoSession protoSession1;

        EXPECT_CALL(*mockRepo_, InsertSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                    &protoSession1),
                    StatusCode::kOK);

        // 在session有效期内，再次openfile，返回kFileOccupied
        ProtoSession protoSession2;
        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                    &protoSession2),
                    StatusCode::kFileOccupied);

        // 等session过期，再open file, 返回新的session
        usleep(sessionOptions_.leaseTime);
        ProtoSession protoSession3;

        EXPECT_CALL(*mockRepo_, InsertSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, DeleteSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                    &protoSession3),
                    StatusCode::kOK);
        ASSERT_NE(protoSession3.sessionid(), protoSession1.sessionid());


        repo::SessionRepo sessionRepo("/file1", "sessionid", "token",
                        sessionOptions_.leaseTime, SessionStatus::kSessionOK,
                                    111, "127.0.0.1");
        EXPECT_CALL(*mockRepo_, QuerySessionRepo(_, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<1>(sessionRepo),
                            Return(repo::OperationOK)));

        sessionManager_.Stop();
    }

    {
        SessionManager sessionManager_(mockRepo_);

        EXPECT_CALL(*mockRepo_, LoadSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createDatabase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, useDataBase())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        EXPECT_CALL(*mockRepo_, createAllTables())
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.Init(sessionOptions_), true);
        sessionManager_.Start();

        // 先open file
        ProtoSession protoSession1;

        EXPECT_CALL(*mockRepo_, InsertSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                    &protoSession1),
                    StatusCode::kOK);

        // 等session过期
        usleep(sessionOptions_.leaseTime);

        ProtoSession protoSession2;
        // 再open file, 从数据库中删除旧session失败
        EXPECT_CALL(*mockRepo_, DeleteSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::SqlException));

        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                            &protoSession2),
            StatusCode::KInternalError);

        // 再open file, 从数据库中删除旧session成功，插入新session失败
        EXPECT_CALL(*mockRepo_, InsertSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::SqlException));

        EXPECT_CALL(*mockRepo_, DeleteSessionRepo(_))
        .Times(1)
        .WillOnce(Return(repo::OperationOK));

        ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                    &protoSession2),
                    StatusCode::KInternalError);

        sessionManager_.Stop();
    }
}

// 测试refresh session过程中的session过期流程
TEST_F(SessionTest, refresh_session_test) {
    SessionManager sessionManager_(mockRepo_);

    EXPECT_CALL(*mockRepo_, LoadSessionRepo(_))
    .Times(1)
    .WillOnce(Return(repo::OperationOK));

    EXPECT_CALL(*mockRepo_, connectDB(_, _, _, _))
    .Times(1)
    .WillOnce(Return(repo::OperationOK));

    EXPECT_CALL(*mockRepo_, createDatabase())
    .Times(1)
    .WillOnce(Return(repo::OperationOK));

    EXPECT_CALL(*mockRepo_, useDataBase())
    .Times(1)
    .WillOnce(Return(repo::OperationOK));

    EXPECT_CALL(*mockRepo_, createAllTables())
    .Times(1)
    .WillOnce(Return(repo::OperationOK));

    ASSERT_EQ(sessionManager_.Init(sessionOptions_), true);
    sessionManager_.Start();

    // 先open file
    ProtoSession protoSession1;

    EXPECT_CALL(*mockRepo_, InsertSessionRepo(_))
    .Times(1)
    .WillOnce(Return(repo::OperationOK));

    ASSERT_EQ(sessionManager_.InsertSession("/file1", "127.0.0.1",
                                                &protoSession1),
                StatusCode::kOK);

    // 等session过期
    usleep(sessionOptions_.leaseTime);

    ASSERT_EQ(sessionManager_.UpdateSession("/file1", protoSession1.sessionid(),
                                        "test_signature", "127.0.0.1"),
                StatusCode::kOK);

    repo::SessionRepo sessionRepo("/file1", "sessionid", "token",
                    sessionOptions_.leaseTime, SessionStatus::kSessionOK,
                                111, "127.0.0.1");
    EXPECT_CALL(*mockRepo_, QuerySessionRepo(_, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(sessionRepo),
                        Return(repo::OperationOK)));

    sessionManager_.Stop();
}
}  // namespace mds
}  // namespace curve
