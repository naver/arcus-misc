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

/**
 * A very simple unit converter.
 *
 * @author hoonmin
 */
public class UnitConverter {

    private static final String KB = "KB";
    private static final String MB = "MB";
    private static final String GB = "GB";
    private static final String TB = "TB";

    public static long toByte(String expr) {
        if (expr == null || "".equals(expr)) {
            return 0;
        }

        long result = 0;

        try {
            int multiplier = 1;

            if (expr.contains(KB)) {
                multiplier = 1024;
            } else if (expr.contains(MB)) {
                multiplier = 1024 * 1024;
            } else if (expr.contains(GB)) {
                multiplier = 1024 * 1024 * 1024;
            } else if (expr.contains(TB)) {
                multiplier = 1024 * 1024 * 1024 * 1024;
            }

            // cut off the non-numeric characters
            String numStr = expr.replaceAll("\\D+","");

            // parse
            long num = Long.parseLong(numStr);
            result = num * multiplier;
        } catch (Exception e) {
            // let's ignore exceptions here
        } finally {
            return result;
        }
    }
}
