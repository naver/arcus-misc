/*
 * arcus-management-toolkit - Arcus management toolkit
 * Copyright 2014 NAVER Corp.
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

package com.navercorp.arcus.util

import org.apache.curator.framework.CuratorFramework
import org.apache.curator.test.InstanceSpec
import org.apache.curator.test.QuorumConfigBuilder
import org.apache.curator.test.TestingZooKeeperServer
import spock.lang.Shared
import spock.lang.Specification

/**
 * An integrated test for CuratorCache.
 *
 * @author hoonmin
 */
class CuratorCacheSpec extends Specification {

    @Shared
    TestingZooKeeperServer server = null

    @Shared
    String connectString = null

    def setupSpec() {
        // run ZooKeeper server instance
        InstanceSpec spec = InstanceSpec.newInstanceSpec()
        QuorumConfigBuilder configBuilder = new QuorumConfigBuilder(spec)
        server = new TestingZooKeeperServer(configBuilder)
        server.start()

        connectString = server.instanceSpec.connectString
    }

    def cleanupSpec() {
        server.stop()
        server.close()
    }

    void 'should return newly created CuratorFramework'() {
        CuratorCache curatorCache = new CuratorCache()

        when:
        CuratorFramework curatorFramework = curatorCache.get(connectString)

        then:
        null != curatorFramework

        curatorCache.closeAll()
    }
}
