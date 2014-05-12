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

import ch.ethz.ssh2.Connection;
import ch.ethz.ssh2.Session;

import java.io.IOException;

public class RemoteShell {

    private Connection connection;
    private Session session;
    private Expect expect;
    private RemoteShellFactory.Builder builder; // debug

    public RemoteShell(RemoteShellFactory.Builder builder) {
        connection = new Connection(builder.getHostname(), builder.getPort());
        this.builder = builder;
    }

    public void start() throws IOException {
        connection.connect(null, builder.getConnectTimeout(), 0);

        // authenticate
        if (builder.isAuthenticateWithPassword()) {
            connection.authenticateWithPassword(builder.getUsername(), builder.getPassword());
        } else if (builder.isAuthenticateWithPublicKey()) {
            connection.authenticateWithPublicKey(builder.getPemUser(), builder.getPemFile(), builder.getPemPassword());
        }

        // open session
        session = connection.openSession();
        session.requestDumbPTY();
        session.startShell();

        // start expect service
        expect = new Expect(session.getStdout(), session.getStdin());
    }

    public void close() {
        expect.close();
        session.close();
        connection.close();
    }

    public ExpectBuilder expect() {
        return new ExpectBuilder(this);
    }

    public Connection getConnection() {
        return connection;
    }

    public Session getSession() {
        return session;
    }

    public Expect getExpect() {
        return expect;
    }

    public RemoteShellFactory.Builder getBuilder() {
        return builder;
    }
}
