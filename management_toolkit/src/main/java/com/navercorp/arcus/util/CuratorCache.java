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

package com.navercorp.arcus.util;

import com.navercorp.arcus.exception.ZooKeeperRuntimeException;
import com.navercorp.arcus.model.CuratorFrameworkWithLock;
import com.navercorp.arcus.service.ArcusService;
import org.apache.curator.RetryPolicy;
import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.CuratorFrameworkFactory;
import org.apache.curator.framework.recipes.locks.InterProcessMutex;
import org.apache.curator.retry.ExponentialBackoffRetry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.context.annotation.Configuration;
import org.springframework.stereotype.Component;

import javax.annotation.PreDestroy;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * A very simple cache containing CuratorFrameworks
 *
 * @author hoonmin
 */
@Component
@ConfigurationProperties(name = "arcus")
public class CuratorCache {

    private String lockDir = "/arcus/lock";

    ConcurrentHashMap<String, CuratorFrameworkWithLock> cache = new ConcurrentHashMap<String, CuratorFrameworkWithLock>();
    Map<String, InterProcessMutex> lockCache = new HashMap<String, InterProcessMutex>();

    public CuratorFrameworkWithLock get(String key) {
        CuratorFrameworkWithLock created = null;

        // I believe this ensures atomicity and avoids retuning null value,
        // though this could block threads waiting for creation of the CuratorFramework.
        // (what about this? http://www.cs.umd.edu/~pugh/java/memoryModel/DoubleCheckedLocking.html)
        if (! cache.containsKey(key)) {
            synchronized (key) {
                if (! cache.containsKey(key)) {
                    created = createCuratorFramework(key);
                    cache.putIfAbsent(key, created);
                }
            }
        }
        CuratorFrameworkWithLock result = cache.get(key);
        if (created != null && result == null) {
            result = created;
        } else if (created == null && result == null) {
            throw new ZooKeeperRuntimeException("assertion error: result is null");
        }
        return result;
    }

    @PreDestroy
    public void closeAll() {
        // FIXME Should close frameworks gracefully... and what about locks...?
        for (Map.Entry<String, CuratorFrameworkWithLock> each : cache.entrySet()) {
            each.getValue().getCuratorFramework().close();
        }
    }

    private CuratorFrameworkWithLock createCuratorFramework(String key) {
        RetryPolicy retryPolicy = new ExponentialBackoffRetry(1000, 1);
        CuratorFramework curatorFramework = CuratorFrameworkFactory.builder()
                .connectString(key)
                .retryPolicy(retryPolicy)
                .connectionTimeoutMs(1000)
                .sessionTimeoutMs(15000)
                .build();
        curatorFramework.start();

        // create a lock on this client
        InterProcessMutex lock = new InterProcessMutex(curatorFramework, lockDir);

        CuratorFrameworkWithLock result = new CuratorFrameworkWithLock(curatorFramework, lock);

        return result;
    }

}
