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
#include "arcus.hpp"
#include "example_dao.hpp"

#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
    char zkinfo = "localhost:2181";

    // create a client
    arcus::base_client client;

    // add 3 clusters
    client.init(zkinfo, "test-1,test-2,test-3", 4, 16, 500);

    /*
     * EXAMPLE : userinfo
     */

    // domain object
    dao::userinfo a_user(&client, "prefix", "harebox"); 

    a_user.userid = "harebox";
    a_user.state = "STATE1";
    a_user.contact = "...srcaddr";
    gettimeofday(&a_user.regitime, NULL);

    // insert
    bool success = a_user.insert();

    if (success) {
        std::cout << "inserted" << std::endl;
        std::cout << "\tuserid : " << a_user.userid << std::endl;
        std::cout << "\tregitime : " << a_user.regitime.tv_sec << std::endl;
        std::cout << "\tstate : " << a_user.state << std::endl;
    } else {
        std::cout << "failed" << std::endl;
    }

    // update
    success = a_user.updateState("STATE2");

    if (success) {
        std::cout << "updated" << std::endl;
    } else {
        std::cout << "failed" << std::endl;
    }

    // get
    dao::userinfo retrieved_user(&client, "prefix", "harebox");

    success = retrieved_user.get();

    if (success) {
        std::cout << "selected" << std::endl;
        std::cout << "\tuserid : " << retrieved_user.userid << std::endl;
        std::cout << "\tregitime : " << retrieved_user.regitime.tv_sec << std::endl;
        std::cout << "\tstate : " << retrieved_user.state << std::endl;
    } else {
        std::cout << "failed" << std::endl;
    }

    // delete
    success = a_user.del();

    if (success) {
        std::cout << "dropped" << std::endl;
    } else {
        std::cout << "failed" << std::endl;
    }

#if 0 // TEST BASIC OPERATIONS
    memcached_return_t rc;
    bool result;

    // set
    result = client.set("prefix:userinfo_harebox", "A", 1, 600, 0, &rc);

    if (not result) std::cout << "failed : set" << std::endl;

    // get
    char *value;
    size_t value_length; 
    uint32_t flags;

    result = client.get("prefix:userinfo_harebox", &value, &value_length, &flags, &rc);

    if (not result) std::cout << "failed : get" << std::endl;

    if (rc == MEMCACHED_SUCCESS) {
        std::cout << "value=" << value << std::endl;
        free(value);
    } else {
        std::cout << "error=" << memcached_strerror(NULL, rc) << std::endl;
    }

    // b+tree insert
    const uint64_t bkeys[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    const char *values[] = { "element1", "element2", "element3", "elemen4", "elemen5", "elemen6", "elemen7", "elemen8", "elemen9", "elemen10" };
    size_t value_lengths[] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 9 };

    memcached_coll_create_attrs_st create_attrs;
    memcached_coll_create_attrs_init(&create_attrs, 0, 600, 1000);

    result = client.atomic_btree_insert_bulk("prefix:user_harebox", 10, bkeys, values, value_lengths, &create_attrs, &rc);

    if (not result) std::cout << "failed : b+tree insert" << std::endl;
#endif

    return 0;
}
