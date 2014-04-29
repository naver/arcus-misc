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
#include "ArcusDao.hpp"

#include <iostream>
#include <vector>

using namespace arcus;

void test()
{
    ::base_client client;
    client.init("localhost:2181", "test-1", 4, 16, 500);

    // domain object
    dao::userinfo a_user(&client, "prefix", "harebox"); 

    a_user.mMID = "01030880966";
    a_user.mProxyIP = "0";
    a_user.mProxyPort = 0;

    a_user.del();
    a_user.insert();

    dao::userinfo ret_user(&client, "prefix", "harebox");
    ret_user.get();
    std::cout << "userid = " << ret_user.mMID << std::endl;
    ret_user.del();
}

int main(int argc, char *argv[])
{
    test();
    return 0;
}
