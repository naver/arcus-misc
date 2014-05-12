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

package com.navercorp.arcus.shell;

/**
 * @author hoonmin
 */
public class ExpectBuilder {

    private static final int DEFAULT_EXPECT_TIMEOUT_SEC = 1;

    private RemoteShell shell;
    private int timeout = DEFAULT_EXPECT_TIMEOUT_SEC;

    public ExpectBuilder(RemoteShell shell) {
        this.shell = shell;
    }

    public ExpectBuilder timeout(int timeoutInSeconds) {
        if (timeoutInSeconds <= 0) {
            throw new RuntimeException("cannot set timeout with 0");
        }
        this.timeout = timeoutInSeconds;
        return this;
    }

    public ExpectBuilder send(String command) {
        shell.getExpect().send(command);
        return this;
    }

    public ExpectBuilder send(byte[] command) {
        shell.getExpect().send(command);
        return this;
    }

    public int forPatterns(Object... patterns) {
        return shell.getExpect().expect(timeout, patterns);
    }

    public int eof() {
        return shell.getExpect().expectEOF(timeout);
    }
}
