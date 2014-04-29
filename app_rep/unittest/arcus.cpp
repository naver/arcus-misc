/*
 * Copyright 2012-2014 NAVER Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gtest/gtest.h>
#include "../arcus.hpp"
#include "../example_dao.hpp"

using namespace arcus;

class ArcusTest : public ::testing::Test
{
protected:
    void SetUp() { }
};

class test_client : public ::base_client
{
public:
    test_client()
    {
        memcached_return_t rc;
        key = "prefix:userinfo_harebox";

        EXPECT_TRUE(init("locahost:2181", "test-1,test-2,test-3", 4, 16, 500));
        EXPECT_TRUE(del(key, &rc));
    }

    bool insert_for_cluster(size_t i, uint64_t bkey, std::string value)
    {
        memcached_return_t rc, piped_rc;
        connection_pool *p = pool(i);
        struct timespec to = { 1, 0 };

        memcached_st *handle = p->fetch(&rc, &to);
        EXPECT_TRUE(handle != NULL);

        memcached_coll_create_attrs_st create_attrs;
        memcached_coll_create_attrs_init(&create_attrs, 0, 0, 100);

        rc = memcached_bop_insert(handle, key.c_str(), key.length(), bkey, NULL, 0,
                value.c_str(), value.length(), &create_attrs);
        EXPECT_EQ(MEMCACHED_SUCCESS, rc);

        p->release(handle, &rc);
    }

public:
    std::string key;
};

const uint64_t FIELD_STATE = 0;
const uint64_t FIELD_MIN = 0;
const uint64_t FIELD_MAX = 10;

/*
 * c1    c2    c3    hint    expect    success    state
 * =============================================================
 * A     A     A     A       A         True       HINT_MATCHED
 */
TEST(ArcusTest, test_Replication_Case1) {
    test_client client;
    memcached_return_t rc;

    client.insert_for_cluster(0, FIELD_STATE, "A");
    client.insert_for_cluster(1, FIELD_STATE, "A");
    client.insert_for_cluster(2, FIELD_STATE, "A");

    ::hint hint(FIELD_STATE, "A", 1);

    cached_data *result = client.get(client.key, FIELD_MIN, FIELD_MAX, &hint, &rc);

    EXPECT_TRUE(result != NULL);
    DEBUG("returned : %s (state=%d)", result->value(FIELD_STATE), result->state);
    EXPECT_STREQ("A", result->value(FIELD_STATE));
    EXPECT_EQ(::hint::HINT_MATCHED, result->state);

    delete result;
}

/*
 * c1    c2    c3    hint    expect    success    state
 * =============================================================
 * A     A     A     X       A         True       NO_HINT
 */
TEST(ArcusTest, test_Replication_Case2) {
    test_client client;
    memcached_return_t rc;

    client.insert_for_cluster(0, FIELD_STATE, "A");
    client.insert_for_cluster(1, FIELD_STATE, "A");
    client.insert_for_cluster(2, FIELD_STATE, "A");

    cached_data *result = client.get(client.key, FIELD_MIN, FIELD_MAX, NULL, &rc);

    EXPECT_TRUE(result != NULL);
    DEBUG("returned : %s (state=%d)", result->value(FIELD_STATE), result->state);
    EXPECT_STREQ("A", result->value(FIELD_STATE));
    EXPECT_EQ(::hint::NO_HINT, result->state);

    delete result;
}

/*
 * c1    c2    c3    hint    expect    success    state
 * =============================================================
 * A     A     A     B       A         True       HINT_INCORRECT
 */
TEST(ArcusTest, test_Replication_Case3) {
    test_client client;
    memcached_return_t rc;

    client.insert_for_cluster(0, FIELD_STATE, "A");
    client.insert_for_cluster(1, FIELD_STATE, "A");
    client.insert_for_cluster(2, FIELD_STATE, "A");

    ::hint hint(FIELD_STATE, "B", 1);

    cached_data *result = client.get(client.key, FIELD_MIN, FIELD_MAX, &hint, &rc);

    EXPECT_TRUE(result != NULL);
    DEBUG("returned : %s (state=%d)", result->value(FIELD_STATE), result->state);
    EXPECT_STREQ("A", result->value(FIELD_STATE));
    EXPECT_EQ(::hint::HINT_INCORRECT, result->state);
}

/*
 * c1    c2    c3    hint    expect    success    state
 * =============================================================
 * X     A     A     A       A         True       HINT_MATCHED
 */
TEST(ArcusTest, test_Replication_Case4) {
    test_client client;
    memcached_return_t rc;

    //client.insert_for_cluster(0, FIELD_STATE, "A");
    client.insert_for_cluster(1, FIELD_STATE, "A");
    client.insert_for_cluster(2, FIELD_STATE, "A");

    ::hint hint(FIELD_STATE, "A", 1);

    cached_data *result = client.get(client.key, FIELD_MIN, FIELD_MAX, &hint, &rc);

    EXPECT_TRUE(result != NULL);
    DEBUG("returned : %s (state=%d)", result->value(FIELD_STATE), result->state);
    EXPECT_STREQ("A", result->value(FIELD_STATE));
    EXPECT_EQ(::hint::HINT_MATCHED, result->state);
}

/*
 * c1    c2    c3    hint    expect    success    state
 * =====================================================================
 * X     A     A     X       A         True       NO_HINT_POSSIBLY_STALE
 */
TEST(ArcusTest, test_Replication_Case5) {
    test_client client;
    memcached_return_t rc;

    //client.insert_for_cluster(0, FIELD_STATE, "A");
    client.insert_for_cluster(1, FIELD_STATE, "A");
    client.insert_for_cluster(2, FIELD_STATE, "A");

    cached_data *result = client.get(client.key, FIELD_MIN, FIELD_MAX, NULL, &rc);

    EXPECT_TRUE(result != NULL);
    DEBUG("returned : %s (state=%d)", result->value(FIELD_STATE), result->state);
    EXPECT_STREQ("A", result->value(FIELD_STATE));
    EXPECT_EQ(::hint::NO_HINT_POSSIBLY_STALE, result->state);
}

/*
 * c1    c2    c3    hint    expect    success    state
 * =============================================================
 * X     A     B     B       B         True       HINT_MATCHED
 */
TEST(ArcusTest, test_Replication_Case6) {
    test_client client;
    memcached_return_t rc;

    //client.insert_for_cluster(0, FIELD_STATE, "A");
    client.insert_for_cluster(1, FIELD_STATE, "A");
    client.insert_for_cluster(2, FIELD_STATE, "B");

    ::hint hint(FIELD_STATE, "B", 1);

    cached_data *result = client.get(client.key, FIELD_MIN, FIELD_MAX, &hint, &rc);

    EXPECT_TRUE(result != NULL);
    DEBUG("returned : %s (state=%d)", result->value(FIELD_STATE), result->state);
    EXPECT_STREQ("B", result->value(FIELD_STATE));
    EXPECT_EQ(::hint::HINT_MATCHED, result->state);
}

TEST(ArcusTest, test_Replication_NOTFOUND) {
    test_client client;
    memcached_return_t rc;

    //client.insert_for_cluster(0, FIELD_STATE, "A");
    //client.insert_for_cluster(1, FIELD_STATE, "A");
    //client.insert_for_cluster(2, FIELD_STATE, "B");

    ::hint hint(FIELD_STATE, "B", 1);

    cached_data *result = client.get(client.key, FIELD_MIN, FIELD_MAX, &hint, &rc);

    EXPECT_TRUE(result == NULL);
}
