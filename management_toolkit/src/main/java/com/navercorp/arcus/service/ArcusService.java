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

import com.fasterxml.jackson.databind.ObjectMapper;
import com.navercorp.arcus.exception.ArcusRuntimeException;
import com.navercorp.arcus.model.ArcusCluster;
import com.navercorp.arcus.model.ArcusClusterMembers;
import com.navercorp.arcus.model.CuratorFrameworkWithLock;
import com.navercorp.arcus.model.RestMessage;
import com.navercorp.arcus.util.CuratorCache;
import com.navercorp.arcus.util.HostPortChecker;
import com.navercorp.arcus.util.UnitConverter;
import org.apache.curator.framework.CuratorFramework;
import org.apache.zookeeper.data.Stat;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Services to manage Arcus clusters.
 *
 * @author hoonmin
 */
@Component
@ConfigurationProperties(name = "arcus")
public class ArcusService {

    @Autowired
    private CuratorCache curatorCache = null;

    private String topLevelDir = "arcus";
    private String lockDir = "/arcus/lock";
    private Integer lockTimeout = 5000;

    // shared object mapper to generate some JSON texts.
    private ObjectMapper objectMapper = new ObjectMapper();

    // default Arcus directory structure
    public static String ARCUS = "/arcus";
    public static String CACHE_LIST = ARCUS + "/cache_list";
    public static String CLIENT_LIST = ARCUS + "/client_list";
    public static String CACHE_SERVER_MAPPING = ARCUS + "/cache_server_mapping";

    @PostConstruct
    public void postConstruct() {
        // re-define the Arcus directory structure
        // TODO somebody refactor here please :-(
        ARCUS = "/" + topLevelDir;
        CACHE_LIST = ARCUS + "/cache_list";
        CLIENT_LIST = ARCUS + "/client_list";
        CACHE_SERVER_MAPPING = ARCUS + "/cache_server_mapping";
    }

    public RestMessage initArcus(String hostport, boolean inTransaction) {
        RestMessage result = new RestMessage();
        CuratorFrameworkWithLock cached = curatorCache.get(hostport);

        try {
            cached.getLock().acquire(lockTimeout, TimeUnit.MILLISECONDS);

            CuratorFramework curatorFramework = cached.getCuratorFramework();
            Stat check = curatorFramework.checkExists().forPath(ARCUS);
            if (check != null) {
                curatorFramework.delete().deletingChildrenIfNeeded().forPath(ARCUS);
            }

            if (inTransaction) {
                curatorFramework.inTransaction()
                        .create().forPath(ARCUS).and()
                        .create().forPath(CACHE_LIST).and()
                        .create().forPath(CLIENT_LIST).and()
                        .create().forPath(CACHE_SERVER_MAPPING).and()
                        .create().forPath(lockDir).and()
                        .commit();
            } else {
                curatorFramework.create().forPath(ARCUS);
                curatorFramework.create().forPath(CACHE_LIST);
                curatorFramework.create().forPath(CLIENT_LIST);
                curatorFramework.create().forPath(CACHE_SERVER_MAPPING);
                curatorFramework.create().forPath(lockDir);
            }
        } catch (Exception e) {
            throw new ArcusRuntimeException("Failed to initialize Arcus: " + e.getMessage());
        } finally {
            try {
                cached.getLock().release();
            } catch (Exception e) {
                throw new ArcusRuntimeException("Failed to initialize Arcus: " + e.getMessage());
            }
        }

        result.setStatus("success");
        result.setMessage("Initialized the Arcus directory structure on ZooKeeper(" + hostport + ").");
        return result;
    }

    public ArcusCluster getCluster(String hostport, String clusterName) {
        ArcusCluster result;
        String clusterPath = CACHE_LIST + "/" + clusterName;

        CuratorFrameworkWithLock cached = curatorCache.get(hostport);

        try {
            cached.getLock().acquire(lockTimeout, TimeUnit.MILLISECONDS);

            CuratorFramework curatorFramework = cached.getCuratorFramework();
            // get data (JSON object in byte stream)
            byte[] value = curatorFramework.getData().forPath(clusterPath);

            // de-serialize the data
            result = objectMapper.readValue(value, ArcusCluster.class);
        } catch (Exception e) {
            throw new ArcusRuntimeException("Failed to get the cluster: " + e.getMessage());
        } finally {
            try {
                cached.getLock().release();
            } catch (Exception e) {
                throw new ArcusRuntimeException("Failed to get the cluster: " + e.getMessage());
            }
        }

        return result;
    }

    public ArcusCluster createEmptyCluster(String hostport, String clusterName, String version, String memlimit, boolean inTransaction) {
        ArcusCluster result;
        String clusterPath = CACHE_LIST + "/" + clusterName;
        String clientPath = CLIENT_LIST + "/" + clusterName;

        CuratorFrameworkWithLock cached = curatorCache.get(hostport);

        try {
            cached.getLock().acquire(lockTimeout, TimeUnit.MILLISECONDS);

            CuratorFramework curatorFramework = cached.getCuratorFramework();

            // generate meta data in JSON format
            result = new ArcusCluster();
            result.setHostport(hostport);
            result.setClusterName(clusterName);
            result.setVersion(version);
            result.setMemlimitPerNode(UnitConverter.toByte(memlimit));
            byte[] data = objectMapper.writeValueAsBytes(result);

            if (inTransaction) {
                // create directories
                curatorFramework.inTransaction()
                        .create().forPath(clusterPath, data).and()
                        .create().forPath(clientPath).and()
                        .commit();
            } else {
                try {

                } catch (Exception e) {
                    // ignore exceptions
                }
            }
        } catch (Exception e) {
            throw new ArcusRuntimeException("Failed to create the cluster: " + e.getMessage());
        } finally {
            try {
                cached.getLock().release();
            } catch (Exception e) {
                throw new ArcusRuntimeException("Failed to initialize Arcus: " + e.getMessage());
            }
        }
        return result;
    }

    public RestMessage removeEmptyCluster(String hostport, String clusterName, boolean inTransaction) {
        RestMessage result = new RestMessage();
        String clusterPath = CACHE_LIST + "/" + clusterName;
        String clientPath = CLIENT_LIST + "/" + clusterName;

        CuratorFrameworkWithLock cached = curatorCache.get(hostport);

        try {
            cached.getLock().acquire(lockTimeout, TimeUnit.MILLISECONDS);

            CuratorFramework curatorFramework = cached.getCuratorFramework();
            // delete cache_list and client_list in transaction
            if (inTransaction) {
                curatorFramework.inTransaction()
                        .delete().forPath(clusterPath).and()
                        .delete().forPath(clientPath).and()
                        .commit();
            } else {
                try {
                    curatorFramework.delete().forPath(clusterPath);
                    curatorFramework.delete().forPath(clientPath);
                } catch (Exception e) {
                    // ignore the exceptions
                }
            }

            // TODO this should be transactional if possible
            Map<String, List<String>> map = getServerToCacheMap(hostport, clusterName);
            for (Map.Entry<String, List<String>> entry : map.entrySet()) {
                for (String node : entry.getValue()) {
                    curatorFramework.delete().deletingChildrenIfNeeded().forPath(CACHE_SERVER_MAPPING + "/" + node);
                }
            }

            result.setStatus("success");
            result.setMessage("removed the cluster: " + clusterName);
        } catch (Exception e) {
            throw new ArcusRuntimeException("Failed to remove the cluster: " + e.getMessage());
        } finally {
            try {
                cached.getLock().release();
            } catch (Exception e) {
                throw new ArcusRuntimeException("Failed to remove the cluster: " + e.getMessage());
            }
        }

        return result;
    }

    public ArcusClusterMembers getClusterMembers(String hostport, String clusterName) {
        ArcusClusterMembers result = new ArcusClusterMembers();
        String clusterPath = CACHE_LIST + "/" + clusterName;

        CuratorFrameworkWithLock cached = curatorCache.get(hostport);

        try {
            cached.getLock().acquire(lockTimeout, TimeUnit.MILLISECONDS);

            CuratorFramework curatorFramework = cached.getCuratorFramework();
            List<String> children = curatorFramework.getChildren().forPath(clusterPath);
            result.setMembers(children);
        } catch (Exception e) {
            throw new ArcusRuntimeException("Failed to list the cluster: " + e.getMessage());
        } finally {
            try {
                cached.getLock().release();
            } catch (Exception e) {
                throw new ArcusRuntimeException("Failed to list the cluster: " + e.getMessage());
            }
        }

        return result;
    }

    public RestMessage addClusterMember(String hostport, String clusterName, String arcusHostPort, boolean inTransaction) {
        RestMessage result = new RestMessage();
        String clusterPath = CACHE_LIST + "/" + clusterName;
        String mapKeyPath = CACHE_SERVER_MAPPING + "/" + arcusHostPort;
        String mapValuePath = mapKeyPath + "/" + clusterName;

        CuratorFrameworkWithLock cached = curatorCache.get(hostport);

        try {
            cached.getLock().acquire(lockTimeout, TimeUnit.MILLISECONDS);

            CuratorFramework curatorFramework = cached.getCuratorFramework();

            // check the arcusHostPort first
            if (!HostPortChecker.validate(arcusHostPort)) {
                throw new ArcusRuntimeException("Invalid Arcus 'host:port' expression: given=" + arcusHostPort);
            }

            // check for the cluster directory
            Stat clusterCheck = curatorFramework.checkExists().forPath(clusterPath);
            if (clusterCheck == null) {
                throw new ArcusRuntimeException(clusterName + " does not exist. Please create it first.");
            }

            // check for the mapping
            Stat mappingCheck = curatorFramework.checkExists().forPath(mapKeyPath);
            if (mappingCheck != null) {
                throw new ArcusRuntimeException("Mapping for " + clusterName + " already exists.");
            }

            // create the map
            if (inTransaction) {
                curatorFramework.inTransaction()
                        .create().forPath(mapKeyPath).and()
                        .create().forPath(mapValuePath).and()
                        .commit();
            } else {
                curatorFramework.create().forPath(mapKeyPath);
                curatorFramework.create().forPath(mapValuePath);
            }
        } catch (Exception e) {
            throw new ArcusRuntimeException("Failed to add a node to the cluster: " + e.getMessage());
        } finally {
            try {
                cached.getLock().release();
            } catch (Exception e) {
                throw new ArcusRuntimeException("Failed to add a node to the cluster: " + e.getMessage());
            }
        }

        result.setStatus("success");
        result.setMessage("Added a node(" + arcusHostPort + ") to the cluster(" + clusterName + ").");
        return result;
    }

    public RestMessage removeClusterMember(String hostport, String clusterName, String arcusHostPort, boolean inTransaction) {
        RestMessage result = new RestMessage();
        String clusterPath = CACHE_LIST + "/" + clusterName;
        String mapKeyPath = CACHE_SERVER_MAPPING + "/" + arcusHostPort;
        String mapValuePath = mapKeyPath + "/" + clusterName;

        CuratorFrameworkWithLock cached = curatorCache.get(hostport);

        try {
            cached.getLock().acquire(lockTimeout, TimeUnit.MILLISECONDS);

            CuratorFramework curatorFramework = cached.getCuratorFramework();

            // check the arcusHostPort first
            if (!HostPortChecker.validate(arcusHostPort)) {
                throw new ArcusRuntimeException("Invalid Arcus 'host:port' expression: given=" + arcusHostPort);
            }

            // check for the cluster directory
            Stat clusterCheck = curatorFramework.checkExists().forPath(clusterPath);
            if (clusterCheck == null) {
                throw new ArcusRuntimeException(clusterName + " does not exist.");
            }

            // check for the mapping
            Stat mappingCheck = curatorFramework.checkExists().forPath(mapKeyPath);
            if (mappingCheck == null) {
                throw new ArcusRuntimeException("Mapping for " + clusterName + " already exists.");
            }

            // remove the map
            if (inTransaction) {
                curatorFramework.inTransaction()
                        .delete().forPath(mapValuePath).and()
                        .delete().forPath(mapKeyPath).and()
                        .commit();
            } else {
                curatorFramework.delete().forPath(mapValuePath);
                curatorFramework.delete().forPath(mapKeyPath);
            }
        } catch (Exception e) {
            throw new ArcusRuntimeException("Failed to remove a node to the cluster: " + e.getMessage());
        } finally {
            try {
                cached.getLock().release();
            } catch (Exception e) {
                throw new ArcusRuntimeException("Failed to remove a node to the cluster: " + e.getMessage());
            }
        }

        result.setStatus("success");
        result.setMessage("Removed a node(" + arcusHostPort + ") to the cluster(" + clusterName + ").");
        return result;
    }

    public RestMessage getClusterMember(String hostport, String clusterName, String arcusHostPort) {
        return null;
    }

    public Map<String, List<String>> getServerToCacheMap(String hostport, String clusterName) {
        Map<String, List<String>> result = new HashMap<String, List<String>>();

        CuratorFrameworkWithLock cached = curatorCache.get(hostport);

        try {
            cached.getLock().acquire(lockTimeout, TimeUnit.MILLISECONDS);

            CuratorFramework curatorFramework = cached.getCuratorFramework();
            List<String> nodes = curatorFramework.getChildren().forPath(CACHE_SERVER_MAPPING);
            for (String node : nodes) {
                // get a service code for the Arcus cache node
                List<String> codes = curatorFramework.getChildren().forPath(CACHE_SERVER_MAPPING + "/" + node);
                if (codes == null || codes.size() < 1) {
                    continue;
                }
                String serviceCode = codes.get(0);

                if (clusterName == null || clusterName.equals(serviceCode)) {
                    if (!result.containsKey(serviceCode)) {
                        result.put(serviceCode, new ArrayList<String>());
                    }
                    result.get(serviceCode).add(node);
                }
            }
        } catch (Exception e) {
            throw new ArcusRuntimeException("Failed to get server-cache map from the cluster: " + e.getMessage());
        } finally {
            try {
                cached.getLock().release();
            } catch (Exception e) {
                throw new ArcusRuntimeException("Failed to get server-cache map from the cluster: " + e.getMessage());
            }
        }

        return result;
    }

    public String getTopLevelDir() {
        return topLevelDir;
    }

    public String getLockDir() {
        return lockDir;
    }
}
