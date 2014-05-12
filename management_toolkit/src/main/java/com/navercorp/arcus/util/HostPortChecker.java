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

import java.net.URI;
import java.net.URISyntaxException;

/**
 * Validates "host:port" expression.
 *
 * @author hoonmin
 */
public class HostPortChecker {

    public static boolean validate(String expr) {
        boolean result = true;

        try {
            URI uri = new URI("dummy://" + expr);
            if (uri.getHost() == null || uri.getPort() < 0 || uri.getPort() > 65535) {
                result = false;
            }
            System.out.println(uri.getPort());
        } catch (URISyntaxException e) {
            result = false;
        }

        return result;
    }

}
