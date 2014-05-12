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

package com.navercorp.arcus.controller;

import com.navercorp.arcus.model.ArcusCluster;
import com.navercorp.arcus.model.ArcusClusterMembers;
import com.navercorp.arcus.model.RestMessage;
import com.navercorp.arcus.service.ArcusService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.PropertySource;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.*;

import javax.servlet.http.HttpServletRequest;

/**
 * Serves APIs to manage Arcus clusters.
 *
 * @author hoonmin
 */
@RestController
@RequestMapping(value = "/api/v1/arcus", produces = MediaType.APPLICATION_JSON_VALUE)
@ConfigurationProperties(name="arcus")
public class ArcusController {

    @Autowired
    private ArcusService arcusService = null;

    private boolean useTx = false;

    @ExceptionHandler(Exception.class)
    @ResponseBody
    public RestMessage handleExceptions(HttpServletRequest req, Exception e) {
        RestMessage result = new RestMessage();
        result.setStatus("error");
        result.setException(e.getClass().getName());
        result.setMessage(e.getMessage());
        return result;
    }

    /**
     * Gets informations for the cluster.
     *
     * @param hostport
     * @param clusterName
     * @return
     */
    @RequestMapping(method = RequestMethod.GET,
            value = "/{hostport:.+}/clusters/{clusterName:.+}")
    @ResponseBody
    public ArcusCluster getCluster(@PathVariable String hostport, @PathVariable String clusterName) {
        return arcusService.getCluster(hostport, clusterName);
    }

    /**
     * Creates an empty Arcus cluster.
     *
     * @param hostport
     * @param clusterName
     * @param version
     * @param memLimitPerNode
     * @return
     */
    @RequestMapping(method = RequestMethod.POST,
            value = "/{hostport:.+}/clusters/{clusterName:.+}",
            params = {"version", "memlimitPerNode"})
    @ResponseBody
    public ArcusCluster createEmptyCluster(@PathVariable String hostport, @PathVariable String clusterName,
                                           @RequestParam String version, @RequestParam String memLimitPerNode) {
        return arcusService.createEmptyCluster(hostport, clusterName, version, memLimitPerNode, useTx);
    }

    /**
     * Removes an Arcus cluster. The cluster should be empty.
     *
     * @param hostport
     * @param clusterName
     * @return
     */
    @RequestMapping(method = RequestMethod.DELETE,
            value = "/{hostport:.+}/clusters/{clusterName:.+}")
    @ResponseBody
    public RestMessage removeEmptyCluster(@PathVariable String hostport, @PathVariable String clusterName) {
        return arcusService.removeEmptyCluster(hostport, clusterName, useTx);
    }

    /**
     * Retrieves members of the Arcus cluster.
     *
     * @param hostport
     * @param clusterName
     * @return
     */
    @RequestMapping(method = RequestMethod.GET,
            value = "/{hostport:.+}/clusters/{clusterName:.+}/members")
    @ResponseBody
    public ArcusClusterMembers getClusterMembers(@PathVariable String hostport, @PathVariable String clusterName) {
        return arcusService.getClusterMembers(hostport, clusterName);
    }

    /**
     * Gets an member in the Arcus cluster.
     *
     * @param hostport
     * @param clusterName
     * @param arcusHostPort
     * @return
     */
    @RequestMapping(method = RequestMethod.GET,
            value = "/{hostport:.+}/clusters/{clusterName:.+}/members/{arcusHostPort:.+}")
    @ResponseBody
    public RestMessage getClusterMember(@PathVariable String hostport, @PathVariable String clusterName, @PathVariable String arcusHostPort) {
        return arcusService.getClusterMember(hostport, clusterName, arcusHostPort);
    }

    /**
     * Adds an member to the Arcus cluster.
     *
     * @param hostport
     * @param clusterName
     * @param arcusHostPort
     * @return
     */
    @RequestMapping(method = RequestMethod.POST,
            value = "/{hostport:.+}/clusters/{clusterName:.+}/members/{arcusHostPort:.+}")
    @ResponseBody
    public RestMessage addClusterMember(@PathVariable String hostport, @PathVariable String clusterName, @PathVariable String arcusHostPort) {
        return arcusService.addClusterMember(hostport, clusterName, arcusHostPort, useTx);
    }

    /**
     * Removes a member from the Arcus cluster.
     *
     * @param hostport
     * @param clusterName
     * @param arcusHostPort
     * @return
     */
    @RequestMapping(method = RequestMethod.DELETE,
            value = "/{hostport:.+}/clusters/{clusterName:.+}/members/{arcusHostPort:.+}")
    @ResponseBody
    public RestMessage removeClusterMember(@PathVariable String hostport, @PathVariable String clusterName, @PathVariable String arcusHostPort) {
        return arcusService.removeClusterMember(hostport, clusterName, arcusHostPort, useTx);
    }
}
