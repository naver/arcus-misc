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

import com.navercorp.arcus.shell.ExpectTask;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @author hoonmin
 */
@Component
public class ShellService {

    private static final Logger logger = LoggerFactory.getLogger(ShellService.class);

    public class ExpectTaskRunner implements Runnable {
        private ExpectTask task;

        public ExpectTaskRunner(ExpectTask task) {
            this.task = task;
        }

        @Override
        public void run() {
            try {
                task.doIt();
            } catch (Exception e) {
                logger.info("failed to run the task: " + task);
            }
        }
    }

    private ExecutorService executor = Executors.newFixedThreadPool(5, new ThreadFactory() {
        private AtomicInteger id = new AtomicInteger(0);

        @Override
        public Thread newThread(Runnable r) {
            Thread t = new Thread();
            t.setName("shell-service-executor-" + id);
            id.incrementAndGet();
            return t;
        }
    });

    public Future<ExpectTask> submitExpectTask(ExpectTask task) {
        ExpectTaskRunner runner = new ExpectTaskRunner(task);
        return executor.submit(runner, task);
    }
}
