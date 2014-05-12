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

package com.navercorp.arcus.util

import spock.lang.Specification

/**
 * A specification for HostPortChecker
 *
 * @author hoonmin
 */
class HostPortCheckerSpec extends Specification {

    void 'should return true in normal cases'() {
        when:
        boolean check = HostPortChecker.validate('my.domain.name:2181')

        then:
        true == check
    }

    void 'should return false with a protocol expression'() {
        when:
        boolean check = HostPortChecker.validate('http://my.domain.name:2181')

        then:
        false == check
    }

    void 'should return false without a port'() {
        when:
        boolean check = HostPortChecker.validate('my.domain.name')

        then:
        false == check
    }

    void 'should fail with a invalid port'() {
        when:
        boolean check = HostPortChecker.validate('my.domain.name:65536')

        then:
        false == check
    }
}
