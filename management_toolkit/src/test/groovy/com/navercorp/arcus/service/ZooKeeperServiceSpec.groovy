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

import com.navercorp.arcus.exception.ZooKeeperRuntimeException
import com.navercorp.arcus.model.ZooKeeperChildren
import org.junit.Ignore
import spock.lang.Shared

/**
 * An integrated test to verify ZooKeeperController APIs.
 *
 * @author hoonmin
 */
class ZooKeeperServiceSpec extends IntegrationTestSpec {

    @Shared
    ZooKeeperService service

    def setup() {
        service = context.getBean(ZooKeeperService.class)

        if (framework.checkExists().forPath(ArcusService.ARCUS) != null) {
            framework.delete().deletingChildrenIfNeeded().forPath('/arcus')
        }
        framework.inTransaction()
                .create().forPath('/arcus').and()
                .create().forPath('/arcus/cache_list').and()
                .create().forPath('/arcus/cache_list/test1').and()
                .create().forPath('/arcus/cache_list/test2').and()
                .commit()
    }

    void "should get children"() {
        when:
        ZooKeeperChildren result = service.getChildren(connectString, 'arcus+cache_list')

        then:
        result != null
        result.getChildren() != null
        result.getChildren().size() == 2
        result.getChildren().get(0) == 'test1'
        result.getChildren().get(1) == 'test2'
    }

    void "should create a node"() {
        when:
        def response = service.create(connectString, 'mynode', 'mydata')

        then:
        response != null
        '/mynode' == response.getPath()
        'mydata'.getBytes() == framework.getData().forPath(response.getPath())
    }

    void "should throw an exception trying to create a existing node"() {
        when:
        def response = service.create(connectString, 'arcus', 'mydata')

        then:
        thrown(ZooKeeperRuntimeException)
    }

    void "should delete a node"() {
        when:
        def response = service.delete(connectString, 'arcus+cache_list+test2')

        then:
        response != null
    }

    void "should throw an exception trying to delete no node"() {
        when:
        def response = service.delete(connectString, 'arcus+cache_list+test3')

        then:
        thrown(ZooKeeperRuntimeException)
    }
}
