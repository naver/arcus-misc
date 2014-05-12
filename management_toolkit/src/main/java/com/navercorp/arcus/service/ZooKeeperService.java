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

package com.navercorp.arcus.service;

import com.navercorp.arcus.exception.ZooKeeperRuntimeException;
import com.navercorp.arcus.model.ZooKeeperChildren;
import com.navercorp.arcus.model.ZooKeeperResponse;
import com.navercorp.arcus.util.CuratorCache;
import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.util.List;

/**
 * Services to manage ZooKeeper clusters.
 *
 * @author hoonmin
 */
@Component
public class ZooKeeperService {

    @Autowired
    private CuratorCache curatorCache;

    public ZooKeeperChildren getChildren(String hostport, String path) {
        ZooKeeperChildren result = new ZooKeeperChildren(hostport, path);

        try {
            CuratorFramework curatorFramework = curatorCache.get(hostport).getCuratorFramework();
            List<String> children = curatorFramework.getChildren().forPath(result.getPath());
            result.setChildren(children);
        } catch (Exception e) {
            throw new ZooKeeperRuntimeException(hostport, path, e.getMessage());
        }

        return result;
    }

    public ZooKeeperResponse getData(String hostport, String path) {
        ZooKeeperResponse result = new ZooKeeperResponse(hostport, path, null);

        try {
            CuratorFramework curatorFramework = curatorCache.get(hostport).getCuratorFramework();
            byte[] data = curatorFramework.getData().forPath(result.getPath());
            result.setValue(new String(data));
        } catch (Exception e) {
            throw new ZooKeeperRuntimeException(hostport, path, e.getMessage());
        }

        return result;
    }

    public ZooKeeperResponse create(String hostport, String path, String value) {
        ZooKeeperResponse result = new ZooKeeperResponse(hostport, path, value);

        try {
            CuratorFramework curatorFramework = curatorCache.get(hostport).getCuratorFramework();

            byte[] valueInByte = null;
            if (value != null) {
                valueInByte = value.getBytes();
            }
            curatorFramework.create().forPath(result.getPath(), valueInByte);
        } catch (Exception e) {
            throw new ZooKeeperRuntimeException(hostport, path, e.getMessage());
        }

        return result;
    }

    public ZooKeeperResponse delete(String hostport, String path) {
        ZooKeeperResponse result = new ZooKeeperResponse(hostport, path, null);

        try {
            CuratorFramework curatorFramework = curatorCache.get(hostport).getCuratorFramework();
            curatorFramework.delete().forPath(result.getPath());
        } catch (Exception e) {
            throw new ZooKeeperRuntimeException(hostport, path, e.getMessage());
        }

        return result;
    }
}
