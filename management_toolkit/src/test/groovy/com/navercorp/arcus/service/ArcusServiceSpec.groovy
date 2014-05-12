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

package com.navercorp.arcus.service

import com.fasterxml.jackson.databind.ObjectMapper
import com.navercorp.arcus.exception.ArcusRuntimeException
import com.navercorp.arcus.model.ArcusCluster
import com.navercorp.arcus.model.RestMessage
import spock.lang.Shared

/**
 * An integrated test to verify ArcusService APIs.
 *
 * @author hoonmin
 */
class ArcusServiceSpec extends IntegrationTestSpec {

    @Shared
    ArcusService service = null;

    def setup() {
        service = context.getBean(ArcusService.class)

        // build some directories for testing
        ObjectMapper mapper = new ObjectMapper()
        ArcusCluster cluster1 = new ArcusCluster()
        cluster1.hostport = connectString
        cluster1.clusterName = 'test1'
        cluster1.memlimitPerNode = 100 * 1024 * 1024L
        cluster1.version = '1.7.0'

        ArcusCluster cluster2 = new ArcusCluster()
        cluster2.hostport = connectString
        cluster2.clusterName = 'test2'
        cluster2.memlimitPerNode = 500 * 1024 * 1024L
        cluster2.version = '1.7.0'

        if (framework.checkExists().forPath(ArcusService.ARCUS) != null) {
            framework.delete().deletingChildrenIfNeeded().forPath(ArcusService.ARCUS)
        }
        framework.inTransaction()
                .create().forPath(ArcusService.ARCUS).and()
                .create().forPath(ArcusService.CACHE_LIST).and()
                .create().forPath(ArcusService.CACHE_LIST + '/test1', mapper.writeValueAsBytes(cluster1)).and()
                .create().forPath(ArcusService.CACHE_LIST + '/test2', mapper.writeValueAsBytes(cluster2)).and()
                .create().forPath(ArcusService.CLIENT_LIST).and()
                .create().forPath(ArcusService.CLIENT_LIST + '/test1').and()
                .create().forPath(ArcusService.CLIENT_LIST + '/test2').and()
                .create().forPath(ArcusService.CACHE_SERVER_MAPPING).and()
                .create().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11211').and()
                .create().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11212').and()
                .create().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11213').and()
                .create().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11211/test1').and()
                .create().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11212/test1').and()
                .create().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11213/test2').and()
                .create().forPath(service.getLockDir()).and()
                .commit()
    }

    void 'should initialize Arcus directory structure'() {
        when:
        RestMessage message = service.initArcus(connectString, true)

        then:
        message != null
        println message.status
        println message.exception
        println message.message
        message.status == 'success'

        null != framework.checkExists().forPath(ArcusService.ARCUS)
        null != framework.checkExists().forPath(ArcusService.CACHE_LIST)
        null != framework.checkExists().forPath(ArcusService.CLIENT_LIST)
        null != framework.checkExists().forPath(ArcusService.CACHE_SERVER_MAPPING)

        0 == framework.getChildren().forPath(ArcusService.CACHE_LIST).size()
        0 == framework.getChildren().forPath(ArcusService.CLIENT_LIST).size()
        0 == framework.getChildren().forPath(ArcusService.CACHE_SERVER_MAPPING).size()
        0 == framework.getChildren().forPath(service.getLockDir()).size()
    }

    void 'should get cluster informations'() {
        when:
        ArcusCluster cluster = service.getCluster(connectString, 'test1')

        then:
        cluster != null
        cluster.hostport == connectString
        cluster.clusterName == 'test1'
        cluster.memlimitPerNode == 100 * 1024 * 1024L
        cluster.version == '1.7.0'
    }

    void 'should create an empty cluster'() {
        when:
        ArcusCluster cluster = service.createEmptyCluster(connectString, 'test3', '1.7.0', '100MB', true)

        then:
        cluster != null
        cluster.hostport == connectString
        cluster.clusterName == 'test3'
        cluster.memlimitPerNode == 100 * 1024 * 1024L
        cluster.version == '1.7.0'

        null != framework.checkExists().forPath(ArcusService.CACHE_LIST + '/test3')
        null != framework.checkExists().forPath(ArcusService.CLIENT_LIST + '/test3')
    }

    void 'should throw an exception trying to create an empty cluster that already exists'() {
        when:
        service.createEmptyCluster(connectString, 'test1', '1.7.0', '100MB', true)

        then:
        thrown(ArcusRuntimeException)
    }

    void 'should get server-cache map'() {
        when:
        Map<String, List<String>> map = service.getServerToCacheMap(connectString, 'test1')

        then:
        print map
    }

    void 'should remove an empty cluster and associated ZNodes'() {
        when:
        RestMessage message = service.removeEmptyCluster(connectString, 'test2', true)

        then:
        message != null
        message.status == 'success'
        message.message.contains('test2') == true
        message.exception == null

        null == framework.checkExists().forPath(ArcusService.CACHE_LIST + '/test2')
        null == framework.checkExists().forPath(ArcusService.CLIENT_LIST + '/test2')
        null == framework.checkExists().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11213/test2')
        null == framework.checkExists().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11213')
    }

    void 'should throw an exception trying to remove no cluster'() {
        when:
        service.removeEmptyCluster(connectString, 'test5', true)

        then:
        thrown(ArcusRuntimeException)
    }

    void 'should add a cluster member'() {
        when:
        RestMessage message = service.addClusterMember(connectString, 'test1', '127.0.0.1:11214', true)

        then:
        message != null
        message.status == 'success'
        message.message.contains('127.0.0.1:11214') == true

        null != framework.checkExists().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11214')
        null != framework.getChildren().forPath(ArcusService.CACHE_SERVER_MAPPING + '/127.0.0.1:11214')
    }

    void 'should throw an exception trying to add an existing cluster member'() {
        when:
        service.addClusterMember(connectString, 'test1', '127.0.0.1:11212', true)

        then:
        thrown(ArcusRuntimeException)
    }

    void 'should remove a cluster member'() {
        when:
        RestMessage message = service.removeClusterMember(connectString, 'test1', '127.0.0.1:11211', true)

        then:
        message != null
        message.status == 'success'
        message.message.contains('127.0.0.1:11211') == true
    }
}
