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

import java.io.File;

/**
 * @author hoonmin
 */
public class RemoteShellFactory {

    public static Builder builder() {
        return new Builder();
    }

    public static class Builder {
        private String hostname;
        private Integer port = 22;
        private Integer connectTimeout = 2000;

        private boolean authenticateWithPassword = false;
        private String username;
        private String password;

        private boolean authenticateWithPublicKey = false;
        private String pemUser;
        private File pemFile;
        private String pemPassword;

        public RemoteShell build() {
            return new RemoteShell(this);
        }

        public Builder hostname(String hostname) {
            this.hostname = hostname;
            return this;
        }

        public Builder port(Integer port) {
            this.port = port;
            return this;
        }

        public Builder connectTimeout(Integer connectTimeoutMillis) {
            this.connectTimeout = connectTimeout;
            return this;
        }

        public Builder authenticateWithPassword(String username, String password) {
            this.username = username;
            this.password = password;
            this.authenticateWithPassword = true;
            return this;
        }

        public Builder authenticateWithPublicKey(String user, File pemFile, String password) {
            this.pemUser = user;
            this.pemFile = pemFile;
            this.pemPassword = password;
            this.authenticateWithPublicKey = true;
            return this;
        }

        public String getHostname() {
            return hostname;
        }

        public Integer getPort() {
            return port;
        }

        public Integer getConnectTimeout() {
            return connectTimeout;
        }

        public boolean isAuthenticateWithPassword() {
            return authenticateWithPassword;
        }

        public String getUsername() {
            return username;
        }

        public String getPassword() {
            return password;
        }

        public boolean isAuthenticateWithPublicKey() {
            return authenticateWithPublicKey;
        }

        public String getPemUser() {
            return pemUser;
        }

        public File getPemFile() {
            return pemFile;
        }

        public String getPemPassword() {
            return pemPassword;
        }
    }

    private RemoteShellFactory() {
        // hide this constructor
    }
}
