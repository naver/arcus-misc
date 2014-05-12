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

class UnitConverterSpec extends Specification {

    void "should parse the KB to Byte"() {
        when:
        long result = UnitConverter.toByte("100KB")
        then:
        result == 100 * 1024L
    }

    void "should parse the MB to Byte"() {
        when:
        long result = UnitConverter.toByte("100MB")
        then:
        result == 100 * 1024 * 1024L
    }

    void "should parse the GB to Byte"() {
        when:
        long result = UnitConverter.toByte("100GB")
        then:
        result == 100 * 1024 * 1024 * 1024L
    }

    void "should parse the whitespaces"() {
        when:
        long result = UnitConverter.toByte("100  GB")
        then:
        result == 100 * 1024 * 1024 * 1024L
    }

    void "should parse the commas"() {
        when:
        long result = UnitConverter.toByte("1,000GB")
        then:
        result == 1000*1024*1024*1024L
    }

    void "invalid unit expressions are ignored and should parse numbers only"() {
        when:
        long invalidUnit = UnitConverter.toByte("100PB")
        then:
        invalidUnit == 100
    }
}
