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
package org.robovm.apple.metal;

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
import org.robovm.apple.foundation.*;
import org.robovm.apple.dispatch.*;
/*</imports>*/

/*<javadoc>*/
/**
 * @since Available in iOS 8.0 and later.
 */
/*</javadoc>*/
/*<annotations>*/@Library("Metal") @NativeClass/*</annotations>*/
/*<visibility>*/public/*</visibility>*/ class /*<name>*/MTLRenderPassColorAttachmentDescriptorArray/*</name>*/ 
    extends /*<extends>*/NSObject/*</extends>*/ 
    /*<implements>*//*</implements>*/ {

    /*<ptr>*/public static class MTLRenderPassColorAttachmentDescriptorArrayPtr extends Ptr<MTLRenderPassColorAttachmentDescriptorArray, MTLRenderPassColorAttachmentDescriptorArrayPtr> {}/*</ptr>*/
    /*<bind>*/static { ObjCRuntime.bind(MTLRenderPassColorAttachmentDescriptorArray.class); }/*</bind>*/
    /*<constants>*//*</constants>*/
    /*<constructors>*/
    public MTLRenderPassColorAttachmentDescriptorArray() {}
    protected MTLRenderPassColorAttachmentDescriptorArray(SkipInit skipInit) { super(skipInit); }
    /*</constructors>*/
    /*<properties>*/
    
    /*</properties>*/
    /*<members>*//*</members>*/
    /*<methods>*/
    @Method(selector = "objectAtIndexedSubscript:")
    public native MTLRenderPassColorAttachmentDescriptor get(@MachineSizedUInt long attachmentIndex);
    @Method(selector = "setObject:atIndexedSubscript:")
    public native void insert(MTLRenderPassColorAttachmentDescriptor attachment, @MachineSizedUInt long attachmentIndex);
    /*</methods>*/
}
