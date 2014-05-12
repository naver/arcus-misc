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

import com.navercorp.arcus.model.RestMessage;
import com.navercorp.arcus.model.ZooKeeperChildren;
import com.navercorp.arcus.model.ZooKeeperResponse;
import com.navercorp.arcus.service.ZooKeeperService;
import com.wordnik.swagger.annotations.ApiOperation;
import com.wordnik.swagger.annotations.ApiParam;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.PropertySource;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.*;

import javax.servlet.http.HttpServletRequest;

/**
 * Serves APIs to manage ZooKeeper clusters.
 *
 * @author hoonmin
 */
@PropertySource("application.properties")
@RestController
@RequestMapping(value = "/api/v1/zookeeper", produces = MediaType.APPLICATION_JSON_VALUE)
public class ZooKeeperController {

    @Autowired
    private ZooKeeperService zooKeeperService = null;

    @ExceptionHandler(Exception.class)
    @ResponseBody
    public RestMessage handleExceptions(HttpServletRequest req, Exception e) {
        RestMessage result = new RestMessage();
        result.setStatus("error");
        result.setException(e.getClass().getName());
        result.setMessage(e.getMessage());
        return result;
    }

    @ApiOperation("Gets children in the ZNode")
    @RequestMapping(method = RequestMethod.GET,
            value = "/{hostport:.+}/path/{path:.+}/children")
    @ResponseBody
    public ZooKeeperChildren getChildren(
            @ApiParam(name = "hostport", value = "Host:Port expression for the ZooKeeper", required = true) @PathVariable String hostport,
            @ApiParam(name = "path", value = "Path of the ZNode. For '/arcus/cache_list', give 'arcus+cache_list'", required = true) @PathVariable String path) {
        return zooKeeperService.getChildren(hostport, path);
    }

    @ApiOperation("Gets data for the ZNode")
    @RequestMapping(method = RequestMethod.GET,
            value = "/{hostport:.+}/path/{path:.+}/data")
    @ResponseBody
    public ZooKeeperResponse getData(
            @ApiParam(name = "hostport", value = "Host:Port expression for the ZooKeeper", required = true) @PathVariable String hostport,
            @ApiParam(name = "path", value = "Path of the ZNode. For '/arcus/cache_list', give 'arcus+cache_list'", required = true) @PathVariable String path) {
        return zooKeeperService.getData(hostport, path);
    }

    @ApiOperation("Creates a ZNode")
    @RequestMapping(method = RequestMethod.POST,
            value = "/{hostport:.+}/path/{path:.+}",
            params = {"data"})
    @ResponseBody
    public ZooKeeperResponse create(
            @ApiParam(name = "hostport", value = "Host:Port expression for the ZooKeeper", required = true) @PathVariable String hostport,
            @ApiParam(name = "path", value = "Path of the ZNode. For '/arcus/cache_list', use 'arcus+cache_list'", required = true) @PathVariable String path,
            @ApiParam(name = "data", value = "Data of the ZNode", required = false) @RequestParam String data) {
        return zooKeeperService.create(hostport, path, data);
    }

    @ApiOperation("Deletes a ZNode")
    @RequestMapping(method = RequestMethod.DELETE,
            value = "/{hostport:.+}/path/{path:.+}")
    @ResponseBody
    public ZooKeeperResponse delete(
            @ApiParam(name = "hostport", value = "Host:Port expression for the ZooKeeper", required = true) @PathVariable String hostport,
            @ApiParam(name = "path", value = "Path of the ZNode. For '/arcus/cache_list', give 'arcus+cache_list'", required = true) @PathVariable String path) {
        return zooKeeperService.delete(hostport, path);
    }
}
