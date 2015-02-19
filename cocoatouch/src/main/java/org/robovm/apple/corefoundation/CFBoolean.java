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
package org.robovm.apple.corefoundation;

/*<imports>*/
import java.io.*;
import java.nio.*;
import java.util.*;
import org.robovm.objc.*;
import org.robovm.objc.annotation.*;
import org.robovm.objc.block.*;
import org.robovm.rt.*;
import org.robovm.rt.bro.*;
import org.robovm.rt.bro.annotation.*;
import org.robovm.rt.bro.ptr.*;
import org.robovm.apple.dispatch.*;
import org.robovm.apple.foundation.*;
/*</imports>*/

/*<javadoc>*/
/*</javadoc>*/
/*<annotations>*/@Library("CoreFoundation")/*</annotations>*/
/*<visibility>*/public/*</visibility>*/ class /*<name>*/CFBoolean/*</name>*/ 
    extends /*<extends>*/CFPropertyList/*</extends>*/ 
    /*<implements>*//*</implements>*/ {
    
    /*<ptr>*/public static class CFBooleanPtr extends Ptr<CFBoolean, CFBooleanPtr> {}/*</ptr>*/
    /*<bind>*/static { Bro.bind(CFBoolean.class); }/*</bind>*/
    
    /*<constants>*//*</constants>*/
    /*<constructors>*/
    protected CFBoolean() {}
    /*</constructors>*/
    /*<properties>*//*</properties>*/
    /*<members>*//*</members>*/
    public static CFBoolean valueOf(boolean b) {
        return b ? True() : False();
    }
    /*<methods>*/
    @GlobalValue(symbol="kCFBooleanTrue", optional=true)
    public static native CFBoolean True();
    @GlobalValue(symbol="kCFBooleanFalse", optional=true)
    public static native CFBoolean False();
    
    @Bridge(symbol="CFBooleanGetTypeID", optional=true)
    public static native @MachineSizedUInt long getClassTypeID();
    @Bridge(symbol="CFBooleanGetValue", optional=true)
    public native boolean booleanValue();
    /*</methods>*/
}
