/*
 * Copyright (C) 2013-2015 Trillian Mobile AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.robovm.apple.foundation;

import org.robovm.rt.bro.LazyGlobalValue;

public class GlobalValueEnumeration<T> {
    private final LazyGlobalValue<T> lazyGlobalValue;

    protected GlobalValueEnumeration (Class<?> clazz, String getterName) {
        lazyGlobalValue = new LazyGlobalValue<>(clazz, getterName);
    }

    public T value () {
        return lazyGlobalValue.value();
    }

    @Override
    public String toString () {
        return value().toString();
    }
}
