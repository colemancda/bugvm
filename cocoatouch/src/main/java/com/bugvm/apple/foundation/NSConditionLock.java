/*
 * Copyright (C) 2013-2015 RoboVM AB
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
package com.bugvm.apple.foundation;

/*<imports>*/

import com.bugvm.objc.ObjCRuntime;
import com.bugvm.objc.annotation.Method;
import com.bugvm.objc.annotation.NativeClass;
import com.bugvm.objc.annotation.Property;
import com.bugvm.objc.*;
import com.bugvm.rt.*;
import com.bugvm.rt.annotation.*;
import com.bugvm.rt.bro.*;
import com.bugvm.rt.bro.annotation.*;
import com.bugvm.rt.bro.ptr.*;
/*</imports>*/

/*<javadoc>*/

/*</javadoc>*/
/*<annotations>*/@Library("Foundation") @NativeClass/*</annotations>*/
/*<visibility>*/public/*</visibility>*/ class /*<name>*/NSConditionLock/*</name>*/ 
    extends /*<extends>*/NSObject/*</extends>*/ 
    /*<implements>*/implements NSLocking/*</implements>*/ {

    /*<ptr>*/public static class NSConditionLockPtr extends Ptr<NSConditionLock, NSConditionLockPtr> {}/*</ptr>*/
    /*<bind>*/static { ObjCRuntime.bind(NSConditionLock.class); }/*</bind>*/
    /*<constants>*//*</constants>*/
    /*<constructors>*/
    public NSConditionLock() {}
    protected NSConditionLock(SkipInit skipInit) { super(skipInit); }
    public NSConditionLock(@MachineSizedSInt long condition) { super((SkipInit) null); initObject(init(condition)); }
    /*</constructors>*/
    /*<properties>*/
    @Property(selector = "condition")
    public native @MachineSizedSInt long getCondition();
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Property(selector = "name")
    public native String getName();
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Property(selector = "setName:")
    public native void setName(String v);
    /*</properties>*/
    /*<members>*//*</members>*/
    /*<methods>*/
    @Method(selector = "initWithCondition:")
    protected native @Pointer long init(@MachineSizedSInt long condition);
    @Method(selector = "lockWhenCondition:")
    public native void lock(@MachineSizedSInt long condition);
    @Method(selector = "tryLock")
    public native boolean tryLock();
    @Method(selector = "tryLockWhenCondition:")
    public native boolean tryLock(@MachineSizedSInt long condition);
    @Method(selector = "unlockWithCondition:")
    public native void unlock(@MachineSizedSInt long condition);
    @Method(selector = "lockBeforeDate:")
    public native boolean lock(NSDate limit);
    @Method(selector = "lockWhenCondition:beforeDate:")
    public native boolean lock(@MachineSizedSInt long condition, NSDate limit);
    @Method(selector = "lock")
    public native void lock();
    @Method(selector = "unlock")
    public native void unlock();
    /*</methods>*/
}