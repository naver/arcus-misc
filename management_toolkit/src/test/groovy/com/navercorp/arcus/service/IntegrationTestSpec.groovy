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
import com.navercorp.arcus.Application
import com.navercorp.arcus.model.ArcusCluster
import org.apache.curator.framework.CuratorFramework
import org.apache.curator.framework.CuratorFrameworkFactory
import org.apache.curator.retry.ExponentialBackoffRetry
import org.apache.curator.test.InstanceSpec
import org.apache.curator.test.QuorumConfigBuilder
import org.apache.curator.test.TestingZooKeeperServer
import org.springframework.boot.SpringApplication
import org.springframework.context.ConfigurableApplicationContext
import spock.lang.Shared
import spock.lang.Specification

import java.util.concurrent.Callable
import java.util.concurrent.Executors
import java.util.concurrent.Future
import java.util.concurrent.TimeUnit

/**
 * A base integrated test framework.
 *
 * @author hoonmin
 */
abstract class IntegrationTestSpec extends Specification {

    @Shared
    ConfigurableApplicationContext context

    @Shared
    TestingZooKeeperServer server

    @Shared
    CuratorFramework framework

    @Shared
    String connectString

    def setupSpec() {
        // waiting for the ApplicationContext
        Future future = Executors.newSingleThreadExecutor().submit(
                new Callable() {
                    @Override
                    ConfigurableApplicationContext call() throws Exception {
                        return (ConfigurableApplicationContext) SpringApplication.run(Application.class)
                    }
                }
        )
        context = future.get(1000, TimeUnit.SECONDS)

        // run ZooKeeper server instance
        InstanceSpec spec = InstanceSpec.newInstanceSpec()
        QuorumConfigBuilder configBuilder = new QuorumConfigBuilder(spec)
        server = new TestingZooKeeperServer(configBuilder)
        server.start()
        connectString = server.instanceSpec.getConnectString()

        // run ZooKeeper client
        framework = CuratorFrameworkFactory.builder()
                .connectString(connectString)
                .retryPolicy(new ExponentialBackoffRetry(1000, 1))
                .build()
        framework.start()
    }

    def cleanSpec() {
        framework.close()
        server.stop()
        server.close()

        if (context != null) {
            context.close()
        }
    }
}
